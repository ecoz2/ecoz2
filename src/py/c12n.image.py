#!/usr/bin/env python3.8

import soundfile as sf
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import gridspec
from PIL import Image

from ecoz2_misc import *


def get_ranks(segments_df: pd.DataFrame,
              c12n: pd.DataFrame
              ):
    img_width = len(segments_df)

    ranks = np.full(img_width, 0)
    min_selection_number = 0
    max_selection_number = 0
    for i, c12n_row in c12n.iterrows():
        selection_number = extract_selection_number(c12n_row, 'seq_filename')
        # print('selection_number={}'.format(selection_number))
        if selection_number > 0:
            rank = c12n_row.get('rank')
            shifted = selection_number - 1
            ranks[shifted] = rank
            if i == 0 or min_selection_number > selection_number:
                min_selection_number = selection_number
            if i == 0 or max_selection_number < selection_number:
                max_selection_number = selection_number

    return ranks, min_selection_number, max_selection_number


def create_image(segments_df: pd.DataFrame,
                 c12n: pd.DataFrame,
                 args):

    img_width = len(segments_df)

    ranks = np.full(img_width, 0)
    for i, c12n_row in c12n.iterrows():
        selection_number = extract_selection_number(c12n_row, 'seq_filename')
        # print('selection_number={}'.format(selection_number))
        if selection_number > 0:
            rank = c12n_row.get('rank')
            shifted = selection_number - 1
            ranks[shifted] = rank

    # max_rank = ranks.max()
    from PIL import ImageDraw
    width1, height = 1, 4
    img = Image.new('RGBA', (width1*img_width, height), (255, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for x, rank in enumerate(ranks):
        # print('x={} rank={}'.format(x, rank))
        if rank > 0:
            x0, y0, x1, y1 = x, 0, x + width1, height
            if rank == 1:
                draw.rectangle([x0, y0, x1, y1], fill=(0, 255, 0))
            elif rank == 2:
                draw.rectangle([x0, y0, x1, y1], fill=(0, 0, 255))
            elif rank == 3:
                draw.rectangle([x0, y0, x1, y1], fill=(255, 0, 0))
            else:
                draw.rectangle([x0, y0, x1, y1], fill=(255, 100, 0))

    prefix = args.out_prefix or ''
    out_file = '{}c12n.png'.format(prefix)
    img.save(out_file, 'PNG')


def plot_spectrogram_and_c12n(signal: Signal,
                              interval: np.ndarray,
                              args):
    fig = plt.figure(figsize=(16, 4))

    gs = gridspec.GridSpec(2, 1, height_ratios=[7, 1])
    index = 0

    ax = plt.subplot(gs[index])
    index += 1
    plot_spectrogram(interval, signal.sample_rate, ax)

    ax = plt.subplot(gs[index])
    index += 1

    prefix = args.out_prefix or ''
    out_file = '{}c12n.png'.format(prefix)
    plt.imshow(plt.imread(out_file))

    plt.tight_layout()
    plt.show()


def parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description='Generated classification image',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--signal', metavar='wav', required=True,
                        help='Sound file to associate results to.')

    parser.add_argument('--max-seconds',
                        help='Max seconds to visualize')

    parser.add_argument('--segments', required=True,
                        help='CSV file with segments.')

    parser.add_argument('--c12n', required=True,
                        help='CSV file classification results.')

    parser.add_argument('--out-prefix',
                        help='Prefix to name output plot file.')

    args = parser.parse_args()

    return args


def main(args):
    segments_df = load_csv(args.segments, sep='\t')
    c12n = load_csv(args.c12n)

    selections_and_c12n = get_selections_and_c12n(segments_df, c12n)
    if not len(selections_and_c12n):
        print('No selections')
        return

    # get complete signal interval from min to max selection:
    min_selection = None
    max_selection = None
    for s, c in selections_and_c12n:
        if min_selection is None or min_selection.selection > s.selection:
            min_selection = s
        if max_selection is None or max_selection.selection < s.selection:
            max_selection = s
    print('min_selection = {}'.format(min_selection))
    print('max_selection = {}'.format(max_selection))
    tot_duration_s = max_selection.end_time_s - min_selection.begin_time_s
    print('tot_duration_s = {}'.format(tot_duration_s))

    signal = load_signal(args.signal)

    interval = get_signal_interval_for_min_max_selections(signal,
                                                          min_selection,
                                                          max_selection,
                                                          max_seconds=args.max_seconds,
                                                          )

    plot_spectrogram_and_c12n(signal, interval, args)


if __name__ == "__main__":
    main(parse_args())
