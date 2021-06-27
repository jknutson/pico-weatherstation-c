# Pico Weatherstation

This project is largely inspired by the [Build your own weather station](https://projects.raspberrypi.org/en/projects/build-your-own-weather-station/7) project on raspberrypi.org.

## History

I planned originally to follow the weather station project as closely as possible, figuring that would give me the greatest chance of a (hopefully quick) frustration-free success.

I did plan on at least two "major" deviations from that project:
  * ESP32 for analog (wind direction) input. Reason: I had extra ESP32s laying around, and no ADCs on hand.
  * MQTT/InfluxDB/Grafana for transport, storage, and visualiation respsectively. Reason: I wanted the first revision to be on premise.

Ultimately the project has made some even greater deviations, to the point of it being a net new code base.

## Hardware

### Argent Wind / Rain Sensor Assembly

This package includes an anemometer and wind vane for measuring wind speed and direction, a tipping bucket type rain gauge with 0.011 inch resolution, a metal mast, cables, and mounting hardware.

(Description from Argent website)

https://www.argentdata.com/catalog/product\_info.php?products\_id=145

### Raspberry Pi Pico

The [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/) is the heart of the weatherstation. It does the raw data collection from almost all of the sensors (rain sensor coming soon), and basic conversions.

I chose the Pico over the ESP32 due to my stubborn insistence on using interrupts for the anemometer. The ESP32 (when running Mongoose OS/mJS) was seemingly not able to handle the speed at which the interrupts were triggered. Each 1 rotation of the anemometer results in 2 triggers of the reed switch. There is a good chance that the ESP32 is up for the task, but I was looking for a reason to purchase a $4 Raspberry Pi Pico and now I had one.

### Raspberry Pi Original

The Raspberry Pi Original interfaces with a couple of sensors that are not yet migrated to the Pico:
  * DS18B20 One-Wire Temperature Probe (Ground Temperature)
  * Rain Gauge (part of Argent Sensor Assembly)

There are no major hurdles to migrating those sensors, though I may move things from a breadboard to a protoboard before adding more sensors.

### Future

I have some additional sensors that would be interesting to add:
  * Adafruit Seesaw Capacative Soil Moisture Sensor
  * BME680 Temp/Humidity/Pressure/VOC Sensor
  * Light (lux) Sensor

## Software

### Raspberry Pi Pico

The Pico is programmed in C with the Raspberry Pi Pico SDK. I chose C for the (probable) performance advantages over Python, as well as the support and documentation of the official SDK.

Like the Pico itself, C is something I was looking for an excuse to explore.

### Raspberry Pi Original

The Raspberry Pi Original runs a python script to poll the serial port for JSON messages containing sensor data, and publishes the data to an MQTT topic. Some minor data manipulation/rollup/etc. is performed in this script as well. It runs in `tmux` but should be turned into a proper systemd service.

### Future

See the Github Issues on this reposotory.
