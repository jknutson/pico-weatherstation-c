/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "weatherstation.h"
#include "wind_direction.h"

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

#define getName(var)  #var

const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;
const uint CM_IN_KM = 100000.0;
const uint SECS_IN_HOUR = 3600;
const float ANEMOMETER_RADIUS = 9;  // cm
const uint ANEMOMETER_PIN = 2;
const uint RAIN_PIN = 3;
const uint ANEMOMETER_DEBOUNCE_MS = 20;
// 12-bit conversion, assume max value == ADC_VREF == 3.3 V
const float ADC_CONVERSION_FACTOR = 3.3f / (1 << 12);
const int SLEEP_INTERVAL_MS = 5000; // ms
const float R2 = 4700.0;
const float VIN = 3.3;

static int gpio_wind_cb_cnt = 0;
static int gpio_rain_cb_cnt = 0;

typedef struct {
	bool crc_match;
	float humidity;
	float temp_celsius;
} dht_reading;

void read_from_dht(dht_reading *result);

void gpio_cb(uint gpio, uint32_t events) {
	if (gpio == ANEMOMETER_PIN) {
		gpio_wind_cb_cnt++;
	}
	if (gpio == RAIN_PIN) {
		gpio_rain_cb_cnt++;
	}
}

// TODO: implement
float calc_rainfall(int tips) {
	return 0.0f;
}

// TODO: move wind stuff to wind_direction.c
float calc_wind_speed_kmh(int rotations) {
	// speed = ( (signals/2) * (2 * pi * radius) ) / time
	float speed_km_s = ((rotations / 2) * (2 * 3.1415 * ANEMOMETER_RADIUS)) / CM_IN_KM;
	float speed_kmh = speed_km_s * SECS_IN_HOUR;
	return speed_kmh;
}

float kmh_to_mph(float kmh) {
	return (kmh * 0.621371);
}

void reset_counter(int* count) {
	*count = 0;
}

int main() {
	stdio_init_all();
	gpio_init(DHT_PIN);

#ifdef LED_PIN
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
#endif

	gpio_set_pulls(ANEMOMETER_PIN, false, true);  // pull down
	gpio_set_irq_enabled_with_callback(ANEMOMETER_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_cb);

	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);

	for (;;) {
		// read ADC
		uint8_t sel_input = adc_get_selected_input();
		uint16_t result = adc_read();
		float result_v = result * ADC_CONVERSION_FACTOR;
		// read DHT
		dht_reading reading;
		reading.crc_match = false;
		read_from_dht(&reading);
		float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
		if (reading.crc_match) {
			printf("{\"humidity\": %.1f, \"temperature_f\": %.1f}\n", reading.humidity, fahrenheit);
		}
		// calculate wind angle
		if (sel_input == 0) {
			printf("{\"wind_angle\": %f}\n", get_angle(R2, VIN, result_v));
		}
		// calculate wind speed
		float wind_speed_kmh = calc_wind_speed_kmh(gpio_wind_cb_cnt);
		float wind_speed_mph = kmh_to_mph(wind_speed_kmh);
		printf("{\"wind_speed\": %.2f, \"wind_speed_kmh\": %.2f, \"gpio_wind_cb_cnt\": %i}\n", wind_speed_mph, wind_speed_kmh, gpio_wind_cb_cnt);
#ifdef DEBUG
		printf("resetting wind counter %i -> 0\n", gpio_wind_cb_cnt);
#endif
		reset_counter(&gpio_wind_cb_cnt);
		// calculate rainfall
		float rainfall_in = calc_rainfall(gpio_rain_cb_cnt);
		printf("{\"rainfall_in\": %.2f, \"gpio_rain_cb_cnt\": %i}\n", rainfall_in, gpio_rain_cb_cnt);
		printf("resetting rain counter %i -> 0\n", gpio_rain_cb_cnt);
		reset_counter(&gpio_rain_cb_cnt);
		sleep_ms(SLEEP_INTERVAL_MS);
	}
}

// TODO: move to dht.c/h
void read_from_dht(dht_reading *result) {
	int data[5] = {0, 0, 0, 0, 0};
	uint last = 1;
	uint j = 0;

	gpio_set_dir(DHT_PIN, GPIO_OUT);
	gpio_put(DHT_PIN, 0);
	sleep_ms(18);
	gpio_put(DHT_PIN, 1);
	sleep_us(40);
	gpio_set_dir(DHT_PIN, GPIO_IN);

#ifdef LED_PIN
	gpio_put(LED_PIN, 1);
#endif
	for (uint i = 0; i < MAX_TIMINGS; i++) {
		uint count = 0;
		while (gpio_get(DHT_PIN) == last) {
			count++;
			sleep_us(1);
			if (count == 255) break;
		}
		last = gpio_get(DHT_PIN);
		if (count == 255) break;

		if ((i >= 4) && (i % 2 == 0)) {
			data[j / 8] <<= 1;
			if (count > 46) data[j / 8] |= 1;
			j++;
		}
	}
#ifdef LED_PIN
	gpio_put(LED_PIN, 0);
#endif
	if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
		result->crc_match = true;
		result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
		if (result->humidity > 100) {
			result->humidity = data[0];
		}
		result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
		if (result->temp_celsius > 125) {
			result->temp_celsius = data[2];
		}
		if (data[2] & 0x80) {
			result->temp_celsius = -result->temp_celsius;
		}
	} else {
		result->crc_match = false;
#ifdef DEBUG
		printf("CRC mismatch - crc:%i actual:%i\n", data[4], ((data[0] + data[1] + data[2] + data[3]) & 0xFF));
#endif
	}
}
