#include <stdio.h>

#include "logs.h"
#include "fast_list.h"
#include "errors.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    list_t list = {};

    ListCtor(&list);

    DUMP_LIST(&list);

    size_t a = 0;
    size_t b = 0;

    ListInsertElem(&list, 0, 23, &a);
    ListInsertElem(&list, a, 28, &b);
    ListInsertElem(&list, a, 29, &b);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 1);

    DUMP_LIST(&list);

    ListRemoveElem(&list, 2);

    DUMP_LIST(&list);

    ErrorInfo error = {};

    error.code = ListRemoveElem(&list, 3);
    EXIT_IF_ERROR(&error);

    DUMP_LIST(&list);

    ListInsertElem(&list, 0, 23, &a);
    ListInsertElem(&list, a, 28, &b);
    ListInsertElem(&list, a, 29, &b);

    DUMP_LIST(&list);

    return 0;
}
