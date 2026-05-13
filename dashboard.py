import sqlite3
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

x = []
y = []

fig, ax = plt.subplots()

def update(frame):

    conn = sqlite3.connect("sensor.db")

    cur = conn.cursor()

    cur.execute("""
        SELECT value
        FROM sensor_data
        WHERE sensor_type='ECG'
        ORDER BY id DESC
        LIMIT 50
    """)

    rows = cur.fetchall()

    values = [r[0] for r in rows]

    ax.clear()

    ax.plot(values)

ani = FuncAnimation(fig,
                    update,
                    interval=1000)

plt.show()