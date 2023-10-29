#include <assert.h>

#include "fast_list.h"

static const int POISON      = -2147483647;
static const int CHANGE_SIGN = -1;

static int* InitDataArray(const size_t capacity, ERRORS* error);
static int* InitNextArray(const size_t capacity, ERRORS* error);
static int* InitPrevArray(const size_t capacity, ERRORS* error);

static inline void AddFreeElemInList(list_t* list, const size_t pos);

static inline void LogPrintArray(const int* array, size_t size, const char* name);

ERRORS ListCtor(list_t* list, size_t capacity)
{
    assert(list);

    ERRORS error = ERRORS::NONE;

    int* data = InitDataArray(capacity, &error);
    RETURN_IF_ERROR(error);

    int* next = InitNextArray(capacity, &error);
    RETURN_IF_ERROR(error);

    int* prev = InitPrevArray(capacity, &error);
    RETURN_IF_ERROR(error);

    list->data     = data;
    list->next     = next;
    list->prev     = prev;
    list->head     = 0;
    list->tail     = 0;
    list->free     = 1;
    list->capacity = capacity;

    return error;
}

//-----------------------------------------------------------------------------------------------------

static int* InitDataArray(const size_t capacity, ERRORS* error)
{
    assert(error);

    int* data = (int*) calloc(capacity, sizeof(int));
    if (data == nullptr)
    {
        *error = ERRORS::ALLOCATE_MEMORY;
        return nullptr;
    }

    for (size_t i = 0; i < capacity; i++)
        data[i] = POISON;

    return data;
}

//-----------------------------------------------------------------------------------------------------

static int* InitNextArray(const size_t capacity, ERRORS* error)
{
    assert(error);

    int* next = (int*) calloc(capacity, sizeof(int));
    if (next == nullptr)
    {
        *error = ERRORS::ALLOCATE_MEMORY;
        return nullptr;
    }

    next[0] = 0;
    //                              v-- we dont need to fill last element
    for (size_t i = 1; i < capacity - 1; i++)
        next[i] = CHANGE_SIGN * (i + 1);

    next[capacity - 1] = 0;

    return next;
}

//-----------------------------------------------------------------------------------------------------

static int* InitPrevArray(const size_t capacity, ERRORS* error)
{
    assert(error);

    int* prev = (int*) calloc(capacity, sizeof(int));
    if (prev == nullptr)
    {
        *error = ERRORS::ALLOCATE_MEMORY;
        return nullptr;
    }

    prev[0] = 0;

    for (size_t i = 1; i < capacity; i++)
        prev[i] = -1;

    return prev;
}

//-----------------------------------------------------------------------------------------------------

void ListDtor(list_t* list)
{
    assert(list);

    free(list->data);
    free(list->next);
    free(list->prev);

    list->head     = POISON;
    list->tail     = POISON;
    list->free     = POISON;
    list->capacity = 0;
}

//-----------------------------------------------------------------------------------------------------

ERRORS ListInsertElem(list_t* list, const size_t pos, const int value)
{
    assert(list);

    int free_pos = list->free;

    if (pos == list->tail)
    {
        int prev_pos = pos;
        int next_pos = 0;
    }
    else
    {
        int prev_pos = pos;
        int next_pos = list->next[pos];
    }


}

//-----------------------------------------------------------------------------------------------------

ERRORS ListRemoveElem(list_t* list, const size_t pos)
{
    assert(list);

    // add realloc

    if (list->head == list->tail)
        return ERRORS::EMPTY_LIST;

    if (list->tail == pos)
    {
        list->tail = list->prev[pos];
        list->next[list->tail] = 0;
    }
    else if (list->head == pos)
    {
        list->head = list->next[pos];
        list->prev[list->head] = 0;
    }
    else
    {
        list->next[list->prev[pos]] = list->next[pos];
        list->prev[list->next[pos]] = list->prev[pos];
    }

    AddFreeElemInList(list, pos);

}

//-----------------------------------------------------------------------------------------------------

static inline void AddFreeElemInList(list_t* list, const size_t pos)
{
    assert(list);

    list->data[pos] = POISON;
    list->prev[pos] = -1;
    list->next[pos] = CHANGE_SIGN * list->free;

    list->free = pos;
}

//-----------------------------------------------------------------------------------------------------

int ListDump(FILE* fp, const void* fast_list, const char* func, const char* file, const int line)
{
    assert(fast_list);

    LOG_START_MOD(func, file, line);

    #pragma GCC diagnostic ignored "-Wcast-qual"
    list_t* list = (list_t*) fast_list;
    #pragma GCC diagnostic warning "-Wcast-qual"

    size_t capacity = list->capacity;

    PrintLog("%9s", "");
    for (size_t i = 0; i < capacity; i++)
        PrintLog(" %3d  ", i);
    PrintLog("\n");

    LogPrintArray(list->data, capacity, "DATA");
    LogPrintArray(list->next, capacity, "NEXT");
    LogPrintArray(list->prev, capacity, "PREV");

    PrintLog("HEAD     > %d\n"
             "TAIL     > %d\n"
             "FREE     > %d\n"
             "CAPACITY > %d\n", list->head, list->tail, list->free, list->capacity);

    LOG_END();

    return (int) ERRORS::NONE;
}

//-----------------------------------------------------------------------------------------------------

static inline void LogPrintArray(const int* array, size_t size, const char* name)
{
    assert(array);
    assert(name);

    PrintLog("%7s: ", name);

    for (size_t i = 0; i < size; i++)
    {
        if (array[i] != POISON)
            PrintLog("[%3d] ", array[i]);
        else
            PrintLog("[NaN] ");
    }

    PrintLog("\n");
}
