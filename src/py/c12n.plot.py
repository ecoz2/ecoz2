#!/usr/bin/env python3
# coding=utf-8

# the following as I started having issues with the final fig.savefig on my mac
import platform
print('platform.system(): {}'.format(platform.system()))
if platform.system() == 'Darwin':
    import matplotlib
    print('Switching from backend: {}'.format(matplotlib.get_backend()))
    matplotlib.use('macosx', force=True)
    print('to : {}'.format(matplotlib.get_backend()))
    # with this, fig.savefig completes ok.

import matplotlib.pyplot as plt
from matplotlib import gridspec
import numpy as np

from ecoz2_misc import *


def get_times(selections_and_c12n, args) -> (np.array, np.array):
    elapsed_times = np.array([s.end_time_s - s.begin_time_s
                              for s, _ in selections_and_c12n])

    if args.concat:
        times = []
        for i in range(len(elapsed_times)):
            times.append(elapsed_times[0:i].sum())

    else:
        begin_times = np.array([s.begin_time_s for s, _ in selections_and_c12n])
        first_being_time = begin_times[0]
        times = begin_times - first_being_time

    return np.array(times), elapsed_times


def plot_vertical_lines(times, elapsed_times, args):
    for x in times:
        plt.axvline(x=x, c='gray', linewidth=1)

    if args.cover:
        # add right line for each selection
        right_times = np.add(times, elapsed_times)
        for x in right_times:
            plt.axvline(x=x, c='gray', linewidth=1)


def get_colors(ranks):
    color_map = ['lightgreen', 'lightblue', 'blue', 'yellow', 'orange', 'red']
    return [color_map[r - 1] if r <= len(color_map) else 'brown' for r in ranks]


def plot_classification(selections_and_c12n,
                        times: np.array,
                        elapsed_times: np.array,
                        duration_s,
                        ax,
                        args):
    # ax.xaxis.tick_top()
    # ax.xaxis.set_label_position('top')

    ranks = np.array([c.get('rank') for _, c in selections_and_c12n])
    print('ranks({}) = {}'.format(len(ranks), ranks))
    colors = get_colors(ranks)

    if args.concat:
        plt.xlim(xmin=0, xmax=elapsed_times.sum())
    else:
        plt.xlim(xmin=0, xmax=duration_s)

    max_rank = np.full((len(ranks)), ranks.max())
    bar_heights = max_rank - ranks + 1
    plt.bar(x=times, height=bar_heights, width=elapsed_times, color=colors, align='edge')

    def add_x_labels_and_vertical_lines():
        plt.xticks(times, rotation=90, ha='left', fontsize=10)

        # ax.set_xticklabels([])

        selection_numbers = [s.selection for s, _ in selections_and_c12n]
        r1s = np.array([c.get('r1') for _, c in selections_and_c12n])
        seq_class_names = np.array([c.get('seq_class_name') for _, c in selections_and_c12n])

        def label(selection_number, rank, r1, seq_class_name):
            if len(ranks) > 140:
                return ''

            prefix = ''
            if args.class_name:
                if rank == 1:
                    pass
                else:
                    # show winning class (r1):
                    prefix = '{}>{}'.format(r1, rank)
            else:
                if rank == 1:
                    # no specific class for the report, so show the sequence class:
                    prefix = '{}'.format(seq_class_name)
                else:
                    # show winning class (r1), and rank of the correct class for the sequence:
                    prefix = '{}>{}:{}'.format(r1, rank, seq_class_name)

            # last piece always `#<selection_number>`:
            return '{}#{}'.format(prefix, selection_number)

        ax.tick_params(axis='x', labelsize=8 if len(ranks) < 50 else 6)
        x_labels = [label(s, rank, r1, scn) for s, rank, r1, scn in zip(selection_numbers, ranks, r1s, seq_class_names)]
        ax.set_xticklabels(x_labels)

    if len(ranks) <= args.msfd:
        add_x_labels_and_vertical_lines()
    else:
        plt.xticks([])

    plt.ylabel('rank')
    plt.yticks([])


def do_plot(signal: Signal,
            signal_interval: SignalInterval,
            title: str,
            out_file: str or None,
            args,
            selections_and_c12n=None
            ):

    if args.concat or args.cover:
        fig = plt.figure(figsize=(12, 4))
    else:
        fig = plt.figure(figsize=(6, 4))

    times = None
    elapsed_times = None

    if selections_and_c12n:
        times, elapsed_times = get_times(selections_and_c12n, args)
        # print('times({}) = {}'.format(len(times), times))
        # print('elapsed_times({}) = {}'.format(len(elapsed_times), elapsed_times))

        if args.lpc:
            gs = gridspec.GridSpec(3, 1, height_ratios=[1, 7, 7])
        else:
            gs = gridspec.GridSpec(2, 1, height_ratios=[1, 7])

    else:
        gs = gridspec.GridSpec(1, 1)

    index = 0

    if selections_and_c12n:
        ax = plt.subplot(gs[index])
        index += 1
        plot_classification(selections_and_c12n,
                            times,
                            elapsed_times,
                            signal_interval.duration_s,
                            ax,
                            args)

        if len(times) <= args.msfd:
            plot_vertical_lines(times, elapsed_times, args)

        plt.title(title)

    ax = plt.subplot(gs[index])
    index += 1

    window_size = 1024
    window_offset = 512

    plot_spectrogram(signal_interval.interval,
                     signal.sample_rate,
                     ax,
                     window_size=window_size,
                     window_offset=window_offset,
                     cmap=args.cmap
                     )

    if len(times) <= args.msfd:
        plot_vertical_lines(times, elapsed_times, args)

    plt.title('Spectrogram (${}/{}$)'.format(window_size, window_offset))

    if args.lpc:
        ax = plt.subplot(gs[index])
        index += 1
        plot_lpc_spectrogram(signal_interval.interval,
                             signal.sample_rate,
                             args.lpc,
                             ax,
                             window_size=window_size,
                             window_offset=window_offset,
                             cmap=args.cmap
                             )
        plt.title('LPC Spectrogram ($P = {}$)'.format(args.lpc))
        # TODO plot_vertical_lines

    if selections_and_c12n is None:
        plt.title(title)

    plt.tight_layout()
    plt.subplots_adjust(hspace=0.3)
    plt.show()
    if out_file:
        print('Saving {}'.format(out_file))
        fig.savefig(out_file, dpi=120)


def dispatch_selection(signal: Signal,
                       interval: np.ndarray,
                       c12n_row: pd.core.series.Series,
                       selection: Selection,
                       args):
    seq_class_name = c12n_row.get('seq_class_name')
    seq_filename = c12n_row.get('seq_filename')
    rank = c12n_row.get('rank')

    title = 'Selection number: {}'.format(selection.selection)
    title += '\n{}: "{}"  Rank: {}'.format(seq_filename, seq_class_name, rank)
    if rank > 1:
        ranked_models = ['{}:{}'.format(r, c12n_row.get('r{}'.format(r))) for r in range(1, rank + 1)]
        title += '\nRanked models: {}'.format(', '.join(ranked_models))

    out_file = None
    if args.out_prefix:
        out_file = '%sc12n_%s.png' % (
            args.out_prefix, selection.selection)

    # FIXME interval is now signal_interval: SignalInterval
    do_plot(signal,
            interval,
            title,
            out_file,
            args
            )


def dispatch_individual_selections(signal: Signal,
                                   segments_df: pd.DataFrame,
                                   c12n: pd.DataFrame,
                                   args):
    for i, c12n_row in c12n.iterrows():
        selection_number = extract_selection_number(c12n_row, 'seq_filename')
        if selection_number < 0:
            continue

        rank = c12n_row.get('rank')
        if args.rank is not None and int(args.rank) != rank:
            continue

        seq_class_name = c12n_row.get('seq_class_name')
        if args.class_name is not None and args.class_name != seq_class_name:
            continue

        seq_filename = c12n_row.seq_filename
        correct = c12n_row.correct
        print(" {:3} {} sel={} '{}' => {} {}".format(
            i, seq_filename, selection_number, seq_class_name,
            correct, rank))

        selection = get_selection(segments_df, selection_number)
        print('selection = {}'.format(selection))

        if selection:
            interval = get_signal_interval_from_selection(signal, selection)
            if interval is not None:
                dispatch_selection(signal, interval, c12n_row, selection, args)


def get_title_for_c12n(selections_and_c12n,
                       args):
    num_selections = len(selections_and_c12n)

    num_correct = 0
    for s, c in selections_and_c12n:
        rank = c.get('rank')
        if rank == 1:
          num_correct += 1

    title_lines = [
        '{} selections.  Correct: {} ({:.2f}%)'.format(
          num_selections,
          num_correct,
          float(num_correct) * 100. / num_selections,
        ),
        '{}{}{}'.format(args.c12n,
                        '  Class: "{}"'.format(args.class_name) if args.class_name else '',
                        '  Rank: {}'.format(args.rank) if args.rank else '',
                        ),
    ]
    return '\n'.join(title_lines)


def concatenate_selections(signal: Signal,
                           selections_and_c12n
                           ) -> np.array:
    intervals = [get_signal_interval_from_selection(signal, s) for s, _ in selections_and_c12n]
    concatenated = np.concatenate(intervals)
    print('concatenated signal: {} samples'.format(len(concatenated)))
    return SignalInterval(concatenated, 0, 0)  # FIXME


# get signal interval from min to max selections but restricted by
# args.delta_begin_seconds and args.max_seconds as indicated
def cover_min_max_selections(signal: Signal,
                             selections_and_c12n,
                             args
                             ) -> (any, SignalInterval):

    begin_time_s = selections_and_c12n[0][0].begin_time_s + float(args.delta_begin_seconds)

    max_seconds = float(args.max_seconds)

    end_time_s = begin_time_s + max_seconds

    min_selection = None
    max_selection = None
    selected = []
    for s, c in selections_and_c12n:
        if s.begin_time_s < begin_time_s:
            continue

        if s.end_time_s > end_time_s:
            continue

        if min_selection is None or min_selection.selection > s.selection:
            min_selection = s
        if max_selection is None or max_selection.selection < s.selection:
            max_selection = s

        selected.append((s, c))

    print('min_selection = {}'.format(min_selection))
    print('max_selection = {}'.format(max_selection))
    print('num_selections = {}'.format(len(selected)))
    tot_duration_s = max_selection.end_time_s - min_selection.begin_time_s
    print('tot_duration_s = {}'.format(tot_duration_s))

    signal_interval = get_signal_interval_for_min_max_selections(signal,
                                                                 min_selection,
                                                                 max_selection,
                                                                 max_seconds)

    return selected, signal_interval


def dispatch_aggregate_selections(signal: Signal,
                                  segments_df: pd.DataFrame,
                                  c12n: pd.DataFrame,
                                  args):

    # get these only by filtering for rank and class name (if any)
    selections_and_c12n = get_selections_and_c12n(segments_df, c12n,
                                                  desired_rank=args.rank,
                                                  desired_class_name=args.class_name
                                                  )
    if not len(selections_and_c12n):
        print('No selections')
        return

    out_file = None
    class_str = '_{}'.format(args.class_name) if args.class_name else ''
    if args.concat:
        # TODO apply args.max_seconds also in this case.
        signal_interval = concatenate_selections(signal, selections_and_c12n)
        if args.out_prefix:
            out_file = '{}c12n_concat{}.png'.format(args.out_prefix, class_str)

    else:
        selections_and_c12n, signal_interval = cover_min_max_selections(signal, selections_and_c12n, args)
        if args.out_prefix:
            out_file = '{}c12n_cover{}.png'.format(args.out_prefix, class_str)

    print('interval with {} samples'.format(len(signal_interval.interval)))

    title = get_title_for_c12n(selections_and_c12n, args)

    do_plot(signal,
            signal_interval,
            title,
            out_file,
            args,
            selections_and_c12n
            )


def parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description='Plot classification results',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--signal', metavar='wav',
                        help='Associated sound file (spectrogram of which is shown).')

    parser.add_argument('--lpc', type=int, metavar='P',
                        help='Order of prediction to display LPC spectra')

    parser.add_argument('--cmap', type=str, metavar='cm',
                        help='Name of color map for spectra')

    parser.add_argument('--delta-begin-seconds', metavar='secs', default=0,
                        help='Increment from first selection time to visualize (default, 0).')

    parser.add_argument('--max-seconds', metavar='secs', default=3 * 60,
                        help='Max seconds to visualize (default, 3min).')

    parser.add_argument('--concat', default=False, action='store_const',
                        const=True,
                        help='Concatenate given selections all together')

    parser.add_argument('--cover', default=False, action='store_const',
                        const=True,
                        help='Show interval covered by the selections')

    parser.add_argument('--segments', required=True, metavar='file',
                        help='CSV file with segments.')

    parser.add_argument('--c12n', required=True, metavar='file',
                        help='CSV file with classification results.')

    parser.add_argument('--rank', metavar='rank',
                        help='Only dispatch given rank')

    parser.add_argument('--class', dest='class_name', metavar='name',
                        help='Only dispatch given class')

    parser.add_argument('--msfd', default=100, type=int, metavar='number',
                        help='Maximum number of selections for detailed plot (default 100)')

    parser.add_argument('--out-prefix', metavar='prefix',
                        help='Prefix to name output plot file.')

    args = parser.parse_args()

    return args


def main(args):
    signal = load_signal(args.signal)
    show_signal(signal)

    segments_df = load_csv(args.segments, sep='\t')
    c12n = load_csv(args.c12n)

    if args.concat or args.cover:
        dispatch_aggregate_selections(signal, segments_df, c12n, args)

    else:
        dispatch_individual_selections(signal, segments_df, c12n, args)


if __name__ == "__main__":
    # print('ARGS = {}'.format(args))
    main(parse_args())
