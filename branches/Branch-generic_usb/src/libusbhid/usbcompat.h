/*
 * usbcompat.h
 *
 * Compatibility definitions from FreeBSD, needed to make libusbhid
 * compile on other platforms.
 */

#ifndef _USBCOMPAT_H
#define _USBCOMPAT_H

#include <stdint.h>
#include <sys/types.h>

/* From /usr/include/dev/usb/usb.h */

/*
 * The USB records contain some unaligned little-endian word
 * components.  The U[SG]ETW macros take care of both the alignment
 * and endian problem and should always be used to access non-byte
 * values.
 */
typedef uint8_t uByte;
typedef uint8_t uWord[2];
typedef uint8_t uDWord[4];

#define USETW2(w,h,l) ((w)[0] = (u_int8_t)(l), (w)[1] = (u_int8_t)(h))

#define UGETW(w) ((w)[0] | ((w)[1] << 8))
#define USETW(w,v) ((w)[0] = (u_int8_t)(v), (w)[1] = (u_int8_t)((v) >> 8))
#define UGETDW(w) ((w)[0] | ((w)[1] << 8) | ((w)[2] << 16) | ((w)[3] << 24))
#define USETDW(w,v) ((w)[0] = (u_int8_t)(v), \
                     (w)[1] = (u_int8_t)((v) >> 8), \
                     (w)[2] = (u_int8_t)((v) >> 16), \
                     (w)[3] = (u_int8_t)((v) >> 24))

struct usb_ctl_report_desc {
        int     ucrd_size;
        u_char  ucrd_data[1024];        /* filled data size will vary */
};


/* From /usr/include/dev/usb/libusb.h */

/* Bits in the input/output/feature items */
#define HIO_CONST 0x001
#define HIO_VARIABLE 0x002
#define HIO_RELATIVE 0x004
#define HIO_WRAP  0x008
#define HIO_NONLINEAR   0x010
#define HIO_NOPREF   0x020
#define HIO_NULLSTATE   0x040
#define HIO_VOLATILE 0x080
#define HIO_BUFBYTES 0x100

#endif
