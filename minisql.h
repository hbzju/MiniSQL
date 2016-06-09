//
//  minisql.h
//  minisql
//
//  Created by 王皓波 on 16/5/26.
//  Copyright © 2016年 王皓波. All rights reserved.
//
#ifndef minisql_h
#define minisql_h

#include "string"
#include "iostream"
using namespace std;

#define INT32 unsigned int
#define BLOCKSIZE 40
#define INDEX_BLOCK_INFO 12

typedef struct {
    string indexname;
    int attr_size;
    int maxKeyNum;
}IndexInfo;

template <class T>
class Tuple
{
public:
    T key;
    INT32 position;
};

class Tuple_Addr
{
private:
    INT32 Index_Block_Addr;
    INT32 Index_Offset;
    INT32 Table_Addr;
    bool Key_Exist;
public:
    Tuple_Addr()
    {
        Index_Block_Addr = 0;
        Index_Offset = 0;
        Table_Addr = 0;
        Key_Exist = 0;
    }
    Tuple_Addr(INT32 IBA, INT32 IO, INT32 TA, bool KE)
    :Index_Block_Addr(IBA), Index_Offset(IO), Table_Addr(TA), Key_Exist(KE){}
    bool Get_Key_Exist(){ return Key_Exist; }
    INT32 getIBA(){ return Index_Block_Addr; }
    INT32 getIO(){ return Index_Offset; }
    INT32 getTable_Addr(){ return Table_Addr; }
    ~Tuple_Addr(){}
};

#endif /* minisql_h */
