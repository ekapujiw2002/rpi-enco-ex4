/* 
 * File:   network_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:31 PM
 */

#ifndef NETWORK_UTIL_H
#define	NETWORK_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //libs
#include <stdio.h>
#include <string.h>
#include "system_util.h"

    //ptoto func
    void get_local_ip(char *ip_result);
    void get_public_ip(char *ip_result);
    void get_hostname(char *hostname_result);

    /*
     * get local ipd
     * */
    void get_local_ip(char *ip_result) {
        unsigned char *res = NULL;

        if (run_command("ifconfig eth0 | grep inet | awk '{print $2}'", (char**) &res)) {
            res[gotchar('\n', (const char*) res)] = 0;
            res[gotchar('\r', (const char*) res)] = 0;
            strcpy(ip_result, (strlen((const char*) res) != 0) ? (char*) res : "0.0.0.0");
        } else {
            strcpy(ip_result, "0.0.0.0");
        }

        if (!res) {
            free(res);
            res = NULL;
        }
    }

    /*
     * get public ip
     * */
    void get_public_ip(char *ip_result) {
        unsigned char *res = NULL;

        //10 sec max
        if (run_command("curl --max-time 10 -s bot.whatismyipaddress.com", (char**) &res)) {
            strcpy(ip_result, (strlen((const char*) res) != 0) ? (char*) res : "0.0.0.0");
        } else {
            strcpy(ip_result, "0.0.0.0");
        }

        if (!res) {
            free(res);
            res = NULL;
        }
    }

    /*
     * get hostname
     * */
    void get_hostname(char *hostname_result) {
        unsigned char *res = NULL;

        if (run_command("cat /etc/hostname", (char**) &res)) {
            res[gotchar('\n', (const char*) res)] = 0;
            res[gotchar('\r', (const char*) res)] = 0;
            strcpy(hostname_result, (strlen((const char*) res) != 0) ? (char*) res : "?");
        } else {
            strcpy(hostname_result, "?");
        }

        if (!res) {
            free(res);
            res = NULL;
        }
    }


#ifdef	__cplusplus
}
#endif

#endif	/* NETWORK_UTIL_H */

