//
//  BPlus_Tree.cpp
//  minisql
//
//  Created by 王皓波 on 16/5/22.
//  Copyright © 2016年 王皓波. All rights reserved.
//

#include "BPlus_Tree.h"

//已经在indexmanager中定义
/*
 传入的参数中：
 table{tablename, attr_num<32, attr_lis* -> {attr_name, type, length, extra(null/unique/primary), inline_offset, position} }
 index{indexname, attr_position(easy to fetch), attr_name, table_name ,attr_size, maxKeyNum}
 文件中的结构组织：
 type + keynum + extra + ptr + key + ..... + ptr
 */

INT32 Left_First_Node;

node::node()
{
    block = NULL;
    BlockAddr = 0;
    maxsize = 0;
    keynum = 0;
    parent = 0;
    type = 0;
    parent = NULL;
    extra = 0;
}

node::node(IndexInfo idx, INT32 offset, node* parent)
{
    block = getBlock(index.indexname.c_str(), offset);
    char *values = block->record;
    BlockAddr = offset;
    memcpy(&type, values, 4);
    memcpy(&keynum, values + 4, 4);
    if (IsRoot()){
        this->parent = NULL;
        extra = -1;
        memcpy(&Left_First_Node, block->record + 8, 4);
        //如果是root，则从根结点的extra位置取出叶结点中的最左侧结点
        //注意，任何在root上的操作都可以更新Left_First_Node的值
        //这代表着即使是程序刚启动，对b+树的任何操作都能够取出最左侧叶结点的位置
    }
    else{
        this->parent = parent;
        memcpy(&extra, block->record + 8, 4);
    }
    maxsize = idx.maxKeyNum;
    index = idx;
    
    //构造node结点使为了将一个块抽象化，并且存储这个块的信息，便于程序使用
}

bool node::IsLeaf()
{
    if (type<2)return true;
    else return false;
}

bool node::IsFull()
{
    if (keynum >= maxsize)return true;
    else return false;
}

bool node::IsHalfFull()
{
    int hf;
    if(IsLeaf()){
        hf=maxsize/2;
        if(maxsize%2)hf++;
    }
    else{
        hf=(maxsize+1)/2;
        if((maxsize+1)%2==0)hf--;
        // up_bound(n/2) -1
    }
    if (keynum <= hf)return true;
    else return false;
}

bool node::IsRoot()
{
    if (type == 0 || type == 3)return true;
    else return false;
}

INT32 node::Find_Real_Brother(INT32 blk_ofst,bool &flag)
{
    int position;
    if(flag){
        position = blk_ofst + index.attr_size;
    }
    else{
        cout<<"00A0D0AWIR214"<<endl;
        position = blk_ofst - 8 - index.attr_size;
    }
    INT32 sibling;
    memcpy(&sibling,block->record+position,4);
    return sibling;
}