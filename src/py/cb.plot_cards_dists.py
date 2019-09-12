#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def cb_plot_cards_dists_twinx(filename, df):
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
    plt.show(block=True)

    fig.savefig('cb_cards_dists.png', bbox_inches='tight')


def cb_plot_cards_dists_scatter(filename, df):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_xlabel('Distortion')
    ax.set_ylabel('Cardinality')
    ax.scatter(df['distortion'], df['cardinality'], color='black', alpha=.3, marker='o')
    fig.tight_layout()
    plt.title(filename)
    plt.show(block=True)
    fig.savefig('cb_cards_dists_scatter.png')


def cb_plot_cards_dists_hist(filename, df):
    fig, ax1 = plt.subplots()
    df.hist(['cardinality', 'distortion'],
            ax=ax1,
            bins=100, grid=False,
            histtype='step'
            )
    plt.show(block=True)
    fig.savefig('cb_cards_dists_hist.png', bbox_inches='tight')


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Plot cell cardinality/distortion.')
    parser.add_argument('--scatter', action='store_true', help='Scatter plot')
    parser.add_argument('--hist', action='store_true', help='Histograms')
    parser.add_argument('filename')

    args = parser.parse_args()

    df = pd.read_csv(args.filename, comment='#')
    # print(df)

    if args.scatter:
        cb_plot_cards_dists_scatter(args.filename, df)
    elif args.hist:
        cb_plot_cards_dists_hist(args.filename, df)
    else:
        cb_plot_cards_dists_twinx(args.filename, df)
