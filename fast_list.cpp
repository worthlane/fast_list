#include <assert.h>
#include <time.h>

#include "fast_list.h"
#include "graphs.h"

static const char*  DOT_FILE         = "tmp.dot";
static const int    POISON           = -2147483647;
static const int    CHANGE_SIGN      = -1;
static const size_t FICTIVE_ELEM_POS =  0;
static const int    CPTY_MULTIPLIER  =  2;

static int* InitDataArray(const size_t capacity, ErrorInfo* error);
static int* InitNextArray(const size_t capacity, ErrorInfo* error);
static int* InitPrevArray(const size_t capacity, ErrorInfo* error);
static inline void InitListElem(list_t* list, const size_t pos, const int value,
                                              const size_t prev_pos, const size_t next_pos);

static inline void   AddFreeElemInList(list_t* list, const size_t pos);
static inline size_t GetFreeElemFromList(list_t* list);

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error);

static inline int GetListHead(const list_t* list);
static inline int GetListTail(const list_t* list);

static inline void FreeListArrays(list_t* list);

// ================== REALLOC FUNCS ===================

static ListErrors MakeListLonger(list_t* list, ErrorInfo* error);
static ListErrors MakeListShorter(list_t* list, ErrorInfo* error);

static int GetShorterCapacity(const size_t old_capacity);

static void FillShorterList(const list_t* old_list, int* new_data, int* new_next, int* new_prev);

static inline void InitFictiveInSortedList(int* new_data, int* new_next, int* new_prev, const size_t size);
static inline void InitMiddleElemsInSortedList(const list_t* old_list,
                                               int* new_data, int* new_next, int* new_prev,
                                               const size_t size, size_t* curr_pos);
static inline void InitTailInSortedList(int* new_data, int* new_next, int* new_prev,
                                        const size_t size, const int value);

static int* ReallocDataArray(const size_t new_capacity, list_t* list, ErrorInfo* error);
static int* ReallocNextArray(const size_t new_capacity, list_t* list, ErrorInfo* error);
static int* ReallocPrevArray(const size_t new_capacity, list_t* list, ErrorInfo* error);

// =====================================================

// ========= TEXT DUMP =======

static void TextListDump(FILE* fp, const list_t* list);

static inline void PrintListInfo(FILE* fp, const list_t* list);
static inline void PrintListElements(FILE* fp, const list_t* list);
static inline void ChooseElementHtmlColor(FILE* fp, const list_t* list, const size_t pos);

// ===========================

// ========= GRAPHS ==========

static void DrawListGraph(list_t* list);

static inline void DrawListInfo(FILE* dotf, const list_t* list);
static inline void DrawListElements(FILE* dotf, const list_t* list);
static inline void CenterListElements(FILE* dotf, const list_t* list);
static inline void DrawListArrows(FILE* dotf, const list_t* list);
static inline void ChooseVertexColor(FILE* dotf, const list_t* list, const size_t pos);
static inline void MarkImportantElements(FILE* dotf, const list_t* list, const size_t pos);

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

    //                           v-- we dont need to fill last element
    for (int i = 1; i < capacity - 1; i++)
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

    for (int i = 1; i < capacity; i++)
        prev[i] = -1;

    return prev;
}

//-----------------------------------------------------------------------------------------------------

void ListDtor(list_t* list)
{
    assert(list);

    FreeListArrays(list);

    list->free     = POISON;

    list->capacity = 0;
    list->size     = 0;
}

//-----------------------------------------------------------------------------------------------------

static inline void FreeListArrays(list_t* list)
{
    assert(list);

    free(list->data);
    free(list->next);
    free(list->prev);
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

ListErrors ListVerify(const list_t* list)
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
    assert(error);

    CHECK_LIST(list);

    if (list->free == FICTIVE_ELEM_POS)
    {
        MakeListLonger(list, error);
        RETURN_IF_LISTERROR((ListErrors) error->code);
    }

    size_t free_pos = GetFreeElemFromList(list);
    *inserted_pos   = free_pos;

    InitListElem(list, free_pos, value, pos, list->next[pos]);

    list->prev[list->next[free_pos]] = free_pos;
    list->next[list->prev[free_pos]] = free_pos;

    list->size++;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

ListErrors ListInsertBeforeElem(list_t* list, const size_t pos, const int value,
                                              size_t* inserted_pos, ErrorInfo* error)
{
    assert(list);
    assert(error);

    CHECK_LIST(list);

    if (list->free == FICTIVE_ELEM_POS)
    {
        MakeListLonger(list, error);
        RETURN_IF_LISTERROR((ListErrors) error->code);
    }

    size_t free_pos = GetFreeElemFromList(list);
    *inserted_pos   = free_pos;

    InitListElem(list, free_pos, value, list->prev[pos], pos);

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

    list->free     = list->capacity;
    list->capacity = new_capacity;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static ListErrors MakeListShorter(list_t* list, ErrorInfo* error)
{
    assert(list);
    assert(error);

    int new_capacity = GetShorterCapacity(list->capacity);

    int* new_data = InitDataArray(new_capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    int* new_next = InitNextArray(new_capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    int* new_prev = InitPrevArray(new_capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    FillShorterList(list, new_data, new_next, new_prev);

    FreeListArrays(list);

    list->capacity      = new_capacity;
    list->data          = new_data;
    list->next          = new_next;
    list->prev          = new_prev;
    list->free          = list->size + 1;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static int GetShorterCapacity(const size_t old_capacity)
{
    int new_capacity = old_capacity / CPTY_MULTIPLIER;
    if (new_capacity < DEFAULT_LIST_CAPACITY)
        new_capacity = DEFAULT_LIST_CAPACITY;

    return new_capacity;
}

//-----------------------------------------------------------------------------------------------------

static void FillShorterList(const list_t* old_list, int* new_data, int* new_next, int* new_prev)
{
    assert(old_list);
    assert(new_data);
    assert(new_next);
    assert(new_prev);

    size_t curr_pos = old_list->next[FICTIVE_ELEM_POS];
    size_t size     = old_list->size;

    InitFictiveInSortedList(new_data, new_next, new_prev, size);

    InitMiddleElemsInSortedList(old_list, new_data, new_next, new_prev, size, &curr_pos);

    InitTailInSortedList(new_data, new_next, new_prev, size, old_list->data[curr_pos]);
}

//-----------------------------------------------------------------------------------------------------

static inline void InitFictiveInSortedList(int* new_data, int* new_next, int* new_prev, const size_t size)
{
    assert(new_data);
    assert(new_next);
    assert(new_prev);

    new_data[FICTIVE_ELEM_POS] = POISON;
    new_prev[FICTIVE_ELEM_POS] = size;
    new_next[FICTIVE_ELEM_POS] = 1;

    new_prev[new_next[FICTIVE_ELEM_POS]] = FICTIVE_ELEM_POS;
    new_next[new_prev[FICTIVE_ELEM_POS]] = FICTIVE_ELEM_POS;
}

//-----------------------------------------------------------------------------------------------------

static inline void InitTailInSortedList(int* new_data, int* new_next, int* new_prev,
                                        const size_t size, const int value)
{
    assert(new_data);
    assert(new_next);
    assert(new_prev);

    new_data[size] = value;
    new_prev[size] = size - 1;
    new_next[size] = FICTIVE_ELEM_POS;

    new_prev[new_next[size]] = size;
    new_next[new_prev[size]] = size;
}

//-----------------------------------------------------------------------------------------------------

static inline void InitMiddleElemsInSortedList(const list_t* old_list,
                                               int* new_data, int* new_next, int* new_prev,
                                               const size_t size, size_t* curr_pos)
{
    assert(old_list);
    assert(curr_pos);
    assert(new_data);
    assert(new_next);
    assert(new_prev);

    for (int i = 1; i < size; i++)
    {
        new_data[i] = old_list->data[*curr_pos];
        new_prev[i] = i - 1;
        new_next[i] = i + 1;

        new_prev[new_next[i]] = i;
        new_next[new_prev[i]] = i;

        *curr_pos = old_list->next[*curr_pos];
    }
}

//-----------------------------------------------------------------------------------------------------

static int* ReallocDataArray(const size_t new_capacity, list_t* list, ErrorInfo* error)
{
    assert(error);

    int* temp_data = (int*) realloc(list->data, new_capacity * sizeof(int));
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

    int* temp_next = (int*) realloc(list->next, new_capacity * sizeof(int));
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

    int* temp_prev = (int*) realloc(list->prev, new_capacity * sizeof(int));
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

static inline size_t GetFreeElemFromList(list_t* list)
{
    assert(list);

    int free_pos = list->free;
    list->free   = CHANGE_SIGN * list->next[free_pos];

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

    CheckRemovingElement(list, pos, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->next[list->prev[pos]] = list->next[pos];
    list->prev[list->next[pos]] = list->prev[pos];

    AddFreeElemInList(list, pos);
    list->size--;

    if (list->size <= list->capacity / (2 * CPTY_MULTIPLIER))
    {
        MakeListShorter(list, error);
        RETURN_IF_LISTERROR((ListErrors) error->code);
    }

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

    TextListDump(fp, list);

    DrawListGraph(list);

    LOG_END();

    return (int) ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

int PrintListError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    #pragma GCC diagnostic ignored "-Wcast-qual"
    struct ErrorInfo* error = (struct ErrorInfo*) err;

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
    #pragma GCC diagnostic warning "-Wcast-qual"
}

//====================================================================================================

static void TextListDump(FILE* fp, const list_t* list)
{
    fprintf(fp, "<b>DUMPING LIST</b><br>\n"
                "FORMAT: IP -> [DATA, NEXT, PREV]<br>\n");

    PrintListElements(fp, list);
    PrintListInfo(fp, list);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void PrintListInfo(FILE* fp, const list_t* list)
{
    fprintf(fp, "HEAD     > %d<br>\n"
                "TAIL     > %d<br>\n"
                "FREE     > %d<br>\n"
                "CAPACITY > %lu<br>\n", GetListHead(list), GetListTail(list), list->free, list->capacity);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void PrintListElements(FILE* fp, const list_t* list)
{
    for (int i = 0; i < list->capacity; i++)
    {
        ChooseElementHtmlColor(fp, list, i);

        if (list->data[i] != POISON)
            fprintf(fp, "%3d -> [%d, %d, %d]</b></font><br>\n", i, list->data[i], list->next[i], list->prev[i]);
        else
            fprintf(fp, "%3d -> [NaN, %d, %d]</b></font><br>\n", i, list->next[i], list->prev[i]);
    }
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void ChooseElementHtmlColor(FILE* fp, const list_t* list, const size_t pos)
{
    if (list->prev[pos] == -1)
        fprintf(fp, "<font color=\"#008000\"><b>");
    else if (list->data[pos] == POISON)
        fprintf(fp, "<font color=\"#474747\"><b>");
    else
        fprintf(fp, "<font color=\"#0000FF\"><b>");
}

//====================================================================================================

static void DrawListGraph(list_t* list)
{
    assert(list);

    FILE* dotf = fopen(DOT_FILE, "w");

    StartGraph(dotf);

    DrawListInfo(dotf, list);
    DrawListElements(dotf, list);
    CenterListElements(dotf, list);
    DrawListArrows(dotf, list);

    EndGraph(dotf);

    fclose(dotf);

    MakeImg(DOT_FILE);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawListInfo(FILE* dotf, const list_t* list)
{
    assert(list);

    fprintf(dotf, "info [shape=record, style=filled, fillcolor=\"yellow\","
                  "label=\"HEAD: %d | TAIL: %d | FREE: %d | SIZE: %lu | CAPACITY: %lu\","
                  "fontcolor = \"black\", fontsize = 25];\n",
                              GetListHead(list), GetListTail(list), list->free, list->size, list->capacity);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void ChooseVertexColor(FILE* dotf, const list_t* list, const size_t pos)
{
    assert(list);

    if (list->prev[pos] == -1)
        fprintf(dotf, "fillcolor=\"lightgreen\", color = darkgreen,");
    else if (list->data[pos] == POISON)
        fprintf(dotf, "fillcolor=\"lightgray\", color = black,");
    else
        fprintf(dotf, "fillcolor=\"lightblue\", color = darkblue,");
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void MarkImportantElements(FILE* dotf, const list_t* list, const size_t pos)
{
    assert(list);

    if (pos == list->free)
        fprintf(dotf, "FREE | ");

    if (pos == GetListHead(list))
        fprintf(dotf, "HEAD | ");

    if (pos == GetListTail(list))
        fprintf(dotf, "TAIL | ");
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawListElements(FILE* dotf, const list_t* list)
{
    assert(list);

    for (int i = 0; i < list->capacity; i++)
    {
        fprintf(dotf, "%d [shape=Mrecord, style=filled, ", i);

        ChooseVertexColor(dotf, list, i);

        fprintf(dotf, " label=\" ");

        MarkImportantElements(dotf, list, i);

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

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

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

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawListArrows(FILE* dotf, const list_t* list)
{
    assert(list);

    for (int i = 0; i < list->capacity; i++)
    {
        if (list->prev[i] != -1)
            fprintf(dotf, "%d -> %d [color = \"red\"];\n", i, list->prev[i]);

        int next = list->next[i];
        if (next < 0 || (next == 0 && list->prev[next] != i))
        {
            next *= CHANGE_SIGN;
            fprintf(dotf, "%d -> %d [color = \"green\"];\n", i, next);
        }
        else
            fprintf(dotf, "%d -> %d [color = \"blue\"];\n", i, next);
    }
}
