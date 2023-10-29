#include <assert.h>

#include "fast_list.h"

static const int POISON      = -2147483647;
static const int CHANGE_SIGN = -1;

static int* InitDataArray(const size_t capacity, ERRORS* error);
static int* InitNextArray(const size_t capacity, ERRORS* error);
static int* InitPrevArray(const size_t capacity, ERRORS* error);
static inline void InitListElem(list_t* list, const size_t pos, const int value,
                                              const size_t prev_pos, const size_t next_pos);

static inline void   AddFreeElemInList(list_t* list, const size_t pos);
static inline size_t GetFreeElemFromList(list_t* list, const size_t pos);

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

    //                              v-- we dont need to fill last element
    for (size_t i = 1; i < capacity - 1; i++)
        next[i] = CHANGE_SIGN * (i + 1);

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

ERRORS ListInsertElem(list_t* list, const size_t pos, const int value, size_t* inserted_pos)
{
    assert(list);

    ERRORS error = ERRORS::NONE;

    int free_pos  = GetFreeElemFromList(list, pos);
    *inserted_pos = free_pos;

    InitListElem(list, free_pos, value, pos, list->next[pos]);

    if (pos != list->tail)
        list->prev[list->next[free_pos]] = free_pos;
    else
        list->tail = free_pos;

    if (list->prev[free_pos] != 0)
        list->next[list->prev[free_pos]] = free_pos;
    else
        list->head = free_pos;

    return error;
}

//-----------------------------------------------------------------------------------------------------

static inline size_t GetFreeElemFromList(list_t* list, const size_t pos)
{
    assert(list);

    int free_pos  = list->free;
    list->free    = CHANGE_SIGN * list->next[free_pos];

    return free_pos;
}

//-----------------------------------------------------------------------------------------------------

static inline void InitListElem(list_t* list, const size_t pos, const int value,
                                              const size_t prev_pos, const size_t next_pos)
{
    assert(list);

    list->data[pos] = value;
    list->prev[pos] = prev_pos;
    list->next[pos] = next_pos;
}

//-----------------------------------------------------------------------------------------------------

ERRORS ListRemoveElem(list_t* list, const size_t pos)
{
    assert(list);

    ERRORS error = ERRORS::NONE;

    // add realloc

    if (list->tail == 0)
        return ERRORS::EMPTY_LIST;

    if (list->tail != pos)
        list->next[list->prev[pos]] = list->next[pos];
    else
    {
        list->tail = list->prev[pos];
        list->next[list->tail] = 0;
    }

    if (list->head != pos)
        list->prev[list->next[pos]] = list->prev[pos];
    else
    {
        list->head = list->next[pos];
        list->prev[list->head] = 0;
    }

    AddFreeElemInList(list, pos);

    return error;
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
