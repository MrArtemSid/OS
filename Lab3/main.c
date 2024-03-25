#include <stdio.h>
#include <string.h>

#define PAGE_CNT 1 << 8
#define PAGE_SIZE 1 << 8
#define TLB_CNT 16
#define FRAME_SIZE 1 << 8

const int offset_mask = (1 << 8) - 1;

FILE *backing_store;

struct page {
    int offset;
    int n;
    int frame_n;
};

struct page tlb [TLB_CNT];
struct page pages [PAGE_CNT];
char ram[PAGE_CNT][FRAME_SIZE];
char buffer[PAGE_SIZE];

char tlb_cnt = 0;
int frame_cnt = 0;
int page_cnt = 0;

int read_from_file(struct page *curr_page) {
    if (frame_cnt >= PAGE_CNT || page_cnt >= PAGE_CNT) {
        return -1;
    }

    fseek(backing_store, curr_page->n * FRAME_SIZE, SEEK_SET);
    fread(buffer, sizeof(signed char), FRAME_SIZE, backing_store);
    memcpy(ram[frame_cnt], buffer, FRAME_SIZE);

    pages[page_cnt].n = curr_page->n;
    pages[page_cnt].frame_n = frame_cnt;
    page_cnt++;

    return frame_cnt++;
}

void insert_tlb(struct page *curr_page) {
    if (tlb_cnt == 16) {
        memmove(&tlb[1], &tlb[2], sizeof(struct page) * 15);
        tlb_cnt--;
    }
    tlb[tlb_cnt++] = *curr_page;
}

int get_frame(struct page *curr_page) {
    for (int i = tlb_cnt; i >= 0; --i) {
        if (tlb[i].n == curr_page->n) {
            curr_page->frame_n = tlb[i].frame_n;
            goto end;
        }
    }

    for (int i = 0; i < page_cnt; ++i) {
        if (pages[i].n == curr_page->n) {
            curr_page->frame_n = pages[i].frame_n;
            goto end;
        }
    }

    curr_page->frame_n = read_from_file(curr_page);

end:
    insert_tlb(curr_page);
    return 0;
}

struct page get_page() {
    int tmp;
    struct page curr_page;
    scanf("%d", &tmp);
    curr_page.offset = tmp & offset_mask;
    curr_page.n = (tmp >> 8) & offset_mask;
    return curr_page;
}

int main(int argc, char *argv[]) {
    if (argc != 2 || !freopen(argv[1], "r", stdin)) {
        printf("input reading error\n");
        return 1;
    }

    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (backing_store == NULL) {
        printf("BACKING_STORE.bin reading error\n");
        return 1;
    }

    freopen("check.txt", "w", stdout);

    for (int i = 0; i < 1000; ++i) {
        struct page curr_page = get_page();
        get_frame(&curr_page);
        printf("Virtual address: %d Physical address: %d Value: %d\n",
               (curr_page.n << 8) | curr_page.offset,
               (curr_page.frame_n << 8) | curr_page.offset,
               ram[curr_page.frame_n][curr_page.offset]);
    }

    return 0;
}