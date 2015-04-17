#ifndef _BASIC_H_
#define _BASIC_H_

#include <stdio.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

#define OK   (0)
#define FAIL (-1)
#define TRUE  (1)
#define FALSE (0)

////////////////////////////////////////////////////////////////////////////////

#define dtrace() do { struct timeval _now = {}; gettimeofday(&_now, NULL); fprintf(stdout, "(%3lds,%3ldms), tid = %ld, line %4d, %s()\n", _now.tv_sec % 1000, _now.tv_usec/1000, pthread_self(), __LINE__, __func__); } while(0)
#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)

#define check_if(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#endif //_BASIC_H_
