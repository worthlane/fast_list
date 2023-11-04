#include <assert.h>
#include <time.h>

#include "fast_list.h"

static const int    POISON           = -2147483647;
static const int    CHANGE_SIGN      = -1;
static const size_t FICTIVE_ELEM_POS =  0;
static const int    CPTY_MULTIPLIER  =  2;
static const size_t MAX_DOT_CMD_LEN  =  200;

static int* InitDataArray(const size_t capacity, ErrorInfo* error);
static int* InitNextArray(const size_t capacity, ErrorInfo* error);
static int* InitPrevArray(const size_t capacity, ErrorInfo* error);
static inline void InitListElem(list_t* list, const size_t pos, const int value,
                                              const size_t prev_pos, const size_t next_pos);

static inline void   AddFreeElemInList(list_t* list, const size_t pos);
static inline size_t GetFreeElemFromList(list_t* list, const size_t pos);

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error);

static ListErrors MakeListLonger(list_t* list, ErrorInfo* error);
static int* ReallocDataArray(const size_t new_capacity, list_t* list, ErrorInfo* error);
static int* ReallocNextArray(const size_t new_capacity, list_t* list, ErrorInfo* error);
static int* ReallocPrevArray(const size_t new_capacity, list_t* list, ErrorInfo* error);

static inline void LogPrintArray(const int* array, size_t size, const char* name);

static inline int GetListHead(const list_t* list);
static inline int GetListTail(const list_t* list);

// ========= GRAPHS ==========

static const char* DOT_FILE = "list.dot";

static void DrawListGraph(list_t* list);

static inline void StartGraph(FILE* dotf, const list_t* list);

static inline void DrawListInfo(FILE* dotf, const list_t* list);
static inline void DrawListElements(FILE* dotf, const list_t* list);
static inline void CenterListElements(FILE* dotf, const list_t* list);
static inline void DrawListArrows(FILE* dotf, const list_t* list);

static inline void EndGraph(FILE* dotf, const list_t* list);

static size_t IMG_CNT = 1;

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

static inline int GetListHead(const list_t* list)
{
    return list->next[FICTIVE_ELEM_POS];
}

//-----------------------------------------------------------------------------------------------------

static inline int GetListTail(const list_t* list)
{
    return list->prev[FICTIVE_ELEM_POS];
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

    list->free     = POISON;

    list->capacity = 0;
    list->size     = 0;
}

//-----------------------------------------------------------------------------------------------------

#ifdef CHECK_LIST
#undef CHECK_LIST

#endif
#define CHECK_LIST(list)    do                                                              \
                            {                                                               \
                                ListErrors list_err_ = ListVerify(list);                    \
                                if (list_err_ != ListErrors::NONE)                          \
                                    return list_err_;                                       \
                            } while(0)

ListErrors ListVerify(list_t* list)
{
    assert(list);

    if (list->size > list->capacity)            return ListErrors::INVALID_SIZE;
    if (list->data[0] != POISON)                return ListErrors::DAMAGED_FICTIVE;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

ListErrors ListInsertAfterElem(list_t* list, const size_t pos, const int value,
                                        size_t* inserted_pos, ErrorInfo* error)
{
    assert(list);

    CHECK_LIST(list);

    if (list->size == list->capacity)
    {
        MakeListLonger(list, error);
        RETURN_IF_LISTERROR((ListErrors) error->code);
    }

    int free_pos  = GetFreeElemFromList(list, pos);
    *inserted_pos = free_pos;

    InitListElem(list, free_pos, value, pos, list->next[pos]);

    list->prev[list->next[free_pos]] = free_pos;
    list->next[list->prev[free_pos]] = free_pos;

    list->size++;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static ListErrors MakeListLonger(list_t* list, ErrorInfo* error)
{
    assert(list);
    assert(error);

    int new_capacity = list->capacity * CPTY_MULTIPLIER;

    ReallocDataArray(new_capacity, list, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    ReallocNextArray(new_capacity, list, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    ReallocPrevArray(new_capacity, list, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->capacity = new_capacity;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static int* ReallocDataArray(const size_t new_capacity, list_t* list, ErrorInfo* error)
{
    assert(error);

    int* temp_data = (int*) realloc(list->data, new_capacity);
    if (temp_data == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "DATA ARRAY";
        return nullptr;
    }

    for (size_t i = list->capacity; i < new_capacity; i++)
        temp_data[i] = POISON;

    list->data = temp_data;

    return temp_data;
}

//-----------------------------------------------------------------------------------------------------

static int* ReallocNextArray(const size_t new_capacity, list_t* list, ErrorInfo* error)
{
    assert(error);

    int* temp_next = (int*) realloc(list->next, new_capacity);
    if (temp_next == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "NEXT ARRAY";
        return nullptr;
    }

    //                                               v-- we dont need to fill last element
    for (size_t i = list->capacity; i < new_capacity - 1; i++)
        temp_next[i] = CHANGE_SIGN * (i + 1);

    list->next = temp_next;

    return temp_next;
}

//-----------------------------------------------------------------------------------------------------

static int* ReallocPrevArray(const size_t new_capacity, list_t* list, ErrorInfo* error)
{
    assert(error);

    int* temp_prev = (int*) realloc(list->prev, new_capacity);
    if (temp_prev == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "PREV ARRAY";
        return nullptr;
    }

    for (size_t i = list->capacity; i < new_capacity; i++)
        temp_prev[i] = -1;

    list->prev = temp_prev;

    return temp_prev;
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

    list->next[list->prev[pos]] = list->next[pos];
    list->prev[list->next[pos]] = list->prev[pos];

    AddFreeElemInList(list, pos);
    list->size--;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error)
{
    if (list->size == 0)
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
    PrintLog("<br>\n");

    LogPrintArray(list->data, capacity, "DATA");
    LogPrintArray(list->next, capacity, "NEXT");
    LogPrintArray(list->prev, capacity, "PREV");

    PrintLog("HEAD     > %d<br>\n"
             "TAIL     > %d<br>\n"
             "FREE     > %d<br>\n"
             "CAPACITY > %d<br>\n", GetListHead(list), GetListTail(list), list->free, list->capacity);

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

    PrintLog("<br>\n");
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
            fprintf(fp, "CAN NOT REMOVE ELEMENT FROM EMPTY LIST<br>\n");
            LOG_END();
            return (int) error->code;

        case (ListErrors::ALLOCATE_MEMORY):
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s FROM LIST<br>\n", (char*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::EMPTY_ELEMENT):
            fprintf(fp, "CAN NOT REMOVE ALREADY EMPTY ELEMENT<br>\n");
            DUMP_LIST((list_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::UNKNOWN):
        // fall through
        default:
            fprintf(fp, "UNKNOWN ERROR WITH LIST<br>\n");
            LOG_END();
            return (int) ListErrors::UNKNOWN;
    }
}

//-----------------------------------------------------------------------------------------------------

static void DrawListGraph(list_t* list)
{
    assert(list);

    FILE* dotf = fopen(DOT_FILE, "w");

    StartGraph(dotf, list);

    DrawListInfo(dotf, list);
    DrawListElements(dotf, list);
    CenterListElements(dotf, list);
    DrawListArrows(dotf, list);

    EndGraph(dotf, list);

    fclose(dotf);

    char img_name[MAX_FILE_NAME_LEN] = {};
    snprintf(img_name, MAX_FILE_NAME_LEN, "img/img%d_%d.png", IMG_CNT++, clock());

    char dot_command[MAX_DOT_CMD_LEN] = {};
    snprintf(dot_command, MAX_DOT_CMD_LEN, "dot %s -T png -o %s", DOT_FILE, img_name);

    system(dot_command);

    PrintLog("<img src=\"%s\">", img_name);
}

//:::::::::::::::::::::::::::::::::::::

static inline void EndGraph(FILE* dotf, const list_t* list)
{
    assert(list);

    fprintf(dotf, "}");
}

//:::::::::::::::::::::::::::::::::::::

static inline void StartGraph(FILE* dotf, const list_t* list)
{
    assert(list);

    fprintf(dotf, "digraph structs {\n"
	              "rankdir=LR;\n"
	              "node[color=\"black\",fontsize=14];\n"
	              "edge[color=\"darkblue\",fontcolor=\"yellow\",fontsize=12];\n");
}

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListInfo(FILE* dotf, const list_t* list)
{
    assert(list);

    fprintf(dotf, "info [shape=record, style=filled, fillcolor=\"yellow\","
                  "label=\"HEAD: %d | TAIL: %d | FREE: %d | SIZE: %d | CAPACITY: %d\","
                  "fontcolor = \"black\", fontsize = 25];\n",
                              GetListHead(list), GetListTail(list), list->free, list->size, list->capacity);
}

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListElements(FILE* dotf, const list_t* list)
{
    assert(list);

    for (int i = 0; i < list->capacity; i++)
    {
        fprintf(dotf, "%d [shape=Mrecord, style=filled, ", i);
        if (list->prev[i] == -1)
            fprintf(dotf, "fillcolor=\"lightgreen\", color = darkgreen,");
        else if (list->data[i] == POISON)
            fprintf(dotf, "fillcolor=\"lightgray\", color = black,");
        else
            fprintf(dotf, "fillcolor=\"lightblue\", color = darkblue,");

        fprintf(dotf, " label=\" ");

        if (i == list->free)
            fprintf(dotf, "FREE | ");

        if (i == GetListHead(list))
            fprintf(dotf, "HEAD | ");

        if (i == GetListTail(list))
            fprintf(dotf, "TAIL | ");

        if (list->data[i] == POISON)
        {
            fprintf(dotf, "ip: %d | data: NaN| next: %d| prev: %d\" ];\n",
                            i, list->next[i], list->prev[i]);
        }
        else
        {
            fprintf(dotf, "ip: %d | data: %d| next: %d| prev: %d\" ];\n",
                            i, list->data[i], list->next[i], list->prev[i]);
        }
    }
}

//:::::::::::::::::::::::::::::::::::::

static inline void CenterListElements(FILE* dotf, const list_t* list)
{
    assert(list);

    fprintf(dotf, "0");
    for (int i = 1; i < list->capacity; i++)
    {
        fprintf(dotf, "->%d", i);
    }
    fprintf(dotf, "[weight = 993, color = \"white\"];\n");
}

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListArrows(FILE* dotf, const list_t* list)
{
    assert(list);

    for (int i = 0; i < list->capacity; i++)
    {
        if (list->prev[i] != -1)
            fprintf(dotf, "%d -> %d [color = \"red\"];\n", i, list->prev[i]);

        int next = list->next[i];
        if (next < 0)
            next *= CHANGE_SIGN;

        fprintf(dotf, "%d -> %d [color = \"green\"];\n", i, next);
    }
}
