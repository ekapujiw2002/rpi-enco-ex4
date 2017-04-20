/* 
 * File:   enstreamer_util.h
 * Author: EX4
 *
 * Created on June 5, 2015, 4:30 PM
 */

#ifndef ENSTREAMER_UTIL_H
#define	ENSTREAMER_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    /* ordinary lib */
#include <stdio.h>
#include <stdlib.h>

    /* custom lib */
#include "system_util.h"
#include "string_util.h"
#include "file_util.h"
#include "keygen_util.h"
#include "datetime_util.h"
#include "network_util.h"
#include "image_util.h"
#include "rlutil.h"

    /*
     function proto
     */
    void get_and_show_network_info(char *ip_public_out, char *ip_local_out, char *dev_name_out);
    void kill_audio_video_player();

    /*
     probing network info
     */
    void get_and_show_network_info(char *ip_public_out, char *ip_local_out, char *dev_name_out) {
        printf(KYEL "Probing network status, please wait...\n" RESET);
        get_public_ip(ip_public_out);
        get_local_ip(ip_local_out);
        get_hostname(dev_name_out);
        printf(KGRN "IP Public\t: %s\nIP Local\t: %s\nDevice Name\t: %s\n" RESET, ip_public_out, ip_local_out, dev_name_out);
    }

    /*
     kill omx and clear mpc
     */
    void kill_audio_video_player() {
        printf(KRED "Killing previous audio video player...\n" RESET);
        system("sudo killall omxplayer.bin");
        system("mpc stop");
        system("mpc clear");
        system("mpc clearerror");
//        system("setterm -cursor on");
//        system("set PS1=\"\\[\\e[1;32m\\][\\u@\\h \\W]\\$\\[\\e[0m\\]\"");
//        system("setterm -reset");
    }


#ifdef	__cplusplus
}
#endif

#endif	/* ENSTREAMER_UTIL_H */

