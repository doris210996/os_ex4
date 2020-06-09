#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <string>
#include <array>
#define SUCCESS 1
#define FAIL 0
#define PAGE_NUM originalAddress >> OFFSET_WIDTH
using namespace std;
typedef array<int, TABLES_DEPTH> Path;

uint64_t offset(uint64_t address){
    return address & ((1 << OFFSET_WIDTH) - 1);
}


uint64_t findAddress(uint64_t frameNum, uint64_t originalAddress, int depth, Path path){
    word_t content = 0; // Always starting from the root
    uint64_t currOffset = offset(originalAddress >> ((TABLES_DEPTH- depth)*OFFSET_WIDTH));
    uint64_t pageNum = PAGE_NUM;
    PMread(frameNum * PAGE_SIZE + currOffset, &content);
    if (content == 0) // It means there is no table in the next layer we should create a frame
    {


    }
    // There was a table so we get inside it (to the next layer)
    path[depth] = content;
    return findAddress((uint64_t)content, originalAddress, depth+1, path);
}

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}

int isValidAddress(uint64_t virtualAddress)
{
    if (to_string(virtualAddress).size() != to_string(VIRTUAL_ADDRESS_WIDTH).size())
    {
        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}






int VMread(uint64_t virtualAddress, word_t* value) {
    if(!isValidAddress(virtualAddress))
    {
        return FAIL;
    }
    Path path;
    uint64_t physicalAdd = findAddress(0, virtualAddress, 0, path);
    PMread(physicalAdd*PAGE_SIZE + offset(virtualAddress),  value);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if(!isValidAddress(virtualAddress))
    {
        return FAIL;
    }
    Path path;
    uint64_t p_address = findAddress(0,virtualAddress,0,path);
    PMwrite(p_address*PAGE_SIZE + offset(virtualAddress),value);
    return SUCCESS;
}
