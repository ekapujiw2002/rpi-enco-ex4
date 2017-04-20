/* 
 * File:   string_util.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:32 PM
 */

#ifndef STRING_UTIL_H
#define	STRING_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

    //used lib
#include <stdio.h>
#include <string.h>
#include <stddef.h>

    //proto func
    char *repl_str(const char *str, const char *old, const char *new);
    char from_hex(char ch);
    char to_hex(char code);
    char *url_encode(char *str);
    char *url_decode(char *str);
    unsigned int count_char(const char *astring, const char achar);
    char gotString(const char *asearch, const char *astring);
    int gotchar(const int asearch, const char *astring);

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
        if (!pos_cache)
            free(pos_cache);
        return ret;
    }

    /*
    URL C ENCODE DECODE
    http://www.geekhideout.com/urlcode.shtml
     */

    /* Converts a hex character to its integer value */
    char from_hex(char ch) {
        return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    }

    /* Converts an integer value to its hex character*/
    char to_hex(char code) {
        static char hex[] = "0123456789abcdef";
        return hex[code & 15];
    }

    /* Returns a url-encoded version of str */

    /* IMPORTANT: be sure to free() the returned string after use */
    char *url_encode(char *str) {
        char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
        while (*pstr) {
            if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
                *pbuf++ = *pstr;
            else if (*pstr == ' ')
                *pbuf++ = '+';
            else
                *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
            pstr++;
        }
        *pbuf = '\0';
        return buf;
    }

    /* Returns a url-decoded version of str */

    /* IMPORTANT: be sure to free() the returned string after use */
    char *url_decode(char *str) {
        char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
        while (*pstr) {
            if (*pstr == '%') {
                if (pstr[1] && pstr[2]) {
                    *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                    pstr += 2;
                }
            } else if (*pstr == '+') {
                *pbuf++ = ' ';
            } else {
                *pbuf++ = *pstr;
            }
            pstr++;
        }
        *pbuf = '\0';
        return buf;
    }

    //count char

    unsigned int count_char(const char *astring, const char achar) {
        unsigned int i = 0, j = 0;

        while (astring[i] != '\0') {
            if (astring[i] == achar)
                j++;
            i++;
        }

        return j;
    }

    /*
     *check for a string in a string
     */
    char gotString(const char *asearch, const char *astring) {
        return (strstr(astring, asearch) ? 1 : 0);
    }

    /*
     * check char
     */
    int gotchar(const int asearch, const char *astring) {
        char *e = strchr(astring, asearch);
        return e ? (int) (e - astring) : -1;
    }


#ifdef	__cplusplus
}
#endif

#endif	/* STRING_UTIL_H */

