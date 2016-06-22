#include "BPlus_Tree.h"
#include "minisql.h"
#include "Interpreter.h"
#include "IndexManager.h"
#include "API.h"
#include "ctime"

#include "fstream"
using namespace std;

/*void Print_Block(IndexInfo index, int k)
{
	
	
	
	
	
	<< "----------------------- " << k << " -----------------------" << endl;
	int type, keynum;
	memcpy(&type, indexBlock[k], 4);
	memcpy(&keynum, indexBlock[k] + 4, 4);
	int ofst = INDEX_BLOCK_INFO;
	cout << type << ' ' << keynum << ' ' << index.maxKeyNum << endl;
	for (int i = 0; i < keynum; i++){
		int position;
		float key;
		memcpy(&position, indexBlock[k] + ofst , 4);
		getElement(&key, indexBlock[k] + ofst + 4, 4);
		ofst += 8;
		cout << position << ' ' << key << endl;
	}
	//验证插入模块是否正确
	int next;
	memcpy(&next, indexBlock[k] + ofst, 4);
	cout << next << endl;
}*/

/*int main()
{
	ifstream infile;
	infile.open("./student/student.txt", ios::in);
	if (!infile) {
		cerr << "Can't open file!" << endl;
		exit(1);
	}

	char cmd[1024];
	char ch;
	int i;

	while (!infile.eof()){
		i = 0;
		memset(cmd, 0, 1024);
		infile.getline(cmd, 1024);
		if (infile.eof()) break;
		string command(cmd);
		handleWithCommand(command);
	}
	return 0;
}*/
int main()
{
	Block();
	cout << "-----------------------------------------MINISQL-----------------------------------------\n"
		"|Copyright(c) 2016, 2016, Wang Haobo, Zhang Jin, Qiu Zhenting and / or its affiliates.  |\n"
		"|All rights reserved.Other names may be trademarks of their respective owners.          |\n"
		"|                                                                                       |\n"
		"|Type ';' to Run your codes.Type 'exit;' quit the execution.                            |\n" 
		"-----------------------------------------------------------------------------------------"
		<< endl;
	
	while (1){
		clock_t start, end;
		cout << "zwqSQL>";
		try{
			string command = getUserCommand();
			start = clock();
			if (command == "exit;") break;
			handleWithCommand(command);
			end = clock();
			cout << "(" << double(end - start)/CLOCKS_PER_SEC << " sec)" << endl;
		}
		catch (File_openfail fo){
			cerr << "Error: 1001 (ER_CANT_OPEN_FILE)" << endl;
			cerr << "Message: Can't open file: '" << fo.filename << "'!" << endl;
		}
		catch (Grammer_error ge){
			cerr << "Error: 1002 (ER_SYNTAX_ERROR)" << endl;
			cerr << "Message: You have an error in your SQL syntax.At position :" << ge.errorPos << "." << endl;
		}
		catch (Conflict_Error ce){
			cerr << "Error: 1169 (ER_DUP_UNIQUE)" << endl;
			cerr << "Message: Can't write, because of unique constraint, to table '" << ce.tablename << "' " << endl;
		}
		catch (Table_Index_Error te){
			cerr << "Error: 1760 (ER_DUP_UNIQUE)" << endl;
			cerr << "Error: " << te.errorType << " " << te.name << " does not existed!" << endl;
		}
		catch (Multip_Error me){
			cerr << "Error: 1760 (ER_DUP_UNIQUE)" << endl;
			cerr << "The attribute maybe multi-valued. Can't create index on it!" << endl;
		}
		catch (...){
			cerr << "Unkonwn Exception." << endl;
		}
		cout << endl;
		/*float id;
		int age;
		INT32 position;
		memcpy(&position, indexBlock[0] + INDEX_BLOCK_INFO, sizeof(INT32));
		memcpy(&id, indexBlock[0] + INDEX_BLOCK_INFO + 4, sizeof(float));
		cout <<id<<' '<<position << endl;

		memcpy(&id, tableBlock[0] + 1, sizeof(float));
		memcpy(&age, tableBlock[0] + 4 + 1, sizeof(int));
		cout << id << " " << age << endl;*/
		getchar();
		Block::flush_all_blocks();
		//Block::Print();
	}
	return 0;
}

/*
create table student(
	id float primary key,
	age int unique);
insert into student values (10001, 18);
insert into student values (10002, 19);
insert into student values (10003, 20);
insert into student values (10004, 21);
insert into student values (10005, 22);
select * from student where id = 10003;
delete from student where id = 10004;
select * from student;
*/


/*drop table student;*/

/*insert into student values('10001', 'zj', 18);
insert into student values('10002', 'hb', 18);
insert into student values('10003', 'qzt', 19);*/

/*
create table student(id int primary key,name int unique);
insert into student values(10001,15);
insert into student values(10002,16);
create index hb on student(name);
select * from student where name > 1000;
*/
