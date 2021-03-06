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
static const int ROOT_VAL = 0;
typedef array<int, TABLES_DEPTH> Path;
typedef struct
{
    int *victimDist;
    int *victim;
    int *victimPage;
    int *victimParent;
} Victim;

word_t jumpToAddress(int frameNum,int offset){
    return frameNum*PAGE_SIZE+offset;
}

uint64_t advanceVirtualPath(uint64_t currVPath,int suffix){
    return (currVPath << OFFSET_WIDTH) + suffix;
}

void newTable(const Victim &victim)
{
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        PMwrite(jumpToAddress(*(victim.victim),i), 0);
    }
}

bool visited(Path path, word_t frameNum)
{
    for (int frame:path)
    {
        if (frame == frameNum)
        {
            return true;
        }
    }
    return false;
}

uint64_t getDistance(uint64_t curPage, uint64_t vPage)
{
    uint64_t distance = vPage - curPage;
    if (NUM_PAGES - distance <= distance)
    {
        return NUM_PAGES - distance;
    }
    else
    {
        return NUM_PAGES;
    }
}

void updateVictim(uint64_t curFrame, uint64_t offset, uint64_t pageNum, const Victim *victim,
                  uint64_t pagePerPath, word_t curAdd)
{
    uint64_t curDist = getDistance(pagePerPath, pageNum);
    if (curDist > *(victim->victimDist))
    {
        *(victim->victimDist) = curDist;
        *(victim->victim) = curAdd;
        *(victim->victimParent) = jumpToAddress(curFrame,offset);
        *(victim->victimPage) = pagePerPath;
    }
}


void DFSUtill(uint64_t curFrame, uint64_t offset, int depth, Path myPath, uint64_t pageNum, word_t *
maxFrame, word_t *emptyFrame, Victim *victim, uint64_t pagePerPath)
{
    if (*emptyFrame != 0)
    {
        return;
    }
    word_t curAdd = 0;
    PMread(jumpToAddress(curFrame,offset), &curAdd);
    if (curAdd != 0)
    {
        // Update max frame index
        if (curAdd > *maxFrame)
        {
            *maxFrame = curAdd;
        }

        if (depth == TABLES_DEPTH)
        {
            updateVictim(curFrame, offset, pageNum, victim, pagePerPath, curAdd);
            return;
        }

        // Check if the current frame is empty
        if (!visited(myPath, curAdd))
        {
            word_t temp = 0;
            for (uint64_t i = 0; i < PAGE_SIZE; i++)
            {
                PMread(jumpToAddress(curAdd,i), &temp);
                if (temp != 0)
                {
                    break;
                }
                if (i == PAGE_SIZE-1)
                {
                    PMwrite(jumpToAddress(curFrame,offset), 0);
                    *emptyFrame = curAdd;
                    return;
                }
            }
        }

        for (int i = 0; i < PAGE_SIZE; i++)
        {
            DFSUtill(curAdd, i, depth + 1, myPath, pageNum, maxFrame, emptyFrame, victim,
                     (advanceVirtualPath(pagePerPath,i)));
        }
    }
}


void DFS(uint64_t fullAdd, Path myPath, word_t *maxFrame, word_t *emptyFrame, Victim *victim,
         uint64_t pagePerPath)
{
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        DFSUtill(0, i, 1, myPath, fullAdd, maxFrame, emptyFrame, victim,
                 (advanceVirtualPath(pagePerPath,i)));
    }
}


void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize()
{
    clearTable(0);
}


uint64_t getOffset(uint64_t address)
{
    return address & ((1 << OFFSET_WIDTH) - 1);
}


word_t newFrame(int cyclicDistFrame, const Victim &victim)
{
    PMwrite(*(victim.victimParent), 0);
    PMevict(cyclicDistFrame, *(victim.victimPage));
    newTable(victim);
    return *(victim.victim);
}


word_t createFrame(Path path, uint64_t pageIndex)
{
    uint64_t pagePerPath = 0; // will be build during the DFS (0 ,1,2,3.. and etc. depends on sizes)
    word_t maxFrame = 0;
    word_t emptyFrameRet = 0;
    int maxCyclicDist = 0;
    int cyclicDistFrame = 0;
    int pageOfCyclicDistFrame = 0;
    int parentOfCyclicDistFrame = 0;
    Victim victimDetails = Victim{&maxCyclicDist, &cyclicDistFrame, &pageOfCyclicDistFrame,
                                  &parentOfCyclicDistFrame};
    DFS(pageIndex, path, &maxFrame, &emptyFrameRet, &victimDetails, pagePerPath);
    if (emptyFrameRet != 0)
    {
        return emptyFrameRet;
    }
    if ((maxFrame + 1 < NUM_FRAMES))
    {
        return maxFrame + 1;
    }
    return newFrame(cyclicDistFrame, victimDetails);
}


uint64_t getFrame(uint64_t curFrame, uint64_t fullAdd, int depth, Path path)
{
    word_t entryVal = ROOT_VAL;
    PMread(CURR_P_ADDRESS, &entryVal);

    // No fit frame
    if (entryVal == ROOT_VAL)
    {
        entryVal = createFrame(path, PAGE_INDEX);

        // link the created  frame to its fit parent
        PMwrite(CURR_P_ADDRESS, entryVal);
    }

    if (!(IS_PAGE))
    {
        path.at(depth) = entryVal;
        return getFrame((uint64_t) entryVal, fullAdd, depth + 1, path);
    }

    else
    {
        PMrestore(entryVal, PAGE_INDEX);
        return entryVal;
    }


}

int isValidAddress(uint64_t virtualAddress)
{
    if (virtualAddress >> VIRTUAL_ADDRESS_WIDTH == 0)
    {
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}

int VMread(uint64_t virtualAddress, word_t *value)
{
    if (isValidAddress(virtualAddress))
    {
        Path path;
        uint64_t physicalAdd = getFrame(ROOT_VAL, virtualAddress, 0, path);
        PMread(jumpToAddress(physicalAdd,getOffset(virtualAddress)), value);
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (isValidAddress(virtualAddress))
    {
        Path path;
        uint64_t physicalAdd = getFrame(ROOT_VAL, virtualAddress, 0, path);
        PMwrite(jumpToAddress(physicalAdd,getOffset(virtualAddress)), value);
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}
