//Copyright (c) 2014, Giovanni Dante Grazioli (deroad)
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "i2c.h"

/*
 * return i2c device pointer
 * */
int open_i2c(const char* dev_path, unsigned char addr) {
    int dev;
    dev = open(dev_path, O_RDWR);

    ioctl(dev, I2C_SLAVE, addr & 0x7F);
    return dev;
}

/*
 * close i2c dev
 * */
void close_i2c(int dev) {
    close(dev);
}

/*
 * read data from i2c dev
 * return : 0 = ERROR, 1 = OK
 * */
int get_i2c_data(int dev, unsigned char *value) {
    int r;
    if ((r = read(dev, value, 1)) < 0) {
        //~ printf("Failed to read from the i2c bus %d\n", r);
        return 0;
    }
    return 1;
}

/*
 * write to i2c dev
 * return : 0 = ERROR, 1 = OK
 * */
int set_i2c_data(int dev, unsigned char val) {
    int r;
    unsigned char buf[2];
    buf[1] = 0;
    buf[0] = val;
    if ((r = write(dev, buf, 1)) < 0) {
        //~ printf("Failed to write from the i2c bus %d\n", r);
        return 0;
    }
    usleep(2);
    return 1;
}

