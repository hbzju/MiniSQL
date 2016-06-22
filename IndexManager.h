#ifndef IndexManager_hpp
#define IndexManager_hpp

#include "minisql.h"
#include "string"
#include "vector"
#include "BPlus_Tree.h"
#include "IndexManager.h"
#include "Interpreter.h"
using namespace std;

bool Create_Index_Init(IndexInfo index);
void Create_Index(IndexInfo index);
void Insert_Key_To_Index(IndexInfo index, TupleIndex tuple);
template<class T>
void Delete_Key_From_Index(IndexInfo index,T key);
template<class T>
void Find_Key_Offset_Index(IndexInfo index, T key, vector<int> &offsetlist, string cond);

class NoKeyFound_Error{};
class Cond_Error{};

template<class T>
void Delete_Key_From_Index(IndexInfo index, T key)
{
    node root(index, 0, 0);
    try{
        root.Delete_Key(key);
    }
    catch (...){
        cout << "Delete Error!" << endl;
    }
}

template<class T>
void Find_Key_Offset_Index(IndexInfo index,T key,vector<int> &offsetlist,string cond)
{
    node root(index, 0, 0);
    vector<Tuple_Addr> vec;
    try{
        if(cond==EQUAL){
			//cout << key << endl;
            Tuple_Addr TA=root.Find_Key(key);
			if(TA.Get_Key_Exist())offsetlist.push_back(TA.getTable_Addr());
        }
        else if(cond == LARGER_THAN_AND_EQUAL){
            root.Find_Larger_Than_Key(key,1,vec);
        }
        else if(cond == LESS_THAN_AND_EQUAL){
            root.Find_Less_Than_Key(key,1,vec);
        }
        else if(cond == LARGER_THAN){
			root.Find_Larger_Than_Key(key, 0, vec);
        }
        else if(cond == LESS_THAN){
			root.Find_Less_Than_Key(key, 0, vec);
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

#endif