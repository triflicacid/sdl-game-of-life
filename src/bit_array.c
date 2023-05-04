#include "bit_array.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

struct bit_array *bit_array_new(size_t size) {
    size_t bytes = ceil(size / 8.0f);
    char *data = (char *) calloc(bytes, sizeof(char));
    struct bit_array *arr = (struct bit_array *) malloc(sizeof(struct bit_array));
    arr->data = data;
    arr->size = size;
    arr->bytes = bytes;
    return arr;
}

void bit_array_free(struct bit_array *arr) {
    free(arr->data);
    free(arr);
}

void bit_array_fprint(struct bit_array *arr, FILE *f) {
    fprintf(f, "{");
    for (int i = 0, j = 0, k = 0; k < arr->size; k++) {
        char c = (arr->data[i] & (1 << j)) != 0;
        fprintf(f, "%u", c);

        if (k < arr->size - 1) {
            fprintf(f, ",");
        }

        if (j == 7) {
            j = 0;
            ++i;
        } else {
            ++j;
        }
    }
    fprintf(f, "}\n");
}


void bit_array_print(struct bit_array *arr) {
    bit_array_fprint(arr, stdout);
}

char bit_array_get(struct bit_array *arr, size_t idx) {
    ldiv_t res = ldiv(idx, 8);
    return (arr->data[res.quot] & (1 << res.rem)) != 0;
}

void bit_array_set(struct bit_array *arr, size_t idx, char val) {
    ldiv_t res = ldiv(idx, 8);

    if (val) {
        arr->data[res.quot] |= (1 << res.rem);
    } else {
        arr->data[res.quot] &= ~(1 << res.rem);
    }
}

void bit_array_invert(struct bit_array *arr, size_t idx) {
    ldiv_t res = ldiv(idx, 8);
    arr->data[res.quot] ^= (1 << res.rem);
}

void bit_array_invert_all(struct bit_array *arr) {
    for (size_t i = 0; i < arr->bytes; ++i)
        arr->data[i] = ~arr->data[i];
}

int main() {
    struct bit_array *arr = bit_array_new(5);
    bit_array_print(arr);
    bit_array_invert_all(arr);
    bit_array_print(arr);
    bit_array_free(arr);
    return 0;
}