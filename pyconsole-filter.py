#!/usr/bin/env python3
import json
import os
import serial
import paho.mqtt.client as mqtt

with serial.Serial('/dev/ttyACM0', 115200) as ser:
    while(True):
        line = ser.readline().strip().decode('ascii')
        if 'counter' in line:
            print(line)
