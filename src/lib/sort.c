#include <mm/heap.h>
#include <memstr.h>
#include <sort.h>


void qsort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b)) {

    if (n > 4) {
        int i = 0, j = n - 1;
        void * tmp = (void *) malloc(e_size);
        void * pivot = (void *) malloc(e_size);

        memcpy(pivot, data, e_size);

        while (1) {
            while ((cmp(data + i * e_size, pivot) < 0) && i < n) i++;
            while ((cmp(data + j * e_size, pivot) >=0) && j > 0) j--;

            if (i<j) {
                memcpy(tmp, data + i * e_size, e_size);
                memcpy(data + i * e_size, data + j * e_size, e_size);
                memcpy(data + j * e_size, tmp, e_size);
            } else {
                break;
            }
        }

        free(tmp);
        free(pivot);

        qsort(data, j + 1, e_size, cmp);
        qsort(data + (j + 1) * e_size, n - j - 1, e_size, cmp);

    } else {
        bubblesort(data, n, e_size, cmp);
    }


}


void bubblesort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b)) {
    bool done = false;
    void * p;
    void * slot = (void *) malloc(e_size);

    while (!done) {
        done = true;
        for (p = data; p < data + e_size * (n - 1); p = p + e_size) {
            if (cmp(p, p + e_size) == 1) {
                memcpy(slot, p, e_size);
                memcpy(p, p + e_size, e_size);
                memcpy(p + e_size, slot, e_size);
                done = false;
            }
        }

    }

    free(slot);
}

/*
 * Comparator returns 1 if a > b, 0 if a == b, -1 if a < b
 */
int int_cmp(void * a, void * b) {
    return (* (int *) a > * (int*)b) ? 1 : (*(int *)a < * (int *)b) ? -1 : 0;
}

int __u8_cmp(void * a, void * b) {
    return (* (__u8 *) a > * (__u8 *)b) ? 1 : (*(__u8 *)a < * (__u8 *)b) ? -1 : 0;
}

int __u16_cmp(void * a, void * b) {
    return (* (__u16 *) a > * (__u16 *)b) ? 1 : (*(__u16 *)a < * (__u16 *)b) ? -1 : 0;
}

int __u32_cmp(void * a, void * b) {
    return (* (__u32 *) a > * (__u32 *)b) ? 1 : (*(__u32 *)a < * (__u32 *)b) ? -1 : 0;
}

