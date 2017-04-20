/* 
 * File:   mpd_util.h
 * Author: EX4
 *
 * Created on November 18, 2015, 3:55 PM
 * This is for mpd status utility function
 */

#ifndef MPD_UTIL_H
#define	MPD_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //lib used
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>
#include <mpd/message.h>

    //#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

    //struc data

    typedef struct {
        int error, connected,
        volume,
        repeat,
        que_version,
        que_length,

        item_pos,
        elapse_sec, elapse_ms, total_sec,

        item_duration;
        char item_uri[300];
        unsigned int item_pos_on_que, state;
    } mpd_status_data_t;

#define MPD_DEBUG_MSG_ON

    //proto function
    int mpd_handle_error(struct mpd_connection *c);
//    void mpd_print_tag(const struct mpd_song *song, enum mpd_tag_type type, const char *label);
    int mpd_get_status_data(mpd_status_data_t *sts_result);

    /*
     report an error
     * return 1
     */
    int mpd_handle_error(struct mpd_connection *c) {
        //        assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);

#if defined(MPD_DEBUG_MSG_ON)
        fprintf(stderr, "%s\n", mpd_connection_get_error_message(c));
#endif
        mpd_connection_free(c);
        return EXIT_FAILURE;
    }

    /*
     print mpd tag
     */
    //    void mpd_print_tag(const struct mpd_song *song, enum mpd_tag_type type, const char *label) {
    //        unsigned i = 0;
    //        const char *value;
    //
    //        while ((value = mpd_song_get_tag(song, type, i++)) != NULL)
    //            printf("%s: %s\n", label, value);
    //    }

    /*
     get status mpd
     */
    int mpd_get_status_data(mpd_status_data_t *sts_result) {
        int resx = 0;
        struct mpd_connection *conn;
        struct mpd_status *status;
        struct mpd_song *song;

        //init result
        sts_result->connected = 0;
        sts_result->elapse_ms = 0;
        sts_result->elapse_sec = 0;
        sts_result->error = 0;
        sts_result->item_duration = 0;
        sts_result->item_pos = 0;
        sts_result->item_pos_on_que = 0;
        memset(sts_result->item_uri, 0, 300);
        sts_result->que_length = 0;
        sts_result->que_version = 0;
        sts_result->repeat = 0;
        sts_result->state = MPD_STATE_UNKNOWN;
        sts_result->total_sec = 0;
        sts_result->volume = 0;

        //open connection
        conn = mpd_connection_new(NULL, 0, 5000);

        //error on connection
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            resx = mpd_handle_error(conn);
            sts_result->error = resx;
            return resx;
        }

        //start query status
        mpd_command_list_begin(conn, true);
        mpd_send_status(conn);
        mpd_send_current_song(conn);
        mpd_command_list_end(conn);

        status = mpd_recv_status(conn);
        if (status == NULL) {
            resx = mpd_handle_error(conn);
            sts_result->error = resx;
            return resx;
        }

        //store result
        sts_result->volume = mpd_status_get_volume(status);
        sts_result->repeat = mpd_status_get_repeat(status);
        sts_result->que_version = mpd_status_get_queue_version(status);
        sts_result->que_length = mpd_status_get_queue_length(status);

        //get player state
        sts_result->state = mpd_status_get_state(status);

        //store result
        sts_result->item_pos = mpd_status_get_song_pos(status);
        sts_result->elapse_sec = mpd_status_get_elapsed_time(status);
        sts_result->elapse_ms = mpd_status_get_elapsed_ms(status);
        sts_result->total_sec = mpd_status_get_total_time(status);

        //free status
        mpd_status_free(status);

        //check conn error
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            resx = mpd_handle_error(conn);
            sts_result->error = resx;
            return resx;
        }

        //get uri
        mpd_response_next(conn);

        while ((song = mpd_recv_song(conn)) != NULL) {
            snprintf(sts_result->item_uri, 300, "%s", mpd_song_get_uri(song));
            sts_result->item_duration = mpd_song_get_duration(song);
            sts_result->item_pos_on_que = mpd_song_get_pos(song);
            mpd_song_free(song);
        }

        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
                !mpd_response_finish(conn)) {
            resx = mpd_handle_error(conn);
            sts_result->error = resx;
            return resx;
        }
        
        sts_result->connected = 1;

        //free connection
        mpd_connection_free(conn);

        return resx;
    }

#ifdef	__cplusplus
}
#endif

#endif	/* MPD_UTIL_H */

