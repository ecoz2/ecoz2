#!/usr/bin/env python3.7
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def read_csv(filename):
    csv = pd.read_csv(filename, comment='#')
    df = pd.DataFrame(csv)
    # print(df)
    return df


def cb_plot_reflections(df_training, df_codebook):
    max_training_points = 8000.

    tsz = len(df_training)
    print('df_training points = %s' % tsz)
    if max_training_points is not None and tsz > max_training_points:
        df_training = df_training.sample(frac=max_training_points/tsz)
        tsz = len(df_training)
        print('df_training plotted points = %s' % tsz)
    tk1 = df_training['k1']
    tk2 = df_training['k2']
    tk = [tk1, tk2]
    tk3 = None

    fig = plt.figure()

    if 'k3' in df_training:
        # This import registers the 3D projection, but is otherwise unused.
        from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 unused import

        filename = 'cb_kkk_training_%d' % tsz
        ax = fig.add_subplot(111, projection='3d')
        tk3 = df_training['k3']
        tk.append(tk3)
        ax.set_zlabel('$k_3$')
    else:
        filename = 'cb_kk_training_%d' % tsz
        ax = fig.add_subplot(111)

    ax.set_xlabel('$k_1$')
    ax.set_ylabel('$k_2$')

    color = 'lightgrey'
    ax.scatter(*tk, color=color, marker='.')

    if df_codebook is not None:
        csz = len(df_codebook)
        ck1 = df_codebook['k1']
        ck2 = df_codebook['k2']
        ck = [ck1, ck2]
        if 'k3' in df_codebook:
            ck3 = df_codebook['k3']
            ck.append(ck3)

        color = 'darkred'
        ax.scatter(*ck, color=color)
        print('df_codebook points = %s' % csz)
        filename += '_codebook_%d' % csz

    fig.tight_layout()
    plt.title('$k_1$ vs. $k_2$' + (' vs. $k_3$' if tk3 is not None else ''))

    filename += '.png'
    fig.savefig(filename, bbox_inches='tight')
    plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 2:
        print('USAGE: cb.plot_evaluation.py <training-csv> [ <codebook-csv> ]')
        exit(1)

    df_training = read_csv(argv[1])
    df_codebook = read_csv(argv[2]) if len(argv) > 2 else None

    cb_plot_reflections(df_training, df_codebook)
