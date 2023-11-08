#ifndef __PTR_LIST_H_
#define __PTR_LIST_H_

#include "errors.h"

struct PtrListElem
{
    int data;
    PtrListElem* next;
    PtrListElem* prev;
};

struct PtrList
{
    PtrListElem* fictive;

    size_t size;
};

enum class PtrListErrors
{
    NONE = 0,

    ALLOCATE_MEMORY,
    EMPTY_LIST,
    REMOVE_FICTIVE,
    DAMAGED_FICTIVE,
    UNKNOWN_ELEMENT,

    UNKNOWN
};

#ifdef EXIT_IF_PTRLISTERROR
#undef EXIT_IF_PTRLISTERROR

#endif
#define EXIT_IF_PTRLISTERROR(error)         do                                                          \
                                            {                                                           \
                                                if ((error)->code != (int) PtrListErrors::NONE)         \
                                                {                                                       \
                                                    return LogDump(PrintPtrListError, error, __func__,  \
                                                                    __FILE__, __LINE__);                \
                                                }                                                       \
                                            } while(0)
#ifdef RETURN_IF_PTRLISTERROR
#undef RETURN_IF_PTRLISTERROR

#endif
#define RETURN_IF_PTRLISTERROR(error)       do                                                          \
                                            {                                                           \
                                                if ((error) != PtrListErrors::NONE)                     \
                                                {                                                       \
                                                    return error;                                       \
                                                }                                                       \
                                            } while(0)

typedef struct PtrList ptrlist_t;

PtrListErrors PtrListCtor(ptrlist_t* list, ErrorInfo* error);
void          PtrListDtor(ptrlist_t* list);
PtrListErrors PtrListInsertAfterElem(ptrlist_t* list, PtrListElem* pos, const int value,
                                                      PtrListElem** inserted_pos, ErrorInfo* error);
PtrListErrors PtrListInsertBeforeElem(ptrlist_t* list, PtrListElem* pos, const int value,
                                                        PtrListElem** inserted_pos, ErrorInfo* error);
PtrListErrors PtrListRemoveElem(ptrlist_t* list, PtrListElem* pos, ErrorInfo* error);
int           PtrListDump(FILE* fp, const void* list, const char* func, const char* file, const int line);
PtrListErrors PtrListVerify(const ptrlist_t* list);

int PrintPtrListError(FILE* fp, const void* err, const char* func, const char* file, const int line);

#ifdef DUMP_PTRLIST
#undef DUMP_PTRLIST

#endif
#define DUMP_PTRLIST(list)  do                                                              \
                            {                                                               \
                                LogDump(PtrListDump, (list), __func__, __FILE__, __LINE__); \
                            } while(0)

#endif
