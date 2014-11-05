#ifndef __IOCTL_COMPAT_H
#define __IOCTL_COMPAT_H

#define TIOCMBIC  1
#define TIOCMBIS  2

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int, int, ...);

#ifdef __cplusplus
};
#endif

#endif // __IOCTL_COMPAT_H
