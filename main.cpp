//
//  main.cpp
//  minisql
//
//  Created by 王皓波 on 16/5/21.
//  Copyright © 2016年 王皓波. All rights reserved.
//

#include <iostream>
#include "IndexManager.h"
#include "BPlus_Tree.h"
using namespace std;
vector<Tuple<int>> vec;
extern char blk[8][4096];

void Print_Block(IndexInfo index,int k);

int main(int argc, const char * argv[]) {
    // insert code here...
    cout << "Hello, World!\n";
    Tuple<int> temp;
    int n;
    cin>>n;
    for (int i = 1; i <= n; i++){
        temp.key=i;
        temp.position=100-i;
        //cin>>temp.key>>temp.position;
        vec.push_back(temp);
    }
    IndexInfo index;
    index.attr_size = sizeof(int);
    index.indexname = "HB_idx";
    index.maxKeyNum = (BLOCKSIZE - INDEX_BLOCK_INFO - 4) / (4 + index.attr_size);
    Create_Index(index);
    
    cout<<"Insert: ---------------------------"<<endl;
    for(int i=0;i<8;i++)Print_Block(index, i);
    
    for(int i=0;i<4;i++){
        int key;
        cin>>key;
        Delete_Key_From_Index(index, key);
        cout<<"Delete:----------------------------"<<endl;
        for(int j=0;j<8;j++)Print_Block(index, j);
    }
    
    temp.key = 15;
    temp.position = 8889;
    try{
        Insert_Key_To_Index(index, temp);
    }
    catch (...){
        cout << "Some Error" << endl;
    }
    //验证错误插入
    
    Tuple_Addr TA;
    TA = Find_Index_Equal_Key(8,index);
    cout << TA.getIBA() << ' ' << TA.getIO() << ' ' << TA.getTable_Addr() << endl;
    //system("pause");
    return 0;
}

void Print_Block(IndexInfo index,int k)
{
    cout<<"----------------------- "<<k<<" -----------------------"<<endl;
    int type, keynum;
    memcpy(&type,blk[k],4);
    memcpy(&keynum, blk[k]+4, 4);
    int ofst = INDEX_BLOCK_INFO;
    cout << type << ' ' << keynum << ' ' << index.maxKeyNum << endl;
    for (int i = 0; i < keynum; i++){
        int key,position;
        memcpy(&key, blk[k]+ofst,4);
        memcpy(&position, blk[k] + ofst+4, 4);
        ofst += 8;
        cout << key <<' ' << position << endl;
    }
    //验证插入模块是否正确
    int next;
    memcpy(&next,blk[k]+ofst,4);
    cout << next << endl;
}