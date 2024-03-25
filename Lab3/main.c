#include <stdio.h>
#include <string.h>

#define PAGE_CAPACITY 1 << 8
#define TLB_CAPACITY 16
#define FRAME_SIZE 1 << 8

const int offset_mask = (1 << 8) - 1;

FILE *backing_store;

struct page {
    int offset;
    int n;
    int frame_n;
};

struct page tlb [TLB_CAPACITY];
struct page pages [PAGE_CAPACITY];
char ram[PAGE_CAPACITY][FRAME_SIZE];

char tlb_len = 0;
int frames_len = 0;
int pages_len = 0;
int processed_pages = 0;

int page_fault_cnt = 0;
int tlb_hit = 0;

int read_from_file(struct page *curr_page) {
    if (frames_len >= PAGE_CAPACITY || pages_len >= PAGE_CAPACITY) {
        return -1;
    }

    fseek(backing_store, curr_page->n * FRAME_SIZE, SEEK_SET);
    fread(ram[frames_len], sizeof(char), FRAME_SIZE, backing_store);

    pages[pages_len].n = curr_page->n;
    pages[pages_len].frame_n = frames_len;
    pages_len++;

    return frames_len++;
}

void insert_tlb(struct page *curr_page) {
    if (tlb_len == 16) {
        memmove(&tlb[0], &tlb[1], sizeof(struct page) * 15);
        tlb_len--;
    }
    tlb[tlb_len++] = *curr_page;
}

int get_frame(struct page *curr_page) {
    for (int i = tlb_len; i >= 0; --i) {
        if (tlb[i].n == curr_page->n) {
            curr_page->frame_n = tlb[i].frame_n;
            ++tlb_hit;
            goto end;
        }
    }

    for (int i = 0; i < pages_len; ++i) {
        if (pages[i].n == curr_page->n) {
            curr_page->frame_n = pages[i].frame_n;
            goto end;
        }
    }

    curr_page->frame_n = read_from_file(curr_page);
    ++page_fault_cnt;

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

    while (!feof(stdin)) {
        struct page curr_page = get_page();
        get_frame(&curr_page);
        printf("Virtual address: %d Physical address: %d Value: %d\n",
               (curr_page.n << 8) | curr_page.offset,
               (curr_page.frame_n << 8) | curr_page.offset,
               ram[curr_page.frame_n][curr_page.offset]);
        ++processed_pages;
    }

    printf("\nPage fault frequency = %0.2f%%\n", page_fault_cnt * 100. / processed_pages);
    printf("TLB frequency = %0.2f%%\n", tlb_hit * 100. / processed_pages);

    return 0;
}