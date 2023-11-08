#include <stdio.h>

#include "logs.h"
#include "fast_list.h"
#include "ptr_list.h"
#include "errors.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    list_t list     = {};
    ErrorInfo error = {};

    ListCtor(&list, &error);
    DUMP_LIST(&list);

    size_t a = 0;
    size_t b = 0;

    ListInsertAfterElem(&list, 0, 23, &a, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);
    ListInsertAfterElem(&list, a, 29, &b, &error);
    ListInsertAfterElem(&list, a, 29, &a, &error);
    ListInsertAfterElem(&list, a, 29, &a, &error);
    ListInsertAfterElem(&list, b, 28, &a, &error);
    ListInsertAfterElem(&list, a, 28, &b, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);
    ListInsertAfterElem(&list, b, 28, &a, &error);
    ListInsertAfterElem(&list, b, 28, &a, &error);
    ListInsertAfterElem(&list, b, 28, &a, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);
    ListInsertAfterElem(&list, a, 28, &a, &error);

    DUMP_LIST(&list);

    ListInsertAfterElem(&list, a, 228, &a, &error);
    ListInsertAfterElem(&list, a, 228, &a, &error);
    ListInsertAfterElem(&list, a, 228, &a, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 10, &error);
    ListRemoveElem(&list, 7, &error);
    ListRemoveElem(&list, 11, &error);
    ListRemoveElem(&list, 2, &error);
    ListRemoveElem(&list, 1, &error);
    ListRemoveElem(&list, 6, &error);
    ListRemoveElem(&list, 18, &error);
    ListRemoveElem(&list, 15, &error);
    ListRemoveElem(&list, 14, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 5, &error);
    ListRemoveElem(&list, 13, &error);
    ListRemoveElem(&list, 12, &error);
    ListRemoveElem(&list, 4, &error);

    DUMP_LIST(&list);

    /*ListRemoveElem(&list, 8, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 4, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 2, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 3, &error);
    EXIT_IF_LISTERROR(&error);

    DUMP_LIST(&list);

    ListInsertAfterElem(&list, 0, 23, &a, &error);
    ListInsertAfterElem(&list, a, 28, &b, &error);
    ListInsertAfterElem(&list, a, 29, &b, &error);

    DUMP_LIST(&list);*/

    ListDtor(&list);

    return 0;
}
