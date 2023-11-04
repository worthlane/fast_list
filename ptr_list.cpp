#include "ptr_list.h"
#include "graphs.h"

static const int POISON = -2147483647;

static PtrListElem* InitListElement(const int data, PtrListElem* prev,
                                    PtrListElem* next, ErrorInfo* error);
static inline void DestructListElement(PtrListElem* elem);

static void CheckRemovingElement(ptrlist_t* list, PtrListElem* pos, ErrorInfo* error);

static inline PtrListElem* GetPtrListHead(const ptrlist_t* list);
static inline PtrListElem* GetPtrListTail(const ptrlist_t* list);

// ========= GRAPHS ==========

static void DrawListGraph(ptrlist_t* list);

static inline void DrawListInfo(FILE* dotf, const ptrlist_t* list);
static inline void DrawListElements(FILE* dotf, const ptrlist_t* list);
static inline void CenterListElements(FILE* dotf, const ptrlist_t* list);
static inline void DrawListArrows(FILE* dotf, const ptrlist_t* list);

// ===========================

PtrListErrors PtrListCtor(ptrlist_t* list, ErrorInfo* error)
{
    assert(list);

    PtrListElem* fictive_elem = InitListElement(POISON, nullptr, nullptr, error);
    RETURN_IF_PTRLISTERROR((PtrListErrors) error->code);

    fictive_elem->next = fictive_elem;
    fictive_elem->prev = fictive_elem;

    list->fictive = fictive_elem;
    list->size    = 0;

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static inline PtrListElem* GetPtrListHead(const ptrlist_t* list)
{
    return list->fictive->next;
}

//-----------------------------------------------------------------------------------------------------

static inline PtrListElem* GetPtrListTail(const ptrlist_t* list)
{
    return list->fictive->prev;
}


//-----------------------------------------------------------------------------------------------------

static PtrListElem* InitListElement(const int data, PtrListElem* prev,
                                    PtrListElem* next, ErrorInfo* error)
{
    assert(error);

    PtrListElem* elem = (PtrListElem*) calloc(1, sizeof(PtrListElem));
    if (elem == nullptr)
    {
        error->code = (int) PtrListErrors::ALLOCATE_MEMORY;
        error->data = "ELEMENT";
        return nullptr;
    }

    elem->data = data;
    elem->prev = prev;
    elem->next = next;

    return elem;
}

//-----------------------------------------------------------------------------------------------------

void PtrListDtor(ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem = list->fictive;
    size_t elem_amt   = list->size + 1;
    // fictive --------------------^

    while (elem_amt--)
    {
        PtrListElem* next_elem = elem->next;

        DestructListElement(elem);

        elem = next_elem;
    }

    list->fictive = nullptr;
    list->size    = 0;
}

//-----------------------------------------------------------------------------------------------------

static inline void DestructListElement(PtrListElem* elem)
{
    assert(elem);
    free(elem);
}

//-----------------------------------------------------------------------------------------------------

PtrListErrors PtrListInsertAfterElem(ptrlist_t* list, PtrListElem* pos, const int value,
                                                      PtrListElem** inserted_pos, ErrorInfo* error)
{
    assert(list);

    PtrListElem* inserted_elem = InitListElement(value, pos, pos->next, error);
    *inserted_pos              = inserted_elem;
    RETURN_IF_PTRLISTERROR((PtrListErrors) error->code);

    PtrListElem* prev_elem = inserted_elem->prev;
    PtrListElem* next_elem = inserted_elem->next;

    next_elem->prev = inserted_elem;
    prev_elem->next = inserted_elem;

    list->size++;

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

PtrListErrors PtrListRemoveElem(ptrlist_t* list, PtrListElem* pos, ErrorInfo* error)
{
    assert(list);
    assert(pos);
    assert(error);

    CheckRemovingElement(list, pos, error);
    RETURN_IF_PTRLISTERROR((PtrListErrors) error->code);

    PtrListElem* prev_elem = pos->prev;
    PtrListElem* next_elem = pos->next;

    prev_elem->next = next_elem;
    next_elem->prev = prev_elem;

    DestructListElement(pos);
    list->size--;

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void CheckRemovingElement(ptrlist_t* list, PtrListElem* pos, ErrorInfo* error)
{
    if (list->size == 0)
    {
        error->code = (int) PtrListErrors::EMPTY_LIST;
        return;
    }

    if (pos->data == POISON)
    {
        error->code = (int) PtrListErrors::FICTIVE_ELEMENT;
        error->data = list;
        return;
    }

    if (((pos->prev)->next) != pos || ((pos->next)->prev) != pos)
    {
        error->code = (int) PtrListErrors::UNKNOWN_ELEMENT;
        error->data = pos;
        return;
    }
}

//-----------------------------------------------------------------------------------------------------

#ifdef CHECK_PTRLIST
#undef CHECK_PTRLIST

#endif
#define CHECK_PTRLIST(list) do                                                              \
                            {                                                               \
                                ListErrors list_err_ = PtrListVerify(list);                 \
                                if (list_err_ != ListErrors::NONE)                          \
                                    return list_err_;                                       \
                            } while(0)

PtrListErrors PtrListVerify(const ptrlist_t* list)
{
    assert(list);

    if (list->fictive->data != POISON)                return PtrListErrors::DAMAGED_FICTIVE;

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

int PrintPtrListError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    #pragma GCC diagnostic ignored "-Wcast-qual"
    struct ErrorInfo* error = (struct ErrorInfo*) err;
    #pragma GCC diagnostic warning "-Wcast-qual"

    switch ((PtrListErrors) error->code)
    {
        case (PtrListErrors::NONE):
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::EMPTY_LIST):
            fprintf(fp, "CAN NOT REMOVE ELEMENT FROM EMPTY LIST<br>\n");
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::ALLOCATE_MEMORY):
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s FROM POINTER LIST<br>\n", (char*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::FICTIVE_ELEMENT):
            fprintf(fp, "CAN NOT REMOVE FICTIVE ELEMENT<br>\n");
            DUMP_PTRLIST((ptrlist_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::UNKNOWN_ELEMENT):
            fprintf(fp, "TRYING TO REMOVE UNKNOWN ELEMENT<br>\n");
            DUMP_PTRLIST((ptrlist_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::UNKNOWN):
        // fall through
        default:
            fprintf(fp, "UNKNOWN ERROR WITH LIST<br>\n");
            LOG_END();
            return (int) PtrListErrors::UNKNOWN;
    }
}

//-----------------------------------------------------------------------------------------------------

int PtrListDump(FILE* fp, const void* ptr_list, const char* func, const char* file, const int line)
{
    assert(ptr_list);

    LOG_START_DUMP(func, file, line);

    #pragma GCC diagnostic ignored "-Wcast-qual"
    ptrlist_t* list = (ptrlist_t*) ptr_list;
    #pragma GCC diagnostic warning "-Wcast-qual"

    size_t elem_amt   = list->size + 1;
    PtrListElem* elem = list->fictive;
    int elem_cnt      = 0;

    PrintLog("LIST[%p]<br>\n"
             "{<br>\n"
             "SIZE    > %d<br>\n"
             "FICTIVE > [%p]<br>\n"
             "}<br>\n"
             "ELEMENTS: <br>\n", list, list->size, list->fictive);

    while (elem_amt--)
    {
        if (elem->data != POISON)
        {
            PrintLog("\t%3d[%p]: data[%d], next[%p], prev[%p]<br>\n", elem_cnt++, elem,
                                                                  elem->data, elem->next, elem->prev);
        }
        else
        {
            PrintLog("\t%3d[%p]: data[NaN], next[%p], prev[%p]<br>\n", elem_cnt++, elem,
                                                                   elem->next, elem->prev);
        }

        elem = elem->next;
    }

    DrawListGraph(list);

    LOG_END();

    return (int) PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void DrawListGraph(ptrlist_t* list)
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

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListInfo(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    fprintf(dotf, "info [shape=record, style=filled, fillcolor=\"yellow\","
                  "label=\"HEAD: %lld | TAIL: %lld | SIZE: %d\","
                  "fontcolor = \"black\", fontsize = 25];\n",
                              GetPtrListHead(list), GetPtrListTail(list), list->size, list->size);
}

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListElements(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    while (elem_amt--)
    {
        fprintf(dotf, "%lld [shape=Mrecord, style=filled, ", elem);
        if (elem->data == POISON)
            fprintf(dotf, "fillcolor=\"lightgray\", color = black,");
        else
            fprintf(dotf, "fillcolor=\"lightblue\", color = darkblue,");

        fprintf(dotf, " label=\" ");

        if (elem == GetPtrListHead(list))
            fprintf(dotf, "HEAD | ");

        if (elem == GetPtrListTail(list))
            fprintf(dotf, "TAIL | ");

        if (elem->data == POISON)
        {
            fprintf(dotf, "elem: %lld | data: NaN| next: %lld| prev: %lld\" ];\n",
                            elem, elem->next, elem->prev);
        }
        else
        {
            fprintf(dotf, "elem: %lld | data: %d| next: %lld | prev: %lld\" ];\n",
                            elem, elem->data, elem->next, elem->prev);
        }

        elem = elem->next;
    }
}

//:::::::::::::::::::::::::::::::::::::

static inline void CenterListElements(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    fprintf(dotf, "%lld", elem);
    while (elem_amt--)
    {
        fprintf(dotf, "->%lld", elem->next);

        elem = elem->next;
    }
    fprintf(dotf, "[weight = 993, color = \"white\"];\n");
}

//:::::::::::::::::::::::::::::::::::::

static inline void DrawListArrows(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    while (elem_amt--)
    {
        fprintf(dotf, "%lld -> %lld [color = \"red\"];\n", elem, elem->prev);
        fprintf(dotf, "%lld -> %lld [color = \"green\"];\n", elem, elem->next);

        elem = elem->next;
    }
}


