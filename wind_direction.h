#ifndef WIND_DIRECTION_H
#define WIND_DIRECTION_H

#define NO_DEBUG

float get_angle(float r2, float vin, float vout);
float calc_wind_speed_kmh(int rotations, int radius);
float kmh_to_mph(float kmh);

#endif
