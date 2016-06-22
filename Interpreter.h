#ifndef _RECORDMANAGER_H
#define	_RECORDMANAGER_H
#define _CRT_SECURE_NO_WARNINGS
#include "minisql.h"
#include <string>
#include <iostream>
#include <vector>
#include <list>

using namespace std;

//建表语法树中的节点。每个节点储存一个字段信息
typedef struct CreateTableNode FieldNode;
struct CreateTableNode{
	string attr_name;  //字段名
	INT32 attr_len;    //字段大小
	int attr_type;     //字段类型（int/char/float）
	int attr_def_type; //字段定义类型（primary key/unique/not null）
	FieldNode* next;   //下一字段
};


//建表语句的语法树
typedef struct CreateTableTree FieldTree;
struct CreateTableTree{
	string tablename;     //表名
	FieldNode* field;  //表中字段
};

//储存索引信息的结构
typedef struct CreateIndexStruct IndexStruct;
struct CreateIndexStruct{
	string index_name;  //索引名
	string table_name;  //索引所属表
	string attr_name;   //索引建立的字段
};

//储存插入的元组的结构
typedef struct InsertTupleStruct TupleStruct;
struct InsertTupleStruct{
	string tuplevalues[32];
	int value_num; //元组中属性数
};

struct Condition{
	string compare_attr;     //要查询的属性名
	string compare_type;     //查询类型（如：==, >=, <=等）
	string compare_value;    //查询的值   
};



//记录符合条件的记录
extern vector <char*> select_list;

int isBlockFull(char* buf, int reclen);  //判断buf块是否还有空位插入长度为reclen的记录
int totalBlockNum(string filename);  //文件中已经有的块数
string getUserCommand(); //获得用户命令
void handleWithCommand(string command); //处理命令
FieldTree* createCreateTableTree(string tbname, string fieldmodule);//表名为tbname, 字段域为fieldmodule部分 (括号内（包括括号）的部分)
IndexStruct* createCreateIndexStruct(string idxname, string info); //建立储存索引信息的结构
TupleStruct* CreateTupleStruct(string tupinfo); //建立储存元组内属性值的结构
list<Condition> CreateConditionlist(string condinfo); //建立储存查询条件的结构
void PushPosition(INT32 BlockAddr, INT32 Offset, char* key, int type, INT32 recordLength);
void PopPosition(INT32 position, INT32 recordLength, INT32 &BlockAddr, INT32 &Offset);
INT32 PushPosition(INT32 BlockAddr, INT32 Offset, INT32 recordLength);


//Attribute：记录表中每个字段的定义信息
class Attribute{
public:
	int attr_id;        //表中的第几个字段
	string attr_name;   //字段名
	int attr_type;      //字段的数据类型(int, char, float)
	int attr_def_type;  //字段的定义类型(PRIMARY, UNIQUE, NULL)
	INT32 attr_len;     //字段大小（字节为单位）
	Attribute(){}
	~Attribute(){}
};


//Table：记录表的定义信息
class Table{
	friend class Tuple;
	friend class RBlock;
private:
	string table_name;    //表名
	INT32 attr_num;       //表中字段数
	INT32 record_len;     //表中每条记录的长度
	Attribute attrs[32];  //表中字段
public:
	Table(const string tbname) : table_name(tbname){}		//初始化表名
	void initializeTable(FieldTree* T);  //初始化表。这一部分由interpreter完成
	//catalog manager只负责处理已经初始化过的table对象
	void CalcRecordLen();       //求得表中每条记录的长度
	bool isTableExist();	//根据返回值判断表是否已经存在
	bool isAttriExist(const string attname);  //判断表中是否有名为attname的字段
	bool isAttriUnique(const string attname); //判断表中名为attname的字段是否唯一
	bool createTable();              //建表
	bool dropTable();                //删除表

	string getTableName(){ return table_name; }
	void getTableInfo();             //已知表名，要获得表的信息
	string getPrimaryKey();          //返回表的primary key
	int getPrimaryKeySize();         //返回表的primary key的大小
	int getAttrNum() { return attr_num; }         //返回表的属性数
	Attribute getAttr(int i){ return attrs[i]; }  //根据属性下标返回属性名
	Attribute getAttr(string AttrName){      
		for (int i = 0; i < attr_num; i++){
			if (attrs[i].attr_name == AttrName){
				return attrs[i];
			}
		}
        return attrs[0];
	}
	string getAttrIndex(string attrname);         //判断attrname是否有索引，如果有则返回索引名
	~Table(){}
};


//Index：记录建立的索引
class Index{
private:
	string index_name;  //索引名
	string table_name;  //索引所属表
	string attr_name;   //索引建立的字段
public:
	Index(const string idxname) : index_name(idxname){}
    void initializeIndex(IndexStruct* Idx); //初始化索引信息。这一部分由interpreter完成

	bool isIndexExist();  //给定索引名，判断索引是否已存在
	bool createIndex();   //建立索引
	bool dropIndex();     //删除索引
	string getIndexName(){
		return index_name;
	}
	string FindIndexTable(){ return table_name; }   //返回索引对应的表名

	~Index(){}
};

//Tuple：元组
//如：('zhangjin', '3140105196', 18, 'Computer Science')是一个元组
class Tuple{
private:
	string tuple_values[32];  //元组中属性的值
	Table T;                  //元组所属的表
public:
	Tuple(string tbname) :T(tbname){}
	void initializeTuple(TupleStruct* tup); //初始化元组。这一部分由interpreter完成
	//初始化完成后，得到元组各属性的值以及该元组所属的表
	char* convertTuple();  //将元组转化为char*类型，方便存入缓冲区
	bool IsUniqueKeyExisted(); //判断该元组的UNIQUR和PRIMARY属性是否已经存在。若已经存在则返回1，不能重复插入

	bool Insert(INT32 &position);  //将元组插入其表的reco文件中。插入时需要在记录前加一个字节的判断是否删除位，在记录末加一个'\0'
	void Select(vector<int> offsetlist, list<Condition> conditionlist);  //从reco文件中选出符合条件的记录
	int Delete(vector<int> offsetlist, list<Condition> conditionlist);  //从reco文件中删除符合条件的记录。返回删除的记录数

	string getTupleValue(int i){
		return tuple_values[i];
	}
	void printSelectResult();  //分属性打印选择出的结果
	void FetchAll(int AttrId);           //找到所有记录的信息，存在TupleIndex里
	int recordLen(){   
		return T.record_len;
	}
	int getPrintLength(int *length);
	void PrintTuple(int totalLength, int FirstOrEnd, int *length);
	void printValue(int i, int type, int width);

	~Tuple(){}
};

//Block：记录一个缓冲块的信息
class RBlock{
	friend class Tuple;
public:
	Table T;
	char* buffer;   //指向缓冲块的指针
	char* ptr;      //缓冲块中指针的位置
	int cnt;        //缓冲块中剩余可读字节数
	int is_Changed; //记录缓冲块是否被修改过
	INT32 BlockAddr;

	RBlock(string tablename) : T(tablename){
		buffer = new char[BLOCKSIZE];  //初始化缓冲块
		memset(buffer, 0, BLOCKSIZE);
		ptr = &buffer[0];   //指针一开始指向缓存区头部
		is_Changed = 0;
		BlockAddr = 0;
	}
	int getBlock(string tablename, int offsetB, int is_want_empty);  //从文件中第offsetB块的位置处开始读取一块缓冲块大小的内容或找到第一个空块。
	void writeBackBlock(const char* filename, int offsetB); //将缓冲块中内容从offsetB块的位置处开始写回文件中
	~RBlock();
};

#endif