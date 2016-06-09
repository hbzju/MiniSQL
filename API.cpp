#include "Block.h"
#include "CatalogManager.h"
#include "minisql.h"
#include "IndexManager.h"
#include "vector"
#include "string"
#include "API.h"

vector<TupleAndPosition<int>> vec_int;
vector<TupleAndPosition<double>> vec_double;
vector<TupleAndPosition<string>> vec_str;
int FetchAllFlag;

#define EQUAL "=="
#define NOT_EQUAL "<>"
#define LARGER_THAN ">"
#define LARGER_THAN_AND_EQUAL ">="
#define LESS_THAN "<"
#define LESS_THAN_AND_EQUAL "<="

void API_Create_Table(TableInfo &tbl)
{
//	try{
		//catalog meta data
		Table newTable(tbl.tablename);
		newTable.initializeTable(tbl.createTableCommand);
		newTable.createTable();
		string primaryIdxName=tbl.name+"PRIMARY_INDEX";
		Index newIndex(primaryIdxName,tbl.tablename);
		newIndex.initializeIndex(tbl.createIndexCommand);
		newIndex.createIndex();
		//End of catalog do
		//IndexManager do
		IndexInfo idxInfo;
		idxInfo.indexname=tbl.getPrimaryKey();
		idxInfo.attr_size=tbl.getPrimaryKeySize();
		//可以用宏定义替换掉
		idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+index.attr_size);
		Create_Index_Init(index);
		//End of IndexManager do
/*	}
	catch(...){
		cout<<"Failed to Create Table."<<endl;
	}*/
}

void API_Drop_Table(TableInfo &tbl)
{
	Table droppedTable(tbl.tablename);
	droppedTable.initializeTable(tbl.createTableCommand);
	droppedTable.dropTable();
	vector<string> idxList;
	droppedTable.getIndexList(idxList);
	for(int i=0;i<idxList.size();i++){
		Index droppedIndex(idxList[i],tbl.tablename);
		droppedIndex.dropIndex();
	}
	//catalog do,已经删除元数据／index数据／reco数据
}

void API_Create_Index(IndexInfo &idx,Table &tbl)
{
	Index newIndex(idx.indexname,tbl.tablename);
	newIndex.createIndex();
	//Catalog do
	tbl.Fetch_All();
	Create_Index(idx);
	//Index do
}

void API_Drop_Index(string indexname)
{
	Index idx(indexname);
	idx.dropIndex();
}

void API_Select(Table tbl,list<Condition> clist)
{
	list<Condition> Have_Index_Condition;
	list<Condition> &Node_Index_Condition=clist;
	list<Condition>::iterator it;
	string IndexNameList[100];
	int i=0;
	for(it=clist.begin();it!=clist.end();it++){
		IndexNameList[i++]=tbl.IsIndexExist(it->compare_attr);
		//从catalog中查找该属性是否有对应的index
		if(!IndexNameList.empty()&&it->compare_type!=NOT_EQUAL){
			Have_Index_Condition.push_back(*it);
			Node_Index_Condition.pop(*it);
		}
		else i--;
		//如果不等于而且在索引上的话，这样的索引查找没有意义，归为无索引查找
		//无索引查找引起索引名列表的下标减1
	}
	//获得condition列表中有索引的属性值
	i=0;
	vector<int> finalOfstList;
	vector<int> tempOfstList;
	vector<int>::iterator endOfList;
	for(it=Have_Index_Condition.begin();it!=Have_Index_Condition.end();it++){
		Index_Select_Position(*it,IndexNameList[i++],tempOfstList);
		//获得相应的offset列表，和已有的offset列表进行
		sort(finalOfstList.begin(),finalOfstList.end());
		sort(tempOfstList.begin(),tempOfstList.end());
		endOfList=set_intersection(finalOfstList.begin(),finalOfstList.end(),tempOfstList.begin(),tempOfstList.end(),finalOfstList.begin());
		finalOfstList.resize(endOfList- finalOfstList.begin());
		sort(finalOfstList.begin(),finalOfstList.end());
		unique(finalOfstList.begin(),finalOfstList.end());
		finalOfstList.erase(finalOfstList.begin(),finalOfstList.end());
		//清除列表中没有的项
		if(finalOfstList.empty()){
			cout<<"Empty tuple!"<<endl;
			return;
		}
	}
	Tuple selectTuple(tbl.tablename);
	selectTuple.select(finalOfstList,Node_Index_Condition);
}

void API_Insert(Table tbl,Tuple Element)
{
	int positon;
	if(Element.insert(positon)==false){
		cerr<<"Conflict Tuple Key!"<<endl;
		return;
	}
	//record do
	for(int i=0;i<tbl.getAttrNum();i++){
		IndexInfo idxInfo;
		idxInfo.indexname=tbl.IsIndexExist(i);		//isindexexist的重载形式，以下标形式访问
		if(!idxInfo.indexname.empty()){
			idxInfo.attr_size=tbl.getAttr(i).attr_len;
			idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+index.attr_size);
			API_Insert_Key_To_Index(idxInfo,Element[i],position,);
			//index manager do
		}
	}
}

void API_Insert_Key_To_Index(IndexInfo idx,string key,int position,int type)
{
	if(idx.attr_type==INT){
		int key;
		TupleAndPosition<int> tpl;
		tpl.position=position;
		tpl.key=key;
		getElement(&key,Element[insertedKey].c_str(),idx.attr_size);
		//TA=Find_Index_Equal_Key(key,insertedIndexInfo);
		Insert_Key_To_Index(idx,tpl);
	}
	else if(insertedKey.attr_type==DOUBLE){
		double key;
		TupleAndPosition<double> tpl;
		tpl.position=position;
		tpl.key=key;
		getElement(&key,Element[insertedKey].c_str(),idx.attr_size);
		Insert_Key_To_Index(idx,tpl);
		//TA=Find_Index_Equal_Key(key,insertedIndexInfo);
	}
	else if(insertedKey.attr_type==CHAR){
		char temp[256];
		getElement(&temp,Element[insertedKey].c_str(),idx.attr_size);
		string key=temp;
		TupleAndPosition<string> tpl;
		tpl.position=position;
		tpl.key=key;
		Insert_Key_To_Index(idx,tpl);
		//TA=Find_Index_Equal_Key(key,insertedIndexInfo);
	}
}

void API_Delete(Table tbl,Tuple Element)//delete就是先查找，再删除而不是打印的过程
{
	list<Condition> Have_Index_Condition;
	list<Condition> &No_Index_Condition=clist;
	list<Condition>::iterator it;
	string IndexNameList[100];
	int Attribute_Size[100];
	int i=0;
	for(it=clist.begin();it!=clist.end();it++){
		Attribute_Size[i]=tbl.getAttrSize(i);
		//通过id获得属性的size
		IndexNameList[i++]=tbl.IsIndexExist(it->compare_attr);
		//从catalog中查找该属性是否有对应的index
		if(!IndexNameList.empty()&&it->compare_type!=NOT_EQUAL){
			Have_Index_Condition.push_back(*it);
			No_Index_Condition.erase(it);
		}
		else i--;
		//如果不等于而且在索引上的话，这样的索引查找没有意义，归为无索引查找
		//无索引查找引起索引名列表的下标减1
	}
	//获得condition列表中有索引的属性值
	i=0;
	vector<int> finalOfstList;
	vector<int> tempOfstList;
	vector<int>::iterator endOfList;
	for(it=Have_Index_Condition.begin();it!=Have_Index_Condition.end();it++){
		Index_Select_Position(*it,IndexNameList[i],tempOfstList,Attribute_Size[i++]);
		//获得相应的offset列表，和已有的offset列表进行
		sort(finalOfstList.begin(),finalOfstList.end());
		sort(tempOfstList.begin(),tempOfstList.end());
		endOfList=set_intersection(finalOfstList.begin(),finalOfstList.end(),tempOfstList.begin(),tempOfstList.end(),finalOfstList.begin());
		finalOfstList.resize(endOfList- finalOfstList.begin());
		sort(finalOfstList.begin(),finalOfstList.end());
		unique(finalOfstList.begin(),finalOfstList.end());
		finalOfstList.erase(finalOfstList.begin(),finalOfstList.end());
		//清除列表中没有的项
		if(finalOfstList.empty()){
			cout<<"Empty tuple!"<<endl;
			return;
		}
	}
	Tuple selectTuple(tbl.tablename);
	selectTuple.delete(finalOfstList,No_Index_Condition);
}

void Index_Select_Position(Condition cond,string indexname,vector<int> &offsetList,int AttrSize)
{
	IndexInfo idxInfo;
	idxInfo.indexname=indexname;
	idxInfo.attr_size=AttrSize;
	idxInfo.maxKeyNum=(BLOCKSIZE-INDEX_BLOCK_INFO-4)/(4+index.attr_size);
	if(cond.attr_type==INT){
		int key;
		getElement(&key,Condition.compare_value.c_str(),AttrSize);
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
	else if(cond.attr_type==DOUBLE){
		double key;
		getElement(&key,Condition.compare_value.c_str(),AttrSize);
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
	else if(cond.attr_type==CHAR){
		char temp[256];
		getElement(&temp,Condition.compare_value.c_str(),AttrSize);
		string key=temp;
		Find_Key_Offset_Index(idxInfo,key,offsetList,cond.compare_type);
	}
}










//Tuple - Insert()的时候返回block和ofst的信息
//Table - 增加primary的获取/primary key size的获取
//Table - 增加用id判定是否存在index，并返回索引的名称
//Table - 增加用indexname找到相关的table
//Table - 增加返回当前所有的元组的函数，存储在三个外部容器中，根据type选择一个放置,并更新FetchAllFlag的值
//Table - 增加用id获取对应attr的size