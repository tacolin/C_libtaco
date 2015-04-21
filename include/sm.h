#ifndef _SM_H_
#define _SM_H_

#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

enum
{
    SM_OK = 0,
    SM_ERROR = -1
};

////////////////////////////////////////////////////////////////////////////////

struct tTransition;
struct tState;
struct tEvent;

typedef struct tTransition
{

} tTransition;

typedef struct tEvent
{

} tEvent;

typedef struct tSm
{
    struct tState* state_table;
    tEvent* event_table;
    tTransition* trans_table;

    struct tState* curr_state;

} tSm;

typedef struct tState
{

    tSm sub_sm;

} tState;

#endif //_SM_H_
