/* 
 * File:   image_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:22 PM
 */

#ifndef IMAGE_UTIL_H
#define	IMAGE_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //libs
#include <stdio.h>

    //proto
    char view_image_file(const char *fname, const int duration);

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


#ifdef	__cplusplus
}
#endif

#endif	/* IMAGE_UTIL_H */

