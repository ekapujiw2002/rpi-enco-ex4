/* 
 * File:   enslide.c
 * Author: EX4
 *
 * Created on May 30, 2015, 9:34 AM
 */

/* library used */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

//#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>

#include <sys/resource.h> 
#include <sys/time.h> 

//konsole utility
//#include "rlutil.h"
//#include "file_util.h"
#include "keygen_util.h"

//to disable all assert function
#define NDEBUG

//program version
#define	PROGRAM_TITLE               "enImaging"
#define	PROGRAM_VERSION             "15092015-084145"

//path define
#define PATH_MPD_PLAYLIST           "/data/media/playlist/"
#define PATH_MPD_MUSIC_DIR          "/data/media/audio/"
#define PATH_MPD_VIDEO_DIR          "/data/media/video/"
#define PATH_MPD_IMAGE_DIR          "/data/media/image/"
#define DEFAULT_PLAYLIST_FILE       "/data/media/playlist/default.pls.img"

//datetime format
//#define DATETIME_FMT_LOG_FILE       "%b%d-%Y"
//#define DATETIME_FMT_TIME           "%H%M"
#define DATETIME_FMT_IN_PLS			"%b%d-%Y"
#define DATETIME_MPC_STATUS			"%H:%M:%S"

//text colorization
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

//image slideshow structure

typedef struct image_slideshow_s {
    time_t start_time;
    char item_name[300];
} image_slideshow_t;


//global var
//image playlist items
#define MAX_IMAGE_ITEM_COUNT    3000
image_slideshow_t image_items[MAX_IMAGE_ITEM_COUNT];

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

/*
 * Check if a file exist using stat() function
 * return 1 if the file exist otherwise return 0
 */
/*
 * char isFileExists(const char *filename) {
    struct stat buffer;

    int exist = stat(filename, &buffer);
    //printf("Stat of %s ==> %d\n", fn, exist);
    return (exist == 0) ? 1 : 0;
}
 * */

/*
 * check char
 */
int gotchar(const int asearch, const char *astring) {
    char *e = strchr(astring, asearch);
    return e ? (int) (e - astring) : -1;
}

//get today playlist fname

char *today_playlist_fname() {
    char *pls_nm = NULL;

    pls_nm = (char*) calloc(300, sizeof (char));
    if (!pls_nm)
        return NULL;

    snprintf(pls_nm, 300, "%s%s.pls.img", PATH_MPD_PLAYLIST, get_current_datetime(DATETIME_FMT_IN_PLS));

    return pls_nm;
}

//get today new pls fname

char *today_new_playlist_fname() {
    char *pls_nm = NULL;

    pls_nm = today_playlist_fname();
    strcat(pls_nm, ".new");
    return pls_nm;
}

//remove old pls and rename new pls

int update_old_pls(const char *old_pls, const char *new_pls) {
    char cmdx[300];

    //setup the command
    snprintf(cmdx, 300, "sudo rm -f %s", old_pls);
    if (system(cmdx) == 0) {
        //rename the new pls
        snprintf(cmdx, 300, "sudo mv -f \"%s\" \"%s\"", new_pls, old_pls);
        return (system(cmdx) == 0) ? 0 : -1;
    } else {
        return -1;
    }
}

//copy default pls to current date pls

int copy_default_playlist(const char *current_pls) {
    char cmdx[300];

    //setup the command
    snprintf(cmdx, 300, "sudo cp -f %s %s", DEFAULT_PLAYLIST_FILE, current_pls);
    return system(cmdx);
}

//signal resgister handler

void signal_handler(int signo) {
    if (signo == SIGINT) {
        //kill all fbi
        system("sudo killall fbi");

        printf(KYEL "Exit program\nThank you\n" RESET);
        exit(EXIT_SUCCESS);
    }
}

/*
 * view image via fbi
 * */
char view_image_file(const char *fname, const int duration) {
    char resx = 0,
            cmdx[500];

    snprintf(cmdx, 500, "fbi -noverbose -a -1 -T 1 -t %d \"%s\"", duration, fname);

    resx = system(cmdx);

    return resx;
}

//parse image playlist

unsigned int parse_image_playlist(const char *playlist_file, image_slideshow_t *image_parse_results) {
    FILE *fp = fopen(playlist_file, "r");
    char buff_data[1024], buff_data2[1024], item_name[300], item_time[16];
    unsigned int id_item = 0;

    //start parsing
    if (fp) {
        //read per line
        memset(buff_data, 0, 1024);
        memset(buff_data2, 0, 1024);

        //read playlist
        while ((fgets(buff_data, 1000, fp) != NULL) && (id_item < MAX_IMAGE_ITEM_COUNT)) {
            //parse : time(hh:mm:ss)	tab	item_name
            sscanf(buff_data, "%[^\t\n]\t%[^\t\n]", &item_time, &item_name);
            snprintf(buff_data2, 1024, "%s%s", PATH_MPD_IMAGE_DIR, item_name);

            //remove any cr lf
            buff_data2[gotchar('\n', buff_data2)] = 0;
            buff_data2[gotchar('\r', buff_data2)] = 0;

            if (isFileExists(buff_data2)) {
                //save to the item result
                image_parse_results[id_item].start_time = strToTime(item_time);
                strncpy(image_parse_results[id_item].item_name, buff_data2, 300);

                //incr id item
                id_item++;

                //debug message
                printf(KYEL "Adding #%4d. %s\t%s\n" RESET, id_item, item_time, buff_data2);
            } else {
                //debug message
                printf(KYEL "File not found %s\n" RESET, buff_data2);
            }
        }
        fclose(fp);

        printf(KYEL "Found %d items\n", id_item);
        return id_item;
    } else {
        return 0;
    }
}

/* img pls exit code */
char get_img_pls_exit_code(const time_t next_switch_time, time_t *prev_date) {
    char ex_code = 0; //normal exit

    /* date change */
    if (*prev_date != strToTime("00:00:00")) {
        ex_code = 1;
        *prev_date = strToTime("00:00:00");
    }/* got new playlist */
    else if (isFileExists(today_new_playlist_fname()))
        ex_code = 3;
        /* switch time reached */
    else if (time(NULL) >= next_switch_time) {
        ex_code = 2;
        //        *prev_date = strToTime("00:00:00");
    } else
        ;

    return ex_code;
}

/*
 * main program start here
 */
int main(int argc, char** argv) {
    //var
    char *current_pls = NULL, *current_pls_new = NULL;
    unsigned int pls_count = 0, id_played_item = 0;
    time_t curr_date;

    //loop must exit
    char must_exit_main_loop = 0, flag_img_pls_exit_code = 0;

    //image playlist var
    time_t end_time, curr_datetime;
    //    int active_img_item_id;

    //start program	
    printf(KGRN "%s\nVersion\t: %s\n\n" RESET, PROGRAM_TITLE, PROGRAM_VERSION);
    
    /* save device id to file */
    save_id_file();
    if (validatekey() != 0) {
        printf(KRED "REGISTER THIS PROGRAM\n" RESET);
        return EXIT_SUCCESS;
    }

    //limit ram usage
    limit_ram_usage(75000000);

    //register signal handler
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf(KRED "Cannot attach signal interrupt handler\n" RESET);
    }

    //kill all fbi instance
    system("sudo killall fbi");

    //check for current playlist and process it
    for (;;) {
        //get current day playlist
        curr_date = strToTime("00:00:00");
        current_pls = today_playlist_fname();
        current_pls_new = today_new_playlist_fname();

        //reset flag exit
        must_exit_main_loop = 0;

        //check exist
        if (isFileExists(current_pls)) {
            printf(KYEL "Playlist %s found. Start parsing...\n" RESET, current_pls);

            //parse playlist for current day
            pls_count = parse_image_playlist(current_pls, image_items);

            //got any result?
            if (pls_count != 0) {
                /* wait for 1st item pls start time */
                printf(KGRN "Waiting start time at %s\n" RESET, ctime(&image_items[0].start_time));
                while (time(NULL) < image_items[0].start_time) {
                    //show current date time
                    printf(KYEL "%s\r" RESET, datetime_to_string(time(NULL), DATETIME_MPC_STATUS));
                    fflush(stdout);
                }
                printf("\n");

                /* play the items */
                id_played_item = 0;
                must_exit_main_loop = 0;
                for (;;) {
                    /* get end time */
                    /* calc next switch time */
                    if (id_played_item != (pls_count - 1)) {
                        end_time = image_items[id_played_item + 1].start_time;
                    } else {
                        end_time = strToTime("23:59:59");
                    }

                    /* check start time */
                    /* play it */
                    curr_datetime = time(NULL);
                    if ((curr_datetime >= image_items[id_played_item].start_time) && (curr_datetime <= end_time)) {
                        /* view it with fbi max timeout is 24 hours */
                        if (isFileExists(image_items[id_played_item].item_name)) {
                            printf(KGRN "Viewing item %s until %s\n" RESET, basename(image_items[id_played_item].item_name), datetime_to_string(end_time, DATETIME_MPC_STATUS));
                            system("sudo killall fbi");
                            usleep(100000);
                            view_image_file(image_items[id_played_item].item_name, 24 * 60 * 60);

                            /* wait until next img start time or new pls or new date */
                            for (;;) {
                                //show current date time
                                printf(KYEL "%s\r" RESET, datetime_to_string(time(NULL), DATETIME_MPC_STATUS));
                                fflush(stdout);

                                /* get exit code */
                                flag_img_pls_exit_code = get_img_pls_exit_code(end_time, &curr_date);

                                /* check result */
                                if (flag_img_pls_exit_code > 0) {
                                    if ((flag_img_pls_exit_code == 1) || (flag_img_pls_exit_code == 3)) {
                                        must_exit_main_loop = 1;
                                    }

                                    /* delay */
                                    sleep(1);

                                    break;
                                }

                                /* delay */
                                sleep(1);
                            }
                        } else {
                            printf(KRED "File %s not found\n" RESET, basename(image_items[id_played_item].item_name));
                        }

                        printf(KGRN "Switching to next item\n" RESET);
                    }

                    /* check exit code again */
                    /* get exit code */
                    flag_img_pls_exit_code = get_img_pls_exit_code(end_time, &curr_date);

                    /* check result */
                    if (flag_img_pls_exit_code > 0) {
                        if ((flag_img_pls_exit_code == 1) || (flag_img_pls_exit_code == 3)) {
                            must_exit_main_loop = 1;
                        }
                    }

                    if (must_exit_main_loop)
                        break;

                    /* update id */
                    id_played_item++;
                    if (id_played_item >= pls_count) {
                        id_played_item = 0;
                    }
                }

                printf(KGRN "Playlist end with exit code = %d\n" RESET, flag_img_pls_exit_code);
            } else {
                printf(KRED "No item found\n" RESET);

                //delay for 5sec
                sleep(5);
            }
        } else {
            //check for default playlist
            printf(KRED "Today playlist %s not found. Searching default playlist...\n" RESET, current_pls);

            if (isFileExists(DEFAULT_PLAYLIST_FILE)) {
                printf("Copying default playlist...%s\n", (copy_default_playlist(current_pls) == 0) ? "OK" : "Fail");
            } else {
                printf("No default playlist %s found. Sleep for 5 seconds...\n", DEFAULT_PLAYLIST_FILE);
                sleep(5);
            }
        }

        //free it up
        if (current_pls) {
            free(current_pls);
            current_pls = NULL;
        }

        if (current_pls_new) {
            free(current_pls_new);
            current_pls_new = NULL;
        }
    }
    return 0;
}

