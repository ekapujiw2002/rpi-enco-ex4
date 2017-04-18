/* 
 * File:   text_color.h
 * Author: EX4
 *
 * Created on May 28, 2015, 7:20 PM
 */

#ifndef TEXT_COLOR_H
#define	TEXT_COLOR_H

#ifdef	__cplusplus
extern "C" {
#endif

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


#ifdef	__cplusplus
}
#endif

#endif	/* TEXT_COLOR_H */

