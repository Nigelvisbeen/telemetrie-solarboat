#!/usr/bin/env python3
"""
Simple live dashboard for solarboat telemetry.

Reads CSV lines from serial in format:
pc_time_ms,seq,uptime_ms,battery_v,battery_i,flags,rssi,snr
"""

import argparse
import collections
import threading

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import serial


DataPoint = collections.namedtuple(
    "DataPoint",
    ["t", "seq", "uptime_ms", "battery_v", "battery_i", "flags", "rssi", "snr"],
)


class TelemetryBuffer:
    def __init__(self, maxlen: int = 600):
        self._data = collections.deque(maxlen=maxlen)
        self._lock = threading.Lock()

    def append(self, p: DataPoint) -> None:
        with self._lock:
            self._data.append(p)

    def snapshot(self):
        with self._lock:
            return list(self._data)


def parse_line(line: str):
    parts = line.strip().split(",")
    if len(parts) != 8:
        return None

    try:
        return DataPoint(
            t=float(parts[0]) / 1000.0,
            seq=int(parts[1]),
            uptime_ms=int(parts[2]),
            battery_v=float(parts[3]),
            battery_i=float(parts[4]),
            flags=int(parts[5]),
            rssi=int(parts[6]),
            snr=float(parts[7]),
        )
    except ValueError:
        return None


def serial_reader(port: str, baud: int, buf: TelemetryBuffer, stop_evt: threading.Event):
    with serial.Serial(port, baudrate=baud, timeout=1) as ser:
        # Drop boot chatter / partial lines
        ser.reset_input_buffer()
        print(f"Connected to {port} @ {baud}")

        while not stop_evt.is_set():
            raw = ser.readline()
            if not raw:
                continue
            try:
                line = raw.decode("utf-8", errors="ignore").strip()
            except UnicodeDecodeError:
                continue

            if not line or line.startswith("Solarboat") or line.startswith("CSV header"):
                continue

            p = parse_line(line)
            if p is not None:
                buf.append(p)


def run_dashboard(port: str, baud: int, history_points: int):
    buf = TelemetryBuffer(maxlen=history_points)
    stop_evt = threading.Event()

    th = threading.Thread(target=serial_reader, args=(port, baud, buf, stop_evt), daemon=True)
    th.start()

    fig, axs = plt.subplots(3, 1, sharex=True, figsize=(10, 8))

    line_v, = axs[0].plot([], [], label="Battery Voltage (V)")
    line_i, = axs[1].plot([], [], label="Battery Current (A)", color="tab:orange")
    line_rssi, = axs[2].plot([], [], label="RSSI (dBm)", color="tab:green")

    axs[0].set_ylabel("V")
    axs[1].set_ylabel("A")
    axs[2].set_ylabel("dBm")
    axs[2].set_xlabel("Receiver time (s)")

    for ax in axs:
        ax.grid(True, alpha=0.3)
        ax.legend(loc="upper left")

    info_text = axs[0].text(0.99, 0.95, "", transform=axs[0].transAxes, ha="right", va="top")

    def update(_frame):
        data = buf.snapshot()
        if not data:
            return line_v, line_i, line_rssi, info_text

        t0 = data[0].t
        ts = [p.t - t0 for p in data]
        vs = [p.battery_v for p in data]
        is_ = [p.battery_i for p in data]
        rssis = [p.rssi for p in data]

        line_v.set_data(ts, vs)
        line_i.set_data(ts, is_)
        line_rssi.set_data(ts, rssis)

        for ax in axs:
            ax.relim()
            ax.autoscale_view()

        latest = data[-1]
        flags_desc = []
        if latest.flags & 0x01:
            flags_desc.append("valid")
        if latest.flags & 0x02:
            flags_desc.append("stale")
        if not flags_desc:
            flags_desc.append("none")

        info_text.set_text(
            f"seq={latest.seq}  V={latest.battery_v:.2f}  I={latest.battery_i:.2f}  "
            f"RSSI={latest.rssi}  SNR={latest.snr:.1f}  flags={'|'.join(flags_desc)}"
        )

        return line_v, line_i, line_rssi, info_text

    _ani = FuncAnimation(fig, update, interval=300, blit=False)

    try:
        plt.tight_layout()
        plt.show()
    finally:
        stop_evt.set()
        th.join(timeout=2)


def main():
    parser = argparse.ArgumentParser(description="Solarboat LoRa telemetry dashboard")
    parser.add_argument("--port", required=True, help="Serial port, e.g. /dev/ttyUSB0 or COM5")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    parser.add_argument("--history", type=int, default=600, help="Number of points to keep")
    args = parser.parse_args()

    run_dashboard(args.port, args.baud, args.history)


if __name__ == "__main__":
    main()
