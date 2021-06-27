#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "wind_direction.h"

const float VIN  = 3.3;   // volts
const float R2   = 4700;  // ohm
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

// TODO: save chars to a char[], not printf
void get_direction4(float ang) {
	switch((int)ang/90) {
		case 0 :
		case 4 :
			printf("north");
			break;
		case 1 :
			printf("east");
			break;
		case 2 :
			printf("south");
			break;
		case 3 :
			printf("west");
			break;
	}
	printf("\n");
}

void get_direction16(float ang) {
	if (ang == 360) {
		ang = ang - 360;
	}
	int bits = ang / 22.5;
#ifdef DEBUG
	printf("get_direction16 ang: %f, bits: %i\n", ang, bits);
#endif
	if ((0 <= bits && bits <= 2) || (14 <= bits && bits <= 15)) {
		printf("north");
		if (bits == 1) {
			printf("-north");
		}
		if (1 <= bits && bits <= 3) {
			printf("east");
		}
	} else if (3 <= bits && bits <= 4) {
		printf("east");
	} else if (6 <= bits && bits <= 10) {
		printf("south");
	} else if (11 <= bits && bits <= 13) {
		printf("west");
	} else {
		printf("something went wrong");
	}
	printf("\n");
}

void get_direction_mask(float ang) {
	if (ang == 360) {
		ang = ang - 360;
	}
	int bits = ang / 22.5;
#ifdef DEBUG
	printf("mask ang: %f, bits: %i\n", ang, bits);
#endif

}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		float vout = atof(argv[1]);
#ifdef DEBUG
		printf("r1: %f\n", r1(R2, VIN, vout));
		int closest_idx = get_closest_idx(r1(R2, VIN, vout));
		printf("closest_idx: %i\n", closest_idx);
		printf("angle: %f resistance: %f\n", angles[closest_idx][0], angles[closest_idx][1]);
#endif
		printf("angle: %f\n", get_angle(R2, VIN, vout));
	} else {
		get_direction16(0.0);
		get_direction16(360.0);
		get_direction16(22.5);
		get_direction16(45);
		get_direction16(67.5);
		/*
		get_direction16(90.0);
		get_direction16(180.0);
		get_direction16(270.0);
		*/
	}
	return 0;
}
