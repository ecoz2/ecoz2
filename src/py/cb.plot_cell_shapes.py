#!/usr/bin/env python3
# coding=utf-8
import pandas as pd
import matplotlib.pyplot as plt


def read_csv(filename):
    df = pd.read_csv(filename, comment='#')
    # print(df)
    return df


def cb_plot_cell_shapes(kd, df_codebook_k,
                        min_dist_threshold,
                        in_cell
                        ):
    tsz = len(kd)

    if in_cell:
        kd = kd.loc[kd['minDistortion'] <= min_dist_threshold]
    else:
        kd = kd.loc[kd['minDistortion'] > min_dist_threshold]

    tk1 = kd['k1']
    tk2 = kd['k2']
    tk = [tk1, tk2]

    fig = plt.figure()

    ax = fig.add_subplot(111)

    ax.set_aspect(aspect='equal')
    ax.set_autoscale_on(False)
    ax.set_xlim([-0.25, -0.1])
    ax.set_ylim([0.25, 0.4])
    # ax.set_xlim([-1, +1])
    # ax.set_ylim([-1, +1])

    ax.set_xlabel('$k_1$')
    ax.set_ylabel('$k_2$')

    color = 'grey'
    ax.scatter(*tk, color=color, marker='.')

    csz = len(df_codebook_k)
    ck1 = df_codebook_k['k1']
    ck2 = df_codebook_k['k2']
    ck = [ck1, ck2]

    color = 'red'
    ax.scatter(*ck, color=color, marker='.')  # , s=4)

    plt.grid(color='blue')

    fig.tight_layout()
    plt.title('Cell shapes'
              + '. minDistortion %s %s' % (
                  '<' if in_cell else '>', min_dist_threshold
              ))

    filename = 'cell_shapes_minDist_%s_%s_training_%d_codebook_%d' % (
        'le' if in_cell else 'gt', min_dist_threshold,
        tsz, csz
    )
    filename += '.png'
    fig.savefig(filename, bbox_inches='tight')
    # plt.show(block=True)


if __name__ == "__main__":
    from sys import argv
    if len(argv) < 5:
        print('USAGE: cb.plot_cell_shapes.py <training-reflections> ' +
              '<training-min-distortions> <codebook-reflections> <min_dist_threshold>')
        exit(1)

    training_k = read_csv(argv[1])
    training_d = read_csv(argv[2])
    codebook_k = read_csv(argv[3])
    min_dist_threshold = float(argv[4])

    tsz = len(training_k)
    print('training points = %s' % tsz)
    assert tsz == len(training_d)

    csz = len(codebook_k)
    print('codebook points = %s' % csz)

    kd = pd.concat([training_k, training_d], axis=1, sort=False)
    print(kd)

    for in_cell in [True, False]:
        cb_plot_cell_shapes(kd, codebook_k,
                            min_dist_threshold=min_dist_threshold,
                            in_cell=in_cell
                            )
