#include <libutil.h>
char* strndup(const char* string, size_t n)
{
        char* copy_string = 0;

        if(0 == string || 0 == n)
                return 0;

        copy_string = (char*) malloc(n + 1);
        if(0 == copy_string)
                return 0;

        memcpy(copy_string, string, n);
        *(copy_string + n) = '\0';

        return copy_string;
}

