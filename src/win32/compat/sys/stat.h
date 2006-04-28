#ifndef HAVE_MINGW
#include "compat.h"
#else
#include_next <sys/stat.h>
#endif
