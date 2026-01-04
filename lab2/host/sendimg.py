import os
import serial

BAUD_RATE = 115200
CHUNK_SIZE = 256


def send_img(ser, kernel):
    print("Please sent the kernel image size:")
    ser.read_until(b"Bootloader Started!\r\n")
    print("Bootloader Ready!")
    kernel_size = os.stat(kernel).st_size
    ser.write((str(kernel_size) + "\n").encode())
    print(ser.read_until(b"Start to load the kernel image... \r\n").decode(), end="")

    with open(kernel, "rb") as image:
        sent_size = 0
        while kernel_size > sent_size:
            chunk = image.read(CHUNK_SIZE)
            ser.write(chunk)
            ack = ser.read(1)
            sent_size += len(chunk)
            print(f"{sent_size} / {kernel_size}", end="\r\n")
        print("all done")
    try:
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode(errors="ignore")
                print(data, end="")
    except KeyboardInterrupt:
        print("\nStopped by user.")
    return


if __name__ == "__main__":
    ser = serial.Serial("/dev/pts/4", BAUD_RATE, timeout=5)
    send_img(ser, "kernel/kernel8.img")
