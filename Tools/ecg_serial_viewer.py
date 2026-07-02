import argparse
import queue
import sys
import threading
import time
import tkinter as tk

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


class ECGViewer:
    def __init__(self, port, baudrate):
        self.port = port
        self.baudrate = baudrate
        self.samples = queue.Queue()
        self.running = True
        self.raw_values = []
        self.status = 1
        self.raw = 0
        self.lop = 0
        self.lon = 0
        self.adc_error = 0
        self.max_points = 760

        self.root = tk.Tk()
        self.root.title("AD8232 ECG Monitor")
        self.root.geometry("860x520")
        self.root.minsize(640, 360)
        self.root.configure(bg="#101317")

        self.title = tk.Label(
            self.root,
            text="AD8232 ECG Monitor",
            fg="#f3f7fb",
            bg="#101317",
            font=("Segoe UI", 18, "bold"),
        )
        self.title.pack(anchor="w", padx=18, pady=(14, 4))

        info = tk.Frame(self.root, bg="#101317")
        info.pack(fill="x", padx=18)

        self.status_label = tk.Label(
            info,
            text="Status: WAITING",
            fg="#ffd166",
            bg="#101317",
            font=("Consolas", 12),
        )
        self.status_label.pack(side="left")

        self.raw_label = tk.Label(
            info,
            text="RAW ADC: ----",
            fg="#d6e2ea",
            bg="#101317",
            font=("Consolas", 12),
        )
        self.raw_label.pack(side="left", padx=(28, 0))

        self.lead_label = tk.Label(
            info,
            text="LO+: -  LO-: -  ADC_ERR: 0",
            fg="#91a4b3",
            bg="#101317",
            font=("Consolas", 12),
        )
        self.lead_label.pack(side="left", padx=(28, 0))

        self.port_label = tk.Label(
            info,
            text=f"Port: {self.port}  Baud: {self.baudrate}",
            fg="#91a4b3",
            bg="#101317",
            font=("Consolas", 10),
        )
        self.port_label.pack(side="right")

        self.canvas = tk.Canvas(
            self.root,
            bg="#05070a",
            highlightthickness=1,
            highlightbackground="#2a333b",
        )
        self.canvas.pack(fill="both", expand=True, padx=18, pady=16)

        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.root.after(20, self.update_ui)

    def close(self):
        self.running = False
        self.root.destroy()

    def serial_worker(self):
        while self.running:
            try:
                with serial.Serial(self.port, self.baudrate, timeout=1) as ser:
                    while self.running:
                        line = ser.readline().decode("ascii", errors="ignore").strip()
                        if not line.startswith("ECG,"):
                            continue

                        parts = line.split(",")
                        if len(parts) < 3:
                            continue

                        status = int(parts[1])
                        raw = int(parts[2])
                        lop = int(parts[3]) if len(parts) > 3 else 0
                        lon = int(parts[4]) if len(parts) > 4 else 0
                        adc_error = int(parts[5]) if len(parts) > 5 else 0
                        self.samples.put((status, raw, lop, lon, adc_error))
            except Exception as exc:
                self.samples.put(("error", str(exc)))
                time.sleep(1)

    def start(self):
        thread = threading.Thread(target=self.serial_worker, daemon=True)
        thread.start()
        self.root.mainloop()

    def update_ui(self):
        while True:
            try:
                item = self.samples.get_nowait()
            except queue.Empty:
                break

            if item[0] == "error":
                self.status_label.configure(text=f"Status: SERIAL ERROR", fg="#ef476f")
                self.port_label.configure(text=str(item[1])[:80])
                continue

            self.status, self.raw, self.lop, self.lon, self.adc_error = item
            self.raw_values.append(self.raw)
            if len(self.raw_values) > self.max_points:
                self.raw_values = self.raw_values[-self.max_points:]

        if self.status == 0:
            self.status_label.configure(text="Status: NORMAL", fg="#4ade80")
        else:
            self.status_label.configure(text="Status: LEADS OFF", fg="#ef476f")

        self.raw_label.configure(text=f"RAW ADC: {self.raw:4d}")
        self.lead_label.configure(text=f"LO+: {self.lop}  LO-: {self.lon}  ADC_ERR: 0x{self.adc_error:08X}")
        self.draw_waveform()
        self.root.after(20, self.update_ui)

    def draw_waveform(self):
        self.canvas.delete("all")
        width = max(self.canvas.winfo_width(), 1)
        height = max(self.canvas.winfo_height(), 1)
        top = 24
        bottom = height - 24
        left = 24
        right = width - 24
        wave_height = max(bottom - top, 1)

        for x in range(left, right + 1, 40):
            self.canvas.create_line(x, top, x, bottom, fill="#182028")
        for y in range(top, bottom + 1, 40):
            self.canvas.create_line(left, y, right, y, fill="#182028")

        center = (top + bottom) // 2
        self.canvas.create_line(left, center, right, center, fill="#27445d")

        if self.status != 0:
            self.canvas.create_text(
                width // 2,
                height // 2,
                text="LEADS OFF",
                fill="#ef476f",
                font=("Segoe UI", 28, "bold"),
            )
            return

        if len(self.raw_values) < 2:
            return

        visible = self.raw_values[-min(len(self.raw_values), self.max_points):]
        step = max((right - left) / max(len(visible) - 1, 1), 1)
        points = []
        for index, raw in enumerate(visible):
            x = left + index * step
            y = bottom - (max(0, min(raw, 4095)) / 4095.0) * wave_height
            points.extend((x, y))

        self.canvas.create_line(points, fill="#38f28b", width=2, smooth=False)


def list_ports():
    if serial is None:
        return []
    return [port.device for port in serial.tools.list_ports.comports()]


def main():
    parser = argparse.ArgumentParser(description="AD8232 ECG serial waveform viewer")
    parser.add_argument("port", nargs="?", help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200, help="UART baudrate")
    args = parser.parse_args()

    if serial is None:
        print("pyserial is required. Install it with: python -m pip install pyserial")
        return 1

    port = args.port
    if port is None:
        ports = list_ports()
        if not ports:
            print("No serial ports found. Pass a port explicitly, for example: python Tools\\ecg_serial_viewer.py COM3")
            return 1
        port = ports[0]
        print(f"Using {port}. Available ports: {', '.join(ports)}")

    viewer = ECGViewer(port, args.baud)
    viewer.start()
    return 0


if __name__ == "__main__":
    sys.exit(main())
