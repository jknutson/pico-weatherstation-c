#!/usr/bin/env python3
import json
import serial
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.connect("192.168.2.104", 1883, 60)

with serial.Serial('/dev/ttyACM0', 115200) as ser:
    while(True):
        line = ser.readline().strip().decode('ascii')
        if line[0] == '{':
            data = json.loads(line)
            keys = data.keys()
            if 'wind_speed' in keys:
                print('publishing wind_speed')
                client.publish('iot/pico/wind_speed', payload=data['wind_speed'])
            if 'wind_direction_deg' in keys:
                print('publishing wind_direction_deg')
                client.publish('iot/pico/wind_direction_deg', payload=data['wind_direction_deg'])
            if 'humidity' in keys:
                print('publishing humidity')
                client.publish('iot/pico/humidity', payload=data['humidity'])
            if 'temperature_f' in keys:
                print('publishing temperature_f')
                client.publish('iot/pico/temperature_f', payload=data['temperature_f'])
