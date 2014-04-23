
#include <math.h>
#include <time.h>

#include "sunrise-sunset.h"


// Implementation of computation of sunrise/sunset as outlined in this
// resource:
// http://williams.best.vwh.net/sunrise_sunset_algorithm.htm


namespace {

// In degrees.
const double zenith = 90;


inline double deg2rad(double deg)
{
    return deg * M_PI / 180;
}

inline double rad2deg(double rad)
{
    return rad * 180 / M_PI;
}

inline double sin_d(double deg)
{
    return sin(deg2rad(deg));
}

inline double asin_d(double val)
{
    return rad2deg(asin(val));
}

inline double cos_d(double deg)
{
    return cos(deg2rad(deg));
}

inline double acos_d(double val)
{
    return rad2deg(acos(val));
}

inline double tan_d(double deg)
{
    return tan(deg2rad(deg));
}

inline double atan_d(double val)
{
    return rad2deg(atan(val));
}

// Convert utc hours to local.
double utc2local(int year, int month, int day, double utc)
{
    struct tm tm_utc;

    tm_utc.tm_hour = int(utc);
    tm_utc.tm_min = int(utc * 60) - tm_utc.tm_hour * 60;
    tm_utc.tm_sec =
        int(utc * 3600) - tm_utc.tm_hour * 3600 - tm_utc.tm_min * 60;
    tm_utc.tm_mday = day;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_year = year - 1900;

    time_t time = timegm(&tm_utc);
    struct tm *tm_local = localtime(&time);

    return double(tm_local->tm_hour) +
           double(tm_local->tm_min) / 60 +
           double(tm_local->tm_sec) / 3600;
}

}


double sunrise_hour(int year, int month, int day, double lat, double lon)
{
    double N1 = floor(275 * month / 9);
    double N2 = floor((month + 9) / 12);
    double N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    double N = N1 - (N2 * N3) + day - 30;

    double lngHour = lon / 15;
    double t = N + ((6 - lngHour) / 24);

    double M = (0.9856 * t) - 3.289;
    double L = M + (1.916 * sin_d(M)) + (0.020 * sin_d(2 * M)) + 282.634;
    while(L < 0) L += 360;
    while(L >= 360) L -= 360;

    double RA = atan_d(0.91764 * tan_d(L));
    while(RA < 0) RA += 360;
    while(RA >= 360) RA -= 360;

    double Lquadrant  = (floor( L/90)) * 90;
    double RAquadrant = (floor(RA/90)) * 90;
    RA = (RA + (Lquadrant - RAquadrant)) / 15;

    double sinDec = 0.39782 * sin_d(L);
    double cosDec = cos_d(asin_d(sinDec));
    double cosH = (cos_d(zenith) - (sinDec * sin_d(lat))) / (cosDec * cos_d(lat));

    if(cosH > 1) {
        return -1;
    }

    double H = (360 - acos_d(cosH)) / 15;
    double T = H + RA - (0.06571 * t) - 6.622;

    double UT = T - lngHour;
    while(UT < 0) UT += 24;
    while(UT >= 24) UT -= 24;

    return utc2local(year, month, day, UT);
}


double sunset_hour(int year, int month, int day, double lat, double lon)
{
    double N1 = floor(275 * month / 9);
    double N2 = floor((month + 9) / 12);
    double N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    double N = N1 - (N2 * N3) + day - 30;

    double lngHour = lon / 15;
    double t = N + ((18 - lngHour) / 24);

    double M = (0.9856 * t) - 3.289;

    double L = M + (1.916 * sin_d(M)) + (0.020 * sin_d(2 * M)) + 282.634;
    while(L < 0) L += 360;
    while(L >= 360) L -= 360;

    double RA = atan_d(0.91764 * tan_d(L));
    while(RA < 0) RA += 360;
    while(RA >= 360) RA -= 360;

    double Lquadrant  = (floor( L/90)) * 90;
    double RAquadrant = (floor(RA/90)) * 90;
    RA = (RA + (Lquadrant - RAquadrant)) / 15;

    double sinDec = 0.39782 * sin_d(L);
    double cosDec = cos_d(asin_d(sinDec));
    double cosH = (cos_d(zenith) - (sinDec * sin_d(lat))) / (cosDec * cos_d(lat));

    if(cosH < -1) {
        return -1;
    }

    double H = acos_d(cosH) / 15;
    double T = H + RA - (0.06571 * t) - 6.622;

    double UT = T - lngHour;
    while(UT < 0) UT += 24;
    while(UT >= 24) UT -= 24;

    return utc2local(year, month, day, UT);
}

