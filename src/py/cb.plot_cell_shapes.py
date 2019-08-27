#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def read_csv(filename):
    csv = pd.read_csv(filename, comment='#')
    df = pd.DataFrame(csv)
    # print(df)
    return df


def cb_plot_cell_shapes(df_training_k, df_training_d, df_codebook_k):
    tsz = len(df_training_k)
    print('training points = %s' % tsz)
    assert tsz == len(df_training_d)

    min_dist_threshold = 0.001

    kd = pd.concat([df_training_k, df_training_d], axis=1, sort=False)
    print(kd)

    in_cell = True
    # in_cell = False
    # kd = kd.loc[(kd['codeword'] == 5) | (kd['codeword'] == 99)]
    if in_cell:
        kd = kd.loc[kd['minDistortion'] <= min_dist_threshold]
    else:
        kd = kd.loc[kd['minDistortion'] > min_dist_threshold]

    tk1 = kd['k1']
    tk2 = kd['k2']
    tk = [tk1, tk2]

    fig = plt.figure()

    ax = fig.add_subplot(111)

    ax.set_autoscaley_on(False)
    ax.set_xlim([-1, +1])
    ax.set_ylim([-1, +1])

    ax.set_xlabel('$k_1$')
    ax.set_ylabel('$k_2$')

    color = 'lightgrey'
    ax.scatter(*tk, color=color, marker='.')

    csz = len(df_codebook_k)
    ck1 = df_codebook_k['k1']
    ck2 = df_codebook_k['k2']
    ck = [ck1, ck2]
    # ck = [[ck1[5], ck1[99]], [ck2[5], ck2[99]]]

    color = 'darkred'
    ax.scatter(*ck, color=color)
    print('codebook points = %s' % csz)

    fig.tight_layout()
    plt.title('$k_1$ vs. $k_2$ Cell Shapes')

    filename = 'cell_shapes_%s_training_%d_codebook_%d' % ('in' if in_cell else 'out', tsz, csz)
    filename += '.png'
    fig.savefig(filename, bbox_inches='tight')
    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 4:
        print('USAGE: cb.plot_cell_shapes.py <training-reflections> <training-min-distortions> <codebook-reflections>')
        exit(1)

    training_k = read_csv(argv[1])
    training_d = read_csv(argv[2])
    codebook_k = read_csv(argv[3])

    cb_plot_cell_shapes(training_k, training_d, codebook_k)
