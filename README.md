# Real-Time ECG & PPG Acquisition Gateway

A multi-threaded biomedical signal acquisition system written in C for Linux-based embedded platforms.

This project acquires **ECG** and **PPG** signals from an ADS1115 ADC over I2C, streams data through TCP sockets, processes the signals in real time, stores them into SQLite, and visualizes the waveforms using Python.

---

# Features

* Real-time ECG and PPG acquisition
* ADS1115 I2C communication
* TCP client/server architecture
* Multi-threaded gateway using POSIX threads
* Thread-safe shared queue implementation
* Real-time signal filtering
* SQLite database storage
* Session database export
* FIFO-based logging system
* Live waveform visualization using Matplotlib
* Designed for Raspberry Pi / embedded Linux environments

---

# System Architecture

```text
+-------------------+
| ADS1115 ADC       |
| ECG / PPG Inputs  |
+---------+---------+
          |
          | I2C
          v
+-------------------+
| read_sensor.c     |
| Sensor Client     |
+---------+---------+
          |
          | TCP Socket
          v
+-------------------+
| main.c            |
| Gateway Server    |
+---------+---------+
          |
          +------------------+
          |                  |
          v                  v
+----------------+   +----------------+
| database.c     |   | log.c          |
| SQLite Storage |   | FIFO Logging   |
+----------------+   +----------------+
          |
          v
+-------------------+
| current_session.db|
+-------------------+
          |
          v
+-------------------+
| plot_data.py      |
| Real-time Plotter |
+-------------------+
```

---

# Project Structure

```text
.
├── main.c              # Gateway server and processing pipeline
├── read_sensor.c       # ADS1115 sensor acquisition client
├── sensor_list.c       # Thread-safe linked-list queue
├── sensor_list.h
├── database.c          # SQLite database operations
├── database.h
├── log.c               # FIFO logging system
├── log.h
├── connection.h        # Socket connection structures
├── plot_data.py        # Real-time ECG/PPG visualization
├── current_session.db  # Active SQLite database
└── sessions/           # Exported database sessions
```

---

# Technologies Used

## Languages

* C
* Python

## Libraries

### C Libraries

* pthread
* sqlite3
* POSIX socket API
* Linux I2C interface

### Python Libraries

* matplotlib
* numpy
* sqlite3

---

# How It Works

## 1. Sensor Acquisition

`read_sensor.c` reads ECG and PPG analog signals from the ADS1115 ADC using the Linux I2C interface.

### ADC Configuration

* ADS1115 address: `0x48`
* Sampling rate: `860 SPS`
* Single-shot conversion mode

The client continuously:

1. Reads ECG from channel AIN0
2. Reads PPG from channel AIN1
3. Sends data to the gateway server via TCP

Example packet:

```text
0.824512 1.104335
```

---

## 2. Gateway Server

`main.c` acts as the central processing gateway.

Responsibilities:

* Accept sensor client connections
* Receive ECG/PPG samples
* Push data into a thread-safe queue
* Apply lightweight digital filtering
* Store filtered data into SQLite
* Handle logging and graceful shutdown

### Multi-threading

The gateway uses POSIX threads:

| Thread            | Responsibility             |
| ----------------- | -------------------------- |
| Connection Thread | Accept sensor connections  |
| Sensor Thread     | Receive sensor packets     |
| Data Thread       | Filter and process samples |
| Database Thread   | Store data into SQLite     |
| Logger Process    | Handle asynchronous logs   |

---

## 3. Signal Filtering

Simple low-pass smoothing filters are applied:

### ECG Filter



### PPG Filter



These filters reduce high-frequency noise while preserving waveform shape.

---

## 4. Database Storage

`database.c` manages SQLite storage.

### Database Schema

```sql
CREATE TABLE sensor_data(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    ecg REAL NOT NULL,
    ppg REAL NOT NULL
);
```

Features:

* WAL mode enabled for concurrent reads/writes
* Session export support
* Timestamped acquisition records

Exported sessions are automatically saved in:

```text
sessions/session_YYYYMMDD_HHMMSS.db
```

---

## 5. Logging System

The project implements asynchronous FIFO-based logging.

### Logged Events

* Sensor connections
* Sensor disconnections
* Database exports
* Shutdown events
* SQLite errors

This avoids blocking the acquisition pipeline during file I/O.

---

## 6. Real-Time Visualization

`plot_data.py` continuously reads new records from SQLite and visualizes ECG and PPG signals.

Features:

* Real-time plotting
* Separate ECG/PPG graphs
* Sliding window visualization
* WAL-mode compatible database access
* Thread-safe data buffering

---

# Build Instructions

## Requirements

### Linux Packages

```bash
sudo apt update
sudo apt install gcc sqlite3 libsqlite3-dev python3 python3-pip
```

### Python Packages

```bash
pip install matplotlib numpy
```

---

# Compile

## Gateway Server

```bash
gcc main.c sensor_list.c database.c log.c \
    -o gateway \
    -lpthread -lsqlite3
```

## Sensor Client

```bash
gcc read_sensor.c -o sensor -lm
```

---

# Run

## 1. Create file 

```bash
makefile
```

## 2. Start system

```bash
./bp-monitoring.sh
```


# Database Example

Example sensor record:

| id | timestamp   | ecg  | ppg  |
| -- | ----------- | ---- | ---- |
| 1  | 0.000001370 | 0.82 | 1.10 |

The timestamp is measured using monotonic clock timing for precise acquisition intervals.

---

# Embedded Linux Concepts Demonstrated

This project demonstrates practical usage of:

* POSIX threads
* Mutexes and condition variables
* TCP/IP socket programming
* SQLite integration
* Linux FIFO communication
* Linux I2C device interface
* Producer-consumer architecture
* Real-time data streaming
* Concurrent processing pipelines

---

# Possible Future Improvements

* FFT signal analysis
* BPM / heart rate detection
* Digital band-pass filtering
* MQTT or WebSocket streaming
* Web dashboard visualization
* Multi-sensor support
* Signal recording playback
* Docker deployment
* REST API integration
* Embedded GUI support

---

# Example Output

## Gateway Console

```text
Waiting for sensor...
New sensor connected!
ECG: 0.82 PPG: 1.10
Filtered → ECG: 0.80 PPG: 1.05
```

## Log Output

```text
1 0.000001370 Database connected
2 0.000000426 New sensor connected
3 0.000000370 Database exported
```

---

# Target Platform

Tested on:

* Raspberry Pi
* Linux Ubuntu
* Embedded Linux systems

---

# Educational Purpose

This project is suitable for learning:

* Embedded Linux programming
* Biomedical signal processing
* Real-time software architecture
* Multi-threaded systems
* Low-level Linux APIs
* Embedded networking

---

# License


[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


---

# Author

* Nguyen Minh Hung
* email: minhhungdenguyn052@gmail.com
