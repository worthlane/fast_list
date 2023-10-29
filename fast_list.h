#ifndef __FAST_LIST_H_
#define __FAST_LIST_H_

#include "errors.h"

struct List
{
    int* data;
    int* next;
    int* prev;
    int head;
    int tail;
    int free;
    size_t capacity;
};

typedef struct List list_t;

static const size_t DEFAULT_LIST_CAPACITY = 16;

ERRORS ListCtor(list_t* list, size_t capacity = DEFAULT_LIST_CAPACITY);
void   ListDtor(list_t* list);
ERRORS ListInsertElem(list_t* list, const size_t pos, const int value);
ERRORS ListRemoveElem(list_t* list, const size_t pos);
int    ListDump(FILE* fp, const void* list, const char* func, const char* file, const int line);

#ifdef DUMP_LIST
#undef DUMP_LIST

#endif
#define DUMP_LIST(list)     do                                                              \
                            {                                                               \
                                LogDump(ListDump, (list), __func__, __FILE__, __LINE__);    \
                            } while(0)

#endif
