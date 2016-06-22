#include "Interpreter.h"
#include <fstream>

//此处只用于调试
//具体需要通过interpreter来实现
/*void Table::initializeTable(const string createTableCommand)
{
	attr_num = 4;
	attrs[0].attr_name = "studend_id";
	attrs[0].attr_id = 0;
	attrs[0].attr_type = CHAR;
	attrs[0].attr_len = 15;
	attrs[0].attr_def_type = PRIMARY;
	
	attrs[1].attr_name = "studend_name";
	attrs[1].attr_id = 1;
	attrs[1].attr_type = CHAR;
	attrs[1].attr_len = 15;
	attrs[1].attr_def_type = 2;

	attrs[2].attr_name = "studend_age";
	attrs[2].attr_id = 2;
	attrs[2].attr_type = INT;
	attrs[2].attr_len = 4;
	attrs[2].attr_def_type = 2;

	attrs[3].attr_name = "department";
	attrs[3].attr_id = 3;
	attrs[3].attr_type = CHAR;
	attrs[3].attr_len = 20;
	attrs[3].attr_def_type = 2;

	CalcRecordLen();
}*/

/////////////////////////////////////////////////////////////////
/***************************Table类*****************************/
////////////////////////////////////////////////////////////////

//根据语法树初始化表
void Table::initializeTable(FieldTree* T)
{
	FieldNode* Tnode, *node;
	//判断是否有重复名称的属性
	for (Tnode = T->field->next; Tnode != NULL; Tnode = Tnode->next){
		for (node = Tnode->next; node != NULL; node = node->next){
			if (Tnode->attr_name == node->attr_name){
				//若有名称重复的属性，报错
				cerr << "duplicated attribute is not allow." << endl;
				throw Grammer_error("create table");
			}
		}
	}
	//根据语法树初始化表
	int i = 0;
	for (Tnode = T->field->next; Tnode != NULL; Tnode = Tnode->next){
		attrs[i].attr_name = Tnode->attr_name;
		attrs[i].attr_type = Tnode->attr_type;
		attrs[i].attr_len = Tnode->attr_len;
		attrs[i].attr_def_type = Tnode->attr_def_type;
		i++;
	}
	attr_num = i;
	CalcRecordLen();  //计算record_len属性的值
}

//建表，更新或创建相关文件
bool Table::createTable()
{
	string filename;
	ofstream outfile;
	if (isTableExist()){  //该表在数据库中已存在
		cerr << "The table has already been existed!" << endl;
		return false;     //建表失败，返回false
	}
	//表不存在，则建立新表

	//更新储存表名的文件
	outfile.open("tablelist.txt", ios::app); //以追加模式打开
	if (!outfile) throw File_openfail("tablelist");

	outfile << table_name << endl;  //向文件中输出表名
	outfile.close();

	//新建储存该表的字段信息的文件
	filename = table_name + "_tableinfo.txt";
	outfile.open(filename.c_str(), ios::out);
	if (!outfile) throw File_openfail(filename.c_str());

	for (int i = 0; i < attr_num; i++){  //将表的字段信息存在文件中
		outfile << attrs[i].attr_name << " ";
		outfile << attrs[i].attr_def_type << " ";
		outfile << attrs[i].attr_type << " ";
		outfile << i << " ";
		outfile << attrs[i].attr_len << endl;
	}
	outfile.close();

	//新建储存该表的记录信息的文件
	filename = table_name + "_tablereco";
	outfile.open(filename.c_str(), ios::out);
	if (!outfile) throw File_openfail(filename.c_str());

	outfile.close();  //建立即可，此时不需要插入任何记录

	return true;
}

bool Table::dropTable()
{
	string filename;
	string newTableList = "";
	ifstream infile;
	ofstream outfile;
	if (!isTableExist()){  //该表不存在
		cerr << "The table doesn't exist!" << endl;
		return false;      //删除失败，返回false
	}
	//表存在，则删除或更新相关文件

	//删除tablelist.txt中表名
	infile.open("tablelist.txt", ios::in); //以只读模式打开文件，读取其中信息
	if (!infile) throw File_openfail("tablelist.txt");

	string tablename;
	while (!infile.eof()){
		getline(infile, tablename);  //每行只储存一个表名信息
		if (tablename != "" && tablename != table_name){
			//过滤掉要删除的表名，保留其余的表名信息，存在newTableList中
			newTableList = newTableList + tablename + '\n';
		}
	}
	outfile.open("tablelist.txt", ios::out);  //打开tablelist.txt并清空
	if (!outfile) throw File_openfail("tablelist.txt");
	outfile << newTableList;  //将保留的表名信息存入文件中
	outfile.close();

	//删除记录表的字段的文件
	filename = table_name + "_tableinfo.txt";
	remove(filename.c_str());  //移除文件

	//删除记录表中所有索引信息的文件
	filename = table_name + "_tableindexinfo.txt";
	remove(filename.c_str());

	//删除记录表中记录的文件
	filename = table_name + "_tablereco";
	remove(filename.c_str());  //移除文件

	return true;
}

//判断表是否已经存在，若已经存在则返回1，否则返回0
bool Table::isTableExist()
{
	ifstream infile;
	infile.open("tablelist.txt", ios::in);
	if (!infile){
		return false; //文件还没有被创建，说明还没有建立表
	}

	string tablename;
	while (!infile.eof()){
		getline(infile, tablename); //读取一个表名
		if (tablename == table_name){
			infile.close();
			return true; //该表已存在
		}
	}
	//该表不存在
	infile.close();
	return false;
}

//判断表中是否有attname这个属性
//表已通过isTableExist函数判断，确定已存在此表
bool Table::isAttriExist(const string attname)
{
	ifstream infile;
	string filename;
	
	filename = table_name + "_tableinfo.txt";
	infile.open(filename.c_str(), ios::in);
	if (!infile) throw File_openfail(filename.c_str());

	//寻找记录该属性信息的一行
	string attrinfo;
	string attrname;
	int i;
	while (!infile.eof()){
		i = 0;
		getline(infile, attrinfo);
		while (attrinfo.at(i) != ' ') i++;
		attrname = attrinfo.substr(0, i);  //获得属性名
		//将属性名与attname比较
		if (attrname == attname){
			//属性名存在
			infile.close();
			return true;
		}
	}

	//遍历文件后没有找到相符的属性名，则说明表中没定义该属性
	infile.close();
	return false;
}

//判断attname这个属性是否唯一
//attname已通过isAttriExist函数判断，确定表中存在这个属性
bool Table::isAttriUnique(const string attname)
{
	ifstream infile;
	string filename;

	filename = table_name + "_tableinfo.txt";
	infile.open(filename.c_str(), ios::in);
	if (!infile) throw File_openfail(filename.c_str());

	//寻找记录该属性信息的一行
	string attrinfo;
	string attrname;
	int attrdef = -1;
	int i;
	while (!infile.eof()){
		i = 0;
		getline(infile, attrinfo);
		while (attrinfo.at(i) != ' ') i++;
		attrname = attrinfo.substr(0, i);
		if (attrname == attname){
			attrdef = attrinfo.at(i + 1) - '0';
			break;
		}
	}

	if (attrdef == PRIMARY || attrdef == UNIQUE){
		//该属性为单值属性
		infile.close();
		return true;
	}
	else{
		infile.close();
		return false;
	}
}

void Table::CalcRecordLen()
{
	record_len = 0;
	for (int i = 0; i < attr_num; i++){
		record_len += attrs[i].attr_len;
	}
	record_len += 2;  //若插入一条记录，必须以'\0'结尾，同时记录的第一个字节用来记录是否已删除或存在
					  //所以必须多两个字节的大小
}


//已知表名，要获得该表信息
void Table::getTableInfo()
{
	ifstream infile;
	string filename;
	filename = table_name + "_tableinfo.txt";

	if (!isTableExist()){
		//表不存在
		throw Table_Index_Error("table", table_name);
	}

	infile.open(filename.c_str(), ios::in);
	if (!infile) throw File_openfail(filename.c_str());
	
	int i = 0;
	while (!infile.eof()){
		infile >> attrs[i].attr_name;
		infile >> attrs[i].attr_def_type;
		infile >> attrs[i].attr_type;
		infile >> attrs[i].attr_id;
		infile >> attrs[i].attr_len;
		i++;
	}
	attr_num = i - 1;
	CalcRecordLen();
}


string Table::getPrimaryKey()
{
	for (int i = 0; i < attr_num; i++){
		if (attrs[i].attr_def_type == 0) 
			return attrs[i].attr_name;
	}
    return "";
}

int Table::getPrimaryKeySize()
{
	for (int i = 0; i < attr_num; i++){
		if (attrs[i].attr_def_type == 0)
			return attrs[i].attr_len;
	}
    return 0;
}


string Table::getAttrIndex(string attrname)
{
	string filename;
	filename = table_name + "_tableindexinfo.txt";

	ifstream infile;
	infile.open(filename.c_str(), ios::in);
	if (!infile){
		//文件不存在，则索引也就不存在
		//返回空字符串
		return "";
	}

	string idxname, atrname;
	while (!infile.eof()){
		infile >> idxname;
		infile >> atrname;
		if (atrname == attrname){
			//索引存在
			return idxname;
		}
	}
	//索引不存在
	return "";
}

/////////////////////////////////////////////////////////////////
/***************************Index类*****************************/
////////////////////////////////////////////////////////////////

//初始化索引信息
void Index::initializeIndex(IndexStruct* Idx)
{
	table_name = Idx->table_name;
	attr_name = Idx->attr_name;
}

bool Index::createIndex()
{
	if (isIndexExist()){
		//索引名已存在
		throw Table_Index_Error("index", index_name);
	}

	Table T(table_name);
	if (!T.isTableExist()){
		//表不存在
		throw Table_Index_Error("table", table_name);
	}

	T.getTableInfo();
	if (!T.isAttriExist(attr_name)){
		//属性不存在
		throw Table_Index_Error("attribute", attr_name);
	}
	if (!T.isAttriUnique(attr_name)){
		//属性不是unique属性
		throw Multip_Error(attr_name);
	}
	
	//满足创建索引的条件
	//向indexlist.txt中追加索引名
	ofstream outfile;
	outfile.open("indexlist.txt", ios::app);
	if (!outfile) throw File_openfail("indexlist.txt");
	outfile << index_name << endl;
	outfile.close();

	//新建记录该索引信息的文件
	string filename;
	filename = index_name + "_indexinfo.txt";
	outfile.open(filename.c_str(), ios::app);
	if (!outfile) throw File_openfail(filename.c_str());
	outfile << index_name << " ";
	outfile << table_name << " ";
	outfile << attr_name << endl;
	outfile.close();

	//新建记录该表所有索引信息的文件
	filename = table_name + "_tableindexinfo.txt";
	outfile.open(filename.c_str(), ios::app);
	if (!outfile) throw File_openfail(filename.c_str());
	outfile << index_name << " ";
	outfile << attr_name << endl;
	outfile.close();

	//新建储存该索引的B+树的文件
	filename = index_name;
	outfile.open(filename.c_str(), ios::app);
	if (!outfile) throw File_openfail(filename.c_str());
	outfile.close();

	return true;
}

//删除索引
bool Index::dropIndex()
{
	if (!isIndexExist()){
		//索引不存在，无法删除
		cerr << "The index doesn't exist!" << endl;
		return false;
	}
	//索引存在，则删除或更新相关文件

	//删除indexlist.txt中索引名
	ifstream infile;
	infile.open("indexlist.txt", ios::in); //以只读模式打开文件，读取其中信息
	if (!infile) throw File_openfail("indexlist.txt");

	string indexname;
	string newIndexList = "";
	while (!infile.eof()){
		getline(infile, indexname);  //每行只储存一个索引信息
		if (indexname != "" && indexname != index_name){
			//过滤掉要删除的索引，保留其余的索引信息，存在newIndexList中
			newIndexList = newIndexList + indexname + '\n';
		}
	}
	infile.close();
	ofstream outfile;
	outfile.open("indexlist.txt", ios::out);  //打开indexlist.txt并清空
	if (!outfile) throw File_openfail("indexlist.txt");
	outfile << newIndexList;  //将保留的索引信息存入文件中
	outfile.close();

	string filename;

	string tbname;
	string idname;
	//删除记录索引信息的文件
	filename = index_name + "_indexinfo.txt";
	infile.open(filename.c_str(), ios::in);
	//读取索引所在的表名和字段信息
	{
		infile >> indexname >>tbname >>idname;
	}
	string thisIndexinfo = indexname + " " + idname;
	infile.close();
	remove(filename.c_str());  //移除文件

	//删除_tableindexinfo.txt中索引信息
	filename = tbname + "_tableindexinfo.txt";
	infile.open(filename.c_str(), ios::in); //以只读模式打开文件，读取其中信息
	if (!infile) throw File_openfail(filename.c_str());

	string newIndexinfo = "";
	string indexinfo;
	while (!infile.eof()){
		getline(infile, indexinfo);  //每行只储存一个索引信息
		if (indexinfo != "" && indexinfo != thisIndexinfo){
			//过滤掉要删除的索引信息，保留其余的索引信息，存在newIndexList中
			newIndexinfo = newIndexinfo + indexinfo + '\n';
		}
	}
	infile.close();
	outfile.open(filename.c_str(), ios::out);  //打开_tableindexlist.txt并清空
	if (!outfile) throw File_openfail(filename.c_str());
	outfile << newIndexinfo;  //将保留的该表的索引信息存入文件中
	outfile.close();

	//删除储存该索引B+树的文件
	filename = index_name;
	remove(filename.c_str());  //移除文件

	return true;
}


//给定index名，判断index是否存在
bool Index::isIndexExist()
{
	//从indexlist.txt中寻找索引名
	ifstream infile;
	string name;
	infile.open("indexlist.txt", ios::in);
	if (!infile) return false;
	while (!infile.eof()){
		infile >> name;
		if (name == index_name) 
			return true; //索引已存在
	}
	infile.close();
	return false;  //索引不存在
}
