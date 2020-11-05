#!/usr/bin/env python3
# coding=utf-8
from math import ceil, floor

import numpy as np
import pandas as pd
import soundfile as sf


def show_counts(min_dists):
    cw_value_counts = min_dists['codeword'].value_counts()
    cb_size = min_dists['codeword'].max() + 1
    print('Codebook size: %d' % cb_size)
    print('Counts:')
    print('  %8s %5s' % ('codeword', 'count'))
    for codeword, count in cw_value_counts.items():
        print('  %8s %5s' % (codeword, count))


def do_select(wav_filename, win_length_ms, off_length_ms,
              min_dists, codewords):

    print('- loading %s' % wav_filename)
    samples, sample_rate = sf.read(wav_filename)

    win_size = int(win_length_ms * sample_rate / 1000)
    offset = int(off_length_ms * sample_rate / 1000)

    # select the indices for the desired codewords:
    print('- selecting codewords = %s' % codewords)
    indices = min_dists[min_dists['codeword'].isin(codewords)].index.values.astype(int)

    # initialize resulting signal:
    selected_samples = np.repeat(0., len(samples))

    for index in indices:
        win_start = offset * index
        # very rough copy (todo: use some more elegant np mechanism)
        for i in range(win_start, win_start + win_size):
            selected_samples[i] = samples[i]

    # write resulting file:
    cb_size = min_dists['codeword'].max() + 1
    res_filename = 'selected_M_%04d_cws_%s.wav' % (cb_size, ','.join([str(cw) for cw in codewords]))
    print('- writing %s' % res_filename)
    sf.write(res_filename, selected_samples, sample_rate)


def get_codewords_and_min_dists(sample_rate, off_length_ms,
                                start_time_ms, duration_ms,
                                min_dists):

    print('- getting sequence of codewords and min distortions in time interval')

    offset = int(off_length_ms * sample_rate / 1000)
    print('offset = %d' % offset)

    num_wins = ceil(duration_ms / off_length_ms)

    start_sample = floor(start_time_ms * sample_rate / 1000)
    start_index = floor(start_sample / offset)

    if start_index + num_wins > len(min_dists):
        num_wins = len(min_dists) - start_index

    # print('len(min_dists) = %d' % len(min_dists))
    print('num_wins = %d' % num_wins)
    print('start_sample = %d' % start_sample)
    print('start_index = %d' % start_index)

    sequence = []
    codewords = min_dists['codeword']
    min_dists = min_dists['minDistortion']
    for _ in range(num_wins):
        codeword = codewords[start_index]
        min_dist = min_dists[start_index]
        sequence.append((start_index, codeword, min_dist, start_time_ms))
        start_index += 1
        start_time_ms += off_length_ms

    return sequence


def do_intersect(sample_rate, off_length_ms,
                 start_time_ms, duration_ms,
                 min_dists):

    sequence = get_codewords_and_min_dists(sample_rate, off_length_ms,
                                           start_time_ms, duration_ms,
                                           min_dists)
    print('  %5s %8s %8s' % ('index', 'codeword', 'ms'))
    for start_index, codeword, _, start_time_ms in sequence:
        print('  %5s %8s %8s' % (start_index, codeword, start_time_ms))


def show_unique_codewords(sample_rate, off_length_ms,
                          start_time_ms, duration_ms,
                          min_dists):

    sequence = get_codewords_and_min_dists(sample_rate, off_length_ms,
                                           start_time_ms, duration_ms,
                                           min_dists)
    only_keywords = set([str(codeword) for (_, codeword, _, _) in sequence])
    print('codewords: %s' % ' '.join(only_keywords))


def do_plot(wav_filename, interval, off_length_ms,
            start_time_ms, duration_ms,
            min_dists):

    import matplotlib.pyplot as plt
    from matplotlib import cm
    from matplotlib import gridspec
    import librosa
    from librosa import display

    info = sf.info(wav_filename)
    sample_rate = info.samplerate

    tot_samples = int(info.duration * sample_rate)
    print('tot_duration = %s  tot_samples = %s' % (info.duration, tot_samples))
    tot_duration_ms = 1000 * info.duration

    if start_time_ms + duration_ms >= tot_duration_ms:
        print('ERROR: interval beyond signal length')
        exit()

    cb_size = min_dists['codeword'].max() + 1

    def get_signal_segment():
        start_sample = floor(start_time_ms * sample_rate / 1000)
        num_samples = ceil(duration_ms * sample_rate / 1000)

        print('Loading %s samples starting at %s' % (num_samples, start_sample))
        samples, _ = sf.read(wav_filename, start=start_sample, frames=num_samples)
        print(' Loaded %s samples' % len(samples))
        return samples

    def plot_spectrogram(signal):
        def compute_stft(window_size, offset):
            stft = np.abs(librosa.stft(y=signal, n_fft=window_size, hop_length=offset))
            stft = librosa.amplitude_to_db(stft, ref=np.max)
            return stft

        def spectrogram(stft):
            display.specshow(stft, y_axis='mel', x_axis='time', sr=sample_rate,
                             cmap='Blues', fmin=0, fmax=16000)

        stft = compute_stft(1024, 512)
        spectrogram(stft)

    def plot_quantization_and_distortion(fig):
        sequence = get_codewords_and_min_dists(sample_rate, off_length_ms,
                                               start_time_ms, duration_ms,
                                               min_dists)

        def get_color_map():
            return cm.get_cmap(name='hsv', lut=cb_size)
            # from palettable.colorbrewer.diverging import BrBG_4 as cm
            # from palettable.colorbrewer.diverging import RdYlBu_9 as cm
            # from palettable.scientific.diverging import Berlin_20 as cm
            # return cm.get_mpl_colormap()

        def plot_quantization():
            codewords = [codeword for _, codeword, _, _ in sequence]

            color_map = get_color_map()
            color = [color_map(val / cb_size) for val in codewords]
            ind = np.arange(len(codewords))
            plt.bar(x=ind, height=2, width=1, color=color, align='edge')
            plt.xlim(xmin=0, xmax=len(codewords))
            plt.ylabel('codeword')
            plt.xlabel('window index')
            plt.yticks([])

        def plot_distortion(ax):
            mindists = [mindist for _, _, mindist, _ in sequence]
            # trick to see last step:
            # a) repeat last value: (https://stackoverflow.com/a/53854732/830737)
            mindists.append(mindists[-1])
            ind = np.arange(len(mindists))
            plt.step(ind, mindists, where='post')
            # b) and set xmax with len -1:
            plt.xlim(xmin=0, xmax=len(mindists) - 1)
            plt.ylabel('distortion')
            plt.xlabel('window index')

        fig.add_subplot(gs[1])
        plot_quantization()

        ax_d = fig.add_subplot(gs[2])
        plot_distortion(ax_d)

    samples = get_signal_segment()

    fig = plt.figure(figsize=(8, 8))
    gs = gridspec.GridSpec(3, 1, height_ratios=[7, 1, 2])

    # spectrogram:
    plt.subplot(gs[0])
    plot_spectrogram(samples)

    plt.title('$M=%d$ Codebook. Interval: @%s +%s' % (cb_size, interval[0], interval[1]))

    # quantization and distortion:
    plot_quantization_and_distortion(fig)

    plt.tight_layout()
    plt.show()
    suffix_out_name = '%s_%s' % (interval[0], interval[1])
    target_file = 'spectrogram_and_quantization_M_%d_%s' % (cb_size, suffix_out_name)
    print('Saving {}.png'.format(target_file))
    fig.savefig(target_file + '.png', dpi=120)


# Adapted from https://stackoverflow.com/a/4628148/830737
# Returns duration in millis.
def parse_duration(dur_str):
    import re
    from datetime import timedelta
    regex = re.compile(r'^((?P<hours>\d+(\.\d+)?)h)?((?P<minutes>\d+(\.\d+)?)m)?((?P<seconds>\d+(\.\d+)?)s)?$')
    parts = regex.match(dur_str)
    if parts:
        parts = parts.groupdict()
        time_params = {}
        for (name, param) in parts.items():
            if param:
                time_params[name] = float(param)
                # print('name=%s val=%s' % (name, time_params[name]))
                time_params[name] = float(param)

        return int(1000 * timedelta(**time_params).total_seconds())
    else:
        print('parse_duration: cannot parse `%s`' % dur_str)


def parse_start_and_duration(strings):
    start_time_ms, duration_ms = [parse_duration(e) for e in strings]
    if start_time_ms is None or duration_ms is None:
        exit(1)

    print('start_time_ms = %s' % start_time_ms)
    print('  duration_ms = %s' % duration_ms)
    return start_time_ms, duration_ms


def parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description='Signal window selection, codeword assignments, counts, etc.',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('-W',  metavar='ms', default=45,
                        help='Window length in ms (default 45).')

    parser.add_argument('-O', metavar='ms', default=15,
                        help='Window offset in ms (default 15).')

    parser.add_argument('--mindists', required=True, metavar='csv',
                        help='CSV file with codeword and min distances.')

    parser.add_argument('--codewords', nargs='+', metavar='cw',
                        help='Creates signal file with windows in source signal having these assigned codewords'
                             '\n(other windows set to zero).')

    parser.add_argument('--signal', metavar='wav',
                        help='Sound file. Required if any of --codewords, --intersect, --unique is given.')

    parser.add_argument('--counts', action='store_true',
                        help='Show codeword value counts from mindists file (in count decreasing order).')

    parser.add_argument('--intersect', nargs=2,  metavar=('start', 'duration'),
                        help='Show sequence of codewords intersecting this signal time interval.'
                             '\nExample: --intersect 1h57m39.828s 1.932s')

    parser.add_argument('--unique', nargs=2,  metavar=('start', 'duration'),
                        help='Show unique codewords in this signal time interval.'
                             '\nExample: --unique 1h57m39.828s 1.932s')

    parser.add_argument('--plot', nargs=2,  metavar=('start', 'duration'),
                        help='Plot spectrogram of this signal time interval along with\n'
                             'associated codeword and minimum distortion.'
                             '\nExample: --plot 1h57m39.828s 1.932s')

    args = parser.parse_args()

    if not args.counts and not args.codewords and not args.intersect and not args.unique and not args.plot:
        print('Indicate at least one of: --counts, --codewords, --intersect, --unique, --plot')
        exit()

    return args


if __name__ == "__main__":
    args = parse_args()

    md_filename = args.mindists
    print('- loading %s' % md_filename)
    min_dists = pd.read_csv(md_filename, comment='#')

    win_length_ms = int(args.W)
    off_length_ms = int(args.O)

    if args.counts:
        show_counts(min_dists)

    if args.codewords:
        codewords = [int(cw) for cw in args.codewords]

        do_select(args.signal, win_length_ms, off_length_ms,
                  min_dists, codewords)

    if args.intersect:
        print('args.intersect', args.intersect)
        start_time_ms, duration_ms = parse_start_and_duration(args.intersect)

        sample_rate = sf.info(args.signal).samplerate
        do_intersect(sample_rate, off_length_ms,
                     start_time_ms, duration_ms,
                     min_dists)

    if args.unique:
        print('args.unique', args.unique)
        start_time_ms, duration_ms = parse_start_and_duration(args.unique)

        sample_rate = sf.info(args.signal).samplerate
        show_unique_codewords(sample_rate, off_length_ms,
                              start_time_ms, duration_ms,
                              min_dists)

    if args.plot:
        print('args.plot', args.plot)
        start_time_ms, duration_ms = parse_start_and_duration(args.plot)

        do_plot(args.signal, args.plot, off_length_ms,
                start_time_ms, duration_ms,
                min_dists)
