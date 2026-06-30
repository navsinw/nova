#include "nova.h"

/* in-place quicksort with an insertion-sort cutoff, plus binary search. */

static void iswap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

static void qsort_int(int *a, int lo, int hi)
{
    while (lo < hi) {
        if (hi - lo < 12) {
            for (int i = lo + 1; i <= hi; i++) {
                int key = a[i], j = i - 1;
                while (j >= lo && a[j] > key) { a[j + 1] = a[j]; j--; }
                a[j + 1] = key;
            }
            return;
        }
        int mid = lo + (hi - lo) / 2;
        if (a[mid] < a[lo]) iswap(&a[mid], &a[lo]);
        if (a[hi] < a[lo]) iswap(&a[hi], &a[lo]);
        if (a[hi] < a[mid]) iswap(&a[hi], &a[mid]);
        int pivot = a[mid];
        int i = lo, j = hi;
        while (i <= j) {
            while (a[i] < pivot) i++;
            while (a[j] > pivot) j--;
            if (i <= j) { iswap(&a[i], &a[j]); i++; j--; }
        }
        if (j - lo < hi - i) { qsort_int(a, lo, j); lo = i; }
        else { qsort_int(a, i, hi); hi = j; }
    }
}

void nova_sort_int(int *a, int n)
{
    if (a && n > 1) qsort_int(a, 0, n - 1);
}

int nova_bsearch_int(const int *a, int n, int key)
{
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == key) return mid;
        if (a[mid] < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

void nova_sort_u32(uint32_t *a, int n)
{
    /* simple heapsort to vary the algorithm coverage */
    if (!a || n < 2) return;
    for (int i = n / 2 - 1; i >= 0; i--) {
        int root = i;
        for (;;) {
            int child = 2 * root + 1;
            if (child >= n) break;
            if (child + 1 < n && a[child] < a[child + 1]) child++;
            if (a[root] >= a[child]) break;
            uint32_t t = a[root]; a[root] = a[child]; a[child] = t;
            root = child;
        }
    }
    for (int end = n - 1; end > 0; end--) {
        uint32_t t = a[0]; a[0] = a[end]; a[end] = t;
        int root = 0;
        for (;;) {
            int child = 2 * root + 1;
            if (child >= end) break;
            if (child + 1 < end && a[child] < a[child + 1]) child++;
            if (a[root] >= a[child]) break;
            uint32_t tt = a[root]; a[root] = a[child]; a[child] = tt;
            root = child;
        }
    }
}
