/* 
 * File:   keygen_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:34 PM
 */

#ifndef KEYGEN_UTIL_H
#define	KEYGEN_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //lib
#include <openssl/hmac.h>
#include "file_util.h"

    //key define
#define	KEY_FILE					"/boot/license.img"
#define	ID_FILE						"/tmp/id"
#define KEY_SOURCE					"1234567890"

    //proto
    unsigned char* generatekey(const char *key, const char *data);
    int serialsave(const char *snfilename, unsigned char *serialnumber);
    unsigned char* serialread(const char *keyfile);
    int validatekey();
    int save_id_file();

    //generate sn using sha512
    //length of return is 64bytes

    unsigned char* generatekey(const char *key, const char *data) {
        //printf("key (%d) = %s\ndata (%d) = %s\n", strlen(key), key, strlen(data), data);

        // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
        return HMAC(EVP_sha512(), key, strlen(key), (unsigned char*) data, strlen(data), NULL, NULL);
    }

    //write the serial to file as binary

    int serialsave(const char *snfilename, unsigned char *serialnumber) {
        FILE *fsn;

        //printf("save to %s\n", snfilename);
        if (serialnumber) {
            fsn = fopen(snfilename, "wb+");
            if (!fsn)
                return -1;

            fwrite(serialnumber, sizeof (unsigned char), 64, fsn);
            fclose(fsn);
            return 0;
        } else
            return -1;
    }

    //read serial file

    unsigned char* serialread(const char *keyfile) {
        FILE *fp;
        unsigned char *snx = NULL;

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

    //validate key

    int validatekey() {
        //unsigned char *keyx = generatekey(KEY_SOURCE,getcpuid());
        //unsigned char *keyy = serialread(KEY_FILE);	
        if (!isFileExists(KEY_FILE))
            return -1;
        return memcmp(serialread(KEY_FILE), generatekey(KEY_SOURCE, (const char*) getcpuid()), 64);
    }

    //save cpuid to ID_FILE

    int save_id_file() {
        char cmdx[300];

        //setup the command
        snprintf(cmdx, 300, "echo `cpuid` > %s", ID_FILE);
        return system(cmdx);
    }


#ifdef	__cplusplus
}
#endif

#endif	/* KEYGEN_UTIL_H */

