//
//  IndexManager.cpp
//  minisql
//
//  Created by 王皓波 on 16/5/21.
//  Copyright © 2016年 王皓波. All rights reserved.
//

#include "IndexManager.h"

bool Create_Index_Init(IndexInfo index)
{
    Block *block;
    block = getBlock(index.indexname, 0);//要看getblock需要什么样的参数
    if (block == NULL)return false;
    char *init_info = block->record;
    int type = 0;
    int keynum = 0;
    int parent = 0;
    memset(init_info, 0, BLOCKSIZE);
    memcpy(init_info, &type, 4);//root with no son
    memcpy(init_info + 4, &keynum, 4);//store key number of each block
    memcpy(init_info + 8, &parent, 4);
    return true;
}

void Create_Index(IndexInfo index)
{
    Create_Index_Init(index);
    for (int i = 0; i<int(vec.size()); i++){
        node root(index, 0, 0);
        if (root.getBlock_Ptr()->record == NULL)root.setBlock(getBlock(index.indexname.c_str(), 0));//可能在insert过程中root的块被收回了
        root.Insert_Key((*vec)[i].key, (*vec)[i].position);//每次都从根结点开始插入
        //cout << i << endl;
    }
}

void Insert_Key_To_Index(IndexInfo index, Tuple<int> tuple)
{
    node root(index, 0, 0);
    try{
        root.Insert_Key(tuple.key, tuple.position);
    }
    catch(...){
        cout<<"Insert Error!"<<endl;
    }
}

