/* 
 * File:   system_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:35 PM
 */

#ifndef SYSTEM_UTIL_H
#define	SYSTEM_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //used lib
#include <stdio.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include <sys/resource.h> 
#include <sys/time.h> 

    //custom lib
#include "text_color.h"

    //constant
#define MAX_BUF 	1024
#define PID_LIST_BLOCK 	32

    //proto func void
    int check_if_number(char *str);
    int *pidof(char *pname);
    char kill_process(char *process_name);
    int spawn(char *fname);
    char run_command(const char *cmd, char **message);
    unsigned char* getcpuid();
    int limit_ram_usage(const unsigned long max_ram);

    //get pid of a process name

    /* checks if the string is purely an integer
     * we can do it with `strtol' also
     */
    int check_if_number(char *str) {
        int i;
        for (i = 0; str[i] != '\0'; i++) {
            if (!isdigit(str[i])) {
                return 0;
            }
        }
        return 1;
    }

    int *pidof(char *pname) {
        DIR *dirp;
        FILE *fp;
        struct dirent *entry;
        int *pidlist, pidlist_index = 0, pidlist_realloc_count = 1;
        char path[MAX_BUF], read_buf[MAX_BUF];

        dirp = opendir("/proc/");
        if (dirp == NULL) {
            perror("Fail");
            return NULL;
        }

        pidlist = malloc(sizeof (int) * PID_LIST_BLOCK);
        if (pidlist == NULL) {
            return NULL;
        }

        while ((entry = readdir(dirp)) != NULL) {
            if (check_if_number(entry->d_name)) {
                strcpy(path, "/proc/");
                strcat(path, entry->d_name);
                strcat(path, "/comm");

                /* A file may not exist, it may have been removed.
                 * dut to termination of the process. Actually we need to
                 * make sure the error is actually file does not exist to
                 * be accurate.
                 */
                fp = fopen(path, "r");
                if (fp != NULL) {
                    fscanf(fp, "%s", read_buf);
                    if (strcmp(read_buf, pname) == 0) {
                        /* add to list and expand list if needed */
                        pidlist[pidlist_index++] = atoi(entry->d_name);
                        if (pidlist_index == PID_LIST_BLOCK * pidlist_realloc_count) {
                            pidlist_realloc_count++;
                            pidlist = realloc(pidlist, sizeof (int) * PID_LIST_BLOCK * pidlist_realloc_count); //Error check todo
                            if (pidlist == NULL) {
                                return NULL;
                            }
                        }
                    }
                    fclose(fp);
                }
            }
        }


        closedir(dirp);
        pidlist[pidlist_index] = -1; /* indicates end of list */
        return pidlist;
    }

    //kill a process

    char kill_process(char *process_name) {
        char resx = 0;
        int *listx;

        listx = pidof(process_name);

        if (listx && (listx[0] != -1)) {
            if (kill(listx[0], SIGTERM) != 0) {
                usleep(100000);
                resx = kill(listx[0], SIGKILL) == 0;
            }
        }
        free(listx);

        return resx;
    }

    /* Spawn a child process running a new program.  PROGRAM is the name
       of the program to run; the path will be searched for this program.
       ARG_LIST is a NULL-terminated list of character strings to be
       passed as the program's argument list.  Returns the process id of
       the spawned process.  */

    //int spawn(char* program, char** arg_list)
    //int spawn(char* program, char *fname)

    int spawn(char *fname) {
        pid_t child_pid;
        //~ int w_status;
        char cmd[600];

        /* Duplicate this process.  */
        child_pid = fork();
        if (child_pid != 0)
            /* This is the parent process.  */
            return child_pid;
        else {
            /* Now execute PROGRAM, searching for it in the path.  */
            //~ execvp (program, arg_list);

            //~ execlp("omxplayer","omxplayer","-b","--no-osd","-o","both",fname);
            /* The execvp function returns only if an error occurs.  */
            //~ fprintf (stderr, "an error occurred in execvp\n");

            //abort - cause abnormal process termination
            //~ abort();

            snprintf(cmd, 600, "omxplayer -b --no-osd -o both \"%s\"", fname);
            system(cmd);
            //~ execlp("omxplayer","omxplayer","-b","--no-osd","-o","both",fname);
            printf(KGRN "Play video done\n" RESET);

            /* 
             * Parent waiting for child to complete.
             * Once done, all file handles used by the child will be closed, this is
             * done to expedite the complete writing of all cached data to the file.
             */
            //~ waitpid(child_pid, &w_status, 0);

            exit(EXIT_SUCCESS);
        }
    }

    //run a command and return result

    char run_command(const char *cmd, char **message) {
        FILE *fp;
        int status;
        char msg_res[1024];

        fp = popen(cmd, "r");
        if (!fp) {
            return 0;
        }

        //allcate message pointer
        *message = (char*) calloc(65536, sizeof (char));

        if (!message)
            return 0;

        while ((fgets(msg_res, 1024, fp) != NULL) && (strlen(*message) < 65000))
            strcat(*message, msg_res);

        status = pclose(fp);
        return (status == -1) ? 0 : 1;
    }

    //get cpuid

    unsigned char* getcpuid() {
        unsigned char *cpuid = NULL;

        if (run_command("sudo cpuid", (char**) &cpuid)) {
            return cpuid;
        } else
            return NULL;
    }

    /*
 limit ram usage
     */
    int limit_ram_usage(const unsigned long max_ram) {
        // Define and object of structure 
        // rlimit. 
        struct rlimit rl;

        // First get the limit on memory 
        if (getrlimit(RLIMIT_AS, &rl) == 0) {
            //        printf(KGRN "Current limit : %lld\n" RESET, (long long int) rl.rlim_cur);
            printf(KGRN "Current limit : %.3fMB\n" RESET, (float) rl.rlim_cur / 1000000);

            // Change the limit in byte
            rl.rlim_cur = max_ram;

            // Now call setrlimit() to set the  
            // changed value. 
            if (setrlimit(RLIMIT_AS, &rl) == 0) {
                printf(KGRN "RAM limit changes\n" RESET);

                // Again get the limit and check 
                getrlimit(RLIMIT_AS, &rl);

                //            printf(KGRN "Current limit : %lld\n" RESET, (long long int) rl.rlim_cur);
                printf(KGRN "Current limit : %.3fMB\n" RESET, (float) rl.rlim_cur / 1000000);
                return (rl.rlim_cur == max_ram) ? 0 : 1;
            } else {
                printf(KRED "Cannot set current limit\n" RESET);
                return 1;
            }
        } else {
            printf(KRED "Cannot get current limit\n" RESET);
            return 1;
        }
    }


#ifdef	__cplusplus
}
#endif

#endif	/* SYSTEM_UTIL_H */

