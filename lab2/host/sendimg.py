import os
import serial

BAUD_RATE = 115200


def send_img(ser, kernel):
    print("Please sent the kernel image size:")
    ser.read_until(b"Bootloader Started!\r\n")
    print("Bootloader Ready!")
    kernel_size = os.stat(kernel).st_size
    ser.write((str(kernel_size) + "\n").encode())
    print(ser.read_until(b"Start to load the kernel image... \r\n").decode(), end="")

    with open(kernel, "rb") as image:
        while kernel_size > 0:
            kernel_size -= ser.write(image.read(1))
            ser.read_until(b".")
    print(ser.read_until(b"$ ").decode(), end="")
    return


if __name__ == "__main__":
    ser = serial.Serial("/dev/pts/3", BAUD_RATE, timeout=5)
    send_img(ser, "kernel/kernel8.img")
