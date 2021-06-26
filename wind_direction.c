#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "wind_direction.h"

const float VIN  = 3.3;   // volts
const float R2   = 4700;  // ohm
// const float angles[16][2] = {
const float angles[5][2] = {
	{0.0,  33000.0},
	{22.5, 6570.0},
	{45, 8200},
	{67.5, 891},
	{90, 1000}
};

int get_closest_idx(float r1) {
	int num_angles = sizeof(angles) / sizeof(angles[0]);
	int i, closest_i = 0;
	float closest = fabsf(r1 - angles[0][1]);
	for(i = 0; i < num_angles; i++) {
		float diff = fabsf(r1 - angles[i][1]);
#ifdef DEBUG
		printf("diff: %f, r1: %f, angles[i][1]: %f\n", diff, r1, angles[i][1]);
#endif
		if (diff < closest) {
			closest = fabsf(r1 - angles[i][1]);
			closest_i = i;
		}
#ifdef DEBUG
		printf("diff: %f, closest: %f, idx: %i, chosen_i: %i\n", diff, closest, i, closest_i);
#endif
	}
	return closest_i;
}


float voltage_divider(float vin, float r1, float r2) {
	return (vin * r2) / (r1 + r2);
}

float vout(float r1) {
	return voltage_divider(VIN, r1, R2);
}

float r1(float vout){
	// TODO: given vout & r2 get r1
	return ((R2 * VIN) / vout) - R2;
	return vout;
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		float vout = atof(argv[1]);
#ifdef DEBUG
		printf("vout: %f\n", vout);
		printf("r1: %f\n", r1(vout));
#endif
		int closest_idx = get_closest_idx(r1(vout));
#ifdef DEBUG
		printf("closest_idx: %i\n", closest_idx);
#endif
		printf("angle: %f resistance: %f\n", angles[closest_idx][0], angles[closest_idx][1]);
	} else {
		printf("1 argument required (r1)\n");
		return 1;
	}
	return 0;
}
