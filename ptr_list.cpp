#include "ptr_list.h"
#include "graphs.h"

static const char* DOT_FILE = "tmp.dot";
static const int   POISON   = -2147483647;

static PtrListElem* InitListElement(const int data, PtrListElem* prev,
                                                    PtrListElem* next, ErrorInfo* error);
static inline void DestructListElement(PtrListElem* elem);

static void CheckRemovingElement(const ptrlist_t* list, PtrListElem* pos, ErrorInfo* error);
static void CheckGettingElement(const ptrlist_t* list, PtrListElem* pos, ErrorInfo* error);

// ========= TEXT DUMP =======

static void TextPtrListDump(FILE* fp, const ptrlist_t* list);

static inline void PrintPtrListInfo(FILE* fp, const ptrlist_t* list);
static inline void ChooseElementHtmlColor(FILE* fp, const PtrListElem* cur_elem);
static void PrintPtrListElements(FILE* fp, const ptrlist_t* list);

// ===========================

// ========= GRAPHS ==========

static void DrawPtrListGraph(const ptrlist_t* list);

static inline void DrawPtrListInfo(FILE* dotf, const ptrlist_t* list);
static inline void DrawPtrListElements(FILE* dotf, const ptrlist_t* list);
static inline void CenterPtrListElements(FILE* dotf, const ptrlist_t* list);
static inline void DrawPtrListArrows(FILE* dotf, const ptrlist_t* list);

// ===========================

#ifdef CHECK_PTRLIST
#undef CHECK_PTRLIST
#endif
#define CHECK_PTRLIST(list) do                                                              \
                            {                                                               \
                                PtrListErrors list_err_ = PtrListVerify(list);              \
                                if (list_err_ != PtrListErrors::NONE)                       \
                                    return list_err_;                                       \
                            } while(0)

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

PtrListElem* GetPtrListHead(const ptrlist_t* list)
{
    return list->fictive->next;
}

//-----------------------------------------------------------------------------------------------------

PtrListElem* GetPtrListTail(const ptrlist_t* list)
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

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    //     fictive --------------------^

    for (int i = 0; i < elem_amt; i++)
    {
        DestructListElement(elem);

        elem = elem->next;
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

    CHECK_PTRLIST(list);

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

PtrListErrors PtrListInsertBeforeElem(ptrlist_t* list, PtrListElem* pos, const int value,
                                                      PtrListElem** inserted_pos, ErrorInfo* error)
{
    assert(list);

    CHECK_PTRLIST(list);

    PtrListElem* inserted_elem = InitListElement(value, pos->prev, pos, error);
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

    CHECK_PTRLIST(list);

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

static void CheckRemovingElement(const ptrlist_t* list, PtrListElem* pos, ErrorInfo* error)
{
    assert(list);
    assert(pos);
    assert(error);

    if (list->size == 0)
    {
        error->code = (int) PtrListErrors::EMPTY_LIST;
        return;
    }

    if (pos->data == POISON)
    {
        error->code = (int) PtrListErrors::FICTIVE_OPERATIONS;
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

static void CheckGettingElement(const ptrlist_t* list, PtrListElem* pos, ErrorInfo* error)
{
    assert(list);
    assert(pos);
    assert(error);

    if (pos->data == POISON)
    {
        error->code = (int) PtrListErrors::FICTIVE_OPERATIONS;
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

PtrListErrors GetPtrListElem(const ptrlist_t* list, PtrListElem* pos, int* destination, ErrorInfo* error)
{
    assert(list);
    assert(pos);
    assert(destination);
    assert(error);

    CheckGettingElement(list, pos, error);
    RETURN_IF_PTRLISTERROR((PtrListErrors) error->code);

    *destination = pos->data;

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

PtrListErrors PtrListVerify(const ptrlist_t* list)
{
    assert(list);

    if (list->fictive->data != POISON)
        return PtrListErrors::DAMAGED_FICTIVE;

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    //     fictive --------------------^

    for (int i = 0; i < elem_amt; i++)
    {
        if (elem->next->prev != elem || elem->prev->next != elem)   return PtrListErrors::UNKNOWN_ELEMENT;
        elem = elem->next;
    }

    return PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

int PrintPtrListError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    const struct ErrorInfo* error = (const struct ErrorInfo*) err;

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
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s FROM POINTER LIST<br>\n", (const char*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::FICTIVE_OPERATIONS):
            fprintf(fp, "CAN NOT OPERATE WITH FICTIVE ELEMENT<br>\n");
            DUMP_PTRLIST((const ptrlist_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::DAMAGED_FICTIVE):
            fprintf(fp, "DAMAGED FICTIVE ELEMENT<br>\n");
            DUMP_PTRLIST((const ptrlist_t*) error->data);
            LOG_END();
            return (int) error->code;

        case (PtrListErrors::UNKNOWN_ELEMENT):
            fprintf(fp, "TRYING TO OPERATE WITH UNKNOWN ELEMENT<br>\n");
            DUMP_PTRLIST((const ptrlist_t*) error->data);
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

    const ptrlist_t* list = (const ptrlist_t*) ptr_list;

    TextPtrListDump(fp, list);
    DrawPtrListGraph(list);

    LOG_END();

    return (int) PtrListErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void TextPtrListDump(FILE* fp, const ptrlist_t* list)
{
    assert(list);

    fprintf(fp, "<pre>");

    fprintf(fp, "<b>DUMPING POINTERS LIST<\\b>\n");

    PrintPtrListInfo(fp, list);
    PrintPtrListElements(fp, list);

    fprintf(fp, "<\pre>");
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void PrintPtrListInfo(FILE* fp, const ptrlist_t* list)
{
    assert(list);

    fprintf(fp, "LIST[%p]<br>\n"
                "{<br>\n"
                "SIZE    > %lu<br>\n"
                "FICTIVE > [%p]<br>\n"
                "}<br>\n"
                "ELEMENTS: <br>\n", list, list->size, list->fictive);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void ChooseElementHtmlColor(FILE* fp, const PtrListElem* cur_elem)
{
    assert(cur_elem);

    if (cur_elem->data == POISON)
        fprintf(fp, "<font color=\"#474747\"><b>");
    else
        fprintf(fp, "<font color=\"#0000FF\"><b>");
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static void PrintPtrListElements(FILE* fp, const ptrlist_t* list)
{
    assert(list);

    size_t       elem_amt   = list->size + 1;
    PtrListElem* cur_elem   = list->fictive;
    int          elem_cnt   = 0;

    for (int i = 0; i < elem_amt; i++)
    {
        ChooseElementHtmlColor(fp, cur_elem);

        if (cur_elem->data != POISON)
        {
            fprintf(fp, "\t%3d[%p]: data[%d], next[%p], prev[%p]</b></font><br>\n",
                                                                         elem_cnt++,     cur_elem,
                                                                         cur_elem->data, cur_elem->next,
                                                                         cur_elem->prev);
        }
        else
        {
            fprintf(fp, "\t%3d[%p]: data[NaN], next[%p], prev[%p]</b></font><br>\n",
                                                                          elem_cnt++,     cur_elem,
                                                                          cur_elem->next, cur_elem->prev);
        }

        cur_elem = cur_elem->next;
    }
}

//-----------------------------------------------------------------------------------------------------

static void DrawPtrListGraph(const ptrlist_t* list)
{
    assert(list);

    FILE* dotf = fopen(DOT_FILE, "w");

    StartGraph(dotf);

    DrawPtrListInfo(dotf, list);
    DrawPtrListElements(dotf, list);
    CenterPtrListElements(dotf, list);
    DrawPtrListArrows(dotf, list);

    EndGraph(dotf);

    fclose(dotf);

    MakeImgFromDot(DOT_FILE);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawPtrListInfo(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    fprintf(dotf, "info [shape=record, style=filled, fillcolor=\"yellow\""
                  "label=\"HEAD: %p | TAIL: %p | SIZE: %lu\""
                  "fontcolor = \"black\", fontsize = 25];\n",
                              GetPtrListHead(list), GetPtrListTail(list), list->size);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawPtrListElements(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    #pragma GCC diagnostic ignored "-Wformat"

    for (int i = 0; i < elem_amt; i++)
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
            fprintf(dotf, "elem: %p | data: NaN| next: %p| prev: %p\" ];\n",
                            elem, elem->next, elem->prev);
        }
        else
        {
            fprintf(dotf, "elem: %p | data: %d| next: %p | prev: %p\" ];\n",
                            elem, elem->data, elem->next, elem->prev);
        }

        elem = elem->next;
    }

    #pragma GCC diagnostic warning "-Wformat"
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void CenterPtrListElements(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    #pragma GCC diagnostic ignored "-Wformat"

    fprintf(dotf, "%lld", elem);
    for (int i = 0; i < elem_amt; i++)
    {
        fprintf(dotf, "->%lld", elem->next);

        elem = elem->next;
    }
    fprintf(dotf, "[weight = 993, color = \"white\"];\n");

    #pragma GCC diagnostic warning "-Wformat"
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawPtrListArrows(FILE* dotf, const ptrlist_t* list)
{
    assert(list);

    PtrListElem* elem     = list->fictive;
    size_t       elem_amt = list->size + 1;
    // fictive ------------------^

    #pragma GCC diagnostic ignored "-Wformat"

    for (int i = 0; i < elem_amt; i++)
    {
        fprintf(dotf, "%lld -> %lld [color = \"red\"];\n", elem, elem->prev);
        fprintf(dotf, "%lld -> %lld [color = \"green\"];\n", elem, elem->next);

        elem = elem->next;
    }

    #pragma GCC diagnostic warning "-Wformat"
}


