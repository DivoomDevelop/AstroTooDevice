#ifndef _DIVOOM_TYPE_TIVOO_H_
#define _DIVOOM_TYPE_TIVOO_H_

typedef unsigned char 	T_U8;

typedef unsigned char 	uint8;
typedef signed char 	int8;

typedef unsigned short 	uint16;
typedef signed short 	int16;

typedef unsigned int    uint32;
typedef signed int 		int32;

typedef unsigned long   DIVOOM_UINT_ID;
#ifndef bool
typedef uint8 bool;
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
#define U8_MAX  0XFF
#define U16_MAX  0XFFFF
#define U32_MAX  0XFFFFFFFF

#if 0 
#define    BIT0	 (1 << 00)
#define    BIT1	 (1 << 01)
#define    BIT2	 (1 << 02)
#define    BIT3	 (1 << 03)
#define    BIT4	 (1 << 04)
#define    BIT5	 (1 << 05)
#define    BIT6	 (1 << 06)
#define    BIT7	 (1 << 07)
#define    BIT8	 (1 << 08)
#define    BIT9	 (1 << 09)
#define    BIT10	 (1 << 10)
#define    BIT11	 (1 << 11)
#define    BIT12	 (1 << 12)
#define    BIT13	 (1 << 13)
#define    BIT14	 (1 << 14)
#define    BIT15	 (1 << 15)
#endif


typedef unsigned char           T_U8;       /* unsigned 8 bit integer */
typedef char                    T_CHR;      /* char */
typedef unsigned short          T_U16;      /* unsigned 16 bit integer */
typedef unsigned int              T_U32;      /* unsigned 32 bit integer */
typedef unsigned long long      T_U64;      /* unsigned 64 bit integer */
typedef signed char             T_S8;       /* signed 8 bit integer */
typedef signed short            T_S16;      /* signed 16 bit integer */
typedef int                     T_S32;      /* signed 32 bit integer */
typedef signed long long        T_S64;      /* signed 64 bit integer */
typedef void                    T_VOID;     /* void */
typedef float                   T_FLOAT;
typedef volatile unsigned int   V_UINT32;
#define    T_U8_MAX             ((T_U8)0xff)                    // maximum T_U8 value
#define    T_U16_MAX            ((T_U16)0xffff)                    // maximum T_U16 value
#define    T_U32_MAX            ((T_U32)0xffffffff)                // maximum T_U32 value
#define    T_U64_MAX            ((T_U32)0xffffffffffffffff)                // maximum T_U64 value
#define    T_S8_MIN             ((T_S8)(-127-1))                // minimum T_S8 value
#define    T_S8_MAX             ((T_S8)127)                        // maximum T_S8 value
#define    T_S16_MIN            ((T_S16)(-32767L-1L))        // minimum T_S16 value
#define    T_S16_MAX            ((T_S16)(32767L))            // maximum T_S16 value
#define    T_S32_MIN            ((T_S32)(-2147483647L-1L))    // minimum T_S32 value
#define    T_S32_MAX            ((T_S32)(2147483647L))        // maximum T_S32 value
#define    T_S64_MIN            ((T_S32)(-9223372036854775807L-1L))    // minimum T_S64 value
#define    T_S64_MAX            ((T_S32)(9223372036854775807L))        // maximum T_S64 value

/* basal type definition for global area */
typedef T_U8                    T_BOOL;     /* BOOL type */

typedef T_VOID *                T_pVOID;    /* pointer of void data */
typedef const T_VOID *          T_pCVOID;   /* const pointer of void data */

typedef T_CHR *                 T_pSTR;     /* pointer of string */
typedef const T_CHR *           T_pCSTR;    /* const pointer of string */


typedef T_U16                   T_WCHR;     /**< unicode char */
typedef T_U16 *                 T_pWSTR;    /* pointer of unicode string */
typedef const T_U16 *           T_pCWSTR;   /* const pointer of unicode string */


typedef T_U8 *                  T_pDATA;    /* pointer of data */
typedef const T_U8 *            T_pCDATA;   /* const pointer of data */

typedef T_U32                   T_COLOR;        /* color value */

typedef T_U32                   T_HANDLE;           /* a handle */



#endif


