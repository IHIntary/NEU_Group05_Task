import serial

PORT = "COM7"        # 改成你的串口号，例如 COM3、COM5
BAUD = 115200        # 改成 STM32 CubeMX 里 USART 设置的波特率

ser = serial.Serial(
    port=PORT,
    baudrate=BAUD,
    bytesize=8,
    parity="N",
    stopbits=1,
    timeout=1
)

print(f"已打开串口：{PORT}, 波特率：{BAUD}")
print("开始接收数据，按 Ctrl+C 退出\n")

try:
    while True:
        data = ser.readline()   # 读取到 \n 或超时
        if data:
            text = data.decode("utf-8", errors="ignore").strip()
            print(text)

except KeyboardInterrupt:
    print("\n已退出")

finally:
    ser.close()