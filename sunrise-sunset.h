
#ifndef SUNRISE_SUNSET_H_
#define SUNRISE_SUNSET_H_


// For the functions below longitude is positive for East and negative for
// West.

// Returns a fractional hour for sunrise on the given day at the given lat/lon
// coordinate. Returns -1 if the sun never rises on that day.
double sunrise_hour(int year, int month, int day, double lat, double lon);

// Returns a fractional hour for sunset on the given day at the given lat/lon
// coordinate. Return -1 if the sun never sets on that day.
double sunset_hour(int year, int month, int day, double lat, double lon);


#endif

