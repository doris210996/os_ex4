#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <string>
#include <array>
#include <vector>
#include <stack>
#define SUCCESS 1
#define FAIL 0
#define PAGE_INDEX virtualAddress >> OFFSET_WIDTH
#define CURR_OFFSET virtualAddress >> ((TABLES_DEPTH - depth) * OFFSET_WIDTH)
#define CURR_PHYSICAL_ADDRESS (frameNum) * (PAGE_SIZE) + (CURR_OFFSET)
#define IS_PAGE depth == TABLES_DEPTH-1
#define IS_LEAF w
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

static const int  ROOT_VAL = 0;


uint64_t offset(uint64_t address){
    return address & ((1 << OFFSET_WIDTH) - 1);
}


uint64_t cyclic(uint64_t curPage, uint64_t vPage) {
    uint64_t diff =  vPage - curPage;
    return (diff < NUM_PAGES - diff) ? (diff):(NUM_PAGES-diff);
}


bool inPath(Path path, word_t frameIndex) {
    for (int i = 0; i < TABLES_DEPTH; ++i){
        if (path[i] == frameIndex){
            return true;
        }
    }
    return false;
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

void DFSUtil(Node currNode,FirstChoice choice,Victim currVictim,int virtualPageIndex){
    word_t w = 0;
    PMread(currNode.frameNum*PAGE_SIZE+ currNode.offset, &w);
    if (w > *(choice.maxFrame)) { // update the current max frame
        *(choice.maxFrame) = w;
    }
    if (!w) {
        return;
    }
    // check if the frame contains all zeros
    if (!inPath(currNode.path, w) && currNode.depth < TABLES_DEPTH && allZeroFrame(w) ) {
        PMwrite(currNode.frameNum*PAGE_SIZE+currNode.offset, 0);
        *(choice.emptyFrame) = w;
        return;
    }
    if (currNode.depth == TABLES_DEPTH) {
        uint64_t curDist = cyclic(virtualPageIndex, currNode.valPage);
        if (curDist > *(currVictim.maxCyclicDist)) {
            *(currVictim.maxCyclicDist) = curDist;
            *(currVictim.cyclicDistFrame) = w;
            *(currVictim.parentOfCyclicDistFrame)= currNode.frameNum*PAGE_SIZE+currNode.offset;
            *(currVictim.pageOfCyclicDistFrame) = virtualPageIndex;
        }
        return;
    }
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        currNode.frameNum = w;
        currNode.offset=i;
        currNode.valPage = ((currNode.valPage)<<(OFFSET_WIDTH))+(currNode.offset);
        currNode.depth += 1;
        DFSUtil(currNode,choice,currVictim,virtualPageIndex);
    }
}

//void DFS(Victim currVictim, FirstChoice choice, Node currNode,int virtualPageIndex)
//{
//    for (int i = 0; i < PAGE_SIZE; ++i)
//    {
//        currNode.offset=i;
//        currNode.valPage = ((currNode.valPage)<<(OFFSET_WIDTH))+(currNode.offset);
//        currNode.depth += 1;
//        DFSUtil(currNode,choice,currVictim,virtualPageIndex);
//    }
//}

word_t getFrame(Path path, uint64_t pageIndex){
    int pageOfCyclicDistFrame =0;
    int cyclicDistFrame = 0;
    int maxCyclicDist=0;
    int parentOfCyclicDistFrame=0;
    Victim currVictim = Victim{&pageOfCyclicDistFrame,& cyclicDistFrame,&maxCyclicDist,&parentOfCyclicDistFrame};
    int emptyFrame=0;
    int maxFrame =0;
    FirstChoice choice= FirstChoice{&emptyFrame,&maxFrame};
    Node currNode = Node{path,0,0,1,(ROOT_VAL) << OFFSET_WIDTH};

    if(*(choice.emptyFrame))
    {
        return *(choice.emptyFrame);
    }
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        currNode.offset=i;
        currNode.valPage = ((currNode.valPage)<<(OFFSET_WIDTH))+i;
        currNode.depth += 1;
        DFSUtil(currNode,choice,currVictim,pageIndex);
    }
    if (!(*(choice.emptyFrame)) && (*(choice.maxFrame) + 1 < NUM_FRAMES)){
        return maxFrame+1;
    }
    PMwrite(*(currVictim.parentOfCyclicDistFrame), 0);
    PMevict(cyclicDistFrame,*(currVictim.cyclicDistFrame));
    for (int i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(*(currVictim.cyclicDistFrame)*PAGE_SIZE + i, 0);
    }
    return *(currVictim.cyclicDistFrame);
}




uint64_t findAddress(uint64_t frameNum, uint64_t virtualAddress, int depth, Path path){
    // Always starting from the root
    word_t physicalContent = ROOT_VAL;
    PMread(CURR_PHYSICAL_ADDRESS, &physicalContent);
    // It means there is no table in the next layer->get frame
    if (physicalContent == ROOT_VAL)
    {
        physicalContent = getFrame(path, PAGE_INDEX);
        // Linking
        PMwrite(CURR_PHYSICAL_ADDRESS, physicalContent);
        // We got to a page
        if (IS_PAGE)
        {
            PMrestore(physicalContent, PAGE_INDEX);
            return physicalContent;
        }
    }
    // There was a table or we created one , so we get inside it (to the next layer)
    path.at(depth) = physicalContent;
    return findAddress((uint64_t)physicalContent, virtualAddress, depth + 1, path);
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
    if(isValidAddress(virtualAddress) ==FAIL)
    {
        return FAIL;
    }
    Path path;
    uint64_t physicalAdd = findAddress(ROOT_VAL, virtualAddress, 0, path);
    PMread(physicalAdd*PAGE_SIZE + offset(virtualAddress),  value);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if(isValidAddress(virtualAddress) == FAIL)
    {
        return FAIL;
    }
    Path path;
    uint64_t p_address = findAddress(ROOT_VAL,virtualAddress,0,path);
    PMwrite(p_address*PAGE_SIZE + offset(virtualAddress),value);
    return SUCCESS;
}
