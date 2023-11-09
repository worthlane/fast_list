#ifndef __FAST_LIST_H_
#define __FAST_LIST_H_

#include "errors.h"

struct ListElem
{
    int data;
    int next;
    int prev;
};

struct List
{

    ListElem* elems;

    int free;

    size_t capacity;
    size_t size;
};

enum class ListErrors
{
    NONE = 0,

    ALLOCATE_MEMORY,
    EMPTY_LIST,
    EMPTY_ELEMENT,
    INVALID_SIZE,
    DAMAGED_FICTIVE,

    UNKNOWN
};

int PrintListError(FILE* fp, const void* err, const char* func, const char* file, const int line);

#ifdef EXIT_IF_LISTERROR
#undef EXIT_IF_LISTERROR
#endif
#define EXIT_IF_LISTERROR(error)            do                                                          \
                                            {                                                           \
                                                if ((error)->code != (int) ListErrors::NONE)            \
                                                {                                                       \
                                                    return LogDump(PrintListError, error, __func__,     \
                                                                    __FILE__, __LINE__);                \
                                                }                                                       \
                                            } while(0)
#ifdef RETURN_IF_LISTERROR
#undef RETURN_IF_LISTERROR
#endif
#define RETURN_IF_LISTERROR(error)          do                                                          \
                                            {                                                           \
                                                if ((error) != ListErrors::NONE)                        \
                                                {                                                       \
                                                    return error;                                       \
                                                }                                                       \
                                            } while(0)


typedef struct List list_t;

static const size_t DEFAULT_LIST_CAPACITY = 16;
ListErrors MakeListShorter(list_t* list, const size_t new_capacity, ErrorInfo* error);

ListErrors ListCtor(list_t* list, ErrorInfo* error, size_t capacity = DEFAULT_LIST_CAPACITY);
void       ListDtor(list_t* list);

ListErrors GetListElement(const list_t* list, const size_t pos, int* destination, ErrorInfo* error);
int        GetListHead(const list_t* list);
int        GetListTail(const list_t* list);

ListErrors ListInsertAfterElem(list_t* list, const size_t pos, const int value,
                               size_t* inserted_pos, ErrorInfo* error);
ListErrors ListInsertBeforeElem(list_t* list, const size_t pos, const int value,
                                size_t* inserted_pos, ErrorInfo* error);
ListErrors ListRemoveElem(list_t* list, const size_t pos, ErrorInfo* error);
int        ListDump(FILE* fp, const void* list, const char* func, const char* file, const int line);
ListErrors ListVerify(const list_t* list);

#ifdef DUMP_LIST
#undef DUMP_LIST
#endif
#define DUMP_LIST(list)     do                                                              \
                            {                                                               \
                                LogDump(ListDump, (list), __func__, __FILE__, __LINE__);    \
                            } while(0)

#endif

