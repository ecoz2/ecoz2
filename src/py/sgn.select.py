#!/usr/bin/env python3.7
# coding=utf-8
import soundfile as sf
import pandas as pd
import numpy as np


def show_counts(min_dists):
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


def get_codeword_sequence(wav_filename, off_length_ms,
                          start_time_ms, duration_ms,
                          min_dists):

    print('- getting codewords')
    sample_rate = sf.info(wav_filename).samplerate

    offset = int(off_length_ms * sample_rate / 1000)
    print('offset = %d' % offset)

    from math import ceil, floor
    num_wins = ceil(duration_ms / off_length_ms)

    start_sample = floor(start_time_ms * sample_rate / 1000)
    start_index = floor(start_sample / offset)

    print('start_sample = %d' % start_sample)
    print('start_index = %d' % start_index)

    codeword_sequence = []
    codewords = min_dists['codeword']
    for _ in range(num_wins):
        codeword = codewords[start_index]
        codeword_sequence.append((start_index, codeword, start_time_ms))
        start_index += 1
        start_time_ms += off_length_ms

    return codeword_sequence


def do_intersect(wav_filename, off_length_ms,
                 start_time_ms, duration_ms,
                 min_dists):

    codeword_sequence = get_codeword_sequence(wav_filename, off_length_ms,
                                              start_time_ms, duration_ms,
                                              min_dists)
    print('  %5s %8s %8s' % ('index', 'codeword', 'ms'))
    for start_index, codeword, start_time_ms in codeword_sequence:
        print('  %5s %8s %8s' % (start_index, codeword, start_time_ms))


def show_unique_codewords(wav_filename, off_length_ms,
                          start_time_ms, duration_ms,
                          min_dists):

    codeword_sequence = get_codeword_sequence(wav_filename, off_length_ms,
                                              start_time_ms, duration_ms,
                                              min_dists)
    only_keywords = set([str(codeword) for (_, codeword, _) in codeword_sequence])
    print('codewords: %s' % ' '.join(only_keywords))


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

    args = parser.parse_args()

    if not args.counts and not args.codewords and not args.intersect and not args.unique:
        print('Indicate at least one of: --counts, --codewords, --intersect, --unique')
        exit()

    return args


if __name__ == "__main__":
    args = parse_args()

    md_filename = args.mindists
    print('- loading %s' % md_filename)
    min_dists = pd.read_csv(md_filename, comment='#')

    cw_value_counts = min_dists['codeword'].value_counts()

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

        do_intersect(args.signal, off_length_ms,
                     start_time_ms, duration_ms,
                     min_dists)

    if args.unique:
        print('args.unique', args.unique)
        start_time_ms, duration_ms = parse_start_and_duration(args.unique)

        show_unique_codewords(args.signal, off_length_ms,
                              start_time_ms, duration_ms,
                              min_dists)
