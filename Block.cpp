#include "Block.h"
#include "iostream"

char blk[10][4096];
int num=7;

Block *getBlock(string filename, int offset)
{
    Block *b = new Block;
    b->record = blk[offset];
    b->BlockAddr = offset;
    return b;
}
Block *getBlock(string filename)
{
    Block *b = new Block;
    b->record = blk[num];
    b->BlockAddr = num--;
    return b;
}
void Release(Block *blk)
{
    memset(blk->record, 0, 4096);
}