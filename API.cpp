#include "minisql.h"
#include "IndexManager.h"
#include "Interpreter.h"
#include "vector"
#include "string"
#include "API.h"
#include "algorithm"

int FetchAllFlag;

void API_Create_Table(string tablename,FieldTree *T)
{
//	try{
		//catalog meta data
		Table tbl(tablename);
		tbl.initializeTable(T);
		tbl.createTable();
		string primaryIdxName=tablename+"PRIMARY_INDEX";
		Index newIndex(primaryIdxName);
		IndexStruct Idx;
		Idx.attr_name = tbl.getPrimaryKey();
		Idx.index_name = primaryIdxName;
		Idx.table_name = tablename;

		newIndex.initializeIndex(&Idx);
		newIndex.createIndex();
		//End of catalog do
		//IndexManager do
		IndexInfo idxInfo;
		idxInfo.indexname = Idx.table_name + "PRIMARY_INDEX";
		idxInfo.attr_size=tbl.getPrimaryKeySize();
		idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+idxInfo.attr_size);
		Create_Index_Init(idxInfo);
		//End of IndexManager do
/*	}
	catch(...){
		cout<<"Failed to Create Table."<<endl;
	}*/
}

void API_Drop_Table(Table &tbl)
{
	vector<string> idxList;
	for(int i=0;i<tbl.getAttrNum();i++){
		string tempstring = tbl.getAttrIndex(tbl.getAttr(i).attr_name);
		if (tempstring.empty()){
			continue;
		}
		Index droppedIndex(tempstring);
		droppedIndex.dropIndex();
	}
	tbl.dropTable();
}
extern vector<TupleIndex_number> TupleList_IDX;
void API_Create_Index(IndexStruct I)
{
	Index idx(I.index_name);
	idx.initializeIndex(&I);
	idx.createIndex();

	Table tb(I.table_name);
	tb.getTableInfo();
	//Catalog do
	Tuple tpl(I.table_name);
	Attribute at = tb.getAttr(I.attr_name);
	tpl.FetchAll(at.attr_id);
	int listSize = TupleList_IDX.size();
	IndexInfo idxInfo;
	idxInfo.attr_size = at.attr_len;
	idxInfo.indexname = I.index_name;
	idxInfo.maxKeyNum = (4096 - 4 - INDEX_BLOCK_INFO) / (4+at.attr_len);
	Create_Index(idxInfo);
	cout << "Query OK, " << listSize;
	if (listSize!=1)cout << " rows affected." << endl;
	else cout << " row affected." << endl;
	//Index do
}

void API_Drop_Index(string indexname)
{
	Index idx(indexname);
	idx.dropIndex();
}

void API_Select(Table tbl,list<Condition> &clist)
{
	if (!tbl.isTableExist()){
		//表不存在
		throw Table_Index_Error("table", tbl.getTableName());
	}
	int flag=0;
	if (clist.empty())flag = 1;
	list<Condition> Have_Index_Condition;
	list<Condition> No_Index_Condition;
	list<Condition>::iterator it;
	string IndexNameList[100];
	int Attribute_Size[100];
	int i=0;
	for(it=clist.begin();it!=clist.end();it++){
		Attribute_Size[i] = tbl.getAttr(it->compare_attr).attr_len;
		IndexNameList[i++] = tbl.getAttrIndex(it->compare_attr);
		if(!IndexNameList[i-1].empty()&&it->compare_type!=NOT_EQUAL){
			Have_Index_Condition.push_back(*it);
		}
		else{
			i--;
			No_Index_Condition.push_back(*it);
		}
	}
	i=0;
	vector<int> finalOfstList;
	vector<int> tempOfstList;
	vector<int> temp2;
	vector<int>::iterator endOfList;
	for(it=Have_Index_Condition.begin();it!=Have_Index_Condition.end();it++){
		if (it == Have_Index_Condition.begin()){
			//cout << IndexNameList[i] << ' ' << Attribute_Size[i] << ' ' << tbl.getAttr((*it).compare_attr).attr_type << endl;
			Index_Select_Position(*it, IndexNameList[i], finalOfstList, Attribute_Size[i], tbl.getAttr((*it).compare_attr).attr_type);
			//Index_Select_Position(*it, IndexNameList[i], tempOfstList, Attribute_Size[i], tbl.getAttr((*it).compare_attr).attr_type);
			i++;
		}
		else{
			cout << IndexNameList[i] << ' ' << Attribute_Size[i] << ' ' << tbl.getAttr((*it).compare_attr).attr_type << endl;
			Index_Select_Position(*it, IndexNameList[i], tempOfstList, Attribute_Size[i], tbl.getAttr((*it).compare_attr).attr_type);
			i++;
			sort(finalOfstList.begin(), finalOfstList.end());
			sort(tempOfstList.begin(), tempOfstList.end());
			endOfList = set_intersection(finalOfstList.begin(), finalOfstList.end(), tempOfstList.begin(), tempOfstList.end(), finalOfstList.begin());
			finalOfstList.resize(endOfList - finalOfstList.begin());
			sort(finalOfstList.begin(), finalOfstList.end());
			finalOfstList.erase(unique(finalOfstList.begin(), finalOfstList.end()), finalOfstList.end());
		}
		if (finalOfstList.empty()){
			cout << "Empty set." << endl;
			return;
		}
		Block::flush_all_blocks();
	}
	Tuple selectTuple(tbl.getTableName());

	if (flag)finalOfstList.push_back(0);//clist为空
	else if (Have_Index_Condition.empty())finalOfstList.push_back(1);//clist不为空，且无索引信息
	else if (finalOfstList.empty())finalOfstList.push_back(2);//正常，但是索引找到的list为空
	else finalOfstList.push_back(3);//正常
	selectTuple.Select(finalOfstList,No_Index_Condition);
	selectTuple.printSelectResult();
}

void API_Insert(Table tbl,Tuple Element)
{
	INT32 position;
	if(Element.Insert(position)==false){
		throw Conflict_Error(tbl.getTableName());
		return;
	}
	//record do
	for(int i=0;i<tbl.getAttrNum();i++){
		IndexInfo idxInfo;
		idxInfo.indexname = tbl.getAttrIndex(tbl.getAttr(i).attr_name);
		if(!idxInfo.indexname.empty()){
			idxInfo.attr_size=tbl.getAttr(i).attr_len;
			idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+idxInfo.attr_size);
			API_Insert_Key_To_Index(idxInfo,Element.getTupleValue(i),position,tbl.getAttr(i).attr_type);
			//index manager do
		}
	}
}

void API_Insert_Key_To_Index(IndexInfo idx,string Key,int position,int type)
{
	TupleIndex Ti;
	Ti.key = Key;
	Ti.position = position;
	Ti.type = type;
	Insert_Key_To_Index(idx,Ti);
}

void API_Delete(Table tbl, list<Condition> &clist)
{
	int flag = 0;
	if (clist.empty())flag = 1;
	list<Condition> Have_Index_Condition;
	list<Condition> No_Index_Condition;
	list<Condition>::iterator it;
	string IndexNameList[100];
	int Attribute_Size[100];
	int i=0;
	for(it=clist.begin();it!=clist.end();it++){
		Attribute_Size[i] = tbl.getAttr(it->compare_attr).attr_len;
		IndexNameList[i++] = tbl.getAttrIndex(it->compare_attr);
		if(!IndexNameList[i-1].empty()&&it->compare_type!=NOT_EQUAL){
			Have_Index_Condition.push_back(*it);
		}
		else{
			i--;
			No_Index_Condition.push_back(*it);
		}
	}
	i=0;
	vector<int> finalOfstList;
	vector<int> tempOfstList;
	vector<int>::iterator endOfList;
	for(it=Have_Index_Condition.begin();it!=Have_Index_Condition.end();it++){
		if (it == Have_Index_Condition.begin()){
			Index_Select_Position(*it, IndexNameList[i], finalOfstList, Attribute_Size[i], tbl.getAttr((*it).compare_attr).attr_type);
			i++;
		}
		else{
			Index_Select_Position(*it,IndexNameList[i],tempOfstList,Attribute_Size[i],tbl.getAttr((*it).compare_attr).attr_type);
			i++;
			sort(finalOfstList.begin(), finalOfstList.end());
			sort(tempOfstList.begin(), tempOfstList.end());
			endOfList = set_intersection(finalOfstList.begin(), finalOfstList.end(), tempOfstList.begin(), tempOfstList.end(), finalOfstList.begin());
			finalOfstList.resize(endOfList - finalOfstList.begin());
			sort(finalOfstList.begin(), finalOfstList.end());
			finalOfstList.erase(unique(finalOfstList.begin(), finalOfstList.end()), finalOfstList.end());
		}
		if(finalOfstList.empty()){
			cout << "Query OK, 0 rows affected." << endl;
			return;
		}
	}
	Tuple selectTuple(tbl.getTableName());

	if (flag)finalOfstList.push_back(0);//clist为空
	else if (Have_Index_Condition.empty())finalOfstList.push_back(1);//clist不为空，且无索引信息
	else if (finalOfstList.empty())finalOfstList.push_back(2);//正常，但是索引找到的list为空
	else finalOfstList.push_back(3);//正常
	int deleteNum=selectTuple.Delete(finalOfstList, No_Index_Condition);
	cout << "Query OK, "<< deleteNum <<" rows affected." << endl;
}

void Index_Select_Position(Condition cond,string indexname,vector<int> &offsetList,int AttrSize,int type)
{
	IndexInfo idxInfo;
	idxInfo.indexname=indexname;
	idxInfo.attr_size=AttrSize;
	idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+idxInfo.attr_size);
	if(type ==INT){
		int key;
		key=atoi(cond.compare_value.c_str());
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
	else if(type==FLOAT){
		float key;
		key = atof(cond.compare_value.c_str());
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
	else if(type==CHAR){
		char temp[256];
		memset(temp, 0, 256);
		memcpy(&temp, cond.compare_value.c_str(), AttrSize);
		string key(temp);
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
}
