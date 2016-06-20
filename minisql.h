#ifndef minisql_h
#define minisql_h

#include "string"
#include "iostream"
#include <iomanip>
#include "block.h"
using namespace std;

#define INT32 unsigned int
#define BLOCKSIZE 4096
#define INDEX_BLOCK_INFO 12


#define PRIMARY 0
#define UNIQUE 1
#define NULLVALUE 2

#define CHAR 0
#define INT 1
#define FLOAT 2

class IndexInfo{
public:
    string indexname;
    int attr_size;
    int maxKeyNum;
};

class TupleIndex
{
public:
    int type;
    string key;
    INT32 position;
};

class TupleIndex_number:public TupleIndex
{
public:
	int intkey;
	float floatkey;
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

#define EQUAL "="
#define NOT_EQUAL "<>"
#define LARGER_THAN ">"
#define LARGER_THAN_AND_EQUAL ">="
#define LESS_THAN "<"
#define LESS_THAN_AND_EQUAL "<="
#define INFI 2147483647

//文件打开相关的异常类
class File_openfail{
public:
	string filename;
	File_openfail(const char* fn){
		filename = fn;
	}
	void openfailed(){
		cerr << "Can't open file " << filename << endl;
	}
};

//语法错误异常类
class Grammer_error{
public:
	string errorPos;
	Grammer_error(string err) : errorPos(err){}
	void SyntaxError(){
		cerr << " Error: illegal command." << endl << "SQL syntax error near \"" << errorPos << "\"" << endl;
	}
};

class Conflict_Error
{
public:
	string tablename;
	Conflict_Error(string tblname)
	{
		tablename = tblname;
	}
};

#endif