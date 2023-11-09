#include <assert.h>
#include <time.h>

#include "fast_list.h"
#include "graphs.h"

static const char*  DOT_FILE             = "tmp.dot";
static const int    POISON               = -2147483647;
static const int    CHANGE_SIGN          = -1;
static const size_t FICTIVE_ELEM_POS     =  0;
static const int    CAPACITY_MULTIPLIER  =  2;

static ListElem*   InitListElemsArray(const size_t capacity, ErrorInfo* error);
static inline void InitListElem(ListElem* elem, const int value,
                                const size_t prev_pos, const size_t next_pos);
static inline void UpdateNeighbourElems(ListElem* elems, const size_t pos);

static inline void   AddFreeElemInList(list_t* list, const size_t pos);
static inline size_t GetFreeElemFromList(list_t* list);

static void CheckRemovingElement(const list_t* list, const size_t pos, ErrorInfo* error);
static void CheckGettingElement(const list_t* list, const size_t pos, ErrorInfo* error);

// ================== REALLOC FUNCS ===================

static ListErrors MakeListLonger(list_t* list, ErrorInfo* error);

static void FillShorterList(const list_t* old_list, ListElem* elems);

static ListElem* ReallocListElemsArray(ListElem* elems, const size_t new_capacity,
                                       const size_t old_capacity, ErrorInfo* error);

// =====================================================

// ========= TEXT DUMP =======

static void TextListDump(FILE* fp, const list_t* list);

static inline void PrintListInfo(FILE* fp, const list_t* list);
static inline void PrintListElements(FILE* fp, const list_t* list);
static inline void ChooseElementHtmlColor(FILE* fp, const list_t* list, const size_t pos);

// ===========================

// ========= GRAPHS ==========

static void DrawListGraph(const list_t* list);

static inline void DrawListInfo(FILE* dotf, const list_t* list);
static inline void DrawListElements(FILE* dotf, const list_t* list);
static inline void CenterListElements(FILE* dotf, const list_t* list);
static inline void DrawListArrows(FILE* dotf, const list_t* list);
static inline void ChooseVertexColor(FILE* dotf, const list_t* list, const size_t pos);
static inline void MarkImportantElements(FILE* dotf, const list_t* list, const size_t pos);

// ===========================

#ifdef CHECK_LIST
#undef CHECK_LIST
#endif
#define CHECK_LIST(list)    do                                                              \
                            {                                                               \
                                ListErrors list_err_ = ListVerify(list);                    \
                                if (list_err_ != ListErrors::NONE)                          \
                                    return list_err_;                                       \
                            } while(0)

ListErrors ListCtor(list_t* list, ErrorInfo* error, size_t capacity)
{
    assert(list);

    ListElem* elems = InitListElemsArray(capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->elems    = elems;

    list->free     = 1;
    list->capacity = capacity;
    list->size     = 0;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static ListElem* InitListElemsArray(const size_t capacity, ErrorInfo* error)
{
    assert(error);

    ListElem* elems = (ListElem*) calloc(capacity, sizeof(ListElem));
    if (elems == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "ELEMENTS ARRAY";
        return nullptr;
    }

    InitListElem(&elems[FICTIVE_ELEM_POS], POISON, 0, 0);

    //                           v------ we dont fill fictive and last elements
    for (int i = 1; i < capacity - 1; i++)
        InitListElem(&elems[i], POISON, -1, CHANGE_SIGN * (i + 1));

    InitListElem(&elems[capacity - 1], POISON, -1, 0);

    return elems;
}

//-----------------------------------------------------------------------------------------------------

int GetListHead(const list_t* list)
{
    return list->elems[FICTIVE_ELEM_POS].next;
}

//-----------------------------------------------------------------------------------------------------

int GetListTail(const list_t* list)
{
    return list->elems[FICTIVE_ELEM_POS].prev;
}

//-----------------------------------------------------------------------------------------------------

ListErrors GetListElement(const list_t* list, const size_t pos, int* destination, ErrorInfo* error)
{
    assert(list);
    assert(error);
    assert(destination);

    CheckGettingElement(list, pos, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    *destination = list->elems[pos].data;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void CheckGettingElement(const list_t* list, const size_t pos, ErrorInfo* error)
{
    if (list->elems[pos].data == POISON)
    {
        error->code = (int) ListErrors::EMPTY_ELEMENT;
        error->data = list;
        return;
    }
}

//-----------------------------------------------------------------------------------------------------

void ListDtor(list_t* list)
{
    assert(list);

    free(list->elems);

    list->free     = POISON;

    list->capacity = 0;
    list->size     = 0;
}

//-----------------------------------------------------------------------------------------------------

ListErrors ListVerify(const list_t* list)
{
    assert(list);

    if (list->size > list->capacity)                  return ListErrors::INVALID_SIZE;
    if (list->elems[0].data != POISON)                return ListErrors::DAMAGED_FICTIVE;

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

    InitListElem(&list->elems[free_pos], value, pos, list->elems[pos].next);
    UpdateNeighbourElems(list->elems, free_pos);

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

    InitListElem(&list->elems[free_pos], value, list->elems[pos].prev, pos);
    UpdateNeighbourElems(list->elems, free_pos);

    list->size++;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static ListErrors MakeListLonger(list_t* list, ErrorInfo* error)
{
    assert(list);
    assert(error);

    int new_capacity = list->capacity * CAPACITY_MULTIPLIER;

    ListElem* new_elems = ReallocListElemsArray(list->elems, new_capacity, list->capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->elems    = new_elems;

    list->free     = list->capacity;
    list->capacity = new_capacity;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static ListElem* ReallocListElemsArray(ListElem* elems, const size_t new_capacity,
                                       const size_t old_capacity, ErrorInfo* error)
{
    assert(elems);
    assert(error);

    ListElem* temp_elems = (ListElem*) realloc(elems, new_capacity * sizeof(ListElem));
    if (temp_elems == nullptr)
    {
        error->code = (int) ListErrors::ALLOCATE_MEMORY;
        error->data = "ELEMENTS ARRAY";
        return nullptr;
    }

    for (size_t i = old_capacity; i < new_capacity - 1; i++)
        InitListElem(&temp_elems[i], POISON, -1, CHANGE_SIGN * (i + 1));

    InitListElem(&temp_elems[new_capacity - 1], POISON, -1, 0);

    return temp_elems;
}

//-----------------------------------------------------------------------------------------------------

ListErrors MakeListShorter(list_t* list, const size_t new_capacity, ErrorInfo* error)
{
    assert(list);
    assert(error);

    if (list->size > new_capacity)
    {
        error->code = (int) ListErrors::INVALID_SIZE;
        return ListErrors::INVALID_SIZE;
    }

    ListElem* new_elems = InitListElemsArray(new_capacity, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    FillShorterList(list, new_elems);

    free(list->elems);
    list->elems         = new_elems;

    list->capacity      = new_capacity;
    list->free          = list->size + 1;

    return ListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void FillShorterList(const list_t* old_list, ListElem* elems)
{
    assert(old_list);
    assert(elems);

    size_t curr_pos = old_list->elems[FICTIVE_ELEM_POS].next;
    size_t size     = old_list->size;

    InitListElem(&elems[FICTIVE_ELEM_POS], POISON, size, 1);
    UpdateNeighbourElems(elems, FICTIVE_ELEM_POS);

    for (int i = 1; i < size; i++)
    {
        InitListElem(&elems[i], old_list->elems[curr_pos].data, i - 1, i + 1);
        UpdateNeighbourElems(elems, i);

        curr_pos = old_list->elems[curr_pos].next;
    }

    InitListElem(&elems[size], old_list->elems[curr_pos].data, size - 1, FICTIVE_ELEM_POS);
    UpdateNeighbourElems(elems, size);
}

//-----------------------------------------------------------------------------------------------------

static inline size_t GetFreeElemFromList(list_t* list)
{
    assert(list);

    int free_pos = list->free;
    list->free   = CHANGE_SIGN * list->elems[free_pos].next;

    return free_pos;
}

//-----------------------------------------------------------------------------------------------------

static inline void InitListElem(ListElem* elem, const int value,
                                const size_t prev_pos, const size_t next_pos)
{
    assert(elem);

    elem->data = value;
    elem->prev = prev_pos;
    elem->next = next_pos;
}

//-----------------------------------------------------------------------------------------------------

static inline void UpdateNeighbourElems(ListElem* elems, const size_t pos)
{
    assert(elems);

    elems[elems[pos].next].prev = pos;
    elems[elems[pos].prev].next = pos;
}

//-----------------------------------------------------------------------------------------------------

ListErrors ListRemoveElem(list_t* list, const size_t pos, ErrorInfo* error)
{
    assert(list);

    CheckRemovingElement(list, pos, error);
    RETURN_IF_LISTERROR((ListErrors) error->code);

    list->elems[list->elems[pos].prev].next = list->elems[pos].next;
    list->elems[list->elems[pos].next].prev = list->elems[pos].prev;

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

    if (list->elems[pos].data == POISON)
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

    list->elems[pos].data = POISON;
    list->elems[pos].prev = -1;
    list->elems[pos].next = CHANGE_SIGN * list->free;

    list->free = pos;
}

//-----------------------------------------------------------------------------------------------------

int ListDump(FILE* fp, const void* fast_list, const char* func, const char* file, const int line)
{
    assert(fast_list);

    LOG_START_DUMP(func, file, line);

    const list_t* list = (const list_t*) fast_list;

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

    const struct ErrorInfo* error = (const struct ErrorInfo*) err;

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
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s FROM LIST<br>\n", (const char*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::EMPTY_ELEMENT):
            fprintf(fp, "CAN NOT OPERATE WITH ALREADY EMPTY ELEMENT<br>\n");
            DUMP_LIST((const list_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (ListErrors::INVALID_SIZE):
            fprintf(fp, "INVALID LIST SIZE<br>\n");
            DUMP_LIST((const list_t*) error->data);
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

//====================================================================================================

static void TextListDump(FILE* fp, const list_t* list)
{
    fprintf(fp, "<pre>");

    fprintf(fp, "<b>DUMPING LIST</b><br>\n");

    PrintListElements(fp, list);
    PrintListInfo(fp, list);

    fprintf(fp, "</pre>");
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void PrintListInfo(FILE* fp, const list_t* list)
{
    assert(list);

    fprintf(fp, "HEAD     > %d<br>\n"
                "TAIL     > %d<br>\n"
                "FREE     > %d<br>\n"
                "CAPACITY > %lu<br>\n", GetListHead(list), GetListTail(list), list->free, list->capacity);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void PrintListElements(FILE* fp, const list_t* list)
{
    assert(list);
    fprintf(fp, "       <b>DATA  NEXT  PREV </b><br>\n");

    for (int i = 0; i < list->capacity; i++)
    {
        ChooseElementHtmlColor(fp, list, i);

        if (list->elems[i].data != POISON)
            fprintf(fp, "%3d -> [%3d, %3d, %3d]</b></font>\n", i, list->elems[i].data, list->elems[i].next, list->elems[i].prev);
        else
            fprintf(fp, "%3d -> [NaN, %3d, %3d]</b></font>\n", i, list->elems[i].next, list->elems[i].prev);
    }
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void ChooseElementHtmlColor(FILE* fp, const list_t* list, const size_t pos)
{
    assert(list);

    if (list->elems[pos].prev == -1)
        fprintf(fp, "<font color=\"#008000\"><b>");
    else if (list->elems[pos].data == POISON)
        fprintf(fp, "<font color=\"#474747\"><b>");
    else
        fprintf(fp, "<font color=\"#0000FF\"><b>");
}

//====================================================================================================

static void DrawListGraph(const list_t* list)
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

    MakeImgFromDot(DOT_FILE);
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

    if (list->elems[pos].prev == -1)
        fprintf(dotf, "fillcolor=\"lightgreen\", color = darkgreen,");
    else if (list->elems[pos].data == POISON)
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

        if (list->elems[i].data == POISON)
        {
            fprintf(dotf, "ip: %d | data: NaN| next: %d| prev: %d\" ];\n",
                            i, list->elems[i].next, list->elems[i].prev);
        }
        else
        {
            fprintf(dotf, "ip: %d | data: %d| next: %d| prev: %d\" ];\n",
                            i, list->elems[i].data, list->elems[i].next, list->elems[i].prev);
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
        if (list->elems[i].prev != -1)
            fprintf(dotf, "%d -> %d [weight = 0, color = \"red\", constraint = false];\n",
                            i,    list->elems[i].prev);

        int next = list->elems[i].next;
        if (next < 0 || (next == 0 && list->elems[next].prev != i))
        {
            next *= CHANGE_SIGN;
            fprintf(dotf, "%d -> %d [weight = 0, color = \"green\", constraint = false];\n", i, next);
        }
        else
            fprintf(dotf, "%d -> %d [weight = 0, color = \"blue\", constraint = false];\n", i, next);
    }
}
