#include <stdio.h>

#include "logs.h"
#include "fast_list.h"
#include "ptr_list.h"
#include "errors.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    ptrlist_t list = {};

    ErrorInfo error = {};

    PtrListCtor(&list, &error);

    DUMP_PTRLIST(&list);

    PtrListElem* last = nullptr;

    PtrListInsertAfterElem(&list, list.fictive, 23, &last, &error);
    PtrListInsertAfterElem(&list, last, 28, &last, &error);
    PtrListInsertAfterElem(&list, last, 29, &last, &error);

    DUMP_PTRLIST(&list);

    PtrListRemoveElem(&list, last, &error);

    DUMP_PTRLIST(&list);

    /*ListRemoveElem(&list, 2, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 3, &error);
    EXIT_IF_LISTERROR(&error);

    DUMP_LIST(&list);

    ListInsertAfterElem(&list, 0, 23, &a, &error);
    ListInsertAfterElem(&list, a, 28, &b, &error);
    ListInsertAfterElem(&list, a, 29, &b, &error);

    DUMP_LIST(&list);*/

    return 0;
}
