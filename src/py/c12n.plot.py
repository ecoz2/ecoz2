#!/usr/bin/env python3.7
# coding=utf-8

import matplotlib.pyplot as plt
from matplotlib import gridspec

from ecoz2_misc import *


def get_accum_times(selections_and_c12n):
    elapsed_times = np.array([s.end_time_s - s.begin_time_s
                              for s, _ in selections_and_c12n])
    accum_times = []
    for i in range(len(elapsed_times)):
        accum_times.append(elapsed_times[0:i].sum())

    return accum_times


def plot_vertical_lines(accum_times):
    for x in accum_times:
        plt.axvline(x=x, c='gray', linewidth=1)


def plot_classification(selections_and_c12n, accum_times, ax, args):
    # ax.xaxis.tick_top()
    # ax.xaxis.set_label_position('top')

    ranks = np.array([c.get('rank') for _, c in selections_and_c12n])

    elapsed_times = np.array([s.end_time_s - s.begin_time_s
                              for s, _ in selections_and_c12n])
    total_time = elapsed_times.sum()
    print('elapsed_times = {}'.format(elapsed_times))
    print('total_time = {}'.format(total_time))

    def get_colors():
        print('ranks({}) = {}'.format(len(ranks), ranks))
        color_map = ['lightgreen', 'lightblue', 'blue', 'yellow', 'orange', 'red']
        return [color_map[r - 1] if r <= len(color_map) else 'brown' for r in ranks]

    colors = get_colors()
    # print('colors = {}'.format(colors))
    plt.xlim(xmin=0, xmax=total_time)

    max_rank = np.full((len(ranks)), ranks.max())
    bar_heights = max_rank - ranks + 1
    plt.bar(x=accum_times, height=bar_heights, width=elapsed_times, color=colors, align='edge')

    def add_x_labels_and_vertical_lines():
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

        x_labels = [label(s, rank, r1, scn) for s, rank, r1, scn in zip(selection_numbers, ranks, r1s, seq_class_names)]
        plt.xticks(accum_times, rotation=90, ha='left', fontsize=10)
        ax.tick_params(axis='x', labelsize=10 if len(ranks) < 50 else 6)
        ax.set_xticklabels(x_labels)

        plot_vertical_lines(accum_times)

    if len(ranks) <= args.msfd:
        add_x_labels_and_vertical_lines()
    else:
        plt.xticks([])

    plt.ylabel('rank')
    plt.yticks([])


def do_plot(signal, interval,
            title,
            out_file,
            args,
            selections_for_classification=None
            ):

    if args.concat:
        fig = plt.figure(figsize=(12, 4))
    else:
        fig = plt.figure(figsize=(6, 4))

    accum_times = None
    if selections_for_classification:
        accum_times = get_accum_times(selections_for_classification)
        gs = gridspec.GridSpec(2, 1, height_ratios=[1, 7])
    else:
        gs = gridspec.GridSpec(1, 1)

    index = 0

    if selections_for_classification:
        ax = plt.subplot(gs[index])
        index += 1
        plot_classification(selections_for_classification, accum_times, ax, args)

        plt.title(title)

    ax = plt.subplot(gs[index])
    index += 1
    plot_spectrogram(signal, interval, ax)
    if accum_times and len(accum_times) <= args.msfd:
        plot_vertical_lines(accum_times)

    if selections_for_classification is None:
        plt.title(title)

    plt.tight_layout()
    plt.show()
    if out_file:
        print('Saving {}'.format(out_file))
        fig.savefig(out_file, dpi=120)


def dispatch_selection(signal: Signal,
                       interval,
                       c12n_row,
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

    do_plot(signal, interval,
            title,
            out_file,
            args
            )


def dispatch_individual_selections(signal: Signal, segments_df: pd.DataFrame, c12n, args):
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
        show_selection(selection)

        if selection:
            interval = get_signal_interval_from_selection(signal, selection)
            if interval is not None:
                dispatch_selection(signal, interval, c12n_row, selection, args)


def dispatch_concatenate_selections(signal: Signal, segments_df: pd.DataFrame, c12n, args):
    selections_and_c12n = get_selections_and_c12n(segments_df, c12n,
                                                  desired_rank=args.rank,
                                                  desired_class_name=args.class_name
                                                  )
    num_selections = len(selections_and_c12n)
    if not num_selections:
        print('No selections')
        return

    intervals = [get_signal_interval_from_selection(signal, s)
                 for s, _ in selections_and_c12n]

    concatenated = np.concatenate(intervals)
    print('concatenated signal: {} samples'.format(len(concatenated)))

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
    title = '\n'.join(title_lines)

    out_file = None
    if args.out_prefix:
        out_file = '{}c12n_concatenated.png'.format(args.out_prefix)

    do_plot(signal, concatenated,
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

    parser.add_argument('--concat', default=False, action='store_const',
                        const=True,
                        help='Concatenate all given selections')

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

    if args.concat:
        dispatch_concatenate_selections(signal, segments_df, c12n, args)

    else:
        dispatch_individual_selections(signal, segments_df, c12n, args)


if __name__ == "__main__":
    # print('ARGS = {}'.format(args))
    main(parse_args())
