#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def cb_plot_codeword_hist(filename, df):
    cb_size = df['codeword'].max() + 1
    fig, ax1 = plt.subplots()
    df.hist(['codeword'],
            ax=ax1,
            bins=cb_size,
            grid=False,
            histtype='step'
            )
    plt.show(block=True)
    fig.savefig('cb_codeword_hist_M_%04d.png' % cb_size,
                bbox_inches='tight')


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Plot codeword histogram')
    parser.add_argument('filename')

    args = parser.parse_args()

    df = pd.read_csv(args.filename, comment='#')

    print('codeword value_counts:')
    print(df['codeword'].value_counts())

    cb_plot_codeword_hist(args.filename, df)
