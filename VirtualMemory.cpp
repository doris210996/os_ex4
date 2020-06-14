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

void DFSUtill(uint64_t curFrame,uint64_t offset, int depth,Path myPath,uint64_t pageNum,word_t*
maxFrame){
    word_t curAdd =0;
    PMread(curFrame*PAGE_SIZE+offset, &curAdd);

    if(curAdd>*maxFrame)
    {
        *maxFrame = curAdd;
    }

    if(!curAdd || depth==TABLES_DEPTH)
    {
        printf("%d\n",curAdd);
        return;
    }
    printf("%d\n",curAdd);
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        DFSUtill(curAdd,i,depth+1,myPath,pageNum,maxFrame);
    }



}
void DFS(uint64_t curFrame, uint64_t fullAdd, int depth, Path myPath,word_t* maxFrame){
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        DFSUtill(0, i, 1, myPath, fullAdd,maxFrame);
    }
}


void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}


void VMinitialize() {
    clearTable(0);
}


uint64_t offset(uint64_t address) {
    return address & ((1 << OFFSET_WIDTH) - 1);
}


bool visitCheck(Path path, word_t frameId) {
    for (int i:path)
    {
        if (path[i] == frameId){
            return true;
        }
    }
    return false;
}


uint64_t getDistance(uint64_t curPage, uint64_t vPage) {
    uint64_t diff =  vPage - curPage;
    return (diff < NUM_PAGES - diff) ? (diff):(NUM_PAGES-diff);
}




word_t getFrame(Path path, uint64_t pageIndex)
{
    word_t maxFrame =0;
    DFS(0,pageIndex,1,path,&maxFrame);
    if ( (maxFrame + 1 < NUM_FRAMES))
    {
        return maxFrame+1;
    }
    return 0;
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
    if (!isValidAddress(virtualAddress))
    {
        return FAIL;
    }
    Path path;
    uint64_t physicalAdd = findAddress(ROOT_VAL, virtualAddress, 0, path);
    PMread(physicalAdd*PAGE_SIZE + offset(virtualAddress), value);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!isValidAddress(virtualAddress))
    {
        return FAIL;
    }
    Path path;
    uint64_t p_address = findAddress(ROOT_VAL,virtualAddress,0,path);
    PMwrite(p_address*PAGE_SIZE + offset(virtualAddress), value);
    return SUCCESS;
}