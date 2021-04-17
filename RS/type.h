#ifndef TYPE_H
#define TYPE_H


#ifdef __cplusplus
extern "C" {
#endif

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#ifdef WIN32
#define ulong64         unsigned long long         // CW: zweites long hinzugefügt (?)
#define sulong64        static unsigned long long  // CW: zweites long hinzugefügt (?)
#else
#define ulong64         unsigned long long
#define sulong64        static unsigned long long
#endif

/* typedef char            int8_t;
typedef short           int16_t;
typedef int             int32_t;
typedef long long       int64_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long uint64_t; */

#ifndef dword
typedef unsigned int     dword;
#endif
#ifndef word
typedef unsigned short   word;
#endif
#ifndef byte
typedef unsigned char    byte;
#endif
#ifndef long64
typedef long long        long64;
#endif

#define rvoid            register void
#define rbyte            register byte
#define rchar            register char
#define rword            register word
#define rdword           register dword
#define rshort           register short
#define rlong            register long
#define rint             register int

#define svoid            static void
#define sbyte            static byte
#define schar            static char
#define sword            static word
#define sdword           static dword
#define sshort           static short
#define slong            static long
#define sint             static int

#define vvoid            volatile void
#define vbyte            volatile byte
#define vchar            volatile char
#define vword            volatile word
#define vdword           volatile dword
#define vshort           volatile short
#define vlong            volatile long
#define vint             volatile int

#define vsvoid           volatile static void
#define vsbyte           volatile static byte
#define vschar           volatile static char
#define vsword           volatile static word
#define vsdword          volatile static dword
#define vsshort          volatile static short
#define vslong           volatile static long
#define vsint            volatile static int

#define ef               else if

#ifndef NULL
#define NULL             0
#endif

#ifndef TRUE
#define TRUE             1
#endif
#ifndef FALSE
#define FALSE            0
#endif

#ifndef _WINDOWS
#ifndef bool
#ifndef __cplusplus
typedef int bool;
#endif
#endif
#endif

#ifdef __cplusplus
};
#endif


#endif    // TYPE_H
