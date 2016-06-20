#ifndef API_H
#define API_H
#include "Interpreter.h"

void API_Create_Table(string tablename,FieldTree *T);
void API_Drop_Table(Table &tbl);
void API_Create_Index(IndexStruct I);
void API_Drop_Index(string indexname);
void API_Select(Table tbl, list<Condition> &clist);
void API_Insert(Table tbl, Tuple Element);
void API_Insert_Key_To_Index(IndexInfo idx,string key,int position,int type);
void API_Delete(Table tbl, list<Condition> &clist);
void Index_Select_Position(Condition cond, string indexname, vector<int> &offsetList, int AttrSize, int type);

#endif // API_H
