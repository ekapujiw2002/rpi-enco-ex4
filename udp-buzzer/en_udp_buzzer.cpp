/* 
 * File:   en_udp_buzzer.cpp
 * Author: EX4
 *
 * Created on February 25, 2017, 2:44 PM
 */

//#include <cstdlib>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>

//pigpio lib        
#include <pigpio.h>

//mongoose
#include "mongoose.h"

//using namespace std;

//gpio pin for switch
#define PIN_SWITCH  21

//global var
//server
static sig_atomic_t server_system_signal_received = 0;
struct mg_mgr server_conn_mgr;
struct mg_connection *server_udp_connection;
static sig_atomic_t server_udp_is_connected = 0, server_data_sent_status = 0;
char server_listen_port[32] = "udp://1234";

//flag to send
static sig_atomic_t is_send_playing_signal = 0;

//tone
uint16_t freq_tone = 1000;

//server udp to connect to
char server_udp_url[64] = "udp://192.168.1.15:1234";

/**
 * Beep stop
 */
void beep_stop() {
    system("pkill sgen 2>/dev/null");
}

/**
 * Beep using dsp sound
 * @param freq
 * @return 
 */
char beep_start(const uint16_t freq) {
    //    char resx = 0;
    char cmdx[64] = {0};
    beep_stop();
    snprintf(cmdx, sizeof (cmdx), "sgen sine %d >/dev/null 2>&1 &", freq);
    return (system(cmdx) == -1 ? 0 : 1);
}

/**
 * monitor pin level change
 * @param gpio
 * @param level
 * @param tick
 */
void gpio_pin_change_function(int gpio, int level, uint32_t tick) {
    //    time_sleep(0.01);
    if (level != PI_TIMEOUT) {
        printf("\r\n%lu\t: GPIO %d became %d\r\n", tick, gpio, level);
        is_send_playing_signal = (level == PI_LOW ? 1 : 0);
        if (server_data_sent_status == 1) {
            server_data_sent_status = 0;
            if (is_send_playing_signal == 1) {
                mg_send(server_udp_connection, "PLAY\x00", 5);
                //                beep_start(1000);
            } else {
                mg_send(server_udp_connection, "STOP\x00", 5);
                //                beep_stop();
            }
        }
    }

}

/**
 * configure the gpio
 * @return 0: ok, 1: error 
 */
char gpio_init() {
    char resx = 0;

    //    disable all other interface
    resx = (gpioCfgInterfaces(PI_DISABLE_FIFO_IF | PI_DISABLE_SOCK_IF | PI_LOCALHOST_SOCK_IF) == 0 ? 0 : 1);

    //    try to init
    if (resx == 0) {
        if (gpioInitialise() >= 0) {
            if (gpioSetMode(PIN_SWITCH, PI_INPUT) == 0) {
                if (gpioSetPullUpDown(PIN_SWITCH, PI_PUD_UP) == 0) {
                    if (gpioSetISRFunc(PIN_SWITCH, EITHER_EDGE, 25, gpio_pin_change_function) == 0) {
                        resx = 0;
                    } else {
                        resx = 1;
                    }

                } else {
                    resx = 1;
                }

            } else {
                resx = 1;
            }

            //            resx = (((gpioSetMode(PIN_SWITCH, PI_INPUT) == 0) && (gpioSetPullUpDown(PIN_SWITCH, PI_PUD_UP) == 0) && (/* gpioSetAlertFunc(PIN_SWITCH, gpio_pin_change_function) */ gpioSetISRFunc(PIN_SWITCH, EITHER_EDGE, 50, gpio_pin_change_function) == 0)) ? 0 : 1);
        } else {
            resx = 1;
        }
    }
    return resx;
}

/**
 * Signal handler for mongoose
 * @param sig_num
 */
static void system_signal_handler(int sig_num) {
    signal(sig_num, system_signal_handler); // Reinstantiate signal handler
    server_system_signal_received = sig_num;
}

/**
 * server udp handler
 * @param nc
 * @param ev
 * @param p
 */
static void server_event_handler(struct mg_connection *nc, int ev, void *p) {
    struct mbuf *io = &nc->recv_mbuf;
    char data_rx[32] = {0};
    (void) p;

    switch (ev) {
            /*
             got some data
             */
        case MG_EV_RECV:
            //            mg_send(nc, "ECHO : ", 8);
            //            mg_send(nc, io->buf, io->len); // Echo message back

            strncpy(data_rx, io->buf, io->len);
            mbuf_remove(io, io->len); // Discard message from recv buffer

            //process the buffer
            if (strncasecmp(data_rx, "PLAY", 4) == 0) {
                //play tone
                printf("%lu\t: Play tone\r\n", time_time());
                beep_start(freq_tone);
            } else if (strncasecmp(data_rx, "STOP", 4) == 0) {
                //stop it
                printf("%lu\t: Stop tone\r\n", time_time());
                beep_stop();
            } else {
                beep_stop();
            }

            // In case of UDP, Mongoose creates new virtual connection for
            // incoming messages
            // We can keep it (and it will be reused for another messages from
            // the same address) or we can close it (this saves some memory, but
            // decreases perfomance, because it forces creation of connection
            // for every incoming dgram)
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;

        default:
            break;
    }
}

/**
 * udp connection handler
 * @param nc
 * @param ev
 * @param p
 */
static void server_udp_connection_event_handler(struct mg_connection *nc, int ev, void *p) {
    struct mbuf *io = &nc->recv_mbuf;
    (void) p;
    int connect_status;

    switch (ev) {
            /*
             when we connected to server
             */
        case MG_EV_CONNECT:
            connect_status = *(int *) p;
            server_udp_is_connected = connect_status == 0 ? 1 : 0;
            printf("Connected to server udp : %d\r\n", server_udp_is_connected);
            break;

            /*
             we send data to server and data has been sent, all of it
             */
        case MG_EV_SEND:
            connect_status = *(int *) p;
            printf("Send %d byte data to server\r\n", connect_status);
            server_data_sent_status = 1;
            break;

            /*
             connection to server closed
             */
        case MG_EV_CLOSE:
            server_udp_is_connected = 0;
            printf("Server connection closed\r\n");
            break;

        default:
            break;
    }
}

/*
 * main program
 */
int main(int argc, char** argv) {

    int option;

    /*
     parse option
     */
    while ((option = getopt(argc, argv, "c:f:s:")) != -1) {
        switch (option) {
            case 'c':
                snprintf(server_udp_url, sizeof (server_udp_url), "%s", optarg);
                break;
            case 'f':
                freq_tone = atoi(optarg);
                break;
            case 's':
                snprintf(server_listen_port, sizeof (server_listen_port), "%s", optarg);
                break;
            default:
                printf("NEED OPTION");
                exit(EXIT_FAILURE);
        }
    }

    printf("Current setting : \r\nServer\t: %s\r\nClient\t: %s\r\nFreq\t: %u\r\n",
            server_listen_port,
            server_udp_url,
            freq_tone);

    /*
     init pigpio first
     */
    printf("\r\nInit gpio = %d\r\n", gpio_init());

    /*
     signal handler
     */
    signal(SIGINT, system_signal_handler);
    signal(SIGTERM, system_signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init server
    mg_mgr_init(&server_conn_mgr, NULL);
    mg_bind(&server_conn_mgr, server_listen_port, server_event_handler);
    printf("Started server on port %s\r\n", server_listen_port);

    //open connection to server udp
    server_udp_connection = mg_connect(&server_conn_mgr, server_udp_url, server_udp_connection_event_handler);
    if (server_udp_connection != NULL) {
        printf("Connection to server opened successfully\r\n");
        mg_send(server_udp_connection, "STOP\x00", 5);
    } else {
        printf("Connection to server cannot be opened\r\n");
    }

    //    main loop
    while (server_system_signal_received == 0) {
        mg_mgr_poll(&server_conn_mgr, 20);
    }

    //    cleanup
    mg_mgr_free(&server_conn_mgr);
    gpioTerminate();
    printf("\nTerminating application done\n");

    return 0;
}

