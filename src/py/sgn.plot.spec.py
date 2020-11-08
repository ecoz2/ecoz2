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
import pandas as pd
import numpy as np
import re


from ecoz2_misc import \
    Signal, Selection, \
    load_signal, show_signal, load_csv, \
    get_selection, get_signal_interval_from_selection, \
    plot_spectrogram, plot_lpc_spectrogram


def do_plot(signal: Signal,
            interval: np.ndarray,
            selection: Selection,
            out_file: str or None,
            args,
            ):

    if args.lpc:
        fig = plt.figure(figsize=(8, 8))
        gs = gridspec.GridSpec(2, 1, height_ratios=[1, 1])
    else:
        fig = plt.figure(figsize=(8, 4))
        gs = gridspec.GridSpec(1, 1, height_ratios=[1])

    index = 0

    ax = plt.subplot(gs[index])

    plot_spectrogram(interval,
                     signal.sample_rate,
                     ax,
                     window_size=args.window_size,
                     window_offset=args.window_offset,
                     )

    if args.title:
        title = '{}\n\n'.format(args.title)
    else:
        title = "'{}' #{}\n\n".format(selection.type_, selection.selection)

    title += 'Spectrogram (${}/{}$)'.format(
        args.window_size, args.window_offset)
    plt.title(title)

    if args.lpc:
        index += 1
        ax = plt.subplot(gs[index])
        plot_lpc_spectrogram(interval,
                             signal.sample_rate,
                             args.lpc,
                             ax,
                             window_size=args.lpc_window_size,
                             window_offset=args.lpc_window_offset,
                             num_points_per_window=args.lpc_num_points_per_window,
                             )
        plt.title('LPC Spectrogram ($P = {}, {}/{}/{}$)'.format(
            args.lpc, args.lpc_window_size, args.lpc_window_offset, args.lpc_num_points_per_window))

    plt.tight_layout()
    plt.subplots_adjust(hspace=0.3)

    if not args.no_plot:
        plt.show()

    if out_file:
        print('Saving {}'.format(out_file))
        fig.savefig(out_file, dpi=120)


def dispatch_selection(signal: Signal,
                       interval: np.ndarray,
                       selection: Selection,
                       args):

    out_file = None
    if args.out_prefix:
        name = args.out_name
        if not name:
            name = 'sgn.plot.spec_{}_{}'.format(selection.type_, selection.selection)

        out_file = '{}{}.png'.format(args.out_prefix, name)

    do_plot(signal,
            interval,
            selection,
            out_file,
            args
            )


def dispatch_individual_selections(signal: Signal,
                                   segments_df: pd.DataFrame,
                                   selection_numbers,
                                   any_intervals: bool,
                                   args):

    num_dispatched = 0
    for selection_number in selection_numbers:
        interval = None

        selection = get_selection(segments_df, selection_number)
        # print('selection = {}'.format(selection))

        if selection and (args.class_name is None or args.class_name == selection.type_):
            interval = get_signal_interval_from_selection(signal, selection)

        if interval is not None:
            print(" sel={}: '{}'".format(selection_number, selection.type_))
            dispatch_selection(signal, interval, selection, args)
            num_dispatched += 1

            if any_intervals and 0 < args.max_selections <= num_dispatched:
                print("max-selections {} reached".format(args.max_selections))
                break


def main(args):
    signal = load_signal(args.signal)
    show_signal(signal)

    segments_df = load_csv(args.segments, sep='\t')

    any_intervals = False
    selection_numbers = []
    regex = re.compile(r'^(\d+)-(\d+)?$')
    for sn in args.selections:
        match = regex.match(sn)
        if match:
            start, end = int(match.group(1)), int(match.group(2))
            if start <= end:
                selection_numbers.extend([*range(start, end + 1)])
                any_intervals = True
        else:
            selection_numbers.append(int(sn))

    selection_numbers = sorted(selection_numbers)
    dispatch_individual_selections(signal, segments_df, selection_numbers,
                                   any_intervals, args)


def parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description='Plot spectrograms for given selections',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--signal', metavar='wav',
                        help='Sound file.')

    parser.add_argument('--segments', required=True, metavar='file',
                        help='CSV file with segments.')

    parser.add_argument('--selections', nargs='+',
                        help='Selection numbers to display. '
                             'Each element a single number or a start-end interval. '
                             'Using this in combination with --class and --max-selections '
                             'may be convenient.')

    parser.add_argument('--window-size', type=int, default=1024, metavar='#',
                        help='Window size for DFT analysis')

    parser.add_argument('--window-offset', type=int, default=512, metavar='#',
                        help='Window offset for DFT analysis')

    parser.add_argument('--lpc', type=int, metavar='P',
                        help='Order of prediction to display LPC spectra')

    parser.add_argument('--lpc-window-size', type=int, default=1024, metavar='#',
                        help='Window size for LPC analysis')

    parser.add_argument('--lpc-window-offset', type=int, default=512, metavar='#',
                        help='Window offset for LPC analysis')

    parser.add_argument('--lpc-num-points-per-window', type=int, default=512, metavar='#',
                        help='Number of points for DFT on each LPC analysis window')

    parser.add_argument('--class', dest='class_name', metavar='name',
                        help='Only dispatch given class. '
                             'Useful when specifying selection ranges.')

    parser.add_argument('--max-selections', default=5, type=int, metavar='number',
                        help='Maximum number of selections to dispatch. '
                             'Useful when specifying selection ranges.')

    parser.add_argument('--no-plot', action='store_true', help='Just save image, do not plot')

    parser.add_argument('--title', metavar='str',
                        help='Plot title')

    parser.add_argument('--out-prefix', metavar='prefix',
                        help='Prefix to name output plot file.')

    parser.add_argument('--out-name', metavar='prefix',
                        help='Name for the output plot file.')

    args = parser.parse_args()

    return args


if __name__ == "__main__":
    # print('ARGS = {}'.format(args))
    main(parse_args())
