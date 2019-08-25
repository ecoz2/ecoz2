#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def cb_plot_cards_dists_hist(filename):
    csv = pd.read_csv(filename, comment='#')

    df = pd.DataFrame(csv)

    fig, ax1 = plt.subplots()
    df.hist(['cardinality', 'distortion'],
            ax=ax1,
            bins=100, grid=False,
            histtype='step'
            )
    fig.savefig('cb_cards_dists_hist.png', bbox_inches='tight')
    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 2:
        print('USAGE: cb.plot_cards_dists_hist.py <csv-filename>')
        exit(1)

    csv_filename = argv[1]
    cb_plot_cards_dists_hist(csv_filename)
