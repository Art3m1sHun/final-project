import matplotlib

matplotlib.use("TkAgg")

import sqlite3
import pandas as pd

import matplotlib.pyplot as plt

from matplotlib.animation import FuncAnimation

#--------------------------------
# FIGURE
#--------------------------------

fig, ax = plt.subplots(figsize=(12,5))

line_ecg, = ax.plot([], [],
                    label="ECG")

line_ppg, = ax.plot([], [],
                    label="PPG")

ax.set_xlabel("Time (s)")
ax.set_ylabel("Amplitude")

ax.set_title("Realtime ECG + PPG")

ax.legend()

ax.grid(True)

#--------------------------------
# UPDATE
#--------------------------------

def update(frame):

    conn = sqlite3.connect("current_session.db")

    query = """
    SELECT timestamp, ecg, ppg
    FROM sensor_data
    ORDER BY id DESC
    LIMIT 200
    """

    df = pd.read_sql_query(query, conn)

    conn.close()

    if len(df) == 0:
        return

    df = df.iloc[::-1]

    t = df["timestamp"].astype(float)

    ecg = df["ecg"]

    ppg = df["ppg"]

    line_ecg.set_data(t, ecg)

    line_ppg.set_data(t, ppg)

    ax.set_xlim(t.min(),
                t.max())

    ymin = min(ecg.min(),
               ppg.min())

    ymax = max(ecg.max(),
               ppg.max())

    ax.set_ylim(ymin - 0.1,
                ymax + 0.1)

#--------------------------------
# ANIMATION
#--------------------------------

ani = FuncAnimation(fig,
                    update,
                    interval=500,
                    cache_frame_data=False)

plt.show()