#include "gps_datetime.h"
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cmath>  // for floorf, fmodf

// Convert NMEA-style ddmmyy + hhmmss.sss to epoch time (UTC)
uint32_t to_epoch(const char* date_str, float time_val) {
    if (!date_str || strlen(date_str) < 6) return 0;

    struct tm t = {0};

    t.tm_mday = (date_str[0] - '0') * 10 + (date_str[1] - '0');
    t.tm_mon  = (date_str[2] - '0') * 10 + (date_str[3] - '0') - 1; // 0-indexed
    t.tm_year = (date_str[4] - '0') * 10 + (date_str[5] - '0');
    t.tm_year += (t.tm_year < 80) ? 100 : 0;  // assume year 2000+

    int hh = int(time_val / 10000.0f);
    int mm = int(fmodf(time_val, 10000.0f) / 100.0f);
    int ss = int(fmodf(time_val, 100.0f));

    t.tm_hour = hh;
    t.tm_min  = mm;
    t.tm_sec  = ss;

    // Assume UTC time
    return (uint32_t)mktime(&t);
}

// Format MM/DD/YYYY from epoch
void date_from_epoch(uint32_t epoch, char* out, size_t len) {
    if (!out || len < 11) return;  // needs at least 11 bytes
    time_t t = (time_t)epoch;
    struct tm* tm_info = gmtime(&t);
    snprintf(out, len, "%02d/%02d/%04d", tm_info->tm_mon + 1,
             tm_info->tm_mday, tm_info->tm_year + 1900);
}

// Format HH:MM:SS from epoch
void time_from_epoch(uint32_t epoch, char* out, size_t len) {
    if (!out || len < 9) return;  // needs at least 9 bytes
    time_t t = (time_t)epoch;
    struct tm* tm_info = gmtime(&t);
    snprintf(out, len, "%02d:%02d:%02d", tm_info->tm_hour,
             tm_info->tm_min, tm_info->tm_sec);
}
