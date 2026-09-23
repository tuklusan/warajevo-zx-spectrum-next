#include <stdlib.h>

int review_workflow_test(int count)
{
    int *values = malloc((size_t)count * sizeof(*values));
    values[count] = 42;
    return values[0];
}
