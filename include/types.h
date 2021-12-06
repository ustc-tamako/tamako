#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#ifndef NULL
    #define NULL 0
#endif

#ifndef TRUE
    #define TRUE  1
    #define FALSE 0
#endif

/* 
 * =============================================================
 * 
 *                         32bits - System
 * 
 *               char :  1B                 short :  2B
 *      unsigned char :  1B        unsigned short :  2B
 *             char * :  4B                  long :  4B
 *                int :  4B         unsigned long :  4B
 *       unsigned int :  4B             long long :  8B
 * 
 * =============================================================
 */

typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

typedef unsigned int   size_t;

#endif  // INCLUDE_TYPES_H_