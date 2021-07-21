#!/usr/bin/env python3
import json
import os
import serial
import time

# USB UART
# with serial.Serial('/dev/ttyACM0', 115200) as ser:
# GPIO UART
with serial.Serial('/dev/serial0', 115200) as ser:
    while(True):
        line = ser.readline().strip().decode('ascii')
        print(line)
