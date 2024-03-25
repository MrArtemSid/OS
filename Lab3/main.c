#include <stdio.h>

const int PAGE_SIZE = 1<<8;
const int FRAME_SIZE = 1<<8;
const int offset_mask = (1<<8) - 1;

char tlb [16];
//char pages[PAGE_SIZE];
FILE *backing_store;

struct page {
    int offset;
    int n;
    int value;
};

struct page get_page() {
    int tmp;
    struct page curr_page;
    scanf("%d", &tmp);
    curr_page.offset = tmp & offset_mask;
    curr_page.n = (tmp >> 8) & offset_mask;
    return curr_page;
}

int main(int argc, char *argv[]) {
    if (!freopen("../addresses.txt", "r", stdin))
        return -1;
    if (!freopen("check.txt", "w", stdout))
        return -1;
    for (int i = 0; i < 1000; ++i) {
        struct page curr_page = get_page();
        printf("%d %d\n", curr_page.offset, curr_page.n);
    }
    backing_store = fopen("BACKING_STORE.bin", "rb");

    return 0;
}