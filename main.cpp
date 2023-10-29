#include <stdio.h>

#include "logs.h"
#include "fast_list.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    list_t list = {};

    ListCtor(&list);

    DUMP_LIST(&list);

    return 0;
}
