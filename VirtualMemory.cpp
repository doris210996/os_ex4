#include "PhysicalMemory.h"
#include "MemoryConstants.h"
#include <array>
#define SUCCESS 1
#define FAIL 0
#define PAGE_INDEX fullAdd >> OFFSET_WIDTH
#define CURR_OFFSET (fullAdd >> ((TABLES_DEPTH- depth)*OFFSET_WIDTH) )& ((1 << OFFSET_WIDTH) - 1)
#define CURR_P_ADDRESS curFrame*PAGE_SIZE + (CURR_OFFSET)
#define IS_PAGE depth == TABLES_DEPTH-1
using namespace std;
typedef array<int, TABLES_DEPTH> Path;
static const int  ROOT_VAL = 0;
using namespace std;
typedef array<int, TABLES_DEPTH> Path;
typedef struct{
    int *maxCyclicDist;
    int *cyclicDistFrame;
    int *pageOfCyclicDistFrame;
    int *parentOfCyclicDistFrame;
}Victim;

typedef struct{
    word_t *emptyFrame;
    word_t *maxFrame;
}FirstChoice;

typedef struct{
    Path path;
    uint64_t frameNum;
    uint64_t offset;
    int depth;
    uint64_t valPage;
}Node;



void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/*
 * Initialize the virtual memory
 */
void VMinitialize() {
    clearTable(0);
}


uint64_t getOffset(uint64_t address) {
    return address & ((1 << OFFSET_WIDTH) - 1);
}


bool inPath(Path path, word_t frameId) {
    for (int i = 0; i < TABLES_DEPTH; ++i){
        if (path[i] == frameId){
            return true;
        }
    }
    return false;
}


uint64_t cyclic(uint64_t curPage, uint64_t vPage) {
    uint64_t diff =  vPage - curPage;
    return (diff < NUM_PAGES - diff) ? (diff):(NUM_PAGES-diff);
}


uint64_t allZeroFrame(uint64_t frameInd) {
    word_t w = 0;
    bool foundFrame = true;
    for (uint64_t offset = 0; offset < PAGE_SIZE; ++offset) {
        PMread(frameInd*PAGE_SIZE + offset, &w);
        if (w != 0) {
            foundFrame = false;
            break;
        }
    }
    if (foundFrame) {
        return frameInd;
    }
    return 0;
}

word_t getFrame(Path path, uint64_t pageIndex)
{
}




uint64_t findAddress(uint64_t curFrame, uint64_t fullAdd, int depth, Path myPath) {
    word_t curAdd = ROOT_VAL;
    PMread(CURR_P_ADDRESS, &curAdd);
    if (curAdd == ROOT_VAL)
    {
        curAdd = getFrame(myPath, PAGE_INDEX);
        PMwrite(CURR_P_ADDRESS, curAdd);
    }
    if (IS_PAGE)
    {
        PMrestore(curAdd, PAGE_INDEX);
        return curAdd;
    }
    myPath.at(depth) = curAdd;
    return findAddress((uint64_t) curAdd, fullAdd, depth+1, myPath);
}

int isValidAddress(uint64_t virtualAddress)
{
    if (virtualAddress >> VIRTUAL_ADDRESS_WIDTH != 0)
    {
        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}

int VMread(uint64_t virtualAddress, word_t* value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0))
    {
        return FAIL;
    }
    Path path;
    uint64_t physicalAdd = findAddress(ROOT_VAL, virtualAddress, 0, path);
    PMread(physicalAdd*PAGE_SIZE + getOffset(virtualAddress),  value);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0)) {
        return FAIL;
    }
    Path path;
    uint64_t p_address = findAddress(ROOT_VAL,virtualAddress,0,path);
    PMwrite(p_address*PAGE_SIZE + getOffset(virtualAddress),value);
    return SUCCESS;
}