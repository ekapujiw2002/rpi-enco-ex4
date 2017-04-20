/* 
 * File:   enstreamer.c
 * Author: EX4
 *
 * Created on May 28, 2015, 7:13 PM
 */

/* ordinary lib */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>

/* rtc lib */
#include <linux/rtc.h>

/* custom lib */
#include "system_util.h"
#include "string_util.h"
#include "file_util.h"
#include "keygen_util.h"
#include "datetime_util.h"
#include "network_util.h"
#include "image_util.h"
#include "rlutil.h"

/* use i2c lcd or not */
//#define LCD_I2C_ON

#ifdef LCD_I2C_ON
#include "i2c.h"
#include "lcd.h"
#endif

/* DEBUG ON */
//#define DEBUG_MODE_ON

/* place it at most below */
#include "enstreamer_util.h"

/* program info */
#define	PROGRAM_TITLE               "enStreamer"
#define	PROGRAM_VERSION             "07092015-155440"

/* playlist config */
#define MAX_PLAYLISTS_CONTROL       200
#define MAX_PLAYLISTS_ITEMS         1000

/* enstreamer path lists */
#define PATH_MPD_PLAYLIST           "/data/media/playlist/"
#define PATH_MPD_MUSIC_DIR          "/data/media/audio/"
#define PATH_MPD_VIDEO_DIR          "/data/media/video/"
#define PATH_MPD_IMAGE_DIR          "/data/media/image/"
#define DEFAULT_PLAYLIST_FILE       "/data/media/playlist/default.pls"

/* logs define */
#define	LOG_FILE_HEADER             "Time	Length	Play Duration	File\r\n"
#define PATH_LOGS_DIR               "/data/media/logs/"

/* datetime format */
#define DATETIME_FMT_LOG_FILE       "%b%d-%Y"
#define DATETIME_FMT_TIME           "%H%M"
#define DATETIME_FMT_IN_PLS         "%b%d-%Y"
#define DATETIME_MPC_STATUS         "%H:%M:%S"

/* item type */
#define	ITEM_AUDIO          0
#define	ITEM_VIDEO          1
#define ITEM_TIME_EVENT     2
#define	ITEM_IMAGE          3
#define ITEM_AUDIO_PLAYLIST 4
#define ITEM_INVALID        0xff

/* online or local */
#define ITEM_LOCAL          0
#define ITEM_ONLINE         1

/* error constant for audio */
#define	ERR_MPC_NOT_PLAYING         0x80

/* info played item */
#define INFO_PLAYED_FILE    "/dev/shm/enstreamer-status"

#ifdef LCD_I2C_ON
/* lcd i2c */
/* i2c dev */
#define I2C_FILE_NAME "/dev/i2c-1"

/* i2c slave addr */
#define	I2C_SLAVE_ADDR	0x27
#endif

/* playlist control struct */
typedef struct playlist_control_struct {
    /* start time for the playlist */
    time_t start_time,

    /* time to switch */
    switch_time;

    /* total items in the playlist */
    int item_total_count,

    /* id for items start and end */
    id_item_start, id_item_end;
} playlist_control_t;

/* struct data for item playlist */
typedef struct playlists_items_struct {
    /* full path for the item */
    char item_full_path[300],

    /* item type */
    item_type,

    /* local or online item */
    is_online;

    /* total duration in seconds */
    int duration,

    /* id playlist for audio item, for mpd only */
    id_playlist_audio,

    /* id start end audio only */
    id_playlist_audio_start, id_playlist_audio_end;
} playlists_items_t;

/* struct data for audio items */
typedef struct playlists_audio_items_struct {
    /* full path for the item */
    char item_full_path[300],

    /* local or online item */
    is_online;

    /* total duration in seconds */
    int duration,

    /* id playlist */
    id_playlist_audio,

    /* crossfade audio only */
    crossfade;
} playlists_audio_items_t;

/* struct mpc status info */
typedef struct mpc_status_info_struct {
    char song[200];
    char status;
    int song_index;
    char time_start[10], time_stop[10], time_elapse[10], time_total[10], time_remaining[10];
} mpc_status_info_t;

/* global var */
playlist_control_t playlist_controls[MAX_PLAYLISTS_CONTROL];
playlists_items_t playlist_items[MAX_PLAYLISTS_ITEMS];
playlists_audio_items_t playlist_audio_items[MAX_PLAYLISTS_ITEMS];

/* ip */
char IP_PUBLIC[32], IP_LOCAL[32], DEV_NAME[32];

#ifdef LCD_I2C_ON
/* lcd i2c var */
int LCD_I2C_HDL;
lcd LCD_DEV;
#endif

/* date record */
time_t DATE_OLD;

/*==============================================================================*/


/* proto func void */
/* system util */
void signal_handler(int signo);

/* playlist util */
char *today_playlist_fname();
char *today_new_playlist_fname();
int copy_default_playlist(const char *current_pls);
char add_playlist_items(const char *playlist_fname, const char *playlist_item);
int parse_playlist(const char *playlist_file, playlist_control_t *playlist_control, playlists_items_t *playlist_items, playlists_audio_items_t *pls_audio_items);
void debug_pls_control(const playlist_control_t *playlist_control, const int len);
void debug_pls_items(const playlists_items_t *playlist_items, const int len);
void debug_pls_audio_items(const playlists_audio_items_t *playlist_audio_items);
int update_old_pls(const char *old_pls, const char *new_pls);
int find_current_time_pls(const unsigned int pls_ttl_count, const playlist_control_t *pls_ctrl_list);
char playlist_wait_to_start();
int playlist_process_active(const playlist_control_t pls_ctrl, playlists_items_t *pls_items, playlists_audio_items_t *pls_audio_items);
int playlist_play_audio_items(const playlist_control_t pls_ctrl, const playlists_items_t pls_items, playlists_audio_items_t *pls_audio_items, playlists_items_t pls_next_items);
int playlist_play_video_items(const playlist_control_t pls_ctrl, playlists_items_t pls_item);
int playlist_get_exit_code(const time_t switch_time, time_t *prev_date);

/* datetime util */
void display_datetime_cursor_back(const char is_cursor_back);

/* keyboard util */
void check_keyboard_for_quit();

/* mpc util */
int mpc_load_playlist(const int pls_id, const char is_random, const char is_repeat);
int mpc_get_info(mpc_status_info_t *mpc_stat, const char *total_duration_str);
char* mpc_format_time(const char *atime_data, long *seconds_out);
int mpc_set_crossfade(const char duration);
void mpc_status_adjust_start_stop_time(mpc_status_info_t *mpc_stat, const char time_substractor, const char update_stop_time);

/* logging util */
int logging_item_info(const char *device_name, const char *ip_ext, const char *ip_int, const char *time_start, const char *total_duration, const char *play_duration, const char *item_name);
char write_status_info_file(const char *status);
void write_play_status_to_file(const char *curr_item_name, const char *curr_item_duration, const char curr_item_is_online, const char *next_item_name, const char *next_item_duration, const char next_item_is_online);

/* omxplayer util */
char omxplayer_is_running();
char omxplayer_load_file(const char *item);

/*  CTRL+C handler */
void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\n" KRED "Interrupt signal received\n" RESET);

        kill_audio_video_player();

        //        system("setterm -cursor on && clear");

        printf(KRED "Exit program\nThank you\n" RESET);
        exit(EXIT_SUCCESS);
    }
}

/* today playlist name */
char *today_playlist_fname() {
    char *pls_nm = NULL;

    pls_nm = (char*) calloc(300, sizeof (char));
    if (!pls_nm)
        return NULL;

    snprintf(pls_nm, 300, "%s%s.pls", PATH_MPD_PLAYLIST, get_current_datetime(DATETIME_FMT_IN_PLS));

    return pls_nm;
}

/* today new playlist name */
char *today_new_playlist_fname() {
    char *pls_nm = NULL;

    pls_nm = today_playlist_fname();
    strcat(pls_nm, ".new");
    return pls_nm;
}

/* copy default playlist */
int copy_default_playlist(const char *current_pls) {
    char cmdx[300];

    //setup the command
    snprintf(cmdx, 300, "sudo cp -f %s %s", DEFAULT_PLAYLIST_FILE, current_pls);
    return system(cmdx);
}

/* add playlist item to playlist file */
char add_playlist_items(const char *playlist_fname, const char *playlist_item) {
    //    FILE *fp = fopen(playlist_fname, "a+");
    //
    //    if (!fp)
    //        return 0;
    //
    //    fputs(playlist_item, fp);
    //    fflush(fp);
    //    fclose(fp);
    //    return 1;

    char cmd[500];
    snprintf(cmd, sizeof (cmd), "echo \"%s\" >> %s", playlist_item, playlist_fname);
    return (system(cmd) == 0);
}

/* debug pls control */
void debug_pls_control(const playlist_control_t *playlist_control, const int len) {
    int i;
    printf(KRED "PLAYLIST CONTROL\n#\tstart\tend\ttotal\tstart time\tswitch\n" RESET);
    for (i = 0; i < len; i++) {
        printf(KRED "%d\t%d\t%d\t%d\t%s\t%s\t\n" RESET,
                i,
                playlist_control[i].id_item_start,
                playlist_control[i].id_item_end,
                playlist_control[i].item_total_count,
                datetime_to_string(playlist_control[i].start_time, DATETIME_MPC_STATUS),
                datetime_to_string(playlist_control[i].switch_time, DATETIME_MPC_STATUS)
                );
    }
}

/* debug pls items */
void debug_pls_items(const playlists_items_t *playlist_items, const int len) {
    int i;
    printf(KRED "PLAYLIST ITEMS\n#\t type\t ol\t pls\t dur\t aus\t aue\t name\t\n" RESET);
    for (i = 0; i < len; i++) {
        printf(KRED "%d\t %d\t %d\t %d\t %d\t %d\t %d\t %s\t\n" RESET,
                i,
                playlist_items[i].item_type,
                playlist_items[i].is_online,
                playlist_items[i].id_playlist_audio,
                playlist_items[i].duration,
                playlist_items[i].id_playlist_audio_start,
                playlist_items[i].id_playlist_audio_end,
                playlist_items[i].item_full_path
                );
    }
}

/* debug audio items */
void debug_pls_audio_items(const playlists_audio_items_t *playlist_audio_items) {
    int i;
    printf(KRED "PLAYLIST AUDIO ITEMS\n#\t ol\t pls\t dur\t cross\t name\t\n" RESET);
    for (i = 0; i < MAX_PLAYLISTS_ITEMS; i++) {
        if (playlist_audio_items[i].id_playlist_audio == -1)
            break;

        printf(KRED "%d\t %d\t %d\t %d\t %d\t %s\t\n" RESET,
                i,
                playlist_audio_items[i].is_online,
                playlist_audio_items[i].id_playlist_audio,
                playlist_audio_items[i].duration,
                playlist_audio_items[i].crossfade,
                playlist_audio_items[i].item_full_path
                );
    }
}

/* parse playlist */
int parse_playlist(const char *playlist_file, playlist_control_t *playlist_control, playlists_items_t *playlist_items, playlists_audio_items_t *pls_audio_items) {
    /* open the file */
    FILE *fp = fopen(playlist_file, "r");
    char buff_data[1024], buff_data2[1024], is_1st_line = 1;
    int id_playlist_control = 0, id_playlist_items = 0;

    /* time var */
    time_t start_time_pls;

    /* result */
    int resx = 0;

    /* parse var items */
    char parse_item_type_str[10], parse_item_duration_str[10], parse_item_full_name[300];
    float parse_item_duration;
    int parse_crossfade, parse_item_type, parse_item_type_previous;

    /* audio playlist var */
    int audio_playlist_id = -1, audio_items_id = 0;
    char audio_pls_name[10], audio_pls_full_name[300];

    /* temp var */
    int i;

    /* init pls control result */
    for (i = 0; i < MAX_PLAYLISTS_CONTROL; i++) {
        playlist_control[i].start_time = strToTime("00:00:00");
        playlist_control[i].switch_time = strToTime("23:59:59");
        playlist_control[i].id_item_start = playlist_control[i].id_item_end = 0;
        playlist_control[i].item_total_count = 0;
    }

    /* init items result */
    for (i = 0; i < MAX_PLAYLISTS_ITEMS; i++) {
        /* ordinary item */
        playlist_items[i].duration = playlist_items[i].is_online = 0;
        playlist_items[i].id_playlist_audio_start = playlist_items[i].id_playlist_audio_end = playlist_items[i].id_playlist_audio = -1;
        playlist_items[i].item_type = ITEM_INVALID;
        snprintf(playlist_items[i].item_full_path, 10, "");

        /* audio items */
        pls_audio_items[i].crossfade = pls_audio_items[i].duration = pls_audio_items[i].is_online = 0;
        pls_audio_items[i].id_playlist_audio = -1;
        snprintf(pls_audio_items[i].item_full_path, 10, "");
    }

    /* open file success? */
    if (fp) {
        /* open file success */
        /* remove all the playlist from folder */
        printf(KYEL "Removing previous m3u files...%s\n" RESET, (system("rm -f " PATH_MPD_PLAYLIST "*.m3u") == 0) ? KGRN "OK" : KRED "ERROR");

        /* init buffer */
        memset(buff_data, 0, 1024);
        memset(buff_data2, 0, 1024);

        /* init var */
        id_playlist_control = 0;
        id_playlist_items = -1;

        /* read playlist */
        while ((fgets(buff_data, 1000, fp) != NULL) && (id_playlist_items < MAX_PLAYLISTS_ITEMS) && (id_playlist_control < MAX_PLAYLISTS_CONTROL)) {
            /* 1st line is the start time in hh:mm[:ss] */
            if (is_1st_line) {
                if (gotString(":", buff_data)) {
                    /* convert to time */
                    start_time_pls = strToTime(buff_data);

                    /* init time start and end default */
                    playlist_control[id_playlist_control].start_time = start_time_pls;
                    //                    playlist_control[id_playlist_control].switch_time = strToTime("23:59:59");

                    //init item count
                    //                    playlist_control[id_playlist_control].item_total_count = 0;

                    //debug
                    printf(KYEL "Playlist start at %s\n" RESET, ctime(&playlist_control[id_playlist_control].start_time));

                    /* incr id pls ctrl */
                    //                    id_playlist_control++;
                } else //invalid playlist
                {
                    printf(KRED "Invalid playlist\n" RESET);
                    break;
                }

                /* reset first line sign */
                is_1st_line = 0;
            } else {
                /* not the first line */

                /* parse content */
                /* file_name    type    duration_in_seconds overlaps */
                sscanf(buff_data, "%[^\t\n]\t%s\t%f\t%d", buff_data2, parse_item_type_str, &parse_item_duration, &parse_crossfade);

                /* get total duration string */
                if (seconds_float_to_time_string(parse_item_duration, parse_item_duration_str) == 0)
                    snprintf(parse_item_duration_str, 10, "00:00:00");

                /* determine file type */
                /* audio */
                if (gotString("audio", parse_item_type_str)) {
                    parse_item_type = ITEM_AUDIO;
                }/* video */
                else if (gotString("video", parse_item_type_str)) {
                    parse_item_type = ITEM_VIDEO;
                }/* image */
                else if (gotString("image", parse_item_type_str)) {
                    parse_item_type = ITEM_IMAGE;
                }/* invalid */
                else {
                    parse_item_type = ITEM_INVALID;
                }

                /* time event */
                if (gotString(":", buff_data2) && !gotString("://", buff_data2)) {
                    parse_item_type = ITEM_TIME_EVENT;
                }

                /* process it */
                /* video */
                if (parse_item_type == ITEM_VIDEO) {
                    /* form the filename */
                    snprintf(parse_item_full_name, 300, "%s%s", (!gotString("://", buff_data2)) ? PATH_MPD_VIDEO_DIR : "", buff_data2);

                    /* check file exists */
                    if (isFileExists(parse_item_full_name) || gotString("://", parse_item_full_name)) {
                        /* incr id items */
                        id_playlist_items++;

                        /* store to playlist items array */
                        playlist_items[id_playlist_items].item_type = parse_item_type;
                        playlist_items[id_playlist_items].duration = parse_item_duration;
                        playlist_items[id_playlist_items].is_online = gotString("://", buff_data2);
                        snprintf(playlist_items[id_playlist_items].item_full_path, 300, "%s", parse_item_full_name);
                        playlist_items[id_playlist_items].id_playlist_audio = -1;
                        playlist_items[id_playlist_items].id_playlist_audio_end = playlist_items[id_playlist_items].id_playlist_audio_start = -1;

                        /* incr playlist item count */
                        playlist_control[id_playlist_control].item_total_count++;

                        /* update end id */
                        playlist_control[id_playlist_control].id_item_end = id_playlist_items;
                    } else {
                        printf(KRED "Error : File %s not found\n" RESET, parse_item_full_name);
                    }

                    /* update previous type */
                    parse_item_type_previous = ITEM_VIDEO;
                }/* audio */
                else if (parse_item_type == ITEM_AUDIO) {
                    /* update id playlist */
                    if ((parse_item_type_previous != ITEM_AUDIO) && parse_item_type_previous != ITEM_INVALID) {
                        audio_playlist_id++;
                        id_playlist_items++;
                    }

                    /* form the filename */
                    snprintf(parse_item_full_name, 300, "%s%s", (!gotString("://", buff_data2)) ? PATH_MPD_MUSIC_DIR : "", buff_data2);

                    /* check file exists */
                    if (isFileExists(parse_item_full_name) || gotString("://", parse_item_full_name)) {
                        /* set playlist fname */
                        snprintf(audio_pls_name, sizeof (audio_pls_name), "%d.m3u", audio_playlist_id);
                        snprintf(audio_pls_full_name, sizeof (audio_pls_full_name), "%s%s", PATH_MPD_PLAYLIST, audio_pls_name);

                        /* add to playlist */
                        //                        strcat(buff_data2, "\r\n");
                        add_playlist_items(audio_pls_full_name, buff_data2);

                        /* store to playlist items and audio array */
                        pls_audio_items[audio_items_id].id_playlist_audio = audio_playlist_id;
                        pls_audio_items[audio_items_id].crossfade = parse_crossfade;
                        pls_audio_items[audio_items_id].duration = parse_item_duration;
                        pls_audio_items[audio_items_id].is_online = gotString("://", buff_data2);
                        snprintf(pls_audio_items[audio_items_id].item_full_path, 300, "%s", buff_data2);

                        playlist_items[id_playlist_items].item_type = parse_item_type;
                        playlist_items[id_playlist_items].id_playlist_audio = audio_playlist_id;
                        //playlist_items[id_playlist_items].is_online = 0;
                        if (playlist_items[id_playlist_items].id_playlist_audio_start == -1)
                            playlist_items[id_playlist_items].id_playlist_audio_start = audio_items_id;
                        playlist_items[id_playlist_items].id_playlist_audio_end = audio_items_id;

                        /* incr playlist item count */
                        playlist_control[id_playlist_control].item_total_count++;

                        /* update end id */
                        playlist_control[id_playlist_control].id_item_end = id_playlist_items;

                        /* incr id items */
                        //                        id_playlist_items++;
                        /* update id items */
                        //                        if ((parse_item_type_previous != ITEM_AUDIO) && parse_item_type_previous != ITEM_INVALID) {
                        //                            id_playlist_items++;
                        //                        }

                        /* inkremen id item audio */
                        audio_items_id++;
                    } else {
                        printf(KRED "Error : File %s not found\n" RESET, parse_item_full_name);
                    }

                    /* update previous type */
                    parse_item_type_previous = ITEM_AUDIO;

                }/* image */
                else if (parse_item_type == ITEM_IMAGE) {

                }/* time event */
                else if (parse_item_type == ITEM_TIME_EVENT) {
                    /* update previous pls switch time */
                    playlist_control[id_playlist_control].switch_time = strToTime(buff_data2);

                    /* init new playlist */
                    id_playlist_control++;
                    playlist_control[id_playlist_control].start_time = strToTime(buff_data2);
                    /* update start id */
                    playlist_control[id_playlist_control].id_item_start = id_playlist_items + 1;

                    /* update previous type */
                    parse_item_type_previous = ITEM_TIME_EVENT;
                }/* invalid */
                else {
                    parse_item_type_previous = ITEM_INVALID;
                    printf(KRED "Invalid item found : %s\n" RESET, buff_data2);
                }
            }
        }
        fclose(fp);

        /* check last playlist if got item */
        //        printf(KYEL "%d = %d %d\n" RESET, id_playlist_control, playlist_control[id_playlist_control].item_total_count, playlist_control[id_playlist_control - 1].item_total_count);

        /* debug info */
        //        debug_pls_control(playlist_control, id_playlist_control+1);
        //        debug_pls_items(playlist_items, id_playlist_items);

        /* return playlist len*/
        resx = id_playlist_control + 1;

    } else {
        /* open file fail */
        resx = -1;
    }

    /* return result */
    return resx;
}

/* remove old pls and rename new pls */
int update_old_pls(const char *old_pls, const char *new_pls) {
    char cmdx[300];

    /* setup the command */
    snprintf(cmdx, 300, "sudo rm -f %s", old_pls);
    if (system(cmdx) == 0) {
        /* rename the new pls */
        snprintf(cmdx, 300, "sudo mv -f \"%s\" \"%s\"", new_pls, old_pls);
        return (system(cmdx) == 0) ? 0 : -1;
    } else {
        return -1;
    }
}

/* find active playlist ide */
int find_current_time_pls(const unsigned int pls_ttl_count, const playlist_control_t *pls_ctrl_list) {
    int i = -1;

    for (i = -1; i < (int) pls_ttl_count; i++) {
        if (i>-1) {
            //printf("check %d at %s\n", i, ctime(&pls_ctrl_list[i].switch_time));
            if ((pls_ctrl_list[i].start_time <= time(NULL)) && (pls_ctrl_list[i].switch_time >= time(NULL))) {
                break;
            }
        }
    }

    return i;
}

/* check for q key to quit */
void check_keyboard_for_quit() {
#ifndef DEBUG_MODE_ON
    if (kbhit()) {
        if (getch() == 'q') {
            raise(SIGINT);
        }
    }
#endif
}

/* display datetime to screen and cursor back */
void display_datetime_cursor_back(const char is_cursor_back) {
    printf(KYEL "%s" RESET, datetime_to_string(time(NULL), "%F %H:%M:%S"));
    fflush(stdout);
    printf(is_cursor_back ? "\r" : "\n");
    fflush(stdout);
}

/* update and run mpd playlist */
int mpc_load_playlist(const int pls_id, const char is_random, const char is_repeat) {
    char cmdx[300];
    int resx = 1;

    //make sure mpc is cleared

    //make sure all omxplayer is gone
    system("sudo killall omxplayer");

#if USE_MPD_CLIENT_LIB == 1
    char buff[10];

    //mpd var
    struct mpd_connection *conn;

    //make connection, timeout 10secs
    conn = mpd_connection_new(NULL, 0, 10000);

    //check it
    if (conn) {
        //clear current playlist
        if (mpd_run_clear(conn)) {
            //clear all error
            mpd_run_clearerror(conn);

            //set random
            mpd_run_random(conn, is_random);

            //set repeat
            mpd_run_repeat(conn, is_repeat);

            //load playlist
            snprintf(buff, 6, "%d", pls_id);
            if (mpd_run_load(conn, buff)) {
                usleep(100000);

                //play it
                if (mpd_run_play(conn)) {
                    resx = 1;
                } else {
                    resx = -3;
                }
            } else {
                resx = -3;
            }
        } else {
            resx = -2;
        }

        //free conn
        mpd_connection_free(conn);
    } else {
        resx = -1;
    }

    //ordinary mpc
#else
    //clear current playlist
    if (system("mpc clear") == 0) {
        //clear error
        system("mpc clearerror");

        //set random option
        if (system((is_random ? "mpc random on" : "mpc random off")) == 0) {
            //set repeat option
            if (system((is_repeat ? "mpc repeat on" : "mpc repeat off")) == 0) {
                //update db mpc
                //if(system("mpc update") == 0){
                //load playlist
                snprintf(cmdx, 300, "mpc load %d", pls_id);
                usleep(50000);
                if (system(cmdx) == 0) {
                    //play it
                    if (system("mpc play") == 0) {
                        resx = 1;
                    } else {
                        resx = -6;
                    }
                } else {
                    resx = -5;
                }
                //}
                //else{
                //	return -4;
                //}
            } else {
                resx = -3;
            }
        } else {
            resx = -2;
        }
    } else {
        resx = -1;
    }
#endif

    return resx;
}

/*
format mpc status as hh:mm:ss
output the formatted string and the time_t value
 */
char* mpc_format_time(const char *atime_data, long *seconds_out) {
    //number of :
    //1 = mm:ss
    //2 = hh:mm:ss
    unsigned int num_sep = count_char(atime_data, ':'),
            hh, mm, ss;
    char *time_data_formated = NULL;
    long sec_total = 0;

    //valid format?
    if (num_sep == 0)
        return NULL;

    //valid buffer
    if (!(time_data_formated = (char*) calloc(16, sizeof (char))))
        return NULL;

    //scan the data according to num_sep
    hh = mm = ss = 0;
    if (num_sep == 1)
        sscanf(atime_data, "%d:%d", &mm, &ss);
    else
        sscanf(atime_data, "%d:%d:%d", &hh, &mm, &ss);

    //update seconds_out
    //max 24*3600
    //by default to time(null)
    sec_total = (hh * 3600) + (mm * 60) + ss + (long) strToTime("0:0:0");
    if (seconds_out)
        *seconds_out = sec_total;

    if (mm > 59) {
        hh = mm / 60;
        mm %= 60;
    }

    //format the time data
    snprintf(time_data_formated, 16, "%2.2d:%2.2d:%2.2d", hh, mm, ss);
    //~ time_data_formated = seconds_to_time_string(sec_total);

    return time_data_formated;
}

/* get mpc info */
int mpc_get_info(mpc_status_info_t *mpc_stat, const char *total_duration_str) {
    char fname[200] = {0}, t_total[10] = {0}, status[30] = {0}, t_elapse[10] = {0};
    unsigned int item_num = 0, total_num = 0;
    long tm_elapse = 0, tm_total = 0, tm_remain = 0;
    time_t tm_start, tm_end;
    struct tm *loc_time;
    char *current_dt = NULL, *ptr_buff = NULL;
    int resx = 0;

    //get curr datetime
    tm_end = time(NULL);
    current_dt = get_current_datetime(DATETIME_MPC_STATUS);

    //init stat result
    //assign to mpc_stat
    strcpy(mpc_stat->song, "");
    mpc_stat->status = ERR_MPC_NOT_PLAYING;
    mpc_stat->song_index = -1;
    strcpy(mpc_stat->time_elapse, "00:00:00");
    strcpy(mpc_stat->time_total, "00:00:00");
    strcpy(mpc_stat->time_start, "00:00:00");
    strcpy(mpc_stat->time_stop, "00:00:00");

#if USE_MPD_CLIENT_LIB == 1
    //mpd var
    struct mpd_connection *conn;
    struct mpd_status *status;
    struct mpd_song *songx;
    enum mpd_state song_state = MPD_STATE_UNKNOWN;
    //~ char song_uri[400];

    //make connection
    conn = mpd_connection_new(NULL, 0, 10000);

    //connection ok
    if (conn) {
        //get current status and song
        songx = mpd_run_current_song(conn);
        status = mpd_run_status(conn);

        //parse status
        if (status) {
            tm_elapse = mpd_status_get_elapsed_time(status);
            tm_total = mpd_status_get_total_time(status);
            song_state = mpd_status_get_state(status);
            item_num = (mpd_status_get_song_pos(status) >= 0) ? mpd_status_get_song_pos(status) : 0;
            mpd_status_free(status);
        }


        //parse song
        if (songx) {
            snprintf(fname, 200, "%s", mpd_song_get_uri(songx));
            mpd_song_free(songx);
        }

        //free up conn
        mpd_connection_free(conn);

        //assign result			
        //assign to mpc_stat
        strcpy(mpc_stat->song, fname);
        mpc_stat->status = (song_state == MPD_STATE_PLAY);
        mpc_stat->song_index = item_num;

        //time elapse
        ptr_buff = seconds_to_time_string(tm_elapse /*+strToTime("0:0:0")*/);
        strcpy(mpc_stat->time_elapse, ptr_buff);

        //time total
        //updet t_total for online tream
        if (total_duration_str)
            strncpy(t_total, total_duration_str, 10);
        ptr_buff = seconds_to_time_string(tm_total + strToTime("0:0:0"));
        strcpy(mpc_stat->time_total, total_duration_str ? t_total : ptr_buff);

        //time remaining = total - elapsed
        //~ tm_remain = strToTime("0:0:0") + tm_total - tm_elapse;
        tm_remain = strToTime(mpc_stat->time_total) - tm_elapse;
        ptr_buff = datetime_to_string(tm_remain, DATETIME_MPC_STATUS);
        strcpy(mpc_stat->time_remaining, ptr_buff);

        //end time
        strcpy(mpc_stat->time_stop, current_dt);

        //start time
        //subtract from t_end - tm_elapse
        tm_start = /*strToTime("0:0:0") +*/ tm_end - tm_elapse;
        //printf("%ld -- %ld -- %ld\n", tm_start, tm_end, tm_elapse);

        // Converting current time to local time
        loc_time = localtime(&tm_start);

        //format datetime
        strftime(fname, 30, DATETIME_MPC_STATUS, loc_time);

        //copy it
        strcpy(mpc_stat->time_start, fname);
    }//mpd conn fail
    else {
        resx = -1;
    }

    //ordinary system calls
#else

    char *mpc_status = NULL;

    if (run_command("mpc -f \"%file%\"", &mpc_status)) {
        //printf("%s\n", mpc_status);

        //check ada tanda #
        if (gotString("#", mpc_status)) {
            //scan the string
            sscanf(mpc_status, "%[^\t\n] %s #%d/%d %[^/] %*[/]%s", fname, status, &item_num, &total_num, t_elapse, t_total);

            //assign to mpc_stat
            strcpy(mpc_stat->song, fname);
            mpc_stat->status = gotString("play", status);
            if (item_num == 0) item_num = 1;
            mpc_stat->song_index = item_num - 1;

            //time elapse
            ptr_buff = mpc_format_time(t_elapse, &tm_elapse);
            strcpy(mpc_stat->time_elapse, ptr_buff);

            //time total
            //updet t_total for online tream
            if (total_duration_str)
                strncpy(t_total, total_duration_str, 10);
            ptr_buff = mpc_format_time(t_total, &tm_total);
            //~ strcpy(mpc_stat->time_total, ptr_buff);	
            strcpy(mpc_stat->time_total, total_duration_str ? t_total : ptr_buff);

            //time remaining = total - elapsed
            tm_remain = strToTime("0:0:0") + tm_total - tm_elapse;
            //~ tm_remain = strToTime(mpc_stat->time_total) - tm_elapse;
            ptr_buff = datetime_to_string(tm_remain, DATETIME_MPC_STATUS);
            strcpy(mpc_stat->time_remaining, ptr_buff);

            //end time
            strcpy(mpc_stat->time_stop, current_dt);

            //start time
            //subtract from t_end - tm_elapse
            tm_start = strToTime("0:0:0") + tm_end - tm_elapse;
            //printf("%ld -- %ld -- %ld\n", tm_start, tm_end, tm_elapse);

            // Converting current time to local time
            loc_time = localtime(&tm_start);

            //format datetime
            strftime(status, 30, DATETIME_MPC_STATUS, loc_time);

            //copy it
            strcpy(mpc_stat->time_start, status);
        } else {
            //assign to mpc_stat
            //~ strcpy(mpc_stat->song, "");
            //~ mpc_stat->status = ERR_MPC_NOT_PLAYING;
            //~ mpc_stat->song_index = 0;
            //~ strcpy(mpc_stat->time_elapse, "00:00:00");
            //~ strcpy(mpc_stat->time_total, "00:00:00");	
            //~ strcpy(mpc_stat->time_start, "00:00:00");
            //~ strcpy(mpc_stat->time_stop, "00:00:00");
            //~ 
            //~ //free buffer
            //~ free(mpc_status);
            //~ free(current_dt);
            //~ free(ptr_buff);

            if (gotString("error", mpc_status)) {
                resx = -1;
            } else {
                mpc_stat->status = 0;
            }
        }

        //free buffer
        free(mpc_status);
        //~ free(current_dt);
        //~ free(ptr_buff);

        //printf("%s %s %d of %d  %s from %s\n", fname, status, item_num, total_num, t_elapse, t_total);
    } else {
        free(mpc_status);
        resx = -1;
    }

#endif

    //free the buffer
    if (!ptr_buff) {
        free(ptr_buff);
        ptr_buff = NULL;
    }

    return resx;
}

/* set crossfading in sec--not exactly */
int mpc_set_crossfade(const char duration) {
    char cmdx[300];

    //setup the command
    snprintf(cmdx, 300, "mpc crossfade %d", duration);
    return system(cmdx);
}

/*
adjust the mpc start time, and play duration
substract end time with elapse time if not complete
or with length duration if complete
time_substractor = 0: with elapse time     1: with length song
 */
void mpc_status_adjust_start_stop_time(mpc_status_info_t *mpc_stat, const char time_substractor, const char update_stop_time) {
    long tm_end, tm_total_length, tm_elapse;
    time_t tm_start;
    char *ptr_buff = NULL;
    struct tm *loc_time;
    char tm_buff[30];

    if (mpc_stat) {
        //update stop time?
        if (update_stop_time) {
            ptr_buff = get_current_datetime(DATETIME_MPC_STATUS);
            strcpy(mpc_stat->time_stop, ptr_buff);
            //printf("stop at %s\n", ptr_buff);
        }

        //get time total
        ptr_buff = mpc_format_time(mpc_stat->time_total, &tm_total_length);

        //get time elapse
        ptr_buff = mpc_format_time(mpc_stat->time_elapse, &tm_elapse);

        //get time end
        ptr_buff = mpc_format_time(mpc_stat->time_stop, &tm_end);

        //update time start
        tm_start = (long) strToTime("0:0:0") + tm_end - (time_substractor ? tm_total_length : tm_elapse);

        //printf("%ld -- %ld -- %ld -- %ld -- %ld\n",(long) tm0, (long) tm_start,  tm_end, tm_total_length, tm_elapse);

        // Converting current time to local time
        loc_time = localtime(&tm_start);

        //format datetime
        strftime(tm_buff, 30, DATETIME_MPC_STATUS, loc_time);

        //copy it
        strcpy(mpc_stat->time_start, tm_buff);

        //update time elapse if needed
        if (time_substractor)
            strcpy(mpc_stat->time_elapse, mpc_stat->time_total);

        //free it
        if (ptr_buff) {
            free(ptr_buff);
            ptr_buff = NULL;
        }
    }
}

/* logging mpc status to file */
int logging_item_info(const char *device_name, const char *ip_ext, const char *ip_int, const char *time_start, const char *total_duration, const char *play_duration, const char *item_name) {
    char log_fname[400], log_file_exists = 0;
    FILE *flog;

    //build log_fname
    snprintf(log_fname, 400, "%s%s.log", PATH_LOGS_DIR, get_current_datetime(DATETIME_FMT_LOG_FILE));

    //open log file for append, create if not exist
    log_file_exists = isFileExists(log_fname);
    flog = fopen(log_fname, "a+");

    //file opened
    if (flog) {
        //add header if neccessary
        if (!log_file_exists)
            //fputs(LOG_FILE_HEADER,flog);
            fprintf(flog, "%s\t%s\t%s\n" LOG_FILE_HEADER, device_name, ip_ext, ip_int);

        //format log data
        //Time                Length             Play Duration             File
        fprintf(flog, "%s\t%s\t%s\t%s\r\n", time_start, total_duration, play_duration, item_name);
    } else {
        return -1;
    }

    fclose(flog);

    return 0;
}

/* playlist wait to start */
char playlist_wait_to_start(const playlist_control_t *pls_ctrl) {
    char resval = 0;
    char *current_pls = NULL, *current_pls_new = NULL;

    /* get curr pls */
    current_pls = today_playlist_fname();
    current_pls_new = today_new_playlist_fname();

    /* wait for start playing */
    printf(KGRN "Wait until %s\n" RESET, datetime_to_string(pls_ctrl->start_time, DATETIME_MPC_STATUS));
    for (;;) {

        /* display datetime to screen */
        display_datetime_cursor_back(1);

        /* check current time against start time */
        if (pls_ctrl->start_time <= time(NULL)) {
            printf(KGRN "Start playlist play\n" RESET);
            resval = 1;
            break;
        }

        /* check new playlist file */
        if (isFileExists(current_pls_new)) {
            /* copy it to old and break from loop */
            printf(KGRN "Load new playlist... %s\n" RESET, update_old_pls(current_pls, current_pls_new) == 0 ? "OK" : KRED "ERROR");

            /* set exit loop */
            resval = 2;
            break;
        }

        /* check for q key to quit */
        check_keyboard_for_quit();

        /* wait */
        usleep(500000);
    }

    return resval;
}

/* process active playlist */
int playlist_process_active(const playlist_control_t pls_ctrl, playlists_items_t *pls_items, playlists_audio_items_t *pls_audio_items) {
    int id_active_items, pls_exit_code;
    playlists_items_t pls_item_empty = {"", 0, 0, 0, -1, -1, -1};

    /* play the playlist */
    for (id_active_items = pls_ctrl.id_item_start; id_active_items <= pls_ctrl.id_item_end; id_active_items++) {

        /* assign status info */
        /* current item not audio? */
        if ((pls_items[id_active_items].item_type != ITEM_AUDIO)) {
            /* last item or not */
            if (id_active_items != pls_ctrl.id_item_end) {
                /* check next item type is not audio */
                if (pls_items[id_active_items + 1].item_type != ITEM_AUDIO) {
                    write_play_status_to_file(
                            /* current item */
                            basename(pls_items[id_active_items].item_full_path),
                            seconds_to_time_string(pls_items[id_active_items].duration),
                            pls_items[id_active_items].is_online,

                            /* next items */
                            basename(pls_items[id_active_items + 1].item_full_path),
                            seconds_to_time_string(pls_items[id_active_items + 1].duration),
                            pls_items[id_active_items + 1].is_online
                            );
                }/* is audio */
                else {
                    write_play_status_to_file(
                            /* current item */
                            basename(pls_items[id_active_items].item_full_path),
                            seconds_to_time_string(pls_items[id_active_items].duration),
                            pls_items[id_active_items].is_online,

                            /* next items */
                            basename(pls_audio_items[pls_items[id_active_items + 1].id_playlist_audio_start].item_full_path),
                            seconds_to_time_string(pls_audio_items[pls_items[id_active_items + 1].id_playlist_audio_start].duration),
                            pls_audio_items[pls_items[id_active_items + 1].id_playlist_audio_start].is_online
                            );
                }
            } else {
                /* last item -> get the info */
                write_play_status_to_file(
                        /* current item */
                        basename(pls_items[id_active_items].item_full_path),
                        seconds_to_time_string(pls_items[id_active_items].duration),
                        pls_items[id_active_items].is_online,

                        /* next items */
                        "-",
                        "00:00:00",
                        0
                        );
            }
        }

        /* cek type */
        /* VIDEO ITEMS */
        if (pls_items[id_active_items].item_type == ITEM_VIDEO) {
            pls_exit_code = playlist_play_video_items(pls_ctrl, pls_items[id_active_items]);
        }/* IMAGE ITEMS */
        else if (pls_items[id_active_items].item_type == ITEM_IMAGE) {
            /* info */
            printf(KYEL "Viewing image %s\n" RESET, pls_items[id_active_items].item_full_path);
        }/* AUDIO PLS ITEMS */
        else if (pls_items[id_active_items].item_type == ITEM_AUDIO) {
            /* info */
            if (pls_items[id_active_items].id_playlist_audio != -1) {
                pls_exit_code = playlist_play_audio_items(pls_ctrl, pls_items[id_active_items], pls_audio_items, (id_active_items != pls_items[id_active_items].id_playlist_audio_end) ? pls_items[id_active_items + 1] : pls_item_empty);
            } else {
                printf(KRED "Invalid playlist audio\n" RESET);
            }
        }/* INVALID ITEMS */
        else {
            printf(KRED "Invalid item\n" RESET);
        }

        /* check kbd */
        check_keyboard_for_quit();

        /* break on date change or new playlist */
        if (pls_exit_code > 0)
            break;

        /* break if date change of new pls found */

    }

    /* return the result */
    return pls_exit_code;
}

/* playlist exit code, 0 = next item (normal) */
int playlist_get_exit_code(const time_t switch_time, time_t *prev_date) {
    int ex_code = 0;

    /* date change */
    if (*prev_date != strToTime("00:00:00")) {
        ex_code = 1;
        *prev_date = strToTime("00:00:00");
    }/* got new playlist */
    else if (isFileExists(today_new_playlist_fname()))
        ex_code = 3;
        /* switch time reached */
    else if (time(NULL) >= switch_time) {
        ex_code = 2;
        //        *prev_date = strToTime("00:00:00");
    } else
        ;

    return ex_code;
}

/* process audio playlist */
int playlist_play_audio_items(const playlist_control_t pls_ctrl, const playlists_items_t pls_items, playlists_audio_items_t *pls_audio_items, playlists_items_t pls_next_items) {
    int cmd_result;
    int i, playlist_exit_code = -1, flag_audio_player_result, flag_player_exit_code;
    mpc_status_info_t mpc_status_now, mpc_status_prev;
    //    time_t curr_date = strToTime("00:00:00");

    /* info */
    printf(KYEL "Playing %d.m3u\n" RESET, pls_items.id_playlist_audio);

    /* load to mpd , retry for max 5 times with interval 5 secs delay */
    for (i = 0; i < 5; i++) {
        cmd_result = mpc_load_playlist(pls_items.id_playlist_audio, 0, 0);
        if (cmd_result != 1) {
            printf(KRED "Playlist %d.m3u cannot be loaded!!! Retry in 5secs...\n" RESET, pls_items.id_playlist_audio);
            sleep(5);
        } else {
            printf(KGRN "Playlist %d.m3u loaded successfully\n" RESET, pls_items.id_playlist_audio);
            break;
        }
    }

    /* success? */
    if (cmd_result == 1) {
        /* play the audio items . the 1st item should have been played */
        /* get player status */
        flag_audio_player_result = mpc_get_info(&mpc_status_prev, pls_audio_items[pls_items.id_playlist_audio_start].is_online ? seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start].duration) : NULL);

        /* continue if ok */
        if (flag_audio_player_result == 0) {
            /* set audio crossfaded */
            printf(KYEL "Set crossfade to %d ... " RESET, pls_audio_items[pls_items.id_playlist_audio_start].crossfade);
            printf("%s\n" RESET, mpc_set_crossfade(pls_audio_items[pls_items.id_playlist_audio_start].crossfade) == 0 ? KGRN "OK" : KRED "ERROR");

            /* update info status */
            /* more then 1 item */
            if (pls_items.id_playlist_audio_start != pls_items.id_playlist_audio_end) {
                /* update the info */
                write_play_status_to_file(
                        /* current item */
                        basename(pls_audio_items[pls_items.id_playlist_audio_start].item_full_path),
                        seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start].duration),
                        pls_audio_items[pls_items.id_playlist_audio_start].is_online,

                        /* next items */
                        basename(pls_audio_items[pls_items.id_playlist_audio_start + 1].item_full_path),
                        seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start + 1].duration),
                        pls_audio_items[pls_items.id_playlist_audio_start + 1].is_online
                        );

            } else {
                /* just 1 item */
                /* update the info */
                write_play_status_to_file(
                        /* current item */
                        basename(pls_audio_items[pls_items.id_playlist_audio_start].item_full_path),
                        seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start].duration),
                        pls_audio_items[pls_items.id_playlist_audio_start].is_online,

                        /* next items */
                        basename(pls_next_items.item_full_path),
                        seconds_to_time_string(pls_next_items.duration),
                        pls_next_items.is_online
                        );
            }

            for (;;) {
                /* get player status */
                flag_audio_player_result = mpc_get_info(&mpc_status_now, pls_audio_items[pls_items.id_playlist_audio_start].is_online ? seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start].duration) : NULL);

                /* ok */
                if (flag_audio_player_result == 0) {
                    /* show info */
                    if (mpc_status_now.song_index != -1) {
                        printf(KYEL "%4d\t%.10s\t%s" RESET, mpc_status_now.song_index, mpc_status_now.time_remaining, mpc_status_now.song);
                        fflush(stdout);
                        printf("\r");
                        fflush(stdout);
                    }

                    /* ========================================================= */
                    /* NORMAL EXIT CONDITION */
                    /* check player end condition */
                    /* normal track change */
                    if (mpc_status_now.song_index != mpc_status_prev.song_index)
                        flag_player_exit_code = 1;

                    /* end of playlist track */
                    if ((mpc_status_now.song_index == -1) && (strlen(mpc_status_now.song) == 0))
                        flag_player_exit_code = 2;

                    /* process exit action */
                    /* track change */
                    if (flag_player_exit_code == 1) {
                        /* update status file */
                        /* not last item played */
                        if (mpc_status_now.song_index != (pls_items.id_playlist_audio_end - pls_items.id_playlist_audio_start)) {
                            /* update the info */
                            write_play_status_to_file(
                                    /* current item */
                                    basename(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].item_full_path),
                                    seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].duration),
                                    pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].is_online,

                                    /* next items */
                                    basename(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index + 1].item_full_path),
                                    seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index + 1].duration),
                                    pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index + 1].is_online
                                    );
                        }/* last item played */
                        else {
                            /* last item played */
                            /* update the info */
                            write_play_status_to_file(
                                    /* current item */
                                    basename(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].item_full_path),
                                    seconds_to_time_string(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].duration),
                                    pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].is_online,

                                    /* next items */
                                    basename(pls_next_items.item_full_path),
                                    seconds_to_time_string(pls_next_items.duration),
                                    pls_next_items.is_online
                                    );
                        }

                        /* show info */
                        printf(KYEL "%4d\t%.10s\t%s" RESET, mpc_status_prev.song_index, mpc_status_prev.time_remaining, mpc_status_prev.song);
                        fflush(stdout);
                        printf("\n");
                        fflush(stdout);

                        /* update prev status */
                        mpc_status_adjust_start_stop_time(&mpc_status_prev, 1, 1);

                        /* info playing previous */
                        printf(KYEL "Status previous = %s %d %d %s %s %s %s %d\n" RESET,
                                mpc_status_prev.song,
                                mpc_status_prev.status,
                                mpc_status_prev.song_index,
                                mpc_status_prev.time_elapse,
                                mpc_status_prev.time_total,
                                mpc_status_prev.time_start,
                                mpc_status_prev.time_stop,
                                pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_prev.song_index].crossfade);

                        //logging previous status
                        printf(KYEL "Logging to file ... %s\n" RESET,
                                (
                                logging_item_info(
                                DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                                mpc_status_prev.time_start, //start time
                                mpc_status_prev.time_total, //total duration of the file
                                mpc_status_prev.time_elapse, //total playing time
                                (const char*) mpc_status_prev.song //item name
                                ) == 0
                                ) ? KGRN "OK" : KRED "ERROR"
                                );

                        /* update mpc status prev */
                        mpc_status_prev = mpc_status_now;

                        /* reset exit code */
                        flag_player_exit_code = 0;

                        /* set audio crossfaded */
                        if (mpc_status_prev.song_index != (pls_items.id_playlist_audio_end - pls_items.id_playlist_audio_start)) {
                            printf(KYEL "Set crossfade to %d ... " RESET, pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_prev.song_index + 1].crossfade);
                            printf("%s\n" RESET, mpc_set_crossfade(pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_prev.song_index + 1].crossfade) == 0 ? KGRN "OK" : KRED "ERROR");
                        }
                    }

                    /* end of playlist */
                    if (flag_player_exit_code == 2) {
                        /* show info */
                        printf(KYEL "%4d\t%.10s\t%s" RESET, mpc_status_prev.song_index, mpc_status_prev.time_remaining, mpc_status_prev.song);
                        fflush(stdout);
                        printf("\n");
                        fflush(stdout);

                        /* update prev status */
                        mpc_status_adjust_start_stop_time(&mpc_status_prev, 1, 1);

                        /* info playing previous */
                        printf(KYEL "Status previous = %s %d %d %s %s %s %s %d\n" RESET,
                                mpc_status_prev.song,
                                mpc_status_prev.status,
                                mpc_status_prev.song_index,
                                mpc_status_prev.time_elapse,
                                mpc_status_prev.time_total,
                                mpc_status_prev.time_start,
                                mpc_status_prev.time_stop,
                                pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_prev.song_index].crossfade);

                        //logging previous status
                        printf(KYEL "Logging to file ... %s\n" RESET,
                                (
                                logging_item_info(
                                DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                                mpc_status_prev.time_start, //start time
                                mpc_status_prev.time_total, //total duration of the file
                                mpc_status_prev.time_elapse, //total playing time
                                (const char*) mpc_status_prev.song //item name
                                ) == 0
                                ) ? KGRN "OK" : KRED "ERROR"
                                );

                        /* assign exit code for the curr pls */
                        playlist_exit_code = 0;

                        break;
                    }
                } else {
                    printf(KRED "Cannot get player status\n" RESET);
                }

                /*==============================================================*/
                /* ABNORMAL EXIT */
                /* get exit code */
                playlist_exit_code = playlist_get_exit_code(pls_ctrl.switch_time, &DATE_OLD);

                /* 1. if day changes */
                if (playlist_exit_code == 1) {
                    if (flag_audio_player_result == 0) {
                        /* update prev status */
                        mpc_status_adjust_start_stop_time(&mpc_status_now, 0, 1);

                        /* info playing previous */
                        printf(KYEL "Status previous = %s %d %d %s %s %s %s %d\n" RESET,
                                mpc_status_now.song,
                                mpc_status_now.status,
                                mpc_status_now.song_index,
                                mpc_status_now.time_elapse,
                                mpc_status_now.time_total,
                                mpc_status_now.time_start,
                                mpc_status_now.time_stop,
                                pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].crossfade);

                        //logging previous status
                        printf(KYEL "Logging to file ... %s\n" RESET,
                                (
                                logging_item_info(
                                DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                                mpc_status_now.time_start, //start time
                                mpc_status_now.time_total, //total duration of the file
                                mpc_status_now.time_elapse, //total playing time
                                (const char*) mpc_status_now.song //item name
                                ) == 0
                                ) ? KGRN "OK" : KRED "ERROR"
                                );
                    } else {
                        printf(KRED "Invalid playlist status, logging aborted\n" RESET);
                    }

                    /* info */
                    printf(KGRN "Switching to new date playlist\n" RESET);

                    break;
                }/* 2. switch time reach */
                else if (playlist_exit_code == 2) {
                    if (flag_audio_player_result == 0) {
                        /* update prev status */
                        mpc_status_adjust_start_stop_time(&mpc_status_now, 0, 1);

                        /* info playing previous */
                        printf(KYEL "Status previous = %s %d %d %s %s %s %s %d\n" RESET,
                                mpc_status_now.song,
                                mpc_status_now.status,
                                mpc_status_now.song_index,
                                mpc_status_now.time_elapse,
                                mpc_status_now.time_total,
                                mpc_status_now.time_start,
                                mpc_status_now.time_stop,
                                pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].crossfade);

                        //logging previous status
                        printf(KYEL "Logging to file ... %s\n" RESET,
                                (
                                logging_item_info(
                                DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                                mpc_status_now.time_start, //start time
                                mpc_status_now.time_total, //total duration of the file
                                mpc_status_now.time_elapse, //total playing time
                                (const char*) mpc_status_now.song //item name
                                ) == 0
                                ) ? KGRN "OK" : KRED "ERROR"
                                );
                    } else {
                        printf(KRED "Invalid playlist status, logging aborted\n" RESET);
                    }

                    printf(KGRN "Switching to new time playlist\n" RESET);

                    break;
                }/* 3. new playlist found */
                else if (playlist_exit_code == 3) {
                    if (flag_audio_player_result == 0) {
                        /* update prev status */
                        mpc_status_adjust_start_stop_time(&mpc_status_now, 0, 1);

                        /* info playing previous */
                        printf(KYEL "Status previous = %s %d %d %s %s %s %s %d\n" RESET,
                                mpc_status_now.song,
                                mpc_status_now.status,
                                mpc_status_now.song_index,
                                mpc_status_now.time_elapse,
                                mpc_status_now.time_total,
                                mpc_status_now.time_start,
                                mpc_status_now.time_stop,
                                pls_audio_items[pls_items.id_playlist_audio_start + mpc_status_now.song_index].crossfade);

                        //logging previous status
                        printf(KYEL "Logging to file ... %s\n" RESET,
                                (
                                logging_item_info(
                                DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                                mpc_status_now.time_start, //start time
                                mpc_status_now.time_total, //total duration of the file
                                mpc_status_now.time_elapse, //total playing time
                                (const char*) mpc_status_now.song //item name
                                ) == 0
                                ) ? KGRN "OK" : KRED "ERROR"
                                );
                    } else {
                        printf(KRED "Invalid playlist status, logging aborted\n" RESET);
                    }

                    printf(KGRN "Switching to new playlist\n" RESET);

                    break;
                } else {

                }

                /* check kbd */
                check_keyboard_for_quit();

                /* delay */
                usleep(500000);
            }

            /* get again exit code */
            playlist_exit_code = playlist_get_exit_code(pls_ctrl.switch_time, &DATE_OLD);

            /* end of playlist */
            printf(KGRN "End of playlist %d.m3u with exit code = %d\n" RESET, pls_items.id_playlist_audio, playlist_exit_code);
        } else {
            printf(KRED "Item not played\n" RESET);
        }
    } else {
        printf(KRED "Playlist %d.m3u fail to load. Exiting...\n" RESET, pls_items.id_playlist_audio);
    }

    return playlist_exit_code;
}

/* play video files */
int playlist_play_video_items(const playlist_control_t pls_ctrl, playlists_items_t pls_item) {
    int playlist_exit_code = -1, fail_count = 0;
    char plyr_status = 0;
    time_t time_start_play, time_end_play, time_must_end;
    time_t diff_dur = 0, t0, t1;

    if (isFileExists(pls_item.item_full_path)) {
        /* info */
        printf(KGRN "Playing video %s\n" RESET, pls_item.item_full_path);

        /* kill previous av player */
        kill_audio_video_player();

        /* load the file */
        t0 = time(NULL);
        plyr_status = omxplayer_load_file(pls_item.item_full_path);
        //        usleep(500000);
        usleep(50000);

        /* wait until it finish*/
        if (plyr_status) {

            /* assign the time start */
            time_start_play = time(NULL);
            time_must_end = time_start_play + pls_item.duration; //add 1 sec 

            //            printf(KGRN "%d\t%s\n" RESET, pls_item.duration, datetime_to_string(time_must_end, DATETIME_MPC_STATUS));

            /* wait for exit */
            for (;;) {

                /* display status info */
                printf(KYEL "%.10s\t%s" RESET, seconds_to_time_string(time_must_end >= time(NULL) ? difftime(time_must_end, time(NULL)) : 0), basename(pls_item.item_full_path));
                fflush(stdout);
                printf("\r");
                fflush(stdout);

                /* normal exit code */
                if (!omxplayer_is_running()) {
                    /* end of playing time */
                    t1 = time(NULL);
                    time_end_play = time(NULL);

                    /* normal exit code */
                    playlist_exit_code = 0;

                    /* play time total exceeded ? */
                    if ((difftime(time_end_play, time_start_play) >= pls_item.duration) || (time_end_play >= time_must_end)) {
                        /* info */
                        printf(KGRN "\nPlay end\n" RESET);

                        break;
                    } else {
                        /* it is fail playing?? */
                        diff_dur = t1 - t0;
                        if (diff_dur < 5)
                            fail_count++;

                        /* play it again */
                        /* info */
                        printf(KGRN "\nPlaying video %s\n" RESET, pls_item.item_full_path);

                        /* kill previous av player */
                        kill_audio_video_player();
                        //                        sleep(1);

                        /* load the file */
                        t0 = time(NULL);
                        plyr_status = omxplayer_load_file(pls_item.item_full_path);
                        usleep(500000);

                        /* break on fail */
                        if ((!plyr_status) || (fail_count >= 3)) {
                            printf(KRED "\nPlaying %s fail\n" RESET, pls_item.item_full_path);
                            break;
                        }
                    }
                }

                /* get exit code */
                playlist_exit_code = playlist_get_exit_code(pls_ctrl.switch_time, &DATE_OLD);

                /* process exit code */
                if ((playlist_exit_code >= 1) && (playlist_exit_code <= 3))
                    /* end of playing time */
                    time_end_play = time(NULL);

                /* check end time vs must end time */
                if ((playlist_exit_code == 0) && (time_must_end < time(NULL)))
                    playlist_exit_code = 2;

                /* 1. date change */
                if (playlist_exit_code == 1) {
                    /* info */
                    printf(KGRN "\nSwitching to new date playlist\n" RESET);

                    break;
                }/* 2. switch time reach */
                else if (playlist_exit_code == 2) {
                    /* info */
                    printf(KGRN "\nSwitching time reached\n" RESET);

                    break;
                }/* 3. new pls */
                else if (playlist_exit_code == 3) {
                    /* info */
                    printf(KGRN "\nSwitching to new playlist\n" RESET);

                    break;
                } else {

                }

                /* check exit keyboard */
                check_keyboard_for_quit();

                /* delay */
                //                usleep(500000);
                usleep(50000);
            }

            /* get exit code */
            playlist_exit_code = playlist_get_exit_code(pls_ctrl.switch_time, &DATE_OLD);

            /* kill av player */
            kill_audio_video_player();

            /* log the status */
            printf(KYEL "Logging to file ... %s\n" RESET,
                    (
                    logging_item_info(
                    DEV_NAME, IP_PUBLIC, IP_LOCAL, //device, ip ext, ip int  
                    datetime_to_string(time_start_play, DATETIME_MPC_STATUS), //start time
                    seconds_to_time_string(pls_item.duration), //total duration of the file
                    seconds_to_time_string(difftime(time_end_play, time_start_play)), //total playing time
                    (const char*) basename(pls_item.item_full_path) //item name
                    ) == 0
                    ) ? KGRN "OK" : KRED "ERROR"
                    );

            /* end of playlist */
            printf(KGRN "End of playing %s with exit code = %d\n" RESET, pls_item.item_full_path, playlist_exit_code);
        } else {
            printf(KRED "Playing %s fail\n" RESET, pls_item.item_full_path);
        }
    } else {
        printf(KRED "File %s not found\n" RESET, pls_item.item_full_path);
    }

    return playlist_exit_code;
}

/* check if omxplayer is running */
char omxplayer_is_running() {
    char resx = 0;

    int *listx = pidof("omxplayer.bin");
    if (listx) {
        resx = listx[0] != -1;
        free(listx);
    }

    return resx;
}

/* play movie to omxplayer*/
char omxplayer_load_file(const char *item) {
    char cmd[600];
    char *fname;
    int resx = -1;

    /* check fname */
    if (gotString("://", item))
        fname = repl_str(item, " ", "%20");
    else {
        fname = (char*) calloc(300, sizeof (char));
        if (fname)
            strncpy(fname, item, 300);
    }

    /* kill previous screen session */
    //    system("screen -S movie -X kill");
    if (omxplayer_is_running()) {
        system("sudo killall omxplayer.bin");
        usleep(50000);
    }

    /* format command */
    //    snprintf(cmd, sizeof (cmd), "screen -dmS movie sh -c 'omxplayer -b --layer 10 --no-osd --vol 100 -o both \"%s\"'", fname);
    //        snprintf(cmd, sizeof (cmd), "screen -dmS movie omxplayer -b --no-osd --vol 100 -o both \"%s\"", fname);
    snprintf(cmd, sizeof (cmd), "nohup omxplayer -b --no-osd --vol 100 -o both \"%s\" > /dev/null 2>&1 &", fname);
    //black out the screen
    //    system("setterm -cursor off && clear");
    //        snprintf(cmd, sizeof (cmd), "omxplayer -b --layer 10 --no-osd --vol 100 -o both \"%s\"", fname);
    resx = system(cmd);
    //    printf("%s = %d\n", cmd, resx);

    return ( resx == 0);
}

/* write status file */
char write_status_info_file(const char *status) {
    char cmd[500];
    snprintf(cmd, 500, "echo \"%s\" | sudo tee " INFO_PLAYED_FILE " > /dev/null", status);
    return (system(cmd) == 0);
}

/* write played file */
void write_play_status_to_file(const char *curr_item_name, const char *curr_item_duration, const char curr_item_is_online, const char *next_item_name, const char *next_item_duration, const char next_item_is_online) {

    char sts[500];

    snprintf(sts, 500, "{\\\"t\\\":\\\"%s\\\",\\\"p\\\":{\\\"name\\\":\\\"%s\\\",\\\"local\\\":\\\"%s\\\",\\\"dur\\\":\\\"%s\\\"},\\\"n\\\":{\\\"name\\\":\\\"%s\\\",\\\"local\\\":\\\"%s\\\",\\\"dur\\\":\\\"%s\\\"}}",
            datetime_to_string(time(NULL), DATETIME_MPC_STATUS),
            curr_item_name, curr_item_is_online ? "STREAMING" : "LOCAL",
            curr_item_duration,
            next_item_name, next_item_is_online ? "STREAMING" : "LOCAL",
            next_item_duration
            );

    /* info */
    printf(KYEL "Write status file ... %s\n" RESET, write_status_info_file(sts) ? KGRN "OK" : KRED "ERROR");
}

#ifdef LCD_I2C_ON

/* lcd init */
char lcd_initialize(int *lcd_hdl_out, lcd *lcd_obj) {
    char resx = 0;
    char lcd_buffer[21];

    /* open i2c dev */
    *lcd_hdl_out = open_i2c(I2C_FILE_NAME, I2C_SLAVE_ADDR);

    /* info */
    printf(KYEL "Opening I2C bus %s ... %s\n" RESET, I2C_FILE_NAME, *lcd_hdl_out >= 0 ? KGRN "OK" : KRED "ERROR");

    /* continue on ok */
    if (*lcd_hdl_out >= 0) {
        /* info */
        printf(KGRN "Opening I2C device %s OK\n" RESET, I2C_FILE_NAME);

        /* init lcd */
        lcd_init(lcd_obj, *lcd_hdl_out);
        sleep(1);
        lcd_init(lcd_obj, *lcd_hdl_out);

        /* clear lcd */
        lcd_clear(lcd_obj);
        usleep(10000);

        /* display hostname */
        snprintf(lcd_buffer, 21, "%-20s", DEV_NAME);
        lcd_print(lcd_obj, lcd_buffer, strlen(lcd_buffer), 0);

        /* display current datetime */
        usleep(50000);
        snprintf(lcd_buffer, 21, "%-20s", datetime_to_string(time(NULL), "%F %H:%M:%S"));
        lcd_print(lcd_obj, lcd_buffer, strlen(lcd_buffer), 1);

        /* info */
        printf(KGRN "LCD initialization done\n" RESET);
    } else {
        printf(KRED "Cannot open I2C device %s\n" RESET, I2C_FILE_NAME);
    }

    return resx;
}
#endif

/* read rtc clock adjusted to local timezone
 * http://stackoverflow.com/questions/202116/how-do-you-programmatically-set-the-hardware-clock-on-linux
 */
time_t rtc_read_clock() {
    int resx = -1;
    struct rtc_time rtcx;
    time_t result = time(NULL);
    int fd = open("/dev/rtc", O_RDONLY);

    if (fd != -1) {
        resx = ioctl(fd, RTC_RD_TIME, &rtcx);
        if (resx != -1) {
            /* get gmt offset in seconds */
            struct tm *loctime = localtime(&result);
            long sec_gmt_off = loctime->tm_gmtoff;

            /* copy yday and isdst */
            rtcx.tm_yday = loctime->tm_yday;
            rtcx.tm_isdst = loctime->tm_isdst;

            /* result in utc */
            result = mktime((struct tm *) &rtcx);

            /* convert to local by adding gmtoffset */
            result += sec_gmt_off;
        }
        close(fd);
    }

    return result;
}

/*
 * main enstreamer program start here 
 */
int main(int argc, char** argv) {

    /* variables */

    /* playlist var */
    char *current_pls = NULL, *current_pls_new = NULL;
    int playlist_total_item = 0;
    char flag_player_start_playing = 0;
    int active_pls_id = -1, flag_playlist_exit_result = 0, id_pls;

    /****************************************************************************/

    //blank the screen
    //    system("setterm -foreground black -background black");
    system("clear");

    /* initialize current date */
    DATE_OLD = get_date(rtc_read_clock());
    //printf("%s\t%s\n", ctime(&DATE_OLD), datetime_to_string(time(NULL), DATETIME_MPC_STATUS));

    /* compare to the system time and updet if neccessary */
    if (time(NULL) < rtc_read_clock()) {
        printf(KGRN "Updating system datetime\n" RESET);
        system("sudo hwclock -s");
    }

    /* start program */
    printf(KGRN "%s\nVersion\t: %s\nID\t: %s\n\n" RESET, PROGRAM_TITLE, PROGRAM_VERSION, getcpuid());

    /* limit ram to max xxx MB */
    //    limit_ram_usage(256000000);

    /* CTRL+C signal handler */
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf(KRED "Cannot attach signal interrupt handler\n" RESET);
    }

    /* save device id to file */
    save_id_file();
    if (validatekey() != 0) {
        printf(KRED "REGISTER THIS PROGRAM\n" RESET);
        return EXIT_SUCCESS;
    }

    /* read ip public, local , and hostname */
    get_and_show_network_info(IP_PUBLIC, IP_LOCAL, DEV_NAME);

    /* stop all previous player */
    kill_audio_video_player();

#ifdef LCD_I2C_ON
    /* init lcd */
    lcd_initialize(&LCD_I2C_HDL, &LCD_DEV);
#endif

    /* main loop program */
ENSTREAMER_MAIN_LOOP:
    for (;;) {
        /* get current day playlist */
        //        curr_date = strToTime("00:00:00");
        current_pls = today_playlist_fname();
        current_pls_new = today_new_playlist_fname();

        /* check if new pls found */
        if (isFileExists(current_pls_new)) {
            /* copy it to old */
            printf(KGRN "Load new playlist... %s\n" RESET, update_old_pls(current_pls, current_pls_new) == 0 ? "OK" : KRED "ERROR");
        }

        /* check current playlist */
        if (isFileExists(current_pls)) {
            /* parse it */
            printf(KYEL "Playlist %s found. Start parsing...\n" RESET, current_pls);

            /* parse it */
            playlist_total_item = parse_playlist(current_pls, playlist_controls, playlist_items, playlist_audio_items);

            /* just info */
            //            debug_pls_control(playlist_controls, playlist_total_item);
            //            debug_pls_items(playlist_items, playlist_controls[playlist_total_item - 1].id_item_end + 1);
            //            debug_pls_audio_items(playlist_audio_items);

            /* process playlist parse */
            if (playlist_total_item != 0) {
                /* process the playlist */
                printf(KGRN "Found %d playlist and %d items to be played\n" RESET, playlist_total_item, playlist_controls[playlist_total_item - 1].id_item_end + 1);

                /* just info */
                //                debug_pls_control(playlist_controls, playlist_total_item);

                /* wait for start playing */
                flag_player_start_playing = playlist_wait_to_start(&playlist_controls[0]);

                /* exit to main loop or play it ? */
                if (flag_player_start_playing == 1) {

                    /* loop all pls */
                    for (id_pls = 0; id_pls < playlist_total_item; id_pls++) {
                        /* is it active pls? */
                        if ((playlist_controls[id_pls].start_time <= time(NULL)) && (playlist_controls[id_pls].switch_time >= time(NULL))) {
                            /* process the playlist */
                            flag_playlist_exit_result = playlist_process_active(playlist_controls[id_pls], playlist_items, playlist_audio_items);

                            /* info */
                            printf(KGRN "Playlist #%d done (%d)\n" RESET, id_pls, flag_playlist_exit_result);

                            /* break */
                            if ((flag_playlist_exit_result == 1) || (flag_playlist_exit_result == 3)) {
                                if (flag_playlist_exit_result == 1)
                                    DATE_OLD = strToTime("00:00:00");
                                break;
                            }
                        }
                    }

                    /* got no active playlist */
                    printf(KRED "Got no active playlist to be played\n" RESET);

                    /* kill all audio video player */
                    kill_audio_video_player();
                }
            } else {
                printf(KRED "No playlist items found!!!\n" RESET);
                sleep(5);
            }
        } else {
            /* file not found , check default playlist */
            printf(KRED "Current playlist %s not found. Searching default playlist...\n" RESET, current_pls);
            if (isFileExists(DEFAULT_PLAYLIST_FILE)) {
                printf(KYEL "Copying default playlist...%s\n" RESET, (copy_default_playlist(current_pls) == 0) ? "OK" : "ERROR");
            } else {
                printf(KRED "No default playlist %s found. Sleep for 5 seconds...\n" RESET, DEFAULT_PLAYLIST_FILE);
                sleep(5);
            }
        }
    }

    /* cleanup */
    free(current_pls);
    free(current_pls_new);

    return EXIT_SUCCESS;
}

