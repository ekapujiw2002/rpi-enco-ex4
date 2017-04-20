/* 
 * File:   ensynch.c
 * Author: EX4
 *
 * Created on June 12, 2015, 11:44 AM
 */

/*
 * playlist synchronizer
 * 
 * compile : gcc -o ensynch ensynch.c
 * get http reaspons header :
 * curl -sw '%{http_code}' -I -o /dev/null http://enco1:1234@enplayer-000-00001.selfip.com:46000/dav/upload2.conf
 # http://fibrevillage.com/index.php/storage/117-curl-http-commands-with-webdav
# upload webdav using curl :
# curl -sw '%{http_code}' -T webdav.conf http://enco1:12345@enplayer-000-00001.selfip.com:46000/dav/upload.conf -o /dev/null
# download :
# curl -sw '%{http_code}' -O http://enco1:1234@enplayer-000-00001.selfip.com:46000/dav/upload2.conf -o /dev/null
# curl -sw '%{http_code}' -o def.pls http://enco1:1234@enplayer-000-00001.selfip.com:46000/dav/playlist/Feb12-2015.pls
# delete
# curl -sw '%{http_code}' -o /dev/null -i -X DELETE http://enco1:12345@enplayer-000-00001.selfip.com:46000/dav/upload2.conf
 * 
 * ftp server 	: ftp://ftp2.enco.com/enStreamer/enstreamer-000-00002
 * ftp user 		: nova
 * ftp pass		: enco
 * */

/*
step donlod dan parsing via curl:
curl -sw '%{http_code}' -o /tmp/dl1.log http://enplayer-000-00001.selfip.com:60000/enco-000-00001/
cek respon/100==2?
cat /tmp/dl1.log | tr '>' '\n' | tr '<' '\n' > /tmp/dl2.log
grep -i '.mp3\|.mp4\|.avi\|.mpg\|.pls\|.pls.new' /tmp/dl2.log | grep -v -i 'a href='
download file dr server, delete file di server
 */

/*
 * ftp command
 * listing folder and files :
 * curl -s -l -m 30 -P 21 --ftp-pasv ftp://nova:enco@ftp2.enco.com/enStreamer/enstreamer-000-00002/
 * 
 * download file with continued:
 * curl -s -m 600 -P 21 -u nova:enco --ftp-pasv -C - -o Apr23-2015.pls ftp://ftp2.enco.com/enStreamer/enstreamer-000-00002/Apr23-2015.pls
 * */

//library used
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include <sys/resource.h> 
#include <sys/time.h> 

//~ #include <ctype.h>
//~ #include <dirent.h>
#include <libgen.h>
#include <signal.h>

#include "keygen_util.h"

//program version
#define	PROGRAM_TITLE				"enSynchronizer"
#define	PROGRAM_VERSION				"12042016-100122"

//curl define
#define	CURL_GET_HEADER			"curl -sw '%{http_code}' -I -o /dev/null %s"
#define CURL_DOWNLOAD_FILE		"curl -sw '%{http_code}' -o %s %s"	//local file --- remote file
#define CURL_UPLOAD_FILE		"curl -sw '%{http_code}' -T %s %s -o /dev/null"	//local file --- remote file
#define CURL_DELETE_REMOTE_FILE         "curl -sw '%{http_code}' -o /dev/null -i -X DELETE %s"	//remote file
#define CURL_LIST_HOME_DIR		"curl -sw '%{http_code}' -o /tmp/dl1.log \"%s\""

//config file
#define ENSTREAMER_CONFIG               "/etc/enstreamer/enstreamer.conf"

//text colorization
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

//datetime format
#define DATETIME_FMT_LOG_FILE   "%b%d-%Y"

//download path
#define DOWNLOAD_PATH	"/data/media/download/"
#define MP3_PATH        "/data/media/audio/"
#define PLAYLIST_PATH	"/data/media/playlist/"
#define VIDEO_PATH      "/data/media/video/"
#define IMAGES_PATH     "/data/media/image/"
#define LOG_PATH        "/data/media/logs/"

/*
 * http://creativeandcritical.net/str-replace-c
 * Replaces in the string str all the occurrences of the source string old with the destination string new. The lengths of the strings old and new may differ. The string new may be of any length, but the string old must be of non-zero length - the penalty for providing an empty string for the old parameter is an infinite loop. In addition, none of the three parameters may be NULL.
 * */
char *repl_str(const char *str, const char *old, const char *new) {

    /* Adjust each of the below values to suit your needs. */

    /* Increment positions cache size initially by this number. */
    size_t cache_sz_inc = 16;
    /* Thereafter, each time capacity needs to be increased,
     * multiply the increment by this factor. */
    const size_t cache_sz_inc_factor = 2;
    /* But never increment capacity by more than this number. */
    const size_t cache_sz_inc_max = 1048576;

    char *pret, *ret = NULL;
    const char *pstr2, *pstr = str;
    size_t i, count = 0;
    ptrdiff_t *pos_cache = NULL;
    size_t cache_sz = 0;
    size_t cpylen, orglen, retlen, newlen, oldlen = strlen(old);

    /* Find all matches and cache their positions. */
    while ((pstr2 = strstr(pstr, old)) != NULL) {
        count++;

        /* Increase the cache size when necessary. */
        if (cache_sz < count) {
            cache_sz += cache_sz_inc;
            pos_cache = realloc(pos_cache, sizeof (*pos_cache) * cache_sz);
            if (pos_cache == NULL) {
                goto end_repl_str;
            }
            cache_sz_inc *= cache_sz_inc_factor;
            if (cache_sz_inc > cache_sz_inc_max) {
                cache_sz_inc = cache_sz_inc_max;
            }
        }

        pos_cache[count - 1] = pstr2 - str;
        pstr = pstr2 + oldlen;
    }

    orglen = pstr - str + strlen(pstr);

    /* Allocate memory for the post-replacement string. */
    if (count > 0) {
        newlen = strlen(new);
        retlen = orglen + (newlen - oldlen) * count;
    } else retlen = orglen;
    ret = malloc(retlen + 1);
    if (ret == NULL) {
        goto end_repl_str;
    }

    if (count == 0) {
        /* If no matches, then just duplicate the string. */
        strcpy(ret, str);
    } else {
        /* Otherwise, duplicate the string whilst performing
         * the replacements using the position cache. */
        pret = ret;
        memcpy(pret, str, pos_cache[0]);
        pret += pos_cache[0];
        for (i = 0; i < count; i++) {
            memcpy(pret, new, newlen);
            pret += newlen;
            pstr = str + pos_cache[i] + oldlen;
            cpylen = (i == count - 1 ? orglen : pos_cache[i + 1]) - pos_cache[i] - oldlen;
            memcpy(pret, pstr, cpylen);
            pret += cpylen;
        }
        ret[retlen] = '\0';
    }

end_repl_str:
    /* Free the cache and return the post-replacement string,
     * which will be NULL in the event of an error. */
    free(pos_cache);
    return ret;
}

/*
 * Check if a file exist using stat() function
 * return 1 if the file exist otherwise return 0
 */
//char isFileExists(const char *filename) {
//    struct stat buffer;
//
//    int exist = stat(filename, &buffer);
//    //printf("Stat of %s ==> %d\n", fn, exist);
//    return (exist == 0) ? 1 : 0;
//}

/*
 * check char
 */
int gotchar(const int asearch, const char *astring) {
    char *e = strchr(astring, asearch);
    return e ? (int) (e - astring) : -1;
}

/*
 *check for a string in a string
 */
char gotString(const char *asearch, const char *astring) {
    return (strstr(astring, asearch) ? 1 : 0);
}

//run a command and return result

//char run_command(const char *cmd, char **message) {
//    FILE *fp;
//    int status;
//    char msg_res[1024];
//
//    fp = popen(cmd, "r");
//    if (!fp) {
//        return 0;
//    }
//
//    //allcate message pointer
//    *message = (char*) calloc(65536, sizeof (char));
//
//    if (!message)
//        return 0;
//
//    while ((fgets(msg_res, 1024, fp) != NULL) && (strlen(*message) < 65000))
//        strcat(*message, msg_res);
//
//    status = pclose(fp);
//    return (status == -1) ? 0 : 1;
//}

//get current datetime formated as datetime_format

char *get_current_datetime(const char *datetime_format) {
    char *buffer;
    time_t curtime;
    struct tm *loc_time;

    buffer = (char*) calloc(30, sizeof (char));
    if (!buffer)
        return NULL;

    //Getting current time of system
    curtime = time(NULL);

    // Converting current time to local time
    loc_time = localtime(&curtime);

    // Displaying date and time in standard format
    //    printf("%s", asctime (loc_time));
    //format datetime
    strftime(buffer, 30, datetime_format, loc_time);

    return buffer;
}

/*
 * get respon header
 * */
int curl_get_header(const char *url) {
    unsigned char *res;
    char cmd[600];
    int result = 0;

    //format command
    snprintf(cmd, 600, CURL_GET_HEADER, url);

    if (run_command(cmd, (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        result = atoi(res);
    }

    free(res);

    return result;
}

/*
 * download file
 * */
int curl_download_file(const char *url, const char *local_file) {
    unsigned char *res;
    char cmd[600];
    int result = 0;

    //format command
    snprintf(cmd, 600, CURL_DOWNLOAD_FILE, local_file, url);

    if (run_command(cmd, (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        result = atoi(res);
    }

    free(res);

    return result;
}

/*
 * upload file
 * */
int curl_upload_file(const char *remote_file, const char *local_file) {
    unsigned char *res;
    char cmd[600];
    int result = 0;

    //format command
    snprintf(cmd, 600, CURL_UPLOAD_FILE, local_file, remote_file);
    //printf("%s\n", cmd);

    if (run_command(cmd, (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        result = atoi(res);
    }

    free(res);

    return result;
}

/*
 * delete remote file
 * */
int curl_delete_file(const char *remote_file) {
    unsigned char *res;
    char cmd[600];
    int result = 0;

    //format command
    snprintf(cmd, 600, CURL_DELETE_REMOTE_FILE, remote_file);
    //printf("%s\n", cmd);

    if (run_command(cmd, (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        result = atoi(res);
    }

    free(res);

    return result;
}

/*
 * curl ftp list folder
 * */
char *curl_ftp_list_dir(const char *ftp_server_url, const int ftp_server_port, const char *username_pass) {
    unsigned char *res;
    char cmd[600];

    //format the command
    snprintf(cmd, 600, "curl -s -l -m 30 -P %d -u %s --ftp-pasv %s || echo $?", ftp_server_port, username_pass, ftp_server_url);

    //printf("%s", cmd);
    //fflush(stdout);

    if (run_command(cmd, (char**) &res)) {
        //printf("\ndata ok\n");

        if (strlen(res) <= 3) {
            free(res);
            return NULL;
        }
    }

    return res;
}

/*
 * get hostname
 * */
char get_hostname(char *hostname_result) {
    unsigned char *res;
    char resx = 0;

    if (run_command("hostname", (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        strcpy(hostname_result, (strlen(res) != 0) ? (char*) res : "?");
        resx = 1;
    } else {
        strcpy(hostname_result, "?");
    }

    free(res);

    return resx;
}

/*
get the webdav url string
cat enstreamer.conf | tr '\n' '=' | sed 's/:\/\//:\/\/=/g' | awk -F= '{ printf "%s%s:%s@%s:%s/", $2,$7,$9,$3,$5}'
 */
int get_webdav_url(char *webdav_url, int *server_port, char *user_pass) {
    char devname[50];
    unsigned char *res, *ftp_port;
    int resx = 0, ftp_port_val = 21;

    if (get_hostname(devname) == 1) {
        //get ftp port
        run_command("grep 'port=' " ENSTREAMER_CONFIG " | sed 's/port=//g'", (char**) &ftp_port);
        ftp_port[gotchar('\n', ftp_port)] = 0;
        ftp_port[gotchar('\r', ftp_port)] = 0;
        ftp_port_val = atoi(ftp_port);
        if (ftp_port_val == 0)
            ftp_port_val = 21;
        if (server_port != NULL)
            *server_port = ftp_port_val;

        //get user:pass
        run_command("cat " ENSTREAMER_CONFIG " | tr '\n' '=' | awk -F= '{ printf \"%s:%s\", $6,$8}'", (char**) &res);
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        if (user_pass != NULL)
            strcpy(user_pass, res);

        //get server url			
        if (run_command("cat " ENSTREAMER_CONFIG " | tr '\n' '=' | awk -F= '{ printf \"%s/\", $2 }'", (char**) &res)) {
            res[gotchar('\n', res)] = 0;
            res[gotchar('\r', res)] = 0;
            if (webdav_url) {
                strcpy(webdav_url, (strlen(res) != 0) ? (char*) res : "?");
                strcat(webdav_url, devname);
                strcat(webdav_url, "/");
            }
            resx = 1;
            //printf("%s", webdav_url);
        } else {
            if (webdav_url)
                strcpy(webdav_url, "?");
        }

        //free it
        free(res);
        free(ftp_port);
    }

    return resx;
}

/*
 * get synch interval
 * */
unsigned int get_synch_interval() {
    unsigned int interval = 30;

    //grep 'interval=' enstreamer.conf | awk -F= '{print $2}'
    unsigned char *res;

    if (run_command("grep 'interval=' " ENSTREAMER_CONFIG " | awk -F= '{print $2}'", (char**) &res)) {
        res[gotchar('\n', res)] = 0;
        res[gotchar('\r', res)] = 0;
        interval = atoi(res);
        if (interval == 0) interval = 30;
    } else {
    }

    free(res);

    return interval;
}

/*
download old or new playlist
 */
char download_playlist(char *pls_out_fname) {
    char resx = 0;
    char webdav_urlx[300];
    char pls_remote_name[400], local_fname[400];
    int curl_resp = 0;

    if (get_webdav_url(webdav_urlx, NULL, NULL)) {
        //copy it
        snprintf(pls_remote_name, 400, "%s%s.pls",
                webdav_urlx,
                //"Mar18-2015"
                get_current_datetime(DATETIME_FMT_LOG_FILE)
                );

        //check the file
        printf(KYEL "Checking %s ... ", pls_remote_name);
        fflush(stdout);
        printf("%s\n" RESET,
                ((curl_resp = curl_get_header(pls_remote_name)) / 100 == 2) ? KGRN "OK" : KRED "ERROR"
                );

        //if ok => get it
        if (curl_resp / 100 == 2) {
            //download the file
            //get the local filename
            snprintf(local_fname, 400, DOWNLOAD_PATH "%s.pls",
                    //"Mar18-2015"
                    get_current_datetime(DATETIME_FMT_LOG_FILE)
                    );
        }//check .new pls
        else {
            strcat(pls_remote_name, ".new");
            //check the file
            printf(KYEL "Checking %s ... ", pls_remote_name);
            fflush(stdout);
            printf("%s\n" RESET,
                    ((curl_resp = curl_get_header(pls_remote_name)) / 100 == 2) ? KGRN "OK" : KRED "ERROR"
                    );

            if (curl_resp / 100 == 2) {
                //download the file
                //get the local filename
                snprintf(local_fname, 400, DOWNLOAD_PATH "%s.pls.new",
                        //"Mar18-2015"
                        get_current_datetime(DATETIME_FMT_LOG_FILE)
                        );
            } else {
                printf(KRED "No new playlist found\n" RESET);
            }
        }

        //download playlist file
        //download it
        if (curl_resp / 100 == 2) {
            printf(KYEL "Downloading %s to %s ... ", pls_remote_name, local_fname);
            fflush(stdout);

            //check the respon download
            printf("%s\n" RESET,
                    ((curl_resp = curl_download_file(pls_remote_name, local_fname)) / 100 == 2) ? KGRN "OK" : KRED "ERROR"
                    );

            if (curl_resp / 100 == 2) {
                //process the file
                printf(KGRN "Download success\n" RESET);
                if (pls_out_fname)
                    strcpy(pls_out_fname, local_fname);
                resx = 1;
            } else {
                printf(KRED "Download failed!!!\n" RESET);
            }
        }
    }//cannot read config
    else {
        resx = 0;
    }

    return resx;
}

/*
 * upload file
 * */
char upload_file(char *upload_fname) {
    char resx = 0;
    char webdav_urlx[300];
    char remote_fname[400], local_fname[400];
    int curl_resp = 0;

    if (get_webdav_url(webdav_urlx, NULL, NULL)) {
        //copy it
        strcpy(local_fname, upload_fname);
        snprintf(remote_fname, 400, "%s%s",
                webdav_urlx,
                basename(local_fname)
                );

        printf(KYEL "Uploading %s to %s ... ", local_fname, remote_fname);
        fflush(stdout);

        //check the respon download
        printf("%s\n" RESET,
                ((curl_resp = curl_upload_file(remote_fname, local_fname)) / 100 == 2) ? KGRN "OK" : KRED "ERROR"
                );

        resx = (curl_resp / 100 == 2) ? 1 : 0;
    } else {
        printf(KRED "Cannot read configuration file\n" RESET);
        resx = 0;
    }

    return resx;

}

/*
 * delete or remove remote or local file
 * if new_file == NULL ==> remote file
 * */
char delete_remove_file(const char *old_file, const char *new_file) {
    char resx = 0;
    char webdav_urlx[300];
    char remote_fname[400], local_fname[400];
    int curl_resp = 0;

    //new_file == NULL
    if (!new_file) {
        //old_file[0] == '/' ==> local file del
        //~ if(old_file[0] == '/'){
        if (gotString("http://", old_file) == 0) {
            printf(KYEL "Deleting %s ... ", old_file);
            fflush(stdout);

            resx = (remove(old_file) == 0) ? 1 : 0;

            printf("%s\n" RESET,
                    resx ? KGRN "OK" : KRED "ERROR"
                    );
        }//del remote file
        else {
            if (get_webdav_url(webdav_urlx, NULL, NULL)) {
                //copy it
                strcpy(local_fname, old_file);
                snprintf(remote_fname, 400, "%s%s",
                        webdav_urlx,
                        basename(local_fname)
                        );

                printf(KYEL "Deleting %s ... ", remote_fname);
                fflush(stdout);

                //check the respon download
                printf("%s\n" RESET,
                        ((curl_resp = curl_delete_file(remote_fname)) / 100 == 2) ? KGRN "OK" : KRED "ERROR"
                        );

                resx = (curl_resp / 100 == 2) ? 1 : 0;
            } else {
                printf(KRED "Cannot read configuration file\n" RESET);
                resx = 0;
            }
        }
    }//move local file
    else {
        printf(KYEL "Moving %s to %s ... ", old_file, new_file);
        fflush(stdout);

        resx = (rename(old_file, new_file) == 0);
        printf("%s\n" RESET,
                resx ? KGRN "OK" : KRED "ERROR"
                );
    }

    return resx;
}

/*
 * parse playlist to item lists
 * */
char parse_playlist(const char *playlist_file, char items[100][100], unsigned int *items_count) {
    char resx = 0;
    char buff_data[1024], buff_data2[1024];
    char isFirstLine = 1;
    FILE *fp = fopen(playlist_file, "r");

    *items_count = 0;

    if (fp) {
        //read and parse
        while (fgets(buff_data, 1000, fp) != NULL) {
            //1st line?
            if (isFirstLine) {
                if (gotString(":", buff_data)) {
                } else //invalid playlist
                {
                    printf(KRED "Invalid playlist\n" RESET);
                    break;
                }

                isFirstLine = 0;
            }//not first line
            else {
                if (*items_count < 100) {
                    //file_name		type	duration_in_seconds		overlaps
                    //get filename only
                    sscanf(buff_data, "%[^\t\n]\t%s\t%f\t%d", &buff_data2, NULL, NULL, NULL);

                    snprintf(items[(*items_count)++], 100, "%s", buff_data2);
                    printf(KGRN "Adding item #%.02d = %s\n" RESET, *items_count, items[*items_count - 1]);
                    //printf(KGRN "item[%d] = %s\n", (*items_count)++, buff_data2);
                } else {
                    printf(KRED "Maximum items reach\n" RESET);
                    break;
                }
            }
        }

        fclose(fp);

        resx = (*items_count > 0) ? 1 : 0;
    } else {
        //cannot open file
    }

    return resx;
}

/*
 * download playlist items
 * */
char download_playlist_items(const char *pls_fname, char items[100][100], const unsigned int count, const char delete_remote_file) {
    int idx, curl_respons;
    char remote_filename[400], local_filename[400], moved_filename[400],
            webdav_urlx[400],
            all_downloaded = 1;

    if (get_webdav_url(webdav_urlx, NULL, NULL) == 1) {
        for (idx = 0; idx < count; idx++) {
            //format the url
            snprintf(remote_filename, 400, "\"%s%s\"",
                    webdav_urlx,
                    repl_str((const char *) &items[idx], " ", "%20"));

            //format output file
            //~ snprintf(local_filename,400,"\"" DOWNLOAD_PATH "%s\"",
            //~ items[idx]);
            snprintf(local_filename, 400, DOWNLOAD_PATH "%s",
                    items[idx]);
            snprintf(moved_filename, 400, "\"%s\"", local_filename);

            //info
            printf(KYEL "Downloading %s to %s ... " RESET, remote_filename, moved_filename);
            fflush(stdout);

            printf("%s\n" RESET,
                    ((curl_respons = curl_download_file(remote_filename, moved_filename)) / 100) == 2 ?
                    KGRN "OK" : KRED "ERROR"
                    );

            //move to MP3 if ok
            if (curl_respons / 100 == 2) {
                //format to destination
                snprintf(
                        moved_filename, 400,
                        MP3_PATH "%s",
                        items[idx]
                        );

                //~ printf(KYEL "Moving %s to %s ... " RESET, local_filename, moved_filename);
                //~ fflush(stdout);

                //move to destination
                if (delete_remove_file(local_filename, moved_filename) == 1) {
                    //~ printf(KGRN "OK\n" RESET);

                    //delete file on server
                    if (delete_remote_file) {
                        //info
                        printf(KYEL "Delete remote file %s ... " RESET, remote_filename);
                        fflush(stdout);

                        printf("%s\n" RESET,
                                (curl_respons = curl_delete_file(remote_filename)) / 100 == 2 ?
                                KGRN "OK" : KRED "ERROR"
                                );

                        if (curl_respons / 100 != 2)
                            all_downloaded = 0;
                    }
                } else {
                    //~ printf(KRED "ERROR\n" RESET);
                    all_downloaded = 0;
                }
            } else {
                all_downloaded = 0;
            }
        }
    } else {
        printf(KRED "Cannot read webdav url. Check configuration file!!!\n" RESET);
        all_downloaded = 0;
    }

    //move playlist if it is all downloaded
    if (all_downloaded) {
        snprintf(
                moved_filename, 400, PLAYLIST_PATH "%s",
                basename((char*)pls_fname)
                );
        //~ printf(KYEL "Moving playlist to %s ... " RESET,moved_filename);
        //~ fflush(stdout);
        //~ 
        //~ printf("%s\n" RESET,
        //~ (delete_remove_file(pls_fname,moved_filename)==1) ?
        //~ KGRN "OK" : KRED "ERROR"
        //~ );
        if (delete_remove_file(pls_fname, moved_filename) == 1) {
            //format the url
            snprintf(remote_filename, 400, "\"%s%s\"",
                    webdav_urlx,
                    repl_str((const char *) basename((char*)pls_fname), " ", "%20"));

            //~ delete_remove_file(local_filename,moved_filename);

            //delete file on server
            if (delete_remote_file) {
                //info
                printf(KYEL "Delete remote file %s ... " RESET, remote_filename);
                fflush(stdout);

                printf("%s\n" RESET,
                        (curl_respons = curl_delete_file(remote_filename)) / 100 == 2 ?
                        KGRN "OK" : KRED "ERROR"
                        );

                if (curl_respons / 100 != 2)
                    all_downloaded = 0;
            }
        }
    }

    return all_downloaded;
}

//convert time_t to string

char *datetime_to_string(const time_t time_input, const char *datetime_format) {
    char *buffer;
    time_t curtime;
    struct tm *loc_time;

    buffer = (char*) calloc(30, sizeof (char));
    if (!buffer)
        return NULL;

    //Getting current time of system
    curtime = time_input;

    // Converting current time to local time
    loc_time = localtime(&curtime);

    // Displaying date and time in standard format
    //    printf("%s", asctime (loc_time));
    //format datetime
    strftime(buffer, 30, datetime_format, loc_time);

    return buffer;
}

/*
upload local log files to server and delete on success	
 */
char upload_log_files(const char *dav_server, const int days_before, const char delete_local_file) {
    char resx = 0, upload_fname[400], local_fname[400], local_fname2[400];
    time_t curr_time = time(NULL);
    int id, curl_respons;

    if (days_before > 0) {
        for (id = 1; id <= days_before; id++) {
            //format remote filename
            curr_time -= 24 * 3600;
            snprintf(upload_fname, 400, "\"%s/logs/%s.log\"",
                    dav_server,
                    repl_str(datetime_to_string(curr_time, DATETIME_FMT_LOG_FILE), " ", "%20")
                    );

            //format local filename
            snprintf(local_fname, 400, "%s%s.log",
                    LOG_PATH,
                    datetime_to_string(curr_time, DATETIME_FMT_LOG_FILE)
                    );
            snprintf(local_fname2, 400, "\"%s\"",
                    local_fname
                    );

            if (isFileExists(local_fname)) {
                //info
                printf(KYEL "Uploading %s to %s ... " RESET, local_fname2, upload_fname);
                fflush(stdout);

                //uploading
                printf(
                        "%s\n" RESET,
                        (curl_respons = curl_upload_file(upload_fname, local_fname2)) / 100 == 2 ?
                        KGRN "OK" : KRED "ERROR"
                        );

                //del loc file?
                if (delete_local_file) {
                    delete_remove_file(local_fname, NULL);
                }
            } else {
                printf(KRED "File %s not found\n" RESET, local_fname);
            }
        }
    }

    return resx;
}

/*
 * download all files on remote server
 * */
char download_remote_dir(const char *remote_url, const char is_delete_remote_file) {
    char resx = 0;
    int curl_respons = 0;
    unsigned char *res;
    char *item_res;
    char remote_filename[400], local_filename[400], moved_filename[400];

    //download remote dir content as text
    printf(KYEL "Downloading remote directory : %s ... " RESET, remote_url);
    if ((curl_respons = curl_download_file(remote_url, "/tmp/dl1.log")) / 100 == 2) {
        printf(KGRN "OK\n" KYEL "Parsing content ... " RESET);

        //parse the result
        //if(system("cat /tmp/dl1.log | tr '>' '\n' | tr '<' '\n' | grep -i '.mp3\|.mp4\|.avi\|.mpg\|.pls\|.pls.new' | grep -v -i 'a href='")==0)
        if (run_command("cat /tmp/dl1.log | tr '>' '\\n' | tr '<' '\\n' | grep -i '.mp3\\|.mp4\\|.avi\\|.mpg\\|.pls\\|.pls.new' | grep -v -i 'a href='", (char**) &res)) {
            printf(KGRN "OK\n" RESET);

            //download the files
            //get 1st token
            item_res = strtok(res, "\n");

            while (item_res != NULL) {
                //format the url
                snprintf(remote_filename, 400, "\"%s%s\"",
                        remote_url,
                        repl_str((const char *) item_res, " ", "%20"));

                //format output file
                snprintf(local_filename, 400, DOWNLOAD_PATH "%s",
                        item_res);
                snprintf(moved_filename, 400, "\"%s\"", local_filename);

                //info
                printf(KYEL "Downloading %s to %s ... " RESET, remote_filename, moved_filename);
                fflush(stdout);

                //download ok?
                printf("%s\n" RESET,
                        ((curl_respons = curl_download_file(remote_filename, moved_filename)) / 100) == 2 ?
                        KGRN "OK" : KRED "ERROR"
                        );

                //move to MP3 if ok
                if (curl_respons / 100 == 2) {
                    //format to destination
                    //only move media files, playlist later
                    if (gotString(".pls", item_res) || gotString(".pls.new", item_res)) {
                        //~ snprintf(
                        //~ moved_filename,400,
                        //~ PLAYLIST_PATH "%s",
                        //~ item_res
                        //~ );
                        printf(KYEL "Playlist file %s found, move later\n" RESET, item_res);

                        //delete file on server
                        if (is_delete_remote_file) {
                            //info
                            printf(KYEL "Delete remote file %s ... " RESET, remote_filename);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (curl_respons = curl_delete_file(remote_filename)) / 100 == 2 ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }
                    } else {
                        snprintf(
                                moved_filename, 400,
                                MP3_PATH "%s",
                                item_res
                                );

                        //~ printf(KYEL "Moving %s to %s ... " RESET, local_filename, moved_filename);
                        //~ fflush(stdout);

                        //move to destination
                        if (delete_remove_file(local_filename, moved_filename) == 1) {
                            //~ printf(KGRN "OK\n" RESET);

                            //delete file on server
                            if (is_delete_remote_file) {
                                //info
                                printf(KYEL "Delete remote file %s ... " RESET, remote_filename);
                                fflush(stdout);

                                printf("%s\n" RESET,
                                        (curl_respons = curl_delete_file(remote_filename)) / 100 == 2 ?
                                        KGRN "OK" : KRED "ERROR"
                                        );
                            }
                        }
                    }
                }

                //get next item
                item_res = strtok(NULL, "\n");
            }

            //move playlist
            printf(KYEL "Moving playlist files ... " RESET);
            printf("%s\n" RESET,
                    (system("sudo mv " DOWNLOAD_PATH "*.pls* " PLAYLIST_PATH) == 0) ?
                    KGRN "OK" : KRED "ERROR"
                    );
        } else {
            printf(KRED "ERROR\n" RESET);
        }
    } else {
        printf(KRED "ERROR\n" RESET);
    }

    free(item_res);
    free(res);

    return resx;
}

char ftp_download_remote_dir(const char *remote_url, const int remote_port, const char *username_pass, const char is_delete_remote_file) {
    char resx = 0, got_item_on_server = 0;
    int curl_respons = 0, cnt_retry = 0;
    unsigned char *res = NULL;
    char *item_res = NULL;
    char remote_filename[400], local_filename[400], moved_filename[400], cmd[400];

    //download remote dir content as text
    printf(KYEL "Downloading remote directory : %s ... " RESET, remote_url);
    fflush(stdout);

    if ((res = curl_ftp_list_dir(remote_url, remote_port, username_pass)) != NULL) {
        //download the files
        //get 1st token
        item_res = strtok(res, "\n");

        //~ printf(KGRN "Got 1st item : %s\n" RESET, item_res);

        while (item_res != NULL) {
            if (strcmp(item_res, "logs") != 0) {
                if (got_item_on_server != 1)
                    got_item_on_server = 1;

                //format the url
                snprintf(remote_filename, 400, "\"%s%s\"",
                        remote_url,
                        item_res);

                //format output file
                //~ snprintf(local_filename,400, DOWNLOAD_PATH "%s",item_res);
                snprintf(moved_filename, 400, "\"" DOWNLOAD_PATH "%s\"", item_res);

                //info
                printf(KYEL "Downloading %s to %s ... \n" RESET, remote_filename, moved_filename);
                fflush(stdout);

                //buil curl command
                snprintf(cmd, 400, "curl -# -m 600 -P %d -u %s --ftp-pasv -C - -o %s %s", remote_port, username_pass, moved_filename, remote_filename);

                //delete after download?
                snprintf(local_filename, 400, "");
                if (is_delete_remote_file) {
                    snprintf(local_filename, 400, " -Q \"-DELE %s\"", item_res);
                    strcat(cmd, local_filename);
                }

                //run it for 10 times max if fail
                for (cnt_retry = 0; cnt_retry < 10; cnt_retry++) {
                    curl_respons = system(cmd);
                    if (curl_respons == 0) {
                        //move to folder destination
                        //audio file
                        if (gotString(".mp3", item_res)) {
                            snprintf(cmd, 400, "sudo mv %s \"" MP3_PATH "%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to " MP3_PATH " ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //video file
                        if (gotString(".mp4", item_res) || gotString(".mpg", item_res) || gotString(".avi", item_res) || gotString(".mkv", item_res)) {
                            snprintf(cmd, 400, "sudo mv %s \"" VIDEO_PATH "%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to " VIDEO_PATH " ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //image file
                        if (gotString(".jpg", item_res) || gotString(".jpeg", item_res) || gotString(".bmp", item_res) || gotString(".png", item_res)) {
                            snprintf(cmd, 400, "sudo mv %s \"" IMAGES_PATH "%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to " IMAGES_PATH " ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //enstreamer file
                        if (gotString("enstreamer", item_res)) {
                            printf(KYEL "Updating enStreamer program ... \nKilling running service and application ...\n" RESET);
                            fflush(stdout);

                            //stop the service
                            system("sudo systemctl stop enstreamer && sleep 5");
                            //kill the app
                            system("sudo pkill enstreamer");
                            system("sudo killall enstreamer && sleep 2");

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/usr/bin/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /usr/bin ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            //restart the service
                            printf(KYEL "Restarting service ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system("sudo systemctl start enstreamer") == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //enclock_synch file
                        if (gotString("enclock_synch", item_res)) {
                            printf(KYEL "Updating enClockSynch program ...\nKilling service and running application ...\n" RESET);
                            fflush(stdout);

                            //stop the service
                            system("sudo systemctl stop enclocksynch && sleep 5");
                            //kill the app
                            system("sudo pkill enclock_synch");
                            system("sudo killall enclock_synch && sleep 2");

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/usr/bin/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /usr/bin ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            //restart the service
                            printf(KYEL "Restarting service ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system("sudo systemctl start enclocksynch") == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //enslide file
                        if (gotString("enslide", item_res)) {
                            printf(KYEL "Updating enSlide program ...\nKilling service and application ... \n " RESET);
                            fflush(stdout);

                            //stop the service
                            system("sudo systemctl stop enslide && sleep 5");
                            //kill the app
                            system("sudo pkill enslide");
                            system("sudo killall enslide && sleep 2");

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/usr/bin/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /usr/bin ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            //restart the service
                            printf(KYEL "Restarting service ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system("sudo systemctl start enslide") == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //ensynch file
                        //need to find a way to update encynsh
                        if (gotString("ensynch", item_res)) {
                            printf(KYEL "Updating enSynch program ... " RESET);
                            fflush(stdout);

                            //stop the service
                            system("sudo systemctl stop ensynch && sleep 5");
                            //kill the app
                            system("sudo pkill ensynch");
                            system("sudo killall ensynch && sleep 2");

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/usr/bin/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /usr/bin ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            //restart the service
                            printf(KYEL "Restarting service ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system("sudo systemctl start ensynch") == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //enstreamer-setup file
                        if (gotString("enstreamer-setup", item_res)) {
                            printf(KYEL "Updating enStreamer Setup program ...\nKilling service and application ...\n " RESET);
                            fflush(stdout);

                            //kill the app
                            system("sudo pkill enstreamer-setup");
                            system("sudo killall enstreamer-setup && sleep 2");

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/usr/bin/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /usr/bin ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //web.zip file
                        if (gotString("web.zip", item_res)) {
                            printf(KYEL "Updating web files ...\n" RESET);
                            fflush(stdout);

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/tmp/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /tmp ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            printf(KYEL "Unzipping files ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system("sudo unzip -o -d /data/media/www /tmp/web.zip") == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );
                        }

                        //.bashrc file
                        if (gotString(".bashrc", item_res)) {
                            printf(KYEL "Updating .bashrc ...\n" RESET);
                            fflush(stdout);

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo mv %s \"/home/enco1/%s\"", moved_filename, item_res);
                            printf(KYEL "Moving file to /home/enco1 ... " RESET);
                            fflush(stdout);

                            printf("%s\n" RESET,
                                    (system(cmd) == 0) ?
                                    KGRN "OK" : KRED "ERROR"
                                    );

                            system("sudo chown enco1:users /home/enco1/.bashrc");
                        }

                        //restart.now file
                        if (gotString("restart.now", item_res)) {
                            printf(KYEL "Restarting ...\n" RESET);
                            fflush(stdout);

                            //move the file to /usr/bin
                            snprintf(cmd, 400, "sudo rm %s", moved_filename);
                            system(cmd);
                            system("sudo reboot now");
                        }


                        break;
                    }

                    printf(KRED "Retry downloading for #%d\n" RESET, (cnt_retry + 1));
                }
            }

            //get next item
            item_res = strtok(NULL, "\n");
        }

        //move playlist files
        if (got_item_on_server == 1) {
            printf(KYEL "Moving playlist audio video files ... " RESET);
            printf("%s\n" RESET,
                    (system("sudo mv " DOWNLOAD_PATH "*.pls* " PLAYLIST_PATH) == 0) ?
                    KGRN "OK" : KRED "ERROR"
                    );
        }
    } else {
        printf(KRED "ERROR\n" RESET);
    }

    //free it
    if (item_res)
        free(item_res);

    if (res)
        free(res);

    return resx;
}

/*
upload local log files to server and delete on success	
 */
char ftp_upload_log_files(const char *remote_url, const int remote_port, const char *username_pass, const int days_before, const char delete_local_file) {
    char resx = 0, upload_fname[400], local_fname[400], local_fname2[400], cmd[400];
    time_t curr_time = time(NULL);
    int id, curl_respons;

    if (days_before > 0) {
        for (id = 1; id <= days_before; id++) {
            //format remote filename
            curr_time -= 24 * 3600;
            snprintf(upload_fname, 400, "\"%slogs/%s.log\"",
                    remote_url,
                    repl_str(datetime_to_string(curr_time, DATETIME_FMT_LOG_FILE), " ", "%20")
                    );

            //format local filename
            snprintf(local_fname, 400, "%s%s.log",
                    LOG_PATH,
                    datetime_to_string(curr_time, DATETIME_FMT_LOG_FILE)
                    );
            snprintf(local_fname2, 400, "\"%s\"",
                    local_fname
                    );

            if (isFileExists(local_fname)) {
                //info
                printf(KYEL "Uploading %s to %s ... " RESET, local_fname2, upload_fname);
                fflush(stdout);

                //uploading
                snprintf(cmd, 400, "curl -# -m 6000 -P %d -u %s --ftp-pasv -C - -T %s %s", remote_port, username_pass, local_fname2, upload_fname);
                printf(
                        "%s\n" RESET,
                        (curl_respons = system(cmd)) == 0 ?
                        KGRN "OK" : KRED "ERROR"
                        );

                //del loc file?
                if (delete_local_file) {
                    //~ delete_remove_file(local_fname2,NULL);
                    snprintf(cmd, 400, "sudo rm %s", local_fname2);
                    printf(KYEL "Delete %s ... " RESET, local_fname2);
                    printf("%s\n" RESET,
                            (system(cmd) == 0) ?
                            KGRN "OK" : KRED "ERROR"
                            );
                }
            } else {
                //~ printf(KRED "File %s not found\n" RESET, local_fname);
            }
        }
    }

    return resx;
}

//signal resgister handler

void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf(KYEL "\nExit program\nThank you\n" RESET);
        exit(EXIT_SUCCESS);
    }
}

/*
 limit ram usage
 */
//void limit_ram_usage(const unsigned long max_ram) {
//    // Define and object of structure 
//    // rlimit. 
//    struct rlimit rl;
//
//    // First get the limit on memory 
//    if (getrlimit(RLIMIT_AS, &rl) == 0) {
//        //        printf(KGRN "Current limit : %lld\n" RESET, (long long int) rl.rlim_cur);
//        printf(KGRN "Current limit : %.3f\n" RESET, (float) rl.rlim_cur / 1000000);
//
//        // Change the limit in byte
//        rl.rlim_cur = max_ram;
//
//        // Now call setrlimit() to set the  
//        // changed value. 
//        if (setrlimit(RLIMIT_AS, &rl) == 0) {
//            printf(KGRN "RAM limit changes\n" RESET);
//
//            // Again get the limit and check 
//            getrlimit(RLIMIT_AS, &rl);
//
//            //            printf(KGRN "Current limit : %lld\n" RESET, (long long int) rl.rlim_cur);
//            printf(KGRN "Current limit : %.3f\n" RESET, (float) rl.rlim_cur / 1000000);
//        } else {
//            printf(KRED "Cannot set current limit\n" RESET);
//        }
//    } else {
//        printf(KRED "Cannot get current limit\n" RESET);
//    }
//}

//send alive status to server

int send_alive_status(const char *remote_url, const int remote_port, const char *username_pass) {
    int resx = 0;
    char cmd[400], upload_fname[400];

    resx = system("echo `sudo cat /etc/hostname` `date` > /tmp/heartbeat.txt");
    if (resx == 0) {
        //send it
        printf(KYEL "Sending alive status file ... ");
        fflush(stdout);

        //format upload fname
        snprintf(upload_fname, 400, "\"%slogs/heartbeat.txt\"",
                remote_url
                );

        //uploading
        snprintf(cmd, 400, "curl -# -m 6000 -P %d -u %s --ftp-pasv -C - -T %s %s", remote_port, username_pass, "/tmp/heartbeat.txt", upload_fname);
        printf(
                "%s\n" RESET,
                (resx = system(cmd)) == 0 ?
                KGRN "OK" : KRED "ERROR"
                );
    } else {
        printf(KRED "Cannot create alive status file!!!\n" RESET);
    }

    return resx;
}

//main program

int main(int argc, char **argv) {
    /*
    program flow :
    1. read hostname
    2. read webdav server, port, username, and pass from enstreamer.conf
    3. assemble the webdav url in form : http://user:password@server:port/hostname/
    4. check log file for any previous date file and upload it
    5. check for current date playlist and download the content to temp download folder
    6. if all ok, move the files to mpc mp3 folder, and playlist to playlist folder
    7. delete server file if any
     */
    char webdavx[400], urlx[400], loc_pls_fname[400], item_list[100][100];
    unsigned int check_interval = 30, items_count = 0;
    char *remote_name;
    int remote_port;
    char usr_pass[40];

    //start program	
    printf(KGRN "%s\nVersion\t: %s\n\n" RESET, PROGRAM_TITLE, PROGRAM_VERSION);

    /* save device id to file */
    save_id_file();
    if (validatekey() != 0) {
        printf(KRED "REGISTER THIS PROGRAM\n" RESET);
        return EXIT_SUCCESS;
    }

    //limit ram usage
    limit_ram_usage(75000000);

    //register signal handler
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf(KRED "Cannot attach signal interrupt handler\n" RESET);
    }

    //loop
    for (;;) {
        //get webdav server name
        if (get_webdav_url(webdavx, &remote_port, usr_pass)) {
            //webdav url show
            printf(KGRN "Remote server : %s at port %d\n" RESET, webdavx, remote_port);

            //send alive status
            send_alive_status(webdavx, remote_port, usr_pass);

            //upload logfile
            //upload files from n days ago
            ftp_upload_log_files(webdavx, remote_port, usr_pass, 7, 1);

            //DOWNLOAD ALL FTP REMOTE CONTENT
            ftp_download_remote_dir(webdavx, remote_port, usr_pass, 1);

            //process file
        } else {
            printf(KRED "Get webdav server address from configuration file failed!!!\n" RESET);
        }

        //wait for interval
        printf(KYEL "\nWaiting for %ds ...\n\n" RESET, check_interval);
        sleep(check_interval);
        check_interval = get_synch_interval();
    }

    //return code
    return 0;
}
