/* 
 * File:   keygen.c
 * Author: EX4
 *
 * Created on July 13, 2015, 2:24 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/hmac.h>
#include <fcntl.h>
#include <sys/stat.h>

//generate sn using sha512
//length of return is 64bytes

unsigned char* generatekey(const char *key, const char *data) {
    // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
    return HMAC(EVP_sha512(), key, strlen(key), (unsigned char*) data, strlen(data), NULL, NULL);
}

//write the serial to file as binary

void serialsave(const char *snfilename, unsigned char *serialnumber) {
    FILE *fsn;

    printf("save to %s\n", snfilename);
    fsn = fopen(snfilename, "wb+");
    fwrite(serialnumber, sizeof (unsigned char), 64, fsn);
    fclose(fsn);
}

//read serial file

unsigned char* serialread(const char *keyfile) {
    FILE *fp;
    unsigned char *snx;

    fp = fopen(keyfile, "rb");
    if (fp) {
        snx = (unsigned char*) calloc(64, sizeof (unsigned char));
        if (snx) {
            fread(snx, sizeof (unsigned char), 64, fp);
        }
        fclose(fp);
        return snx;
    } else
        return NULL;
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

//validate key
//0 if ok

int validatekey(const char *KEY_FILE, const char *KEY_SOURCE, const char *KEY_DATA) {
    if (!isFileExists(KEY_FILE))
        return -1;
    return memcmp(serialread(KEY_FILE), generatekey(KEY_SOURCE, KEY_DATA), 64);
}

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage : keygen [license file] [key] [id]\n");
        return 1;
    }

    //validate only with argv[5]='v'
    if (argc >= 5) {
        if (strncmp("v", argv[4], 1) == 0) {
            //printf("validating %s %s %s %s\n", argv[1], argv[2], argv[3], argv[4]);
            printf("%d", validatekey(argv[1], argv[2], argv[3]));
            return EXIT_SUCCESS;
        }
    }

    printf("Generating key license with\nKey\t: %s \nData\t: %s\nTo file %s...", argv[2], argv[3], argv[1]);
    serialsave(argv[1], generatekey(argv[2], argv[3]));
    printf("DONE\nSave %s to designated folder\n", argv[1]);

    return (EXIT_SUCCESS);
}

