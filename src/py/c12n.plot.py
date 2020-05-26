#!/usr/bin/env python3.7
# coding=utf-8

import matplotlib.pyplot as plt
from matplotlib import gridspec

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
            if args.class_name:
                if rank == 1:
                    return '{}'.format(selection_number) if len(ranks) < 50 else ''
                else:
                    return '{}~ {}:{}'.format(r1, rank, selection_number)
            else:
                if rank == 1:
                    return '{}#{}'.format(seq_class_name, selection_number)
                else:
                    return '1={}/ {}={}#{}'.format(r1, rank, seq_class_name, selection_number)

        ax.tick_params(axis='x', labelsize=10 if len(ranks) < 50 else 6)
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
    plot_spectrogram(signal_interval.interval, signal.sample_rate, ax)

    if len(times) <= args.msfd:
        plot_vertical_lines(times, elapsed_times, args)

    if selections_and_c12n is None:
        plt.title(title)

    plt.tight_layout()
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


# get complete signal interval from min to max selections:
def cover_min_max_selections(signal: Signal,
                             selections_and_c12n,
                             args
                             ) -> SignalInterval:
    min_selection = None
    max_selection = None
    for s, _ in selections_and_c12n:
        if min_selection is None or min_selection.selection > s.selection:
            min_selection = s
        if max_selection is None or max_selection.selection < s.selection:
            max_selection = s
    print('min_selection = {}'.format(min_selection))
    print('max_selection = {}'.format(max_selection))
    tot_duration_s = max_selection.end_time_s - min_selection.begin_time_s
    print('tot_duration_s = {}'.format(tot_duration_s))

    return get_signal_interval_for_min_max_selections(signal,
                                                      min_selection,
                                                      max_selection,
                                                      max_seconds=args.max_seconds)


def dispatch_aggregate_selections(signal: Signal,
                                  segments_df: pd.DataFrame,
                                  c12n: pd.DataFrame,
                                  args):

    selections_and_c12n = get_selections_and_c12n(segments_df, c12n,
                                                  desired_rank=args.rank,
                                                  desired_class_name=args.class_name
                                                  )
    if not len(selections_and_c12n):
        print('No selections')
        return

    if args.concat:
        signal_interval = concatenate_selections(signal, selections_and_c12n)
    else:
        signal_interval = cover_min_max_selections(signal, selections_and_c12n, args)

    print('interval with {} samples'.format(len(signal_interval.interval)))

    title = get_title_for_c12n(selections_and_c12n, args)

    out_file = None
    if args.out_prefix:
        out_file = '{}c12n_concatenated.png'.format(args.out_prefix)

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
                        help='Sound file to associate results to.')

    parser.add_argument('-W',  metavar='ms', default=45,
                        help='Window length in ms (default 45).')

    parser.add_argument('-O', metavar='ms', default=15,
                        help='Window offset in ms (default 15).')

    parser.add_argument('--max-seconds',
                        help='Max seconds to visualize')

    parser.add_argument('--concat', default=False, action='store_const',
                        const=True,
                        help='Concatenate all given selections')

    parser.add_argument('--cover', default=False, action='store_const',
                        const=True,
                        help='Interval covered by the selections')

    parser.add_argument('--segments', required=True,
                        help='CSV file with segments.')

    parser.add_argument('--c12n', required=True,
                        help='CSV file classification results.')

    parser.add_argument('--rank',
                        help='Only dispatch given rank')

    parser.add_argument('--class', dest='class_name',
                        help='Only dispatch given class')

    parser.add_argument('--msfd', default=110, metavar='number',
                        help='Maximum number of selections for detailed plot (default 100)')

    parser.add_argument('--out-prefix',
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
