//
//  Block.hpp
//  minisql
//
//  Created by 王皓波 on 16/5/28.
//  Copyright © 2016年 王皓波. All rights reserved.
//
#ifndef Block_hpp
#define Block_hpp

#include <string>
using namespace std;

typedef struct  {
    char *record;
    int BlockAddr;
}Block;

Block *getBlock(string filename, int offset);
Block *getBlock(string filename);
void Release(Block *blk);
#endif /* Block_hpp */