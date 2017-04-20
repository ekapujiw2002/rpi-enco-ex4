/* 
 * File:   file_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:35 PM
 */

#ifndef FILE_UTIL_H
#define	FILE_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //used lib
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

    //proto func
    char* read_file(const char* filename, size_t* length);
    char isFileExists(const char *filename);

    /* Read the contents of FILENAME into a newly-allocated buffer.  The
       size of the buffer is stored in *LENGTH.  Returns the buffer, which
       the caller must free.  If FILENAME doesn't correspond to a regular
       file, returns NULL.  
     * http://www.advancedlinuxprogramming.com/listings/appendix-b/read-file.c
     * */

    char* read_file(const char* filename, size_t* length) {
        int fd;
        struct stat file_info;
        char* buffer;

        /* Open the file.  */
        fd = open(filename, O_RDONLY);

        /* Get information about the file.  */
        fstat(fd, &file_info);
        *length = file_info.st_size;
        /* Make sure the file is an ordinary file.  */
        if (!S_ISREG(file_info.st_mode)) {
            /* It's not, so give up.  */
            close(fd);
            return NULL;
        }

        /* Allocate a buffer large enough to hold the file's contents.  */
        buffer = (char*) malloc(*length);
        /* Read the file into the buffer.  */
        read(fd, buffer, *length);

        /* Finish up.  */
        close(fd);
        return buffer;
    }

    /*
     * Check if a file exist using stat() function
     * return 1 if the file exist otherwise return 0
     */
    char isFileExists(const char *filename) {
        struct stat buffer;

        int exist = stat(filename, &buffer);
        //printf("Stat of %s ==> %d\n", fn, exist);
        return (exist == 0) ? 1 : 0;
    }


#ifdef	__cplusplus
}
#endif

#endif	/* FILE_UTIL_H */

