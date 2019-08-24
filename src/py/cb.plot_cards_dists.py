#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def cb_plot_cards_dists(filename):
    csv = pd.read_csv(filename, comment='#')

    df = pd.DataFrame(csv)
    # print(df)
    index = df['index']

    fig, ax1 = plt.subplots()

    color = 'blue'
    ax1.set_xlabel('Cell index')
    ax1.set_ylabel('Cardinality', color=color)
    ax1.plot(index, df['cardinality'], color=color)
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()

    color = 'red'
    ax2.set_ylabel('Distortion', color=color)
    ax2.plot(index, df['distortion'], color=color, linestyle=':')
    # ax2.scatter(index, df['distortion'], color=color)
    ax2.tick_params(axis='y', labelcolor=color)

    fig.tight_layout()
    plt.title(filename)

    fig.savefig('cb_cards_dists.png', bbox_inches='tight')
    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 2:
        print('USAGE: cb.plot_cards_dists.py <csv-filename>')
        exit(1)

    csv_filename = argv[1]
    cb_plot_cards_dists(csv_filename)
