#include <stdlib.h>

struct bit_array {
    char *data;
    size_t size;
    size_t bytes;
};

struct bit_array *bit_array_new(size_t size);

char bit_array_get(struct bit_array *arr, size_t idx);

void bit_array_set(struct bit_array *arr, size_t idx, char val);

void bit_array_invert(struct bit_array *arr, size_t idx);

void bit_array_invert_all(struct bit_array *arr);

void bit_array_free(struct bit_array *arr);

void bit_array_print(struct bit_array *arr);

void bit_array_fprint(struct bit_array *arr, FILE *f);