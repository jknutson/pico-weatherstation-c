#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wind_direction.h"

const float angles[16][2] = {  // TODO: move to separate file
	{0,  33000},
	{22.5, 6570},
	{45, 8200},
	{67.5, 891},
	{90, 1000},
	{112.5, 688},
	{135, 2200},
	{157.5, 1410},
	{180, 3900},
	{202.5, 3140},
	{225, 16000},
	{247.5, 14120},
	{270, 120000},
	{292.5, 42120},
	{315, 64900},
	{337.5, 21880}
};

int get_closest_idx(float r1) {
	int angles_len = sizeof(angles) / sizeof(angles[0]);
	float closest = fabsf(r1 - angles[0][1]);
	int closest_i = 0;
	for(int i = 0; i < angles_len; i++) {
		float diff = fabsf(r1 - angles[i][1]);
		if (diff < closest) {
			closest = fabsf(r1 - angles[i][1]);
			closest_i = i;
		}
	}
	return closest_i;
}

float r1(float r2, float vin, float vout){
	return ((r2 * vin) / vout) - r2;
	return vout;
}

// this is the library-style interface to this code
float get_angle(float r2, float vin, float vout) {
	int idx = get_closest_idx(r1(r2, vin, vout));
	return angles[idx][0];
}

// get direction of wind with 4 bits of precision
void get_direction4(float ang, char *direction) {
	switch((int)ang/90) {
		case 0 :
		case 4 :
			strncpy(direction, "north", 5);
			break;
		case 1 :
			strncpy(direction, "east", 4);
			break;
		case 2 :
			strncpy(direction, "south", 5);
			break;
		case 3 :
			strncpy(direction, "west", 4);
			break;
	}
	// TODO: does it matter that we don't "fill" `direction` for east/west?
	direction[5] = '\0';
}

float deg2rad(float deg) {
	return deg * (M_PI / 180.0);
}
float rad2deg(float rad) {
	return rad * (180.0 / M_PI);
}

float get_avg_angle(float arr_angles[], int num_arr_angles) {
	float sin_sum = 0.0;
	float cos_sum = 0.0;

	for (int i = 0; i < num_arr_angles; i++) {
		if (arr_angles[i] == 0.0) {
			arr_angles[i] = 360.0;
		}
		float r = deg2rad(arr_angles[i]);
		sin_sum = sin_sum + sin(r);
		cos_sum = cos_sum + cos(r);
	}
	float sin_avg = sin_sum / num_arr_angles;
	float cos_avg = cos_sum / num_arr_angles;
	float arc = rad2deg(atan(sin_avg / cos_avg));
	float avg_angle = 0.0;
	if (sin_sum > 0 && cos_sum > 0) {
		avg_angle = arc;
	} else if (cos_sum < 0) {
		avg_angle = arc + 180;
	} else if (sin_sum < 0 && cos_sum > 0) {
		avg_angle = arc + 360;
	} else {
		// TODO: better error handling here
		printf("something went wrong getting avg angle\n");
	}
	if (avg_angle == 360) {
		avg_angle = 0;
	}
	return avg_angle;
}
