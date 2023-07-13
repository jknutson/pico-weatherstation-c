#!/usr/bin/python3 -u
import json
import os
import serial
import time
import paho.mqtt.client as mqtt
from statistics import mean, mode, StatisticsError

client = mqtt.Client()
mqtt_host = os.getenv('MQTT_HOST', 'localhost')
try:
    client.connect(mqtt_host, 1883, 60)
except ConnectionRefusedError:
    print(f"error connecting to mqtt_host:{mqtt_host}")
    print('did you set the `MQTT_HOST` environment variable?')
    exit(1)

publish_interval = 30  # seconds
publish_every = False  # emit every sample as well as mean

# TODO: make this a dict of sensordata, or class
# sensors[sensorname].append(data)
# sensors[sensorname].mean, etc
wind_speed_data = []
wind_direction_data = []
humidity_data = []
temperature_data = []

with serial.Serial('/dev/ttyACM0', 115200) as ser:
    start = time.time()
    while(True):
        line = ser.readline().strip().decode('ascii')
        # print(line)
        if line[0] == '{':
            data = json.loads(line)
            # print(data)  # print every line
            keys = data.keys()
            # TODO: DRY up this if block, make generic fn for append, if, publish
            # if 'wind_speed' in keys:
            #     wind_speed_data.append(data['wind_speed'])
            #     if publish_every:
            #         print('publishing wind_speed')
            #         client.publish('iot/pico/wind_speed', payload=data['wind_speed'])
            # if 'wind_angle' in keys:
            #     if data['wind_angle'] <= 337.5: # anything greater is "invalid"
            #         wind_direction_data.append(data['wind_angle'])
            #         if publish_every:
            #             print('publishing wind_angle')
            #             client.publish('iot/pico/wind_angle', payload=data['wind_angle'])
            if 'humidity' in keys:
                humidity_data.append(data['humidity'])
                if publish_every:
                    print('publishing humidity')
                    client.publish('iot/pico/humidity', payload=data['humidity'])
            if 'temperature_f' in keys:
                temperature_data.append(data['temperature_f'])
                if publish_every:
                    print('publishing temperature_f')
                    client.publish('iot/pico/temperature_f', payload=data['temperature_f'])
        if time.time() - start > publish_interval:
            # print('publishing averages')
            # if len(wind_speed_data) > 0:
            #     wind_speed_avg = mean(wind_speed_data)
            #     print(f" wind_speed_avg: {wind_speed_avg}")
            #     client.publish('iot/pico/wind_speed_avg', payload=wind_speed_avg)
            #     wind_speed_data = []
            # if len(wind_direction_data) > 0:
            #     # use mode (most common) here, as we are measuring discrete values/chunks
            #     # averaging the degrees wouldn't (always) make sense
            #     try:
            #         wind_angle_avg = mode(wind_direction_data)
            #         print(f" wind_angle_avg: {wind_angle_avg}")
            #         client.publish('iot/pico/wind_angle_avg', payload=wind_angle_avg)
            #         wind_angle_data = []
            #     except StatisticsError:
            #         # sometimes there are multiple modes, e.g. [1,1,2,2,3]
            #         # in python3.8 we can use multimode, for now we just publish a value
            #         print(f" wind_angle_data[0]: {wind_angle_data[0]}")
            #         client.publish('iot/pico/wind_angle_avg', payload=wind_direction_data[0])
            #         pass
            if len(humidity_data) > 0:
                humidity_avg = mean(humidity_data)
                client.publish('iot/pico/humidity_avg', payload=humidity_avg)
                print(f" humidity_avg: {humidity_avg}")
                humidity_data = []
            if len(temperature_data) > 0:
                temperature_avg = mean(temperature_data)
                client.publish('iot/pico/temperature_avg', payload=temperature_avg)
                print(f" temperature_avg: {temperature_avg}")
                temperature_data = []
            if len(humidity_data) > 0 and len(temperature_data) > 0:
                json_data = json.dumps({"temperature_avg": temperature_avg, "humidity_avg": humidity_avg})
                client.publish('iot/pico/readings_json', payload=json_data)
                json_data = None
            start = time.time()  # reset counter
