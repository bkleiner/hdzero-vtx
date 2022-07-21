import time

from datetime import datetime

import serial

ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=5)

MSP_DISPLAYPORT = 182

SUBCMD_HEARTBEAT = 0
SUBCMD_RELEASE = 1
SUBCMD_CLEAR_SCREEN = 2
SUBCMD_WRITE_STRING = 3
SUBCMD_DRAW_SCREEN = 4
SUBCMD_SET_OPTIONS = 5


def send_msp(cmd, payload):
    payload_len = len(payload)
    size = payload_len + 6

    buf = bytearray([0]*size)
    buf[0] = ord('$')
    buf[1] = ord('M')
    buf[2] = ord('>')
    buf[3] = payload_len
    buf[4] = cmd

    checksum = buf[3] ^ buf[4]
    for i in range(payload_len):
        buf[i + 5] = payload[i]
        checksum ^= buf[i + 5]

    buf[-1] = checksum

    print(buf)
    ser.write(buf)
    ser.flush()


def send_msp_displayport(subcmd, payload=bytearray()):
    buf = bytearray([0]*1)
    buf[0] = subcmd
    buf += payload

    send_msp(MSP_DISPLAYPORT, buf)


def send_msp_send_string(x, y, string):
    buf = bytearray([0]*3)
    buf[0] = y
    buf[1] = x
    buf[2] = 0

    buf.extend(string.upper().encode('utf-8'))

    send_msp_displayport(SUBCMD_WRITE_STRING, buf)


buf = bytearray([0]*2)
buf[0] = 0
buf[1] = 1
send_msp_displayport(SUBCMD_SET_OPTIONS, buf)

send_msp_displayport(SUBCMD_CLEAR_SCREEN)
send_msp_displayport(SUBCMD_DRAW_SCREEN)

tick = True
while True:
    time_str = (datetime.now().strftime('%H:%M:%S:%f'))
    send_msp_send_string(0, 0, time_str)

    # for y in range(17):
    #     send_msp_send_string(0, y+1, ("0" if tick else "1")*49)

    send_msp_displayport(SUBCMD_DRAW_SCREEN)
    time.sleep(1)

    tick = not tick
