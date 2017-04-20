/* 
 * File:   datetime_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:30 PM
 */

#ifndef DATETIME_UTIL_H
#define	DATETIME_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //libs
#include <stdio.h>
#include <time.h>

    //proto
    char *get_current_datetime(const char *datetime_format);
    char *datetime_to_string(const time_t time_input, const char *datetime_format);
    time_t strToTime(const char *timeString);
    unsigned int str_to_seconds(const char *time_string);
    char *seconds_to_time_string(const long sec_in);
    int seconds_float_to_time_string(const float secs, char *str_res);
    time_t get_date(time_t input);

    //get current datetime formated as : ddmmyyyy_hhmmss

    char *get_current_datetime(const char *datetime_format) {
        char *buffer = NULL;
        time_t curtime;
        struct tm *loc_time;

        buffer = (char*) calloc(30, sizeof (char));
        if (!buffer)
            return NULL;

        //Getting current time of system
        curtime = time(NULL);

        // Converting current time to local time
        loc_time = localtime(&curtime);

        // Displaying date and time in standard format
        //    printf("%s", asctime (loc_time));
        //format datetime
        strftime(buffer, 30, datetime_format, loc_time);

        return buffer;
    }

    //convert time_t to string

    char *datetime_to_string(const time_t time_input, const char *datetime_format) {
        char *buffer = NULL;
        time_t curtime;
        struct tm *loc_time = NULL;

        buffer = (char*) calloc(30, sizeof (char));
        if (!buffer)
            return NULL;

        //Getting current time of system
        curtime = time_input;

        // Converting current time to local time
        loc_time = localtime(&curtime);

        // Displaying date and time in standard format
        //    printf("%s", asctime (loc_time));
        //format datetime
        strftime(buffer, 30, datetime_format, loc_time);

        return buffer;
    }

    //string to time

    time_t strToTime(const char *timeString) {
        time_t t = time(NULL);
        struct tm when = *localtime(&t);

        sscanf(timeString, "%d:%d:%d", &when.tm_hour, &when.tm_min, &when.tm_sec);
        //when.tm_sec = 59;

        time_t tt = mktime(&when);

        //display time
        //printf("%s", ctime(&tt));

        return tt;
    }

    //string time hh[:mm][:ss] to seconds

    unsigned int str_to_seconds(const char *time_string) {
        unsigned int h = 0, m = 0, s = 0;
        sscanf(time_string, "%d:%d:%d", &h, &m, &s);
        return (h * 3600)+(m * 60) + s;
    }

    //second to time string hh:mm:ss

    char *seconds_to_time_string(const long sec_in) {
        char *time_data_formated = NULL;

        //allocate it
        time_data_formated = (char*) calloc(16, sizeof (char));
        if (!time_data_formated)
            return NULL;

        //format the time data
        snprintf(time_data_formated, 16, "%2.2ld:%2.2ld:%2.2ld", sec_in / 3600, (sec_in % 3600) / 60, sec_in % 60);

        return time_data_formated;
    }

    //float second to str format hh:mm:ss

    int seconds_float_to_time_string(const float secs, char *str_res) {
        int sect = (int) secs;
        if (sect < 0) {
            return 0;
        } else {
            snprintf(str_res, 10, "%.02d:%.02d:%.02d", sect / 3600, (sect % 3600) / 60, sect % 60);
            return 1;
        }
    }

    /* get only date portion from time_t input */
    time_t get_date(time_t input) {
        time_t ti = input;
        struct tm loctm = *localtime(&ti);

        /* erase the time portion */
        loctm.tm_hour = loctm.tm_min = loctm.tm_sec = 0;
        
        /* return it */
        return mktime(&loctm);
    }


#ifdef	__cplusplus
}
#endif

#endif	/* DATETIME_UTIL_H */

