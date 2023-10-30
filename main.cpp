#include <stdio.h>

#include "logs.h"
#include "fast_list.h"
#include "errors.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    list_t list = {};

    ErrorInfo error = {};

    ListCtor(&list, &error);

    DUMP_LIST(&list);

    size_t a = 0;
    size_t b = 0;

    ListInsertElem(&list, 0, 23, &a, &error);
    ListInsertElem(&list, a, 28, &b, &error);
    ListInsertElem(&list, a, 29, &b, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 1, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 2, &error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 8, &error);
    EXIT_IF_LISTERROR(&error);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 3, &error);
    EXIT_IF_LISTERROR(&error);

    DUMP_LIST(&list);

    return 0;
}
