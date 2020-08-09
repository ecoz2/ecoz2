#!/usr/bin/env python3.7
# coding=utf-8
from math import ceil, floor

import librosa
from librosa import display

import numpy as np
import pandas as pd
import soundfile as sf
from collections import namedtuple
import re


def load_csv(filename: str, sep=',') -> pd.DataFrame:
    # print('- loading %s' % filename)
    csv = pd.read_csv(filename, comment='#', sep=sep)
    # print(csv)
    return csv


Signal = namedtuple('Signal', [
    'filename',
    'info',
    'sample_rate',
    'tot_samples',
    'tot_duration_ms',
])


def load_signal(filename: str) -> Signal:
    print('- loading %s' % filename)
    info = sf.info(filename)
    sample_rate = info.samplerate
    tot_samples = int(info.duration * sample_rate)
    tot_duration_ms = 1000 * info.duration
    return Signal(
        filename,
        info,
        sample_rate,
        tot_samples,
        tot_duration_ms)


def show_signal(signal: Signal):
    print('%s\n tot_duration: %s  tot_samples: %s  sample_rate: %s\n' % (
        signal.filename,
        signal.info.duration,
        signal.tot_samples,
        signal.sample_rate,
    ))


def plot_spectrogram(interval: np.ndarray,
                     sample_rate,
                     ax,
                     window_size=1024,
                     window_offset=512,
                     fmin=0,
                     fmax=16000,
                     cmap=None,
                     ):
    # ax.xaxis.tick_top()
    # ax.xaxis.set_label_position('top')

    def compute_stft():
        stft = np.abs(librosa.stft(y=interval,
                                   n_fft=window_size,
                                   hop_length=window_offset))
        return librosa.amplitude_to_db(stft, ref=np.max)

    def spectrogram(stft):
        display.specshow(stft, y_axis='mel', x_axis='time',
                         sr=sample_rate,
                         cmap=cmap or 'Blues',
                         fmin=fmin, fmax=fmax,
                         ax=ax)

    spectrogram(compute_stft())


def short_term_lpc(y: np.ndarray,
                   sample_rate: float,
                   window_size: int,
                   window_offset: int,
                   prediction_order: int,
                   ):
    """Short-term LPC"""

    def windows():
        num_windows = 1 + (len(y) - window_size) // window_offset
        print('num_windows = {}'.format(num_windows))
        for w in range(0, num_windows * window_offset, window_offset):
            yield y[w: w + window_size]

    def lpc(window):
        return librosa.lpc(window, prediction_order)

    def lpc_and_envelope(window):
        lpc_coeffs = lpc(window)
        abs = np.abs(np.fft.rfft(a=lpc_coeffs, n=1024))
        ft = librosa.amplitude_to_db(abs, ref=np.max)
        return ft

    res = np.array([lpc_and_envelope(window) for window in windows()])
    return np.transpose(res)


def plot_lpc_spectrogram(interval: np.ndarray,
                         sample_rate,
                         prediction_order,
                         ax,
                         window_size=1024,
                         window_offset=512,
                         cmap=None
                         ):

    lpcs = short_term_lpc(y=interval,
                          sample_rate=sample_rate,
                          window_size=window_size,
                          window_offset=window_offset,
                          prediction_order=prediction_order)

    ax.imshow(lpcs,
              origin='lower',
              aspect='auto',
              cmap=cmap or 'pink',
              )


def extract_selection_number(c12n_row: pd.core.series.Series,
                             column_name: str
                             ):
    # eg, "data/sequences/M256/M/01971.seq"
    #                            ^^^^^
    col_value = c12n_row.get(column_name)
    match = re.search(r'([^/]+)\.seq$', col_value)
    if match:
        return int(match.group(1))
    else:
        return -1


Selection = namedtuple('Selection', [
    'selection',
    'view',
    'channel',
    'begin_time_s',
    'end_time_s',
    'low_freq_hz',
    'high_freq_hz',
    'type_',
])


def get_selection(segments_df: pd.DataFrame, selection_number: int) -> Selection:
    rows = segments_df.loc[segments_df['Selection'] == selection_number]
    if not rows.empty:
        row = rows.iloc[0, :]
        return Selection(
            row.get('Selection'),
            row.get('View'),
            row.get('Channel'),
            row.get('Begin Time (s)'),
            row.get('End Time (s)'),
            row.get('Low Freq (Hz)'),
            row.get('High Freq (Hz)'),
            row.get('Type'),
        )


def get_selections_and_c12n(segments_df: pd.DataFrame,
                            c12n: pd.DataFrame,
                            desired_rank=None,
                            desired_class_name=None
                            ) -> [(Selection, pd.core.series.Series)]:
    selections_and_c12n = []
    for i, c12n_row in c12n.iterrows():
        selection_number = extract_selection_number(c12n_row, 'seq_filename')
        if selection_number < 0:
            continue

        if desired_rank is not None:
            if int(desired_rank) != c12n_row.get('rank'):
                continue

        if desired_class_name is not None:
            if desired_class_name != c12n_row.get('seq_class_name'):
                continue

        selection = get_selection(segments_df, selection_number)
        selections_and_c12n.append((selection, c12n_row))

    return selections_and_c12n


def get_signal_interval(signal, start_time_ms, duration_ms) -> np.ndarray:
    start_sample = floor(start_time_ms * signal.sample_rate / 1000)
    num_samples = ceil(duration_ms * signal.sample_rate / 1000)

    # print('- loading %s samples starting at %s' % (num_samples, start_sample))
    samples, _ = sf.read(signal.filename, start=start_sample, frames=num_samples)
    # print('  loaded %s samples\n' % len(samples))
    return samples


def get_signal_interval_from_selection(signal: Signal,
                                       selection: Selection
                                       ) -> np.ndarray or None:
    start_time_ms = 1000.0 * selection.begin_time_s
    end_time_ms = 1000.0 * selection.end_time_s
    duration_ms = end_time_ms - start_time_ms

    if start_time_ms + duration_ms < signal.tot_duration_ms:
        return get_signal_interval(signal, start_time_ms, duration_ms)
    else:
        print('WARN: interval beyond signal length')
        return None


SignalInterval = namedtuple('SignalInterval', [
    'interval',       # np.ndarray
    'begin_time_s',   # float
    'duration_s',     # float
])


def get_signal_interval_for_min_max_selections(signal: Signal,
                                               min_selection: Selection,
                                               max_selection: Selection,
                                               max_seconds: float
                                               ) -> SignalInterval:
    max_ms = 1000 * max_seconds

    start_time_ms = 1000.0 * min_selection.begin_time_s
    end_time_ms = 1000.0 * max_selection.end_time_s
    duration_ms = end_time_ms - start_time_ms

    if duration_ms > max_ms:
        print('applying max_seconds={}'.format(max_seconds))
        duration_ms = max_ms

    if start_time_ms + duration_ms < signal.tot_duration_ms:
        interval = get_signal_interval(signal, start_time_ms, duration_ms)
        return SignalInterval(interval,
                              min_selection.begin_time_s,
                              duration_ms / 1000.
                              )
    else:
        print('WARN: interval beyond signal length')
        exit(1)
