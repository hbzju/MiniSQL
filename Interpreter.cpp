#include "Interpreter.h"
#include "API.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>

string getUserCommand()
{
	char* command;  //用户输入的命令
	command = (char*)malloc(1024);
	char ch;
	int i = 0;

	while (1){
		cin.get(ch);
		command[i++] = ch;
		if (ch == ';') break;
		else if (ch == '\n'){
			cout << "     ->";
		}
	}
	command[i] = '\0';  //结束符
	string s(command);
	delete[] command;
	return s;
}


void handleWithCommand(string command)
{
	int i, j, opos, npos;

	i = 0; j = 0;
	opos = command.find_first_not_of(" \n", 0);
	npos = command.find_first_of(" ", opos);
	string comtype = command.substr(opos, npos - opos);
	//判断命令类型
	if (comtype == "execfile"){
		//打开存储待插入记录的文件
		opos = command.find_first_not_of(" ", npos);
		npos = command.find_first_of(";", opos);
		string filename = command.substr(opos, npos - opos);
		
		ifstream infile;
		char buf[1024];
		infile.open("./student/" + filename, ios::in);
		while (!infile.eof()){
			memset(buf, 0, 1024);
			infile.getline(buf, 1024);
			if (infile.eof()) break;
			string cmd(buf);
			handleWithCommand(cmd);
		}
		cout << "Query OK." << endl;
	}
	else if (comtype == "create"){
		opos = npos + 1;
		npos = command.find_first_of(" ", opos); 
		string object = command.substr(opos, npos - opos);
		if (object == "table"){
			//建表命令
			opos = npos + 1;
			npos = command.find_first_of(" (", opos);
			string tbname = command.substr(opos, npos - opos);
			//创建建表的语法树
			FieldTree* T;
			T = createCreateTableTree(tbname, command.substr(npos, command.length() - npos));
			
			//处理语法树，传参给API
			API_Create_Table(tbname, T);
			/*Table Table(tbname);
			Table.initializeTable(T);
			Table.createTable(); //此处为调用API中create table命令*/

			cout << "Query OK, 0 rows affected." << endl;
			return;
		}
		else if (object == "index"){
			//建索引命令
			opos = npos + 1;
			opos = command.find_first_not_of(" ", opos);  //索引名开始的位置
			npos = command.find_first_of(" ", opos);  //索引名结束后的第一个空格位置
			string indexname = command.substr(opos, npos - opos);
			//创建索引结构
			IndexStruct* I;
			I = createCreateIndexStruct(indexname, command.substr(npos, command.length() - npos));
			
			//传参给API
			API_Create_Index(*I);
			/*Index Idx(indexname);
			Idx.initializeIndex(I);
			Idx.createIndex();  //调用建索引的API*/

		}
		else{
			//非法命令
			throw Grammer_error(object);
		}
	}
	else if (comtype == "drop"){
		opos = command.find_first_not_of(" ", npos);
		npos = command.find_first_of(" ", opos);
		string dropOrient = command.substr(opos, npos - opos);
		if (dropOrient == "table"){
			//删除表命令
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" ;", opos);
			string tbname = command.substr(opos, npos - opos);
			//下一部分非空格符应只有分号
			opos = command.find_first_not_of(" ", npos);
			if (command.at(opos) != ';'){
				//语法错误
				throw Grammer_error(tbname);
			}

			//传参给API
			Table T(tbname);
			T.getTableInfo();
			API_Drop_Table(T);
			/*
			T.dropTable();  //调用API的删除表命令*/
		}
		else if (dropOrient == "index"){
			//删除索引命令
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" ;", opos);
			string idxname = command.substr(opos, npos - opos);
			//下一部分非空格符应只有分号
			opos = command.find_first_not_of(" ", npos);
			if (command.at(opos) != ';'){
				//语法错误
				throw Grammer_error(idxname);
			}

			//传参给API
			API_Drop_Index(idxname);
			/*Index Idx(idxname);
			Idx.dropIndex(); //调用API中删除索引的命令*/
		}
		else{
			//非法命令
			throw Grammer_error(dropOrient);
		}
	}
	else if (comtype == "insert"){
		//插入记录指令
		opos = command.find_first_not_of(" ", npos);
		npos = command.find_first_of(" ", opos);
		string into = command.substr(opos, npos - opos);
		if (into == "into"){
			//符合语法要求
			//表名
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" ", opos);
			string tbname = command.substr(opos, npos - opos);

			//"values"
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" (", opos);
			string values = command.substr(opos, npos - opos);
			if (values == "values"){
				//符合语法要求
				TupleStruct* tup;
				tup = CreateTupleStruct(command.substr(npos, command.length() - npos));

				//传tuple给API
				Table tbl(tbname);
				tbl.getTableInfo();
				Tuple tuple(tbname);
				tuple.initializeTuple(tup);

				API_Insert(tbl, tuple);
				/*
				tuple.Insert();   //调用API中的insert命令*/
			}
			else{
				//不符合语法要求
				throw Grammer_error(into);
			}
		}
		else{
			//不符合语法要求
			throw Grammer_error(into);
		}
		cout << "Query OK, 1 rows affected." << endl;
	}
	else if (comtype == "select"){
		//select * from student;
		//select * from student where...
		opos = command.find_first_not_of(" ", npos);
		npos = command.find_first_of(" ", opos);
		string selectall = command.substr(opos, npos - opos);
		if (selectall == "*"){
			//符合语法要求
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" ", opos);
			string from = command.substr(opos, npos - opos);
			if (from == "from"){
				//符合语法要求
				opos = command.find_first_not_of(" ", npos);
				npos = command.find_first_of(" ;", opos);
				string tbname = command.substr(opos, npos - opos);
				list<Condition> conditionlist;   //储存查询条件
				conditionlist = CreateConditionlist(command.substr(npos, command.length() - npos));

				//传参给API
				Table tbl(tbname);
				tbl.getTableInfo();
				API_Select(tbl,conditionlist);
				/*vector<int> offsetlist;
				//API从索引里查找，返回offserlist
				Tuple tuple(tbname);
				tuple.Select(offsetlist, conditionlist); //API调用select命令*/
			}
			else{
				//不符合语法要求
				throw Grammer_error(from);
			}
		}
		else{
			//不符合语法要求
			throw Grammer_error(selectall);
		}
	}
	else if (comtype == "delete"){
		//delete from student;
		//delete from student where sno = ‘88888888’;
		opos = command.find_first_not_of(" ", npos);
		npos = command.find_first_of(" ", opos);
		string from = command.substr(opos, npos - opos);
		if (from == "from"){
			//符合语法要求
			opos = command.find_first_not_of(" ", npos);
			npos = command.find_first_of(" ;", opos);
			string tbname = command.substr(opos, npos - opos);
			list<Condition> conditionlist;   //储存查询条件
			conditionlist = CreateConditionlist(command.substr(npos, command.length() - npos));

			//传参给API
			Table tbl(tbname);
			tbl.getTableInfo();
			API_Delete(tbl, conditionlist);
			/*vector<int> offsetlist;
			//API从索引里查找，返回offserlist
			Tuple tuple(tbname);
			tuple.Delete(offsetlist, conditionlist); //API调用delete命令*/
		}
		else{
			//不符合语法要求
			throw Grammer_error(from);
		}
	}
	else{
		//非法命令
		throw Grammer_error(comtype);
	}
}



/*创建建表的语法树
create table student(
	sno char(8),
	sname char(16) unique,
	sage int,
	sgender char(1),
	primary key(sno)
	);
*/
//表名为tbname, 字段域为fieldmodule部分 (括号内（包括括号）的部分)
FieldTree* createCreateTableTree(string tbname, string fieldmodule)
{
	int i, opos, npos, endpos;
	int primary_num;
	string primary_attr = "";
	FieldTree* T;
	FieldNode* Tnode, *Lnode;
	string field;   //一条字段信息

	T = new FieldTree;
	T->tablename = tbname;
	T->field = new CreateTableNode;//树根节点
	T->field->next = NULL;
	Lnode = T->field;

	i = 0;
	primary_num = 0;
	opos = fieldmodule.find_first_not_of(" (\n");

	while (1){
		fieldmodule = fieldmodule.substr(opos, fieldmodule.length() - opos);
		npos = fieldmodule.find_first_of(",", 0);
		if (npos == -1){
			//最后一个字段
			//右括号和分号结束
			npos = fieldmodule.find_first_of(";", 0);
		}
		else npos++;

		opos = fieldmodule.find_first_not_of(" ", 0);
		if (opos == -1 || fieldmodule.at(opos) == ';') break;   //语句结束

		string field = fieldmodule.substr(0, npos);
		//sno char(8) unique,
		//primary key(sno))

		//确定字段名
		opos = field.find_first_not_of(" \n(", 0);
		npos = field.find_first_of(" ", opos);
		if (npos == -1 || opos > npos){
			//字段为空，报错，提示至少要有一个属性
			cerr << "At least one attribute is aquired." << endl;
			throw Grammer_error(field);
		}
		string def = field.substr(opos, npos - opos);
		if (def == "primary"){
			//主键声明语句，而非字段定义语句
			opos = field.find_first_not_of(" ", npos);
			npos = field.find_first_of(" (", opos);
			string key = field.substr(opos, npos - opos);
			if (key == "key"){
				opos = field.find_first_not_of(" (", npos);
				npos = field.find_first_of(" )", opos);
				if (opos == -1 || npos == -1){
					//括号不匹配
					throw Grammer_error(def + " " + key);
					return NULL;
				}
				primary_num++;
				if (primary_num == 2){
					//主键声明不止一个
					cerr << "Only one primary key is allowed!" << endl;
					return NULL;
				}
				primary_attr = field.substr(opos, npos - opos);
			}
			else{
				//语法错误
				throw Grammer_error(def);
				return NULL;
			}
		}
		else{
			Tnode = new CreateTableNode;  //创建新节点
			Lnode->next = Tnode;
			Tnode->next = NULL;

			Tnode->attr_name = field.substr(opos, npos - opos);  //字段名

			//确定字段属性
			opos = field.find_first_not_of(" \n,", npos);
			npos = field.find_first_of(" \n(),", opos);
			string type = field.substr(opos, npos - opos);

			if (type == "char"){
				Tnode->attr_type = 0;
				opos = field.find_first_not_of(" (\n", npos);
				npos = field.find_first_of(')', opos);
				if (opos == -1 || (opos != -1 && npos == -1)){
					//没有括号 or 左右括号不匹配
					throw Grammer_error(type);
					return NULL;
				}
				int len = atoi(field.substr(opos, npos - opos).c_str());  //char类型属性的长度
				Tnode->attr_len = len;
			}
			else if (type == "int"){
				Tnode->attr_type = 1;
				Tnode->attr_len = 4;
			}
			else if (type == "float"){
				Tnode->attr_type = 2;
				Tnode->attr_len = 4;
			}
			else{
				//字段类型非法
				cerr << type << " is not a legal type." << endl;
				throw Grammer_error(type);
				return NULL;
			}

			//确定字段定义的约束属性
			opos = field.find_first_not_of(" )\n", npos);
			if (opos == -1 || opos == field.length() - 1){
				//未显式定义约束条件
				Tnode->attr_def_type = -1;
			}
			else{
				//显式定义了约束条件
				npos = field.find_first_of(" \n),", opos);
				string deftype = field.substr(opos, npos - opos);

				if (deftype == "primary"){
					//主键定义
					opos = field.find_first_not_of(" \n", npos);
					npos = field.find_first_of(" ,)\n", opos);
					string key = field.substr(opos, npos - opos);
					if (key == "key"){
						//检查key后还有没有非法符号
						opos = field.find_first_not_of(" \n", npos);
						if (opos == -1 || (field.at(opos) != ',' && field.at(opos) != ')')){
							//语法错误
							throw Grammer_error(key);
						}
						primary_num++;
						Tnode->attr_def_type = 0; //primary
					}
					else{
						//语法错误
						throw Grammer_error(deftype);
					}
				}
				else if (deftype == "unique"){
					//检查unique后还有没有非法符号
					opos = field.find_first_not_of(" \n", npos);
					if (opos == -1 || (field.at(opos) != ',' && field.at(opos) != ')')){
						//语法错误
						throw Grammer_error(deftype);
					}
						Tnode->attr_def_type = 1; //unique
				}
				else{
					//非法定义
					throw Grammer_error(deftype);
				}
			}//判断约束条件
			Lnode = Tnode;
			//下一条字段信息
		}//字段定义语句
		opos = 0;
		while (opos < field.length()) opos++;
	}//while
	int flag = 0;
	if (primary_attr != ""){
		//有附加的主键定义
		primary_num = 0;
		for (Tnode = T->field->next; Tnode != NULL; Tnode = Tnode->next){
			if (Tnode->attr_name == primary_attr){
				flag = 1;
				Tnode->attr_def_type = 0;
			}
			if (Tnode->attr_def_type == 0)
				primary_num++;
		}
		if (!flag){
			//没有主键定义的该字段
			cout<< "There is no attribute \"" << primary_attr << "\"" << endl;
			return NULL;
		}
		if (primary_num >= 2){
			//多于一个主键定义
			cout << "Only one primary key is allowed!" << endl;
			return NULL;
		}
		if (primary_num == 0){
			//没有主键定义
			cout << "At least one attribute is acquired! 1" << endl;
		}
	}
	else if (primary_num == 0){
		cout << "At least one attribute is acquired! 2" << endl;
	}
	return T;
}

/*创建储存索引信息的结构
create index stunameidx on student ( sname );
*/
//idxname为索引名，info为索引名之后的信息
IndexStruct* createCreateIndexStruct(string idxname, string info)
{
	IndexStruct* Indexs;
	int i, opos, npos;

	opos = 0;
	opos = info.find_first_not_of(" ", opos);
	npos = info.find_first_of(" ", opos);
	string on = info.substr(opos, npos - opos);
	if (on == "on"){
		//符合语法要求
		Indexs = new IndexStruct;
		Indexs->index_name = idxname;
		Indexs->attr_name = "";
		Indexs->table_name = "";  //初始化

		//下一部分为索引所在的表的名字
		opos = info.find_first_not_of(" ", npos);
		npos = info.find_first_of(" (", opos);
		string tbname = info.substr(opos, npos - opos);
		Indexs->table_name = tbname; //表名

		//下一部分为索引建立的字段名
		opos = info.find_first_not_of(" (", npos);
		npos = info.find_first_of(" )", opos);
		string attrname = info.substr(opos, npos - opos);
		Indexs->attr_name = attrname; //字段名

		//下一部分非空格符应只有分号
		opos = info.find_first_not_of(" )", npos);
		if (info.at(opos) != ';'){
			//语法错误
			cout << "SQL syntax error near \"" << attrname << "\"" << endl;
			return NULL;
		}
		return Indexs;
	}
	else{
		//不符合语法要求，报错
		cout << "SQL syntax error near \"" << idxname <<" "<< on << "\"" << endl;
		return NULL;
	}
	return NULL;
}

//insert into student values(‘12345678’, ’wy’, 22, ’M’);
//tupinfo为values后[(...);]部分
TupleStruct* CreateTupleStruct(string tupinfo)
{
	int opos, npos;
	int num; //属性个数
	TupleStruct* tup;
	tup = new TupleStruct;

	num = 0;
	opos = tupinfo.find_first_not_of(" \n(", 0);
	while (1){
		tupinfo = tupinfo.substr(opos, tupinfo.length() - opos);
		npos = tupinfo.find_first_of(",)", 0);
		if (npos == -1){
			//所有属性值都已处理完
			npos = tupinfo.find_first_not_of(" \n");
			if (tupinfo.at(npos) != ';'){
				//语法错误
				cout << "SQL syntax error near \"" << tupinfo.at(npos) << "\"" << endl;
				return NULL;
			}
			else break; //处理完毕
		}
		string value = tupinfo.substr(0, npos + 1); //获得一个属性值(包括','或')')
		opos = value.find_first_not_of(" '", 0);
		npos = value.find_first_of("' ,)", opos);
		if (npos == -1) npos = value.length();
		tup->tuplevalues[num++] = value.substr(opos, npos - opos); //获得处理后的属性值（去除首尾空格和'）

		opos = 0;
		while (opos < value.length()) opos++;
		//处理下一个属性值
	}//循环处理括号中每个属性值
	tup->value_num = num;
	return tup;
}

//建立储存查询条件的结构
//where sage > 20 and sgender = ‘F’;
list<Condition> CreateConditionlist(string condinfo)
{
	list<Condition> condlist;
	int opos, npos;

	condlist.clear();
	//判断是否有where语句
	opos = condinfo.find_first_not_of(" ", 0);
	if (condinfo.at(opos) == ';'){ 
		//没有where语句,查询条件为空
		return condlist;
	}
	else{
		//有where语句
		npos = condinfo.find_first_of(" ", opos);
		string where = condinfo.substr(opos, npos - opos);
		if (where == "where"){
			//符合语法要求
			opos = npos;  //从where后面的语句开始处理
			while (1){
				condinfo = condinfo.substr(opos, condinfo.length() - opos);
				opos = condinfo.find_first_not_of(" \n", 0);
				npos = condinfo.find("and", opos);
				if (opos == -1 || condinfo.at(opos) == ';') break;   //语句结束
				if (npos == string::npos){
					//最后一个查询条件
					//分号结束语句(分号前可能有空格)
					npos = condinfo.find_first_of(";", opos);
					npos++;
				}
				else npos = condinfo.find_first_of(" \n", npos);
				string condstr = condinfo.substr(opos, npos - opos);  
				//id >= '10010' (and/;)

				//所要查询的属性名
				opos = condstr.find_first_not_of(" \n", 0);
				npos = condstr.find_first_of(" \n", opos);
				if (opos == -1 || npos == -1){
					//语法错误
					throw Grammer_error(condstr);
				}
				string attr = condstr.substr(opos, npos - opos);

				//查询的比较条件（== / >= / <...）
				opos = condstr.find_first_not_of(" \n", npos);
				npos = condstr.find_first_of(" \n", opos);
				if (opos == -1 || npos == -1){
					//语法错误
					throw Grammer_error(condstr);
				}
				string type = condstr.substr(opos, npos - opos);
				if (type != "=" && type != ">" && type != ">="
					&& type != "<" && type != "<=" && type != "<>"){
					//比较符号错误
					throw Grammer_error(type);
				}

				//比较值
				opos = condstr.find_first_not_of(" ';\n", npos);
				npos = condstr.find_first_of(" ';\n", opos);  //可能以分号结尾
				if (opos == -1 || npos == -1){
					//语法错误
					throw Grammer_error(condstr);
				}
				string value = condstr.substr(opos, npos - opos);

				struct Condition oneCondition;
				oneCondition.compare_attr = attr;
				oneCondition.compare_type = type;
				oneCondition.compare_value = value;
				
				condlist.push_back(oneCondition);  //向链表中插入一条记录
				
				while (opos++ < condstr.length());

			} //while
		}
		else throw Grammer_error(where); //抛出异常
	}
	return condlist;
}