#include <stdio.h>
#include <stdlib.h>

const float VREF = 3.3;   // volts
const float R2   = 4700;  // ohm

float voltage_divider(float vin, float r1, float r2) {
	return (vin * r2) / (r1 + r2);
}

float vout(float r1) {
	return voltage_divider(VREF, r1, R2);
}

int main(int argc, char *argv[]) {
	// printf("hello\n");
	if (argc == 2) {
		float r1 = atof(argv[1]);
		printf("vout: %f\n", vout(r1));
	} else {
		printf("1 argument required (r1)\n");
		return 1;
	}
	return 0;
}
