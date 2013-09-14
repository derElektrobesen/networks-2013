#ifndef MACRO_H
#define MACRO_H

#include "config.h"

#ifdef DEBUG
#   define log(...) \
    printf(__VA_ARGS__)
#else
#   define log(...)
#endif

#endif
