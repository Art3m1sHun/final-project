import matplotlib
matplotlib.use("TkAgg")

import sqlite3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import threading
import time

#--------------------------------
# CONFIG
#--------------------------------
WINDOW_SIZE = 500       # số điểm hiển thị
FETCH_INTERVAL = 0.05   # fetch DB mỗi 50ms (20Hz) — Pi không kham nổi hơn
PLOT_INTERVAL = 50      # vẽ mỗi 50ms

#--------------------------------
# SHARED BUFFER (thread-safe)
#--------------------------------
ecg_buf = deque(maxlen=WINDOW_SIZE)
ppg_buf = deque(maxlen=WINDOW_SIZE)
buf_lock = threading.Lock()
last_id = 0  # chỉ fetch data MỚI, không fetch lại cũ

#--------------------------------
# DB FETCH THREAD (tách riêng khỏi UI)
#--------------------------------
def fetch_thread():
    global last_id

    # Mở connection 1 lần duy nhất, không đóng mở liên tục
    conn = sqlite3.connect("current_session.db",
                           check_same_thread=False)
    conn.execute("PRAGMA journal_mode=WAL")   # tránh lock với gateway
    conn.execute("PRAGMA cache_size=1000")

    while True:
        try:
            cur = conn.execute("""
                SELECT id, timestamp, ecg, ppg
                FROM sensor_data
                WHERE id > ?
                ORDER BY id ASC
                LIMIT 200
            """, (last_id,))

            rows = cur.fetchall()

            if rows:
                with buf_lock:
                    for row in rows:
                        ecg_buf.append(row[2])
                        ppg_buf.append(row[3])
                    last_id = rows[-1][0]  # cập nhật ID mới nhất

        except Exception as e:
            print(f"DB error: {e}")

        time.sleep(FETCH_INTERVAL)

#--------------------------------
# FIGURE
#--------------------------------
fig, (ax_ecg, ax_ppg) = plt.subplots(2, 1, figsize=(10, 6))
fig.patch.set_facecolor('white')

# --- ECG axes ---
ax_ecg.set_facecolor('white')
ax_ecg.set_title("ECG", color='black')
ax_ecg.set_ylabel("Amplitude", color='black')
ax_ecg.tick_params(colors='black')
ax_ecg.set_xlim(0, WINDOW_SIZE)
ax_ecg.grid(False)

# --- PPG axes ---
ax_ppg.set_facecolor('white')
ax_ppg.set_title("PPG", color='black')
ax_ppg.set_ylabel("Amplitude", color='black')
ax_ppg.set_xlabel("Sample", color='black')
ax_ppg.tick_params(colors='black')
ax_ppg.set_xlim(0, WINDOW_SIZE)
ax_ppg.grid(False)

line_ecg, = ax_ecg.plot([], [], color='red', lw=1)
line_ppg, = ax_ppg.plot([], [], color='green', lw=1)

#--------------------------------
# UPDATE
#--------------------------------
def update(frame):
    with buf_lock:
        if len(ecg_buf) < 2:
            return line_ecg, line_ppg

        ecg = np.array(ecg_buf)
        ppg = np.array(ppg_buf)

    x = np.arange(len(ecg))

    line_ecg.set_data(x, ecg)
    line_ppg.set_data(x, ppg)

    ax_ecg.set_ylim(ecg.min() - 0.5, ecg.max() + 0.5)
    ax_ppg.set_ylim(ppg.min() - 0.5, ppg.max() + 0.5)

    return line_ecg, line_ppg

#--------------------------------
# KHỞI ĐỘNG FETCH THREAD
#--------------------------------
t = threading.Thread(target=fetch_thread, daemon=True)
t.start()

#--------------------------------
# ANIMATION — bật blit để chỉ vẽ lại phần thay đổi
#--------------------------------
ani = FuncAnimation(fig,
                    update,
                    interval=PLOT_INTERVAL,
                    blit=True,              # KEY: chỉ redraw lines, không redraw axes
                    cache_frame_data=False)

plt.tight_layout()
plt.show()