#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def plot(sizes, metric, metric_label, xlabel=None, title=None):
    plt.plot(sizes, metric, linestyle='--')
    plt.scatter(sizes, metric)
    plt.xscale('log')
    plt.xticks(sizes, sizes)
    if xlabel:
        plt.xlabel(xlabel)
    plt.ylabel(metric_label)
    if title:
        plt.title(title)
    # plt.grid(True)


def plot_csv(filename):
    df = pd.read_csv(filename, comment='#')
    print(df)
    sizes = df['M']

    fig = plt.figure()

    plt.subplot(311)
    plot(sizes, df['DDprm'], 'Distortion', title='Codebook Evaluation')

    plt.subplot(312)
    plot(sizes, df['σ'], r'$\sigma$ ratio')

    plt.subplot(313)
    plot(sizes, df['inertia'], 'Inertia', xlabel='Codebook size (log scale)')

    fig.savefig('cb_evaluation.png', bbox_inches='tight')
    # fig.savefig('cb_evaluation.pdf', bbox_inches='tight')

    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 2:
        print('USAGE: cb.plot_evaluation.py <csv-filename>')
        exit(1)

    csv_filename = argv[1]
    plot_csv(csv_filename)
