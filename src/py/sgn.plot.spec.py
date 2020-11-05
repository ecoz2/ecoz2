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

from ecoz2_misc import \
    Signal, Selection, \
    load_signal, show_signal, load_csv, \
    get_selection, get_signal_interval_from_selection, \
    plot_spectrogram, plot_lpc_spectrogram


def do_plot(signal: Signal,
            interval: np.ndarray,
            title: str,
            out_file: str or None,
            args,
            ):

    fig = plt.figure(figsize=(9, 9))

    if args.lpc:
        gs = gridspec.GridSpec(2, 1, height_ratios=[1, 1])
    else:
        gs = gridspec.GridSpec(1, 1, height_ratios=[1])

    index = 0

    ax = plt.subplot(gs[index])

    window_size = 1024
    window_offset = 512

    plot_spectrogram(interval,
                     signal.sample_rate,
                     ax,
                     window_size=window_size,
                     window_offset=window_offset,
                     cmap=args.cmap
                     )

    plt.title('Spectrogram (${}/{}$)'.format(window_size, window_offset))

    if args.lpc:
        index += 1
        ax = plt.subplot(gs[index])
        plot_lpc_spectrogram(interval,
                             signal.sample_rate,
                             args.lpc,
                             ax,
                             window_size=window_size,
                             window_offset=window_offset,
                             cmap=args.cmap
                             )
        plt.title('LPC Spectrogram ($P = {}$)'.format(args.lpc))
        # TODO plot_vertical_lines

    else:
        plt.title(title)

    plt.tight_layout()
    plt.subplots_adjust(hspace=0.3)
    plt.show()
    if out_file:
        print('Saving {}'.format(out_file))
        fig.savefig(out_file, dpi=120)


def dispatch_selection(signal: Signal,
                       interval: np.ndarray,
                       selection: Selection,
                       args):

    title = 'Selection number: {}'.format(selection.selection)

    out_file = None
    if args.out_prefix:
        out_file = '{}sgn.plot.spec_{}.png'.format(
            args.out_prefix, selection.selection)

    do_plot(signal,
            interval,
            title,
            out_file,
            args
            )


def dispatch_individual_selections(signal: Signal,
                                   segments_df: pd.DataFrame,
                                   selection_numbers,
                                   args):
    for i, selection_number in enumerate(selection_numbers):
        print(" {:3} sel={}".format(i, selection_number))

        selection = get_selection(segments_df, selection_number)
        print('selection = {}'.format(selection))

        if selection:
            interval = get_signal_interval_from_selection(signal, selection)
            if interval is not None:
                dispatch_selection(signal, interval, selection, args)



def main(args):
    signal = load_signal(args.signal)
    show_signal(signal)

    segments_df = load_csv(args.segments, sep='\t')

    selection_numbers = [int(sn) for sn in args.selections]

    dispatch_individual_selections(signal, segments_df, selection_numbers, args)


def parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description='Plot classification results',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--signal', metavar='wav',
                        help='Associated sound file (spectrogram of which is shown).')

    parser.add_argument('--segments', required=True, metavar='file',
                        help='CSV file with segments.')

    parser.add_argument('--selections', nargs='+',
                        help='Selection numbers to display')

    parser.add_argument('--lpc', type=int, metavar='P',
                        help='Order of prediction to display LPC spectra')

    parser.add_argument('--cmap', type=str, metavar='cm',
                        help='Name of color map for spectra')

    parser.add_argument('--delta-begin-seconds', metavar='secs', default=0,
                        help='Increment from first selection time to visualize (default, 0).')

    parser.add_argument('--max-seconds', metavar='secs', default=3 * 60,
                        help='Max seconds to visualize (default, 3min).')

    parser.add_argument('--msfd', default=100, type=int, metavar='number',
                        help='Maximum number of selections for detailed plot (default 100)')

    parser.add_argument('--out-prefix', metavar='prefix',
                        help='Prefix to name output plot file.')

    args = parser.parse_args()

    return args


if __name__ == "__main__":
    # print('ARGS = {}'.format(args))
    main(parse_args())
