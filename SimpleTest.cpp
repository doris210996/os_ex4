#include "VirtualMemory.h"
#include <cstdio>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    int x= 0;
    if(!x)
    {
        std::cout<<"the table size is : "<<TABLES_DEPTH<<"\n";
        std::cout<<"the page size is : "<<PAGE_SIZE<<"\n";

    }
    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        printf("writing to %llu\n", (long long int) i);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        word_t value;
        VMread(5 * i * PAGE_SIZE, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}
