"""
Name: Sanjay D
ID Number: 2025AAPS0288H
SEDS BPHC - Avionics Induction: Task 1 (Finding the Sea Floor)
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

def load_and_clean_data(file_path: str) -> pd.DataFrame:
    df = pd.read_csv(file_path)
    depth_col = [c for c in df.columns if 'depth' in c.lower()][0]

    # Convert non-numeric errors (like '#VALUE!') to NaN and take absolute depth
    df['depth_clean'] = pd.to_numeric(df[depth_col], errors='coerce').abs()

    # Outlier rejection: remove severe drops (e.g. point 151 at ~1271m) and 0m dropouts
    rolling_med = df['depth_clean'].rolling(window=7, center=True, min_periods=1).median()
    is_outlier = ((df['depth_clean'] - rolling_med).abs() > 80) | (df['depth_clean'] <= 0)
    df.loc[is_outlier, 'depth_clean'] = np.nan

    # Linear interpolation for corrupted indices
    df['depth_clean'] = df['depth_clean'].interpolate(method='linear').bfill().ffill()

    # Noise reduction: 5-point moving average
    df['depth_filtered'] = df['depth_clean'].rolling(window=5, center=True, min_periods=1).mean()
    df['time_sec'] = np.arange(len(df))

    return df

df = load_and_clean_data('Depth Data.csv')

fig, ax = plt.subplots(figsize=(10, 5))
ax.set_facecolor('#f8f9fa')

# Invert Y-axis so deeper sea floor points downward 
ax.set_ylim(df['depth_clean'].max() * 1.15, 0)
ax.set_xlim(0, len(df))

ax.set_title("Odysseus Navigation: Real-Time Sea Floor Depth Profile", fontsize=12, fontweight='bold')
ax.set_xlabel("Time (seconds)", fontsize=10)
ax.set_ylabel("Depth Below Surface (m)", fontsize=10)
ax.grid(True, linestyle='--', alpha=0.5)

raw_line, = ax.plot([], [], color='#a0c4ff', linestyle=':', label='Raw / Repaired Sensor Feed')
filt_line, = ax.plot([], [], color='#003049', linewidth=2, label='Filtered Profile (Moving Avg)')
ship_marker, = ax.plot([], [], marker='v', color='#d62828', markersize=8, label="Odysseus' Ship")

ax.legend(loc='upper right')

def update(frame):
    t = df['time_sec'][:frame + 1]
    raw = df['depth_clean'][:frame + 1]
    filt = df['depth_filtered'][:frame + 1]

    raw_line.set_data(t, raw)
    filt_line.set_data(t, filt)
    ship_marker.set_data([t.iloc[-1]], [filt.iloc[-1]])

    return raw_line, filt_line, ship_marker

ani = FuncAnimation(fig, update, frames=len(df), interval=1000, blit=False, repeat=False)
plt.tight_layout()
plt.show()