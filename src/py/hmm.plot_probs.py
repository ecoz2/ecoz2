#!/usr/bin/env python3
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def plot(iteration, metric, metric_label, xlabel=None, title=None):
    plt.plot(iteration, metric, linestyle='--')
    plt.scatter(iteration, metric)
    if xlabel:
        plt.xlabel(xlabel)
    plt.ylabel(metric_label)
    if title:
        plt.title(title)
    # plt.grid(True)


def plot_csv(filename):
    df = pd.read_csv(filename, comment='#')
    print(df)
    iteration = df['I']
    sum_log_prob = df['Σ log(P)']

    fig = plt.figure()

    plt.subplot(111)
    plot(iteration, sum_log_prob, r'$\sum\log(P)$',
         title='HMM training',
         xlabel='Iteration')

    fig.savefig('hmm_probs.png', bbox_inches='tight')
    # fig.savefig('hmm_probs.pdf', bbox_inches='tight')

    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 2:
        print('USAGE: hmm.plot_probs.py <csv-filename>')
        exit(1)

    csv_filename = argv[1]
    plot_csv(csv_filename)
