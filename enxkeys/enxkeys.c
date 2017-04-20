/* 
 * File:   enxkeys.c
 * Author: EX4
 *
 * Created on January 28, 2016, 9:54 AM
 */
/* gnu source specific function */
#define _GNU_SOURCE

/* no xkey reading, just for simulation */
//#define _XKEY_ACTIVE

/* via keyboard for testing */
#define _KEYBOARD_ON

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

//xkey library
#include <PieHid32.h>

//pool
#include "thpool.h"

//parser ini
#include "iniparser.h"

//utility
#include "datetime_util.h"
#include "string_util.h"
#include "system_util.h"
#include "file_util.h"
#include "text_color.h"
#include "mpd_util.h"

//config file
#define COMMAND_FILE_PATH    "/data/enxkeys/enxkeys.conf"

//program detail
#define PROGRAM_INFO    "ENXCMD\n" \
"03032016154611\n" \
"================\n\n"

/* playlist config */
#define MAX_PLAYLISTS_CONTROL       200
#define MAX_PLAYLISTS_ITEMS         100

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

/* info played item */
#define INFO_PLAYED_FILE    "/dev/shm/enstreamer-status"

/* item type */
#define ITEM_NONE           0xfe
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

//command language

typedef enum {
    CMD_VIDEO_FILE, CMD_VIDEO_FILE_LOOP,
    CMD_VIDEO_PLAYLIST, CMD_VIDEO_PLAYLIST_LOOP,
    CMD_IMAGE, CMD_IMAGE_LOOP,
    CMD_SLIDESHOW, CMD_SLIDESHOW_LOOP,
    CMD_AUDIO_FILE, CMD_AUDIO_FILE_LOOP,
    CMD_AUDIO_PLAYLIST, CMD_AUDIO_PLAYLIST_LOOP,
    CMD_DEFAULT
} cmd_type_enum;

//command string
const char *cmd_string[] = {
    "video", "video_loop",
    "plsvid", "plsvid_loop",
    "image", "image_loop",
    "slideshow", "slideshow_loop",
    "audio", "audio_loop",
    "plsaud", "plsaud_loop",
    "default"
};

//command detail type

typedef struct {
    cmd_type_enum cmd_type;
    char file_item[64];
    char is_looped;
} cmd_item_t;

//command list for all button

typedef struct {
    uint8_t cmd_count,
    item_start_id;
} cmd_list_t;

//xkey cmd struct

typedef struct {
    char btn_id, rerun;
} xkey_cmd_t;

/* image slideshow structure */
typedef struct image_slideshow_s {
    time_t start_time;
    char item_name[300];
} image_slideshow_t;

/* max item count image */
#define MAX_IMAGE_ITEM_COUNT    300

/* struct data for audio items */
typedef struct {
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

//proto
#ifdef _XKEY_ACTIVE
long xkey_find_and_attach();
unsigned int xkey_handle_data_event(unsigned char *pData, unsigned int deviceID, unsigned int error);
#endif

void dbg_btn_cmd(cmd_list_t *adata);
void dbg_cmd_item(cmd_item_t *items);
char parse_string_to_command_item(char *input, cmd_item_t *acmd_item);
void parse_command_file(dictionary *cfgx);
void kill_all_media_programs(const char kill_fbi, const char kill_mpc);
char omxplayer_is_running();
char omxplayer_load_file(const char *item);
char write_status_info_file(const char *status);
void write_play_status_to_file(const char *curr_item_name, const char *curr_item_duration, const char curr_item_is_online, const char *next_item_name, const char *next_item_duration, const char next_item_is_online);


//global var
//for xkey stick handle
long xkey_handle = -1;
xkey_cmd_t xkey_cmd = {0};

//thread var
uint8_t ALL_THREAD_END = 0;
threadpool thpool;

//parser var
dictionary *config_obj;

//button cmd list
cmd_list_t button_command_list[4] = {0};
cmd_item_t command_item[32] = {0};


#ifdef _XKEY_ACTIVE

/*
 xkey data handle event
 */
unsigned int xkey_handle_data_event(unsigned char *pData, unsigned int deviceID, unsigned int error) {

    //assign to button data
    //1: pressed
    //0: released
    uint8_t btn1 = pData[3] & 1,
            btn2 = pData[4] & 1,
            btn3 = pData[5] & 1,
            btn4 = pData[6] & 1;

    //do action
    if (btn1) {
        xkey_cmd.btn_id = 1;
    } else if (btn2) {
        xkey_cmd.btn_id = 2;
    } else if (btn3) {
        xkey_cmd.btn_id = 3;
    } else if (btn4) {
        xkey_cmd.btn_id = 4;
    } else {

    }

    //rerun??
    if (btn1 || btn2 || btn3 || btn4) {
        xkey_cmd.rerun = 1;
    }

    //return the call
    //always true
    return 1;
}

/*
 find an attached xkey device
 */
long xkey_find_and_attach() {
    TEnumHIDInfo info[128];
    long count, handle = -1;
    int i;
    unsigned int result;

    result = EnumeratePIE(PI_VID, info, &count);

    //no xkey device found
    if (result != 0) {
        return -1;
    }

    //no xkey found
    if (count == 0) {
        return -1;
    }

    //attach the 1st xkey found
    for (i = 0; i < count; i++) {
        TEnumHIDInfo *dev = &info[i];
        printf(KYEL "Found XKeys Device:\n");
        printf("\tPID: %04x\n", dev->PID);
        printf("\tUsage Page: %04x\n", dev->UP);
        printf("\tUsage:      %04x\n", dev->Usage);
        printf("\tVersion: %d\n\n" KRESET, dev->Version);

        //        if (dev->UP == 0x000c && dev->Usage == 0x0001) 
        {
            /* This is the splat interface, */

            handle = dev->Handle;

            result = SetupInterfaceEx(handle);
            if (result != 0) {
                printf(KRED "Error Setting up PI Engineering Device\n" KRESET);
            } else {
                if (dev->PID == 1027) {
                    printf(KGRN "Found Device: X-keys XK-24, PID=1027\n" KRESET);
                } else if (dev->PID == 1028) {
                    printf(KGRN "Found Device: X-keys XK-24, PID=1028\n" KRESET);
                } else if (dev->PID == 1029) {
                    printf(KGRN "Found Device: X-keys XK-24, PID=1029\n" KRESET);
                } else if (dev->PID == 1127) {
                    printf(KGRN "Found Device: X-keys XK-4, PID=1127\n" KRESET);
                }

                /* Set up the Data Callback. */
                result = SetDataCallback(handle, xkey_handle_data_event);
                if (result != 0) {
                    printf(KRED "Error setting event callback\n" KRESET);
                }

                SuppressDuplicateReports(handle, 0);
                break;
            }
        }
    }

    return handle;
}
#endif

/*
 parse string to get cmd type and cmd file if any
 */
char parse_string_to_command_item(char *input, cmd_item_t *acmd_item) {
    uint8_t i, maxlen;
    char resx = 0, *pcmd;

    //reset result
    acmd_item->cmd_type = CMD_DEFAULT;
    strncpy(acmd_item->file_item, "", 1);

    maxlen = sizeof (cmd_string) / sizeof (*cmd_string);
    for (i = 0; i < maxlen; i++) {
        //we got a command?
        //        if ((pcmd = strstr(input, cmd_string[i]))) {
        if ((pcmd = strcasestr(input, cmd_string[i]))) {
            /* cmd type */
            acmd_item->cmd_type = i;

            /* item file */
            if (acmd_item->cmd_type != CMD_DEFAULT)
                strncpy(acmd_item->file_item, pcmd + strlen(cmd_string[i]) + 1, strlen(input) - strlen(cmd_string[i]));

            /* loop or not */
            acmd_item->is_looped = ((i == CMD_AUDIO_FILE_LOOP) || (i == CMD_AUDIO_PLAYLIST_LOOP) || (i == CMD_IMAGE_LOOP) || (i == CMD_SLIDESHOW_LOOP) || (i == CMD_SLIDESHOW_LOOP) || (i == CMD_VIDEO_FILE_LOOP) || (i == CMD_VIDEO_PLAYLIST_LOOP));

            /* got a match command */
            resx = 1;
            //            break;
        }
    }

    return resx;
}

/*
 print btn cmd list data
 */
void dbg_btn_cmd(cmd_list_t *adata) {
    char i;
    for (i = 0; i < 4; i++) {
        printf("%d\t%d\t%d\n", i + 1, adata[i].item_start_id, adata[i].cmd_count);
    }
}

/*
 print cmd item
 */
void dbg_cmd_item(cmd_item_t *items) {
    char i;
    for (i = 0; i < 10; i++) {
        //        if(items[i].cmd_type!=0){
        {
            printf("%d\t%d\t%s\n", i + 1, items[i].cmd_type, items[i].file_item);
        }
    }
}

/*
 parse command file to button action and items
 */
void parse_command_file(dictionary *cfgx) {
    //parse command file
    uint8_t idx, id_cmd_item = 0;
    char keyx[32], *token_cmd, *buffstr;
    const char *cmdx;

    //reset content
    memset(button_command_list, 0, sizeof (button_command_list));
    memset(command_item, 0, sizeof (command_item));

    //parse it
    for (idx = 0; idx < 4; idx++) {
        //get the command content if any
        snprintf(keyx, 32, ":%d", idx + 1);
        cmdx = iniparser_getstring(cfgx, keyx, "");

        //parse the content
        if (strlen(cmdx) != 0) {
            button_command_list[idx].item_start_id = id_cmd_item;

            printf(KYEL "Parsing : %s\n" KRESET, cmdx);

            //duplicate the string
            buffstr = strdup(cmdx);

            //loop to the token
            while (buffstr != NULL) {
                token_cmd = strsep(&buffstr, ",");
                printf(KYEL "Got %s\n" KRESET, token_cmd);

                //process it
                if (parse_string_to_command_item(token_cmd, &command_item[id_cmd_item])) {
                    id_cmd_item++;
                    button_command_list[idx].cmd_count++;
                }
            }
        } else {
            button_command_list[idx].cmd_count = 0;
            button_command_list[idx].item_start_id = 0;
        }
    }

    /*
     print it for debugging
     */
    //        dbg_btn_cmd(button_command_list);
    //        dbg_cmd_item(command_item);
}

//kill all media player

void kill_all_media_programs(const char kill_fbi, const char kill_mpc) {
    //clear console
    //    system("clear");

    //stop enstreamer service
    system("sudo systemctl stop enstreamer > /dev/null 2>&1");

    //stop and reset mpc
    if (kill_mpc)
        system("mpc stop; mpc clear; mpc clearerror; mpc random off; mpc repeat off; mpc volume 100");

    //stop omxplayer
    system("sudo killall omxplayer.bin");

    //stop fbi
    if (kill_fbi)
        system("sudo killall fbi");
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
    if (omxplayer_is_running()) {
        system("sudo killall omxplayer.bin");
        usleep(50000);
    }

    /* format command */
    snprintf(cmd, sizeof (cmd), "nohup omxplayer -b --no-osd --vol 100 -o both \"%s\" > /dev/null 2>&1 &", fname);
    //    printf("Running : %s\n", cmd);
    resx = system(cmd);

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
    printf(KYEL "Write status file ... %s\n" KRESET, write_status_info_file(sts) ? KGRN "OK" : KRED "ERROR");
}

/*
 play video file looped or not
 */
int play_video_file(const char *vidname, const char is_looped, char *exit_flag) {

    /* sanity check */
    if ((strlen(vidname) == 0) || (!isFileExists(vidname)))
        return -1;

    /* info */
    //    printf(KGRN "Playing video %s\n" RESET, vidname);

    /* kill previous av player */
    kill_all_media_programs(1, 1);

    /* load the file */
    char plyr_status = omxplayer_load_file(vidname);
    usleep(500000);

    /* update player info */
    //    write_play_status_to_file(basename(vidname), "-", 1, "?", "?", 0);

    /* loop it */
    for (;;) {
        /* check if player is running */
        if (!omxplayer_is_running()) {
            /* kill previous av player */
            kill_all_media_programs(1, 1);

            /* restart it */
            if (is_looped) {
                plyr_status = omxplayer_load_file(vidname);
            } else {
                break;
            }
        }

        /* need to exit? */
        if (*exit_flag) {
            /* kill previous av player */
            kill_all_media_programs(1, 1);

            break;
        }

        /* delay loop */
        usleep(500000);
    }

    /* resturn */
    return 0;
}

/*
 view image file
 */
char view_image_file(const char *fname, const int duration) {
    char resx = 0,
            cmdx[500];

    snprintf(cmdx, 500, "sudo fbi -noverbose -a -1 -T 1 -t %d \"%s\" > /dev/null 2>&1", duration, fname);

    resx = system(cmdx);

    return resx;
}

/*
 play image file looped or not
 */
int play_image_file(const char *imagename, const char is_looped, xkey_cmd_t *key_cmd) {

    /* sanity check */
    if ((strlen(imagename) == 0) || (isFileExists(imagename)))
        return -1;

    /* info */
    printf(KGRN "Playing image %s\n" RESET, imagename);

    /* kill previous av player */
    kill_all_media_programs(1, 0);

    /* load the file */
    char plyr_status = view_image_file(imagename, 24 * 3600);
    usleep(50000);

    /* update player info */
    write_play_status_to_file(imagename, "-", 1, "?", "?", 0);

    /* loop it */
    for (;;) {
        /* check if player is running */
        if (!process_is_running("fbi")) {
            /* kill previous av player */
            kill_all_media_programs(1, 0);

            /* restart it */
            if (is_looped) {
                plyr_status = view_image_file(imagename, 24 * 3600);
            } else {
                break;
            }
        }

        /* need to exit? */
        if (key_cmd->rerun) {
            /* kill previous av player */
            kill_all_media_programs(1, 0);

            break;
        }

        /* delay loop */
        usleep(500000);
    }

    /* resturn */
    return 0;
}

/*
 play single audio file
 */
char mpc_play_audio(const char *fname, const char looped) {
    char cmdx[256];
    int retx;

    /* build up the command */
    snprintf(cmdx, sizeof (cmdx), "mpc add \"%s\"", fname);

    /* run it */
    retx = system(cmdx);
    if (retx != -1) {
        /* repeat or not */
        if (looped)
            system("mpc repeat on");
        else
            system("mpc repeat off");

        /* play it */
        retx = system("mpc play");
    }

    /* return */
    return (retx != -1) ? 1 : 0;
}

/*
 play audio file looped or not
 */
int play_audio_file(const char *audioname, const char is_looped, char *exit_flag) {

    char resx = 0;

    //mpd status var
    mpd_status_data_t mpd_old_status, mpd_new_status;

    /* sanity check */
    //    if ((strlen(audioname) == 0) || (isFileExists(audioname)))
    if (strlen(audioname) == 0)
        return -1;

    /* info */
    printf(KGRN "Playing audio %s\n" RESET, audioname);

    /* kill previous av player */
    kill_all_media_programs(0, 1);

    /* load the file */
    char plyr_status = mpc_play_audio(audioname, is_looped);
    usleep(50000);

    //get mpd status  
    mpd_get_status_data(&mpd_new_status);

    //save info to file
    write_play_status_to_file(mpd_new_status.item_uri, seconds_to_time_string(mpd_new_status.item_duration), 0, "", "", 0);

    //show the info
    system("mpc");

    /* loop it */
    for (;;) {
        /* check status */
        mpd_get_status_data(&mpd_new_status);
        if (mpd_new_status.error != 0) {
            break;
        } else {
            if (mpd_new_status.elapse_sec == mpd_new_status.item_duration) {
                if (!is_looped) {
                    sleep(1);
                    break;
                }
            }
        }

        /* need to exit? */
        if (*exit_flag) {
            resx = 0xff;
            break;
        }

        /* delay loop */
        usleep(10000);
    }

    /* kill previous av player */
    kill_all_media_programs(0, 1);

    /* resturn */
    return resx;
}

/* parse image playlist */
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
                printf(KYEL "Adding #%4d. %s\t%s\n" KRESET, id_item, item_time, buff_data2);
            } else {
                //debug message
                printf(KYEL "File not found %s\n" KRESET, buff_data2);
            }
        }
        fclose(fp);

        printf(KYEL "Found %d items\n" KRESET, id_item);
        return id_item;
    } else {
        return 0;
    }
}

/*
 view an image file or an image playlist file
 item must be in full name path
 */
char play_image_file_or_playlist(char *item, const char is_looped, char *exit_flag) {
    /* var */
    char resx = 0;
    image_slideshow_t item_slideshow[MAX_IMAGE_ITEM_COUNT] = {0};
    unsigned int items_count = 0, id_item;
    time_t end_time, curr_datetime, curr_date;

    /*
     check if it is playlist or file
     */
    char is_playlist = gotString(".img.pls", item);

    /*
     load to items
     */
    if (is_playlist) {
        //parse playlist
        items_count = parse_image_playlist(item, item_slideshow);
    } else {
        //load the items
        strncpy(item_slideshow[0].item_name, item, sizeof (item_slideshow[0].item_name));
        item_slideshow[0].start_time = strToTime("00:00:00");
        items_count = 1;
    }

    /*
     view it
     */
    if (items_count > 0) {
        resx = 1;

        /*
         check first start time
         */
        printf(KGRN "Waiting start time at %s\n" KRESET, ctime(&item_slideshow[0].start_time));
        while (time(NULL) < item_slideshow[0].start_time) {
            //show current date time
            printf(KYEL "%s\r" KRESET, datetime_to_string(time(NULL), DATETIME_MPC_STATUS));
            fflush(stdout);
        }
        printf("\n");

        /*
         play it
         */
        id_item = 0;
        for (;;) {
            /* get current date */
            curr_date = strToTime("00:00:00");

            /* get end time */
            /* calc next switch time */
            if (id_item != (items_count - 1)) {
                end_time = item_slideshow[id_item + 1].start_time;
            } else {
                end_time = strToTime("23:59:59");
            }

            /*
             check start time and play it
             */
            curr_datetime = time(NULL);
            if ((curr_datetime >= item_slideshow[id_item].start_time) && (curr_datetime <= end_time)) {
                /* view it */
                if (isFileExists(item_slideshow[id_item].item_name)) {
                    printf(KGRN "Viewing item %s until %s\n" KRESET, basename(item_slideshow[id_item].item_name), datetime_to_string(end_time, DATETIME_MPC_STATUS));
                    system("sudo killall fbi");
                    usleep(100000);
                    view_image_file(item_slideshow[id_item].item_name, 24 * 3600);

                    /* play status info */
                    write_play_status_to_file(
                            basename(item_slideshow[id_item].item_name), "-", 0,
                            (id_item != (items_count - 1)) ? basename(item_slideshow[id_item + 1].item_name) : "-", "-", 0);

                    /* wait until next img start time or new date or new command */
                    for (;;) {
                        //show current date time
                        printf(KYEL "%s\r" KRESET, datetime_to_string(time(NULL), DATETIME_MPC_STATUS));
                        fflush(stdout);

                        /*
                        got new command
                         */
                        if (*exit_flag) {
                            resx = 0xff;
                            break;
                        }

                        /*
                         new date
                         */
                        if (curr_date != strToTime("00:00:00")) {
                            break;
                        }

                        /*
                         curr time >= end time
                         */
                        if (end_time <= time(NULL)) {
                            break;
                        }

                        /* delay */
                        usleep(500000);
                    }
                } else {
                    printf(KRED "File %s not found\n" KRESET, basename(item_slideshow[id_item].item_name));
                }

                /*
                 switch to next item
                 */
                printf(KGRN "Switching to next item\n" KRESET);
            }

            /*
             looped or not
             update id item
             */
            id_item++;
            if (id_item >= items_count) {
                if (is_looped) {
                    id_item = 0;
                } else {
                    break;
                }
            }

            /*
             got new command
             */
            if (*exit_flag) {
                resx = 0xff;
                break;
            }
        }
    } else {
        printf(KRED "No image item found\n" KRESET);
    }

    /* clear all console */
    //    system("clear");

    /* return */
    return resx;
}

/*
 parse video playlist
 */
unsigned int parse_video_playlist(const char *playlist_file, char **parse_results) {
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
            //parse : item_name
            sscanf(buff_data, "%[^\t\n]", &item_name);
            snprintf(buff_data2, 1024, PATH_MPD_VIDEO_DIR "%s", item_name);

            //remove any cr lf
            buff_data2[gotchar('\n', buff_data2)] = 0;
            buff_data2[gotchar('\r', buff_data2)] = 0;

            if (isFileExists(buff_data2)) {
                //save to the item result             
                parse_results[id_item] = (char*) calloc(strlen(buff_data2) + 1, sizeof (char));
                strcpy(parse_results[id_item], buff_data2);

                //incr id item
                id_item++;

                //debug message
                printf(KYEL "Adding #%4d. %s\t%s\n" KRESET, id_item, item_time, buff_data2);
            } else {
                //debug message
                printf(KYEL "File not found %s\n" KRESET, buff_data2);
            }
        }
        fclose(fp);

        printf(KYEL "Found %d items\n" KRESET, id_item);
        return id_item;
    } else {
        return 0;
    }
}

/*
 play video file or video playlist
 item must be in full path name
 */
char play_video_file_or_playlist(char *item, const char is_looped, char *exit_flag) {
    /* var */
    char resx = 0;
    char *videos[100];
    unsigned int items_count = 0, id_item;

    /*
     check if it is playlist or file
     */
    char is_playlist = gotString(".pls", item);

    /*
     load to items
     */
    if (is_playlist) {
        //parse playlist
        items_count = parse_video_playlist(item, videos);
    } else {
        //load the items
        //        printf("%d\n", strlen(item));
        videos[0] = (char*) calloc(strlen(item) + 1, sizeof (char));
        strncpy(videos[0], item, strlen(item));
        items_count = 1;
    }

    /*
     view it
     */
    if (items_count > 0) {
        resx = 1;

        /*
         play it
         */
        id_item = 0;
        for (;;) {

            /*
             play it
             */
            write_play_status_to_file(basename(videos[id_item]), "-", 0,
                    (id_item < (items_count - 1)) ? videos[id_item + 1] : "", "-", 0);
            printf(KYEL "Playing %s ... " KRESET, videos[id_item]);
            fflush(stdout);
            if (play_video_file(videos[id_item], 0, exit_flag) == 0) {
                printf(KGRN "OK\n" KRESET);
            } else {
                printf(KRED "ERROR\n" KRESET);
            }

            /*
             got new command
             */
            if (*exit_flag) {
                resx = 0xff;
                break;
            }

            /*
             looped or not
             update id item
             */
            id_item++;
            if (id_item >= items_count) {
                if (is_looped) {
                    id_item = 0;
                } else {
                    break;
                }
            }
        }
    } else {
        printf(KRED "No video item found\n" KRESET);
    }

    /* free memory */
    for (id_item = 0; id_item < items_count; id_item++) {
        free(videos[id_item]);
    }
    //        free(videos);

    /* return */
    return resx;
}

/* set crossfading in sec--not exactly */
int mpc_set_crossfade(const char duration) {
    char cmdx[300];

    //setup the command
    snprintf(cmdx, 300, "mpc crossfade %d", duration);
    return system(cmdx);
}

/* add playlist item to playlist file */
char add_playlist_items(const char *playlist_fname, const char *playlist_item) {
    char cmd[500];
    snprintf(cmd, sizeof (cmd), "echo \"%s\" | sudo tee -a %s > /dev/null 2>&1", playlist_item, playlist_fname);
    return (system(cmd) == 0);
}

/* parse playlist file */

int parse_and_add_playlist_file(const char *playlist_file, playlists_audio_items_t *pls_audio_items) {
    /* open the file */
    FILE *fp = fopen(playlist_file, "r");
    char buff_data[1024], buff_data2[1024], is_1st_line = 1;
    int/* id_playlist_control = 0, */ id_playlist_items = 0;

    /* time var */
    //    time_t start_time_pls;

    /* result */
    int resx = -1;

    /* parse var items */
    char parse_item_type_str[10], parse_item_duration_str[10], parse_item_full_name[300];
    float parse_item_duration;
    int parse_crossfade, parse_item_type, parse_item_type_previous = ITEM_NONE;

    /* audio playlist var */
    int audio_playlist_id = -1, audio_items_id = 0;
    char audio_pls_name[10], audio_pls_full_name[300];

    /* temp var */
    int i;

    /* init items result */
    for (i = 0; i < MAX_PLAYLISTS_ITEMS; i++) {
        /* audio items */
        pls_audio_items[i].crossfade = pls_audio_items[i].duration = pls_audio_items[i].is_online = 0;
        pls_audio_items[i].id_playlist_audio = -1;
        memset(pls_audio_items[i].item_full_path, 0, sizeof (300));
        //snprintf(pls_audio_items[i].item_full_path, 10, "");
    }

    /* open file ok? */
    if (fp) {
        /* open file success */
        /* remove all the playlist from folder */
        printf(KYEL "Removing previous m3u files...%s\n" KRESET, (system("sudo rm -f " PATH_MPD_PLAYLIST "*.m3u") == 0) ? KGRN "OK" : KRED "ERROR");

        /* init buffer */
        memset(buff_data, 0, 1024);
        memset(buff_data2, 0, 1024);

        /* init var */
        id_playlist_items = -1;

        /* read playlist */
        while ((fgets(buff_data, 1000, fp) != NULL) && (id_playlist_items < MAX_PLAYLISTS_ITEMS)) {
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

            /* audio item */
            if (parse_item_type == ITEM_AUDIO) {
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

                    /* inkremen id item audio */
                    audio_items_id++;
                } else {
                    printf(KRED "Error : File %s not found\n" KRESET, parse_item_full_name);
                }

                /* update previous type */
                parse_item_type_previous = ITEM_AUDIO;
            }
        }

        fclose(fp);

        /* return playlist len*/
        resx = audio_items_id;

    } else {
        printf(KRED "Open playlist failed!!!\n" KRESET);
        resx = 0;
    }

    return resx;
}

/*
 play audio file or audio playlist
 audio file in filename only
 audio playlist in fullname
 */
char play_audio_file_or_playlist(char *item, const char is_looped, char *exit_flag) {
    /* var */
    char resx = 0;
    int pls_item_num = 0, retx;

    //mpd status var
    mpd_status_data_t mpd_old_status, mpd_new_status;

    //playlist var
    playlists_audio_items_t playlist_audio_items[MAX_PLAYLISTS_ITEMS];

    /* check playlist or just files */
    char is_playlist = gotString(".pls", item);

    /* parse or just play it */
    if (is_playlist) {
        /* parse it */
        pls_item_num = parse_and_add_playlist_file(item, playlist_audio_items);

        printf(KYEL "Got %d items\n" KRESET, pls_item_num);
        if (pls_item_num > 0) {
            retx = system("mpc load 0 && mpc play");
        } else {
            retx = -1;
        }
        printf(KYEL "\nLoading and playing %s with return code = %d\n" KRESET, item, retx);

        /* monitor if the playing is ok */
        if (retx == 0) {
            resx = 1;

            //loop or not
            if (is_looped) {
                system("mpc repeat on");
            } else {
                system("mpc repeat off");
            }

            //get mpd status  
            mpd_get_status_data(&mpd_new_status);

            //save info to file
            write_play_status_to_file(
                    //current item
                    playlist_audio_items[mpd_new_status.item_pos_on_que].item_full_path,
                    seconds_to_time_string(playlist_audio_items[mpd_new_status.item_pos_on_que].duration),
                    playlist_audio_items[mpd_new_status.item_pos_on_que].is_online,

                    //next item
                    playlist_audio_items[mpd_new_status.item_pos_on_que + 1].item_full_path,
                    seconds_to_time_string(playlist_audio_items[mpd_new_status.item_pos_on_que + 1].duration),
                    playlist_audio_items[mpd_new_status.item_pos_on_que + 1].is_online);

            //set crossfade
            mpc_set_crossfade(playlist_audio_items[mpd_new_status.item_pos_on_que].crossfade);

            //wait until all pls played
            printf(KYEL "Waiting new command or playlist end...\n" KRESET);
            for (;;) {
                //get mpd status  
                mpd_get_status_data(&mpd_new_status);

                //check error or not
                if (mpd_new_status.error != 0) {
                    break;
                } else {
                    //end of playtime
                    if (mpd_new_status.elapse_sec == mpd_new_status.item_duration) {
                        sleep(1);

                        //break if it is last item
                        if (mpd_new_status.item_pos_on_que == (pls_item_num - 1))
                            break;

                        //get new status and update info
                        mpd_get_status_data(&mpd_new_status);

                        //save info to file
                        write_play_status_to_file(
                                //current item
                                playlist_audio_items[mpd_new_status.item_pos_on_que].item_full_path,
                                seconds_to_time_string(playlist_audio_items[mpd_new_status.item_pos_on_que].duration),
                                playlist_audio_items[mpd_new_status.item_pos_on_que].is_online,

                                //next item
                                (mpd_new_status.item_pos_on_que < pls_item_num) ? playlist_audio_items[mpd_new_status.item_pos_on_que + 1].item_full_path : "",
                                (mpd_new_status.item_pos_on_que < pls_item_num) ? seconds_to_time_string(playlist_audio_items[mpd_new_status.item_pos_on_que + 1].duration) : "",
                                (mpd_new_status.item_pos_on_que < pls_item_num) ? playlist_audio_items[mpd_new_status.item_pos_on_que + 1].is_online : 0);

                        //set crossfade
                        mpc_set_crossfade(playlist_audio_items[mpd_new_status.item_pos_on_que].crossfade);

                        //show the info
                        system("mpc");
                    }
                }

                /*
             got new command
                 */
                if (*exit_flag) {
                    resx = 0xff;
                    break;
                }

                usleep(10000);
            }
        }

    } else {
        /* just a file, play it */
        resx = play_audio_file(item, is_looped, exit_flag);
    }

    /* return result */
    return resx;
}

/*
 thread for process command
 */
void thread_task_processs_command() {
    char i, full_filename[256], must_exit_main_loop = 0;
    int retx;

    while (!ALL_THREAD_END) {

        /* respon to new command */
        if (xkey_cmd.rerun) {
            //kill all media program
//            kill_all_media_programs(0, 1);

            //run it
            if ((xkey_cmd.btn_id >= 1) && (xkey_cmd.btn_id <= 4)) {
                /* reset rerun info */
                xkey_cmd.rerun = 0;

                for (i = button_command_list[xkey_cmd.btn_id - 1].item_start_id; i < (button_command_list[xkey_cmd.btn_id - 1].item_start_id + button_command_list[xkey_cmd.btn_id - 1].cmd_count); i++) {
                    switch (command_item[i].cmd_type) {
                            /* default */
                        case CMD_DEFAULT:
                            /* kill all media programs */
                            kill_all_media_programs(1, 1);
                            usleep(100000);
                            /* restart enstreamer */
                            retx = system("sudo systemctl start enstreamer > /dev/null 2>&1");
                            printf(KYEL "Restart enstreamer ... %s\n" KRESET, (retx == 0) ? KGRN "OK" : KRED "ERROR");
                            must_exit_main_loop = 1;
                            break;

                            /* play a video file or playlist */
                        case CMD_VIDEO_FILE:
                        case CMD_VIDEO_FILE_LOOP:
                            /* video playlist */
                        case CMD_VIDEO_PLAYLIST:
                        case CMD_VIDEO_PLAYLIST_LOOP:
                            /* video file or video playlist */
                            if ((command_item[i].cmd_type == CMD_VIDEO_FILE) || (command_item[i].cmd_type == CMD_VIDEO_FILE_LOOP)) {
                                snprintf(full_filename, 256, PATH_MPD_VIDEO_DIR "%s", command_item[i].file_item);
                            } else {
                                snprintf(full_filename, 256, PATH_MPD_PLAYLIST "%s", command_item[i].file_item);
                            }

                            must_exit_main_loop = (play_video_file_or_playlist(full_filename, command_item[i].is_looped, &xkey_cmd.rerun) == 0xff);
                            break;

                            /* play image file or playlist */
                        case CMD_IMAGE:
                        case CMD_IMAGE_LOOP:
                        case CMD_SLIDESHOW:
                        case CMD_SLIDESHOW_LOOP:
                            /* image file or playlist full name */
                            if ((command_item[i].cmd_type == CMD_IMAGE) || (command_item[i].cmd_type == CMD_IMAGE_LOOP)) {
                                snprintf(full_filename, 256, PATH_MPD_IMAGE_DIR "%s", command_item[i].file_item);
                            } else {
                                snprintf(full_filename, 256, PATH_MPD_PLAYLIST "%s", command_item[i].file_item);
                            }

                            must_exit_main_loop = (play_image_file_or_playlist(full_filename, command_item[i].is_looped, &xkey_cmd.rerun) == 0xff);
                            break;

                            /* audio file or playlist */
                        case CMD_AUDIO_FILE:
                        case CMD_AUDIO_FILE_LOOP:
                            /* audio playlist */
                        case CMD_AUDIO_PLAYLIST:
                        case CMD_AUDIO_PLAYLIST_LOOP:
                            /* image file or playlist full name */
                            if ((command_item[i].cmd_type == CMD_AUDIO_FILE) || (command_item[i].cmd_type == CMD_AUDIO_FILE_LOOP)) {
                                snprintf(full_filename, 256, "%s", command_item[i].file_item);
                            } else {
                                snprintf(full_filename, 256, PATH_MPD_PLAYLIST "%s", command_item[i].file_item);
                            }
                            must_exit_main_loop = (play_audio_file_or_playlist(full_filename, command_item[i].is_looped, &xkey_cmd.rerun) == 0xff);
                            break;

                        default:
                            break;
                    }

                    // must exit
                    if (must_exit_main_loop)
                        break;
                }
            }

            //debug
            //            printf("%d pressed\n", xkey_cmd.btn_id);

            /* reset rerun info */
            //            xkey_cmd.rerun = 0;
            must_exit_main_loop = 0;
        }

        //delay
        usleep(10000);
    }

}

//void fillx(char **inx) {
//    int i;
//    for (i = 0; i < 5; i++) {
//        inx[i] = (char*) calloc(32, sizeof(char));
//        snprintf(inx[i], 32, "ITEM %d", i);
//    }
//}

#ifdef _KEYBOARD_ON

/* keyboard test */
void keyboard_test() {
    char keyx = 0;

    //get key
    //just for simulation
    printf(KGRN "Key(1-4) : ");
    scanf("%d", &keyx);
    printf("%d\n" KRESET, keyx);

    //update xkeycmd
    xkey_cmd.btn_id = keyx;
    xkey_cmd.rerun = 1;
}
#endif

/*
 * main program
 */
int main(int argc, char** argv) {

    //info
    printf(KBLU PROGRAM_INFO KRESET);

    //test pidof
    //    for (;;) {
    //        printf("%d\n", process_is_running("fbi"));
    //        usleep(500000);
    //    }

    //    printf("%d\n",
    //            sizeof (cmd_string) / sizeof (*cmd_string)
    //            );

    //test
    //    char *data[32];
    //    fillx(data);
    //
    //    int i;
    //    for (i = 0; i < 5; i++) {
    //        printf("%s\n", data[i]);
    //    }
    //    
    //    return 0;

    //tes video play file and list

#ifdef _XKEY_ACTIVE
    //wait attached device
    printf(KYEL "Waiting device connection...\n" KRESET);
    while ((xkey_handle = xkey_find_and_attach()) == -1) {
        sleep(1);
    }
#endif

    //open command file
    config_obj = iniparser_load(COMMAND_FILE_PATH);
    if (config_obj == NULL) {
        printf(KRED "Cannot parse command file: %s\n" KRESET, COMMAND_FILE_PATH);
    } else {
        printf(KYEL "Parsing command file: %s\n" KRESET, COMMAND_FILE_PATH);
        parse_command_file(config_obj);

        //test video
        //        char fnx[256];
        //        snprintf(fnx, 256, PATH_MPD_VIDEO_DIR "%s", command_item[4].file_item);
        //        snprintf(fnx, 256, PATH_MPD_PLAYLIST "%s", command_item[4].file_item);
        //        play_video_file_or_playlist(fnx, command_item[4].is_looped, &xkey_cmd.rerun);
        //        for(;;);
    }

    //create init pool
    thpool = thpool_init(4);

    //add task to it
    thpool_add_work(thpool, (void*) thread_task_processs_command, NULL);

    printf(KGRN "Waiting for key input from user ....\n" KRESET);

    //just dummy loop
    for (;;) {
#ifdef _KEYBOARD_ON
        keyboard_test();
#endif

        //delay 100ms
        usleep(100000);
    }

    //kill thread pool
    thpool_destroy(thpool);

    //clean iniparser
    iniparser_freedict(config_obj);

    return (EXIT_SUCCESS);
}

