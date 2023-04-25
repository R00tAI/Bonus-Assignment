#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CACHE_SIZE 32
#define BLOCK_SIZE 4
#define NUM_LINES (CACHE_SIZE / BLOCK_SIZE)

typedef enum { DIRECT_MAPPED, TWO_WAY, FOUR_WAY, FULLY_ASSOCIATIVE } CacheType;
typedef enum { LRU, RANDOM } ReplacementPolicy;

typedef struct {
    uint32_t tag;
    int valid;
    int counter;
} CacheLine;

void init_cache(CacheLine cache[][NUM_LINES], int num_sets, int associativity);
int access_cache(CacheLine cache[][NUM_LINES], uint32_t address, CacheType type, ReplacementPolicy policy);
void print_stats(const char *name, int hits, int accesses);

int main() {
    FILE *file = fopen("traces.txt", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    srand(time(NULL));

    CacheLine direct_mapped[1][NUM_LINES];
    CacheLine two_way[2][NUM_LINES / 2];
    CacheLine four_way[4][NUM_LINES / 4];
    CacheLine fully_associative[NUM_LINES][1];

    init_cache(direct_mapped, 1, NUM_LINES);
    init_cache(two_way, 2, NUM_LINES / 2);
    init_cache(four_way, 4, NUM_LINES / 4);
    init_cache(fully_associative, NUM_LINES, 1);

    uint32_t address;
    int direct_mapped_hits = 0, two_way_hits = 0, four_way_hits = 0, fully_associative_hits = 0;
    int total_accesses = 0;

    while (fscanf(file, "%x", &address) == 1) {
        total_accesses++;

        direct_mapped_hits += access_cache(direct_mapped, address, DIRECT_MAPPED, LRU);
        two_way_hits += access_cache(two_way, address, TWO_WAY, LRU);
        four_way_hits += access_cache(four_way, address, FOUR_WAY, LRU);
        fully_associative_hits += access_cache(fully_associative, address, FULLY_ASSOCIATIVE, LRU);
    }

    fclose(file);

    print_stats("Direct-mapped", direct_mapped_hits, total_accesses);
    print_stats("2-way", two_way_hits, total_accesses);
    print_stats("4-way", four_way_hits, total_accesses);
    print_stats("Fully associative", fully_associative_hits, total_accesses);

    return 0;
}

void init_cache(CacheLine cache[][NUM_LINES], int num_sets, int associativity) {
    for (int i = 0; i < num_sets; i++) {
        for (int j = 0; j < associativity; j++) {
            cache[i][j].valid = 0;
            cache[i][j].counter = 0;
        }
    }
}

int access_cache(CacheLine cache[][NUM_LINES], uint32_t address, CacheType type, ReplacementPolicy policy) {
    int num_sets = NUM_LINES;
    int set_bits = 0;

    switch (type) {
    case DIRECT_MAPPED:
        num_sets = NUM_LINES;
        set_bits = 5;
        break;
    case TWO_WAY:
        num_sets = NUM_LINES / 2;
        set_bits = 4;
        break;
    case FOUR_WAY:
        num_sets = NUM_LINES / 4;
        set_bits = 3;
        break;
    case FULLY_ASSOCIATIVE:
        num_sets = 1;
        set_bits = 0;
        break;
    }

    int set = (address >> 2) & ((1 << set_bits) - 1);
    uint32_t tag = address >> (2 + set_bits);

    for (int i = 0; i < NUM_LINES / num_sets; i++) {
        if (cache[set][i].valid && cache[set][i].tag == tag) {
            for (int j = 0; j < NUM_LINES / num_sets; j++) {
                cache[set][j].counter += (j != i);
            }
            cache[set][i].counter = 0;
            return 1;
        }
    }

    int replacement = 0;
    if (policy == RANDOM) {
        replacement = rand() % (NUM_LINES / num_sets);
    } else {
        for (int i = 1; i < NUM_LINES / num_sets; i++) {
            if (!cache[set][i].valid || cache[set][i].counter > cache[set][replacement].counter) {
                replacement = i;
            }
        }
    }

    cache[set][replacement] = (CacheLine){.tag = tag, .valid = 1, .counter = 0};
    for (int i = 0; i < NUM_LINES / num_sets; i++) {
        cache[set][i].counter += (i != replacement);
    }

    return 0;
}

void print_stats(const char *name, int hits, int accesses) {
    printf("%s:\n", name);
    printf(" Hits: %d\n", hits);
    printf(" Total accesses: %d\n", accesses);
    printf(" Hit rate: %.2f%%\n", (float)hits / accesses * 100);
}