#include <assert.h>

#include "fast_list.h"

static const int    POISON           = -2147483647;
static const int    CHANGE_SIGN      = -1;
static const size_t FICTIVE_ELEM_POS =  0;

static int* InitDataArray(const size_t capacity, ErrorInfo* error);
static int* InitNextArray(const size_t capacity, ErrorInfo* error);
static int* InitPrevArray(const size_t capacity, ErrorInfo* error);
static inline void InitListElem(list_t* list, const size_t pos, const int value,
                                              const size_t prev_pos, const size_t next_pos);

static inline void   AddFreeElemInList(list_t* list, const size_t pos);
static inline size_t GetFreeElemFromList(list_t* list, const size_t pos);

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error);

static inline void LogPrintArray(const int* array, size_t size, const char* name);

// ========= GRAPHS ==========

static const char* DOT_FILE = "list.dot";

static void DrawListGraph(list_t* list);

// ===========================

ListErrors ListCtor(list_t* list, ErrorInfo* error, size_t capacity)
{
    assert(list);

    int* data = InitDataArray(capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    int* next = InitNextArray(capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    int* prev = InitPrevArray(capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->data     = data;
    list->next     = next;
    list->prev     = prev;

    list->head     = 0;
    list->tail     = 0;
    list->free     = 1;

    list->capacity = capacity;
    list->size     = 0;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static int* InitDataArray(const size_t capacity, ErrorInfo* error)
{
    assert(error);

    int* data = (int*) calloc(capacity, sizeof(int));
    if (data == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "DATA ARRAY";
        return nullptr;
    }

    for (size_t i = 0; i < capacity; i++)
        data[i] = POISON;

    return data;
}

//-----------------------------------------------------------------------------------------------------

static int* InitNextArray(const size_t capacity, ErrorInfo* error)
{
    assert(error);

    int* next = (int*) calloc(capacity, sizeof(int));
    if (next == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "NEXT ARRAY";
        return nullptr;
    }

    //                              v-- we dont need to fill last element
    for (size_t i = 1; i < capacity - 1; i++)
        next[i] = CHANGE_SIGN * (i + 1);

    return next;
}

//-----------------------------------------------------------------------------------------------------

static int* InitPrevArray(const size_t capacity, ErrorInfo* error)
{
    assert(error);

    int* prev = (int*) calloc(capacity, sizeof(int));
    if (prev == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "PREV ARRAY";
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
    list->size     = 0;
}

//-----------------------------------------------------------------------------------------------------

ListErrors ListInsertAfterElem(list_t* list, const size_t pos, const int value,
                                        size_t* inserted_pos, ErrorInfo* error)
{
    assert(list);

    int free_pos  = GetFreeElemFromList(list, pos);
    *inserted_pos = free_pos;

    InitListElem(list, free_pos, value, pos, list->next[pos]);

    if (pos != list->tail)
        list->prev[list->next[free_pos]] = free_pos;
    else
    {
        list->tail                   = free_pos;
        list->prev[FICTIVE_ELEM_POS] = list->tail;
    }

    if (list->prev[free_pos] != 0)
        list->next[list->prev[free_pos]] = free_pos;
    else
    {
        list->head                   = free_pos;
        list->next[FICTIVE_ELEM_POS] = list->head;
    }

    list->size++;

    return ListErrors::NONE;
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

ListErrors ListRemoveElem(list_t* list, const size_t pos, ErrorInfo* error)
{
    assert(list);

    // add realloc

    CheckRemovingElement(list, pos, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

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
    list->size--;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error)
{
    if (list->tail == 0)
    {
        error->code = (int) ListErrors::EMPTY_LIST;
        return;
    }

    if (list->data[pos] == POISON)
    {
        error->code = (int) ListErrors::EMPTY_ELEMENT;
        error->data = list;
        return;
    }
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

    LOG_START_DUMP(func, file, line);

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

    DrawListGraph(list);

    LOG_END();

    return (int) ListErrors::NONE;
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

//-----------------------------------------------------------------------------------------------------

int PrintListError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    #pragma GCC diagnostic ignored "-Wcast-qual"
    struct ErrorInfo* error = (struct ErrorInfo*) err;
    #pragma GCC diagnostic warning "-Wcast-qual"

    switch ((ListErrors) error->code)
    {
        case (ListErrors::NONE):
            LOG_END();
            return (int) error->code;

        case (ListErrors::EMPTY_LIST):
            fprintf(fp, "CAN NOT REMOVE ELEMENT FROM EMPTY LIST\n");
            LOG_END();
            return (int) error->code;

        case (ListErrors::ALLOCATE_MEMORY):
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s FROM LIST\n", (char*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::EMPTY_ELEMENT):
            fprintf(fp, "CAN NOT REMOVE ALREADY EMPTY ELEMENT\n");
            DUMP_LIST((list_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::UNKNOWN):
        // fall through
        default:
            fprintf(fp, "UNKNOWN ERROR WITH LIST\n");
            LOG_END();
            return (int) ListErrors::UNKNOWN;
    }
}

//-----------------------------------------------------------------------------------------------------

static void DrawListGraph(list_t* list)
{
    assert(list);

    FILE* dotf = fopen(DOT_FILE, "w");

    fprintf(dotf, "digraph structs {\n"
	              "rankdir=LR;\n"
	              "node[color=\"black\",fontsize=14];\n"
	              "edge[color=\"darkblue\",fontcolor=\"yellow\",fontsize=12];\n");

    fprintf(dotf,   "head [shape=signature, label=\"HEAD\", fillcolor=\"yellow\", style=filled ];\n"
	                "tail [shape=signature, label=\"TAIL\", fillcolor=\"yellow\", style=filled ];\n"
	                "free [shape=signature, label=\"FREE\", fillcolor=\"yellow\", style=filled ];\n"
                    "head->%d[color = \"black\"];\n"
	                "tail->%d[color = \"black\"];\n"
	                "free->%d[color = \"black\"];\n",
                    list->head, list->tail, list->free);

    for (int i = 0; i < list->capacity; i++)
    {
        fprintf(dotf, "%d [shape=Mrecord, style=filled, ", i);
        if (list->prev[i] == -1)
            fprintf(dotf, "fillcolor=\"lightgreen\", color = darkgreen,");
        else if (list->data[i] == POISON)
            fprintf(dotf, "fillcolor=\"lightgray\", color = black,");
        else
            fprintf(dotf, "fillcolor=\"lightblue\", color = darkblue,");

        if (list->data[i] == POISON)
        {
            fprintf(dotf, " label=\" ip: %d | data: NaN| <n%d> next: %d| <p%d> prev: %d\" ];\n",
                            i, i, list->next[i], i, list->prev[i]);
        }
        else
        {
            fprintf(dotf, " label=\" ip: %d | data: %d| <n%d> next: %d| <p%d> prev: %d\" ];\n",
                            i, list->data[i], i, list->next[i], i, list->prev[i]);
        }
    }

    fprintf(dotf, "0");
    for (int i = 1; i < list->capacity; i++)
    {
        fprintf(dotf, "->%d", i);
    }
    fprintf(dotf, "[weight = 993, color = \"white\"];\n");

    for (int i = 0; i < list->capacity; i++)
    {
        if (list->prev[i] != -1)
            fprintf(dotf, "%d -> %d [color = \"red\"];\n", i, list->prev[i]);

        int next = list->next[i];
        if (next < 0)
            next *= CHANGE_SIGN;

        fprintf(dotf, "%d -> %d [color = \"green\"];\n", i, next);
    }

    fprintf(dotf, "}");

    fclose(dotf);
}
