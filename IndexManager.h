//
//  IndexManager.hpp
//  minisql
//
//  Created by 王皓波 on 16/5/21.
//  Copyright © 2016年 王皓波. All rights reserved.
//

#ifndef IndexManager_hpp
#define IndexManager_hpp

#include "minisql.h"
#include "string"
#include "vector"
#include "BPlus_Tree.h"
#include "IndexManager.h"
#include "CatalogManager.h"
using namespace std;

bool Create_Index_Init(IndexInfo index);
void Create_Index(IndexInfo index);
template<class T>
//Tuple_Addr Find_Index_Equal_Key(T key, IndexInfo index);
void Insert_Key_To_Index(IndexInfo index, Tuple<int> tuple);
template<class T>
void Delete_Key_From_Index(IndexInfo index,T key);
template<class T>
void Find_Key_Offset_Index(IndexInfo index,T key,int cond);

class NoKeyFound_Error{};
class Cond_Error{};

extern vector<Tuple<int>> vec;

//对primary key自动建立的索引
/*
 传入的参数中：
 table{tablename, attr_num<32, attr_lis* -> {attr_name, type, length, extra(null/unique/primary), inline_offset, position} }
 index{indexname, attr_position(easy to fetch), attr_name, table_name ,attr_size, maxKeyNum}
 */

/*template<class T>
Tuple_Addr Find_Index_Equal_Key(T key, IndexInfo index)
{
    node root(index, 0, NULL);
    Tuple_Addr TA;
    try{
        TA = root.Find_Key(key);
        if (TA.Get_Key_Exist() == false){
            NoKeyFound_Error nkfe;
            throw nkfe;
        }
    }
    catch(...){
        cout<<"Find Key Error!"<<endl;
    }
    return TA;
}*/


template<class T>
void Delete_Key_From_Index(IndexInfo index,T key)
{
    node root(index, 0, 0);
    try{
        root.Delete_Key(key);
    }
    catch(...){
        cout<<"Delete Error!"<<endl;
    }
}

template<class T>
void Find_Key_Offset_Index(IndexInfo index,T key,vector<int>offsetlist,int cond)
{
    node root(index, 0, 0);
    vector<Tuple_Addr> vec;
    try{
        if(Condition==EQUAL){
            Tuple_Addr TA=root.Find_Key(key);
            offsetlist.push_back(TA.getTable_Addr());
        }
        /*else if(Condition==NOT_EQUAL){
            //这个不会传入
        }*/
        else if(Condition==LARGER_THAN_AND_EQUAL){
            Find_Larger_Than_Key(key,1,vec);
        }
        else if(Condition==LESS_THAN_AND_EQUAL){
            Find_Less_Than_Key(key,1,vec);
        }
        else if(Condition==LARGER_THAN){
            Find_Larger_Than_Key(key,0,vec);
        }
        else if(Condition==LESS_THAN){
            Find_Less_Than_Key(key,0,vec);
        }
        else {
            Cond_Error ce;
            throw ce;
        }
        for(int i=0;i<vec.size();i++){
            offsetlist.push_back(vec[i].getTable_Addr());
        }
    }
    catch(...){
        cerr<<"Find Error!"<<endl;
    }
}

#endif /* IndexManager_hpp */