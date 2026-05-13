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
# FIGURE — tắt mọi thứ không cần
#--------------------------------
fig, ax = plt.subplots(figsize=(10, 4))
fig.patch.set_facecolor('black')
ax.set_facecolor('black')

# Dùng màu tương phản cao để render nhanh hơn
line_ecg, = ax.plot([], [], color='lime',  lw=1, label="ECG")
line_ppg, = ax.plot([], [], color='cyan',  lw=1, label="PPG")

ax.set_xlabel("Sample", color='white')
ax.set_ylabel("Amplitude", color='white')
ax.set_title("Realtime ECG + PPG", color='white')
ax.tick_params(colors='white')
ax.legend(facecolor='gray')
ax.grid(False)  # tắt grid — tốn CPU trên Pi

# Pre-set xlim cố định, không tính lại mỗi frame
ax.set_xlim(0, WINDOW_SIZE)

#--------------------------------
# UPDATE — chỉ set_data, không redraw axes
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

    # Chỉ update ylim khi cần (không gọi mỗi frame)
    ymin = min(ecg.min(), ppg.min())
    ymax = max(ecg.max(), ppg.max())
    ax.set_ylim(ymin - 0.5, ymax + 0.5)

    return line_ecg, line_ppg  # blit=True cần return artists

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