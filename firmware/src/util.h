#ifndef __UTIL_H__
#define __UTIL_H__

// likely / unlikely stolen from linux kernel
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif
