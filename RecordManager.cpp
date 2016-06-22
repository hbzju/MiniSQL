#include "Interpreter.h"
#include "IndexManager.h"
#include <fstream>
#include <stdlib.h>
#include <string.h>


vector <char*> select_list;

RBlock::~RBlock()
{
	if (buffer != NULL){
		delete[] buffer;  //释放buffer块内存
	}
}

//is_want_empty=1，表示想要找文件中能插入记录的块
//is_want_empty=0, 表示想找文件中第offsetB块


int RBlock::getBlock(string filename, int offsetB, int is_want_empty)
{
	ifstream infile;

	infile.open(filename.c_str(), ios::binary | ios::in);  //二进制读写模式打开文件
	if (!infile) throw File_openfail(filename.c_str());     //若打开失败则抛出异常

	int offsetNow = 0;    //初始偏移块数
	char tmpc;  //临时储存从文件中读取的字符

	if (is_want_empty == 2){
		//找文件中第一个可以存放新纪录的块
		char* record;
		T.getTableInfo();  //读取表名为tablename的表的信息
		record = (char*)malloc(T.record_len);   //记录的长度(第一个字节用来记录该条记录是否存在or已删除)

		int i = 0;
		while (!infile.eof()){
			while (i < BLOCKSIZE){
				//向tmp内读入一个块的内容
				tmpc = infile.get();
				if (infile.eof()) break;
				buffer[i++] = tmpc;
			}
			if (isBlockFull(buffer, T.record_len) >= 0){
				//若读入的块还有存放记录的空间
				ptr = &buffer[0];   //指针一开始指向缓存区头部
				break;
			}
			memset(buffer, 0, BLOCKSIZE);
			offsetNow++;   //当前所读块的位置加一;
			i = 0;
		}
		infile.close();
		return offsetNow;
	}

	else if (is_want_empty == 0){
		//找文件中第offsetB块
		int i = 0;
		while (!infile.eof()){
			while (i < BLOCKSIZE && !infile.eof()){
				//向tmp内读入一个块的内容
				infile.read(&tmpc, 1);
				buffer[i++] = tmpc;
			}
			if (offsetNow == offsetB){
				//找到第offsetB块
				break;
			}
			else if (infile.eof()){
				infile.close();
				return -1;  //文件不够offsetB块大小，则返回-1表示找不到偏移量为offsetB块的位置
			}
			memset(buffer, 0, BLOCKSIZE);
			offsetNow++;
			i = 0;
		}
		infile.close();
		return offsetB;
	}
		
	else if(is_want_empty == 1){
		//要一整块空块
		//索引文件
		int i = 0;
		while (!infile.eof()){
			while (i < BLOCKSIZE){
				//向tmp内读入一个块的内容
				tmpc = infile.get();
				if (infile.eof()) break;
				buffer[i++] = tmpc;
			}
			int isEmpty;
			memcpy(&isEmpty, buffer + 4, 4);
			if (isEmpty == 0){
				//空块
				ptr = &buffer[0];   //指针一开始指向缓存区头部
				break;
			}
			memset(buffer, 0, BLOCKSIZE);
			offsetNow++;
			i = 0;
		}
		infile.close();
		return offsetNow;
	}
    return 0;
}


void RBlock::writeBackBlock(const char* filename, int offsetB)
{
	fstream iofile;
	iofile.open(filename, ios::binary | ios::in | ios::out);
	if (!iofile) throw File_openfail(filename);     //若打开失败则抛出异常
	
	//找到第offsetB块
	iofile.seekp(offsetB * BLOCKSIZE, ios::beg);
	iofile.write(buffer, BLOCKSIZE); //将缓冲区里的内容写回第offsetB块的位置
}

//返回文件中块数
int totalBlockNum(string filename)
{
	ifstream infile(filename.c_str(), ios::in);
	infile.seekg(0, ios::end); //定位到文件尾
	streampos pos = infile.tellg();  //文件长度
	return (pos / BLOCKSIZE);
}

int isBlockFull(char* buf, int reclen)
{
	if (buf == NULL) return 0;
	int i = 0;
	int lenCur = 0;
	while (lenCur < BLOCKSIZE){
		if (lenCur + reclen >= BLOCKSIZE) break;
		if (buf[lenCur] == '0' || buf[lenCur] == '\0'){
			//该条记录的标志位为0，说明此处没有记录（或曾经有记录但已经被删除），新纪录可以插入到这里
			return lenCur;
		}
		lenCur += reclen;
	}
	//buf中没有空位置存放新纪录，返回-1
	return -1;
}



//根据传来的tup处理待插入元组的信息
void Tuple::initializeTuple(TupleStruct* tup)
{
	T.getTableInfo();
	T.CalcRecordLen();
	if (T.attr_num == tup->value_num){
		//可以插入
		for (int i = 0; i < T.attr_num; i++){
			tuple_values[i] = tup->tuplevalues[i];
		}
	}
	else{
		//属性数和元组内值的数不符，报错
		cerr << "values number in tuple is less or more than attributes number. Insert failed" << endl;
		throw Grammer_error("insert");
	}
}

char* Tuple::convertTuple()
{
	char* value = new char[T.record_len];
	memset(value, 0, T.record_len);
	char *p = value;
	*p = '1';  p++;   //标记已插入记录
	for (int i = 0; i < T.attr_num; i++){
		if (T.attrs[i].attr_type == CHAR){
			//属性为char类型
			strcpy(p, tuple_values[i].data());
			p += T.attrs[i].attr_len;
		}
		else if (T.attrs[i].attr_type == INT){
			//属性为int类型
			int ivalue;
			ivalue = atoi(tuple_values[i].c_str());

			memcpy(p, &ivalue , 4);
			p += 4;
		}
		else if (T.attrs[i].attr_type == FLOAT){
			//属性为float类型
			float fvalue;
			fvalue = atof(tuple_values[i].c_str());

			memcpy(p, &fvalue, 4);
			p += 4;
		}
	}
	*p = '\0'; //标记结束

	return value;
}

bool Tuple::Insert(INT32 &position)
{
	int offsetblock;
	int lenCur = 0;   //当前偏移量
	RBlock* bufblock = new RBlock(T.table_name);
	string filename;
	filename = T.table_name + "_tablereco";

	if (!IsUniqueKeyExisted()){
		//若已存在的记录中有重复的单值属性
		return false;
	}
	offsetblock =  bufblock->getBlock(T.table_name + "_tablereco", 0, 2); //找到文件中可存入新记录的块偏移
	bufblock->ptr = &bufblock->buffer[0];
	while (lenCur < BLOCKSIZE){
		if (*(bufblock->ptr) == '0' || *(bufblock->ptr) == '\0'){
			//可插入新记录
			bufblock->is_Changed = 1; //缓存区被改变
			char* value = convertTuple();  //将元组的值转化为字符串
			memcpy(bufblock->ptr, value, T.record_len); //将值存到缓冲区
			position=PushPosition(offsetblock, lenCur, T.record_len);
			break;
		}
		lenCur += T.record_len;
		bufblock->ptr += T.record_len;
	}

	if (bufblock->is_Changed){
		bufblock->writeBackBlock(filename.c_str(), offsetblock);  //将这块buffer写回文件
		return true;
	}
	return false;
}

bool Tuple::IsUniqueKeyExisted()
{
	int offsetblock, offsetNow, maxoffset;
	ifstream infile;
	string filename = T.table_name + "_tablereco";

	RBlock* bufblock = new RBlock(T.getTableName());
	maxoffset = totalBlockNum(filename);  //文件中已有块数
	if (maxoffset == 0){
		//文件为空，还未写进任何记录
		return true;
	}

	int i, len;
	int is_Duplic = 0; //判断是否重复
	char *tmpAttr; //临时存放待比较的属性值
	for (offsetNow = 0; offsetNow < maxoffset; offsetNow++){
		offsetblock = bufblock->getBlock(T.table_name + "_tablereco", offsetNow, 0);  //找到第offsetNow块并读进缓冲块
		bufblock->ptr = &(bufblock->buffer[0]);
		for (len = 0; len < BLOCKSIZE; len += T.record_len){
			//从缓冲区中读出每条记录，并将每个属性的值与待插入记录比较。
			bufblock->ptr = &bufblock->buffer[len];
			if (*(bufblock->ptr) == '1'){
				//该条记录存在
				bufblock->ptr++; //指针后移一位
				for (i = 0; i < T.attr_num; i++){
					//对比每个属性
					if (T.attrs[i].attr_def_type == PRIMARY || T.attrs[i].attr_def_type == UNIQUE){
						//若属性为UNIQUE或PRIMARY，判断该属性值是否与已存在的记录重复
						if (T.attrs[i].attr_type == CHAR){
							//属性为char类型
							tmpAttr = new char[T.attrs[i].attr_len + 1];
							memset(tmpAttr, 0, T.attrs[i].attr_len + 1);
							memcpy(tmpAttr, bufblock->ptr, T.attrs[i].attr_len);  //将该条属性的值存在tmpAttr中
							if (tuple_values[i] == tmpAttr){
								//属性相同
								is_Duplic = 1;
								delete[] tmpAttr;
								break;
							}
						}
						else if (T.attrs[i].attr_type == INT){
							//属性为int类型
							int tmpivalue, ivalue;
							memcpy(&tmpivalue, bufblock->ptr, 4);
							ivalue = atoi(tuple_values[i].c_str());
							if (ivalue == tmpivalue){
								//属性相同
								is_Duplic = 1;
								break;
							}
						}
						else if (T.attrs[i].attr_type == FLOAT){
							//属性为float类型
							float tmpfvalue, fvalue;
							memcpy(&tmpfvalue, bufblock->ptr, 4);
							fvalue = atof(tuple_values[i].c_str());
							if (fvalue == tmpfvalue){
								//属性相同
								is_Duplic = 1;
								break;
							}
						}
					}//if:UNIQUE or PRIMARY
					bufblock->ptr += T.attrs[i].attr_len;  //指针后移一条记录的长度
				}//for (i = 0; i < T.attr_num; i++)
				if (is_Duplic){
					//若有重复的单值属性
					return false;
				}
			}//if:该位置处有记录存在
		}//for (len = 0; len < BLOCKSIZE; len += T.record_len)
	}//for (offsetNow = 0; offsetNow < maxoffset; offsetNow++)
	return true;
}


//查找
void Tuple::Select(vector<int> offsetlist, list<Condition> conditionlist)
{

	int offsetNow = 0; //当前偏移块
	int maxoffset = 0;     //最大偏移块
	int is_index_exist = 1;      //标记索引返回结果是否为空
	string filename = T.table_name + "_tablereco";
	select_list.clear();  //清空存放记录的vector

	list<Condition>::iterator it;
	RBlock* bufblock = new RBlock(T.getTableName());
	int vi = 0;  //vector的第i个元素
	int i, len, ptroffset;

	char* tmpAttr;
	int ivalue, tmpivalue;
	float fvalue, tmpfvalue;

	T.getTableInfo(); //获得表的信息

	int sign = *(offsetlist.end() - 1);
	offsetlist.pop_back(); //删除尾元素
	if (sign == 0 || sign == 1) {
		//查询条件原本就为空 or 查询条件不为空，但没有任何索引信息
		//从文件中顺序查找所有满足条件的记录
		maxoffset = totalBlockNum(filename);  //文件内最大块数
		//遍历每个块中的每条记录，判断条件
		for (offsetNow = 0; offsetNow < maxoffset; offsetNow++){
			offsetNow = bufblock->getBlock(T.table_name + "_tablereco", offsetNow, 0);
			bufblock->ptr = &(bufblock->buffer[0]);
			for (len = 0; len < BLOCKSIZE; len += T.record_len){
				//从缓冲区中读出每条记录
				bufblock->ptr = &bufblock->buffer[len];
				ptroffset = 0;
				if (*(bufblock->ptr) == '1'){
					//该条记录存在
					if (!conditionlist.empty()){
						//若有查询条件，则判断是否满足所有条件
						for (it = conditionlist.begin(); it != conditionlist.end(); it++){
							ptroffset = 1;  //从该条记录的首位开始查找属性
							for (i = 0; i < T.attr_num; i++){
								if (T.attrs[i].attr_name == it->compare_attr) break;
								ptroffset += T.attrs[i].attr_len;  //找到第i个属性的偏移位置
							}//找到对应属性
							if (T.attrs[i].attr_type == CHAR){
								//属性为char类型
								tmpAttr = new char[T.attrs[i].attr_len + 1];
								memset(tmpAttr, 0, T.attrs[i].attr_len + 1);
								memcpy(tmpAttr, bufblock->ptr + ptroffset, T.attrs[i].attr_len);  //将该条属性的值存在tmpAttr中
								if (it->compare_type == "="){
									if (tmpAttr != it->compare_value) break;
								}
								else if (it->compare_type == "<>"){
									if (tmpAttr == it->compare_value) break;
								}
								else if (it->compare_type == ">"){
									if (tmpAttr <= it->compare_value) break;
								}
								else if (it->compare_type == ">="){
									if (tmpAttr < it->compare_value) break;
								}
								else if (it->compare_type == "<"){
									if (tmpAttr >= it->compare_value) break;
								}
								else if (it->compare_type == "<="){
									if (tmpAttr > it->compare_value) break;
								}
							}
							else if (T.attrs[i].attr_type == INT){
								memcpy(&tmpivalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								ivalue = atoi(it->compare_value.c_str());
								if (it->compare_type == "="){
									if (tmpivalue != ivalue) break;
								}
								if (it->compare_type == "<>"){
									if (tmpivalue == ivalue) break;
								}
								if (it->compare_type == ">"){
									if (tmpivalue <= ivalue) break;
								}
								if (it->compare_type == ">="){
									if (tmpivalue > ivalue) break;
								}
								if (it->compare_type == "<"){
									if (tmpivalue >= ivalue) break;
								}
								if (it->compare_type == "<="){
									if (tmpivalue > ivalue) break;
								}
							}
							else if (T.attrs[i].attr_type == FLOAT){
								memcpy(&tmpfvalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								fvalue = atof(it->compare_value.c_str());
								if (it->compare_type == "="){
									if (tmpfvalue != fvalue) break;
								}
								if (it->compare_type == "<>"){
									if (tmpfvalue == fvalue) break;
								}
								if (it->compare_type == ">"){
									if (tmpfvalue <= fvalue) break;
								}
								if (it->compare_type == ">="){
									if (tmpfvalue > fvalue) break;
								}
								if (it->compare_type == "<"){
									if (tmpfvalue >= fvalue) break;
								}
								if (it->compare_type == "<="){
									if (tmpfvalue > fvalue) break;
								}
							}
						}
					}//若有查询条件
					if (conditionlist.empty() || it == conditionlist.end()){
						//若没有查询条件/或者满足所有的查询条件
						//则将此条记录插入到select_list中
						bufblock->ptr++;
						char* value = new char[T.record_len];
						memset(value, 0, T.record_len);
						memcpy(value, bufblock->ptr, T.record_len - 1);
						select_list.push_back(value);  //把该条记录值存入select_list
					}
				}//该条记录存在
			}
		}
	}
	
	else if (sign == 2){
		//查询条件不为空，有索引信息但没有符合条件的任何记录
		//select_list置空，直接return
		return;
	}

	else if (sign == 3){
		//正常情况
		INT32 blockAddr = 0, offset = 0;		
		vector<int>::iterator offsetIt;
		for (offsetIt = offsetlist.begin(); offsetIt != offsetlist.end(); offsetIt++){
			//遍历offsetlist
			PopPosition((*offsetIt), T.record_len, blockAddr, offset);
			bufblock->getBlock(T.table_name + "_tablereco", blockAddr, 0);
			bufblock->ptr = &bufblock->buffer[offset];
			ptroffset = 0;
			if (*(bufblock->ptr) == '1'){
				//该条记录存在
				if (!conditionlist.empty()){
					//若有查询条件，则判断是否满足所有条件
					for (it = conditionlist.begin(); it != conditionlist.end(); it++){
						ptroffset = 1;  //从该条记录的首位开始查找属性
						for (i = 0; i < T.attr_num; i++){
							if (T.attrs[i].attr_name == it->compare_attr) break;
							ptroffset += T.attrs[i].attr_len;  //找到第i个属性的偏移位置
						}//找到对应属性
						if (T.attrs[i].attr_type == CHAR){
							//属性为char类型
							tmpAttr = new char[T.attrs[i].attr_len + 1];
							memset(tmpAttr, 0, T.attrs[i].attr_len + 1);
							memcpy(tmpAttr, bufblock->ptr + ptroffset, T.attrs[i].attr_len);  //将该条属性的值存在tmpAttr中
							if (it->compare_type == "="){
								if (tmpAttr != it->compare_value) break;
							}
							else if (it->compare_type == "<>"){
								if (tmpAttr == it->compare_value) break;
							}
							else if (it->compare_type == ">"){
								if (tmpAttr <= it->compare_value) break;
							}
							else if (it->compare_type == ">="){
								if (tmpAttr < it->compare_value) break;
							}
							else if (it->compare_type == "<"){
								if (tmpAttr >= it->compare_value) break;
							}
							else if (it->compare_type == "<="){
								if (tmpAttr > it->compare_value) break;
							}
						}
						else if (T.attrs[i].attr_type == INT){
							memcpy(&tmpivalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
							ivalue = atoi(it->compare_value.c_str());
							if (it->compare_type == "="){
								if (tmpivalue != ivalue) break;
							}
							if (it->compare_type == "<>"){
								if (tmpivalue == ivalue) break;
							}
							if (it->compare_type == ">"){
								if (tmpivalue <= ivalue) break;
							}
							if (it->compare_type == ">="){
								if (tmpivalue > ivalue) break;
							}
							if (it->compare_type == "<"){
								if (tmpivalue >= ivalue) break;
							}
							if (it->compare_type == "<="){
								if (tmpivalue > ivalue) break;
							}
						}
						else if (T.attrs[i].attr_type == FLOAT){
							memcpy(&tmpfvalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
							fvalue = atof(it->compare_value.c_str());
							if (it->compare_type == "="){
								if (tmpfvalue != fvalue) break;
							}
							if (it->compare_type == "<>"){
								if (tmpfvalue == fvalue) break;
							}
							if (it->compare_type == ">"){
								if (tmpfvalue <= fvalue) break;
							}
							if (it->compare_type == ">="){
								if (tmpfvalue > fvalue) break;
							}
							if (it->compare_type == "<"){
								if (tmpfvalue >= fvalue) break;
							}
							if (it->compare_type == "<="){
								if (tmpfvalue > fvalue) break;
							}
						}
					}
				}//若有查询条件
				if (conditionlist.empty() || it == conditionlist.end()){
					//若没有查询条件/或者满足所有的查询条件
					//则将此条记录插入到select_list中
					bufblock->ptr++;
					char* value = new char[T.record_len];
					memset(value, 0, T.record_len);
					memcpy(value, bufblock->ptr, T.record_len - 1);
					select_list.push_back(value);  //把该条记录值存入select_list
				}
			}//该条记录存在
		}
	}
	return;
}

char value[32][256];
void Tuple::printSelectResult()
{
	char* tuplevalue;
	for (int vi = 0; vi < select_list.size(); vi++){
		tuplevalue = select_list.at(vi);
		for (int i = 0; i < T.attr_num; i++){
			memset(value[i], 0, T.attrs[i].attr_len+1);
			memcpy(value[i], tuplevalue, T.attrs[i].attr_len);
			tuplevalue += T.attrs[i].attr_len;
		}
		int length[32],totalLength;
		totalLength=getPrintLength(length);
		if (select_list.size()==1)
			PrintTuple(totalLength, 3, length);//既是最后一条也是第一条
		else if (vi == select_list.size()-1)
			PrintTuple(totalLength, 2, length);//最后一条记录
		else if (vi == 0)
			PrintTuple(totalLength, 0, length);//第一条记录
		else 
			PrintTuple(totalLength, 1, length);
	}

	if (select_list.size() == 0){
		cout << "Empty set."<< endl;
	}
	else if (select_list.size() == 1){
		cout << "1 row in set." << endl;
	}
	else{
		cout << select_list.size() << " row in set." << endl;
	}

/*	char* tuplevalue = new char[T.record_len];
	char* charvalue;
	int intvalue;
	float floatvalue;
	int i;

	for (int vi = 0; vi < select_list.size(); vi++){
		tuplevalue = select_list.at(vi);
		for (i = 0; i < T.attr_num; i++){
			if (T.attrs[i].attr_type == CHAR){
				//char类型
				charvalue = new char[T.attrs[i].attr_len];
				memset(charvalue, 0, T.attrs[i].attr_len);
				memcpy(charvalue, tuplevalue, T.attrs[i].attr_len); //得到该属性值的字符串
				cout << T.attrs[i].attr_name << ":" << charvalue << endl;
				delete[] charvalue;
			}
			else if (T.attrs[i].attr_type == INT){
				//INT类型
				memcpy(&intvalue, tuplevalue, 4);  //得到该属性的值
				cout << T.attrs[i].attr_name << ":" << intvalue << endl;
			}
			else if (T.attrs[i].attr_type == FLOAT){
				//FLOAT类型
				memcpy(&floatvalue, tuplevalue, 4);  //得到该属性的值
				cout << T.attrs[i].attr_name << ":" << floatvalue << endl;
			}
			tuplevalue += T.attrs[i].attr_len;
		}
	}*/
}

int Tuple::getPrintLength(int *length)
{
	int sum = 1;
	for (int i = 0; i<T.attr_num; i++){
		int al = T.attrs[i].attr_type == CHAR ? T.attrs[i].attr_len : 12;
		int len = al>T.attrs[i].attr_name.size() ? al:T.attrs[i].attr_name.size();
		length[i] = len;
		sum += length[i]+3;
	}
	return sum;
}

void Tuple::PrintTuple(int totalLength, int FirstOrEnd,int *length)
{
	if (FirstOrEnd == 0 || FirstOrEnd == 3){
		for (int i = 0; i<totalLength; i++){
			cout << '-';
		}
		cout << endl;
		for (int i = 0; i<T.attr_num; i++){
			//int length = T.attrs[i].attr_len>T.attrs[i].attr_name.size() ? T.attrs[i].attr_len : T.attrs[i].attr_name.size();
			cout << "| ";
			cout << setw(length[i]) << T.attrs[i].attr_name;
			cout << " ";
		}
		cout << "|" << endl;
		for (int i = 0; i<totalLength; i++){
			cout << '-';
		}
		cout << endl;
	}
	for (int i = 0; i<T.attr_num; i++){
		int type = T.attrs[i].attr_type;
		cout << "| ";
		printValue(i,type,length[i]);
		cout << " ";
	}
	cout << "|" << endl;
	if (FirstOrEnd == 2 || FirstOrEnd == 3){
		for (int i = 0; i<totalLength; i++){
			cout << '-';
		}
		cout << endl;
	}
}

void Tuple::printValue(int i, int type,int width)
{
	if (type == CHAR){
		cout << setw(width) << value[i];
	}
	else if (type == FLOAT){
		float floatvalue;
		memcpy(&floatvalue,value[i],4);
		cout << setw(width) << floatvalue;
	}
	else if (type == INT){
		int intvalue;
		memcpy(&intvalue, value[i], 4);
		cout << setw(width) << intvalue;
	}
}

int Tuple::Delete(vector<int> offsetlist, list<Condition> conditionlist)
{
	int offsetNow = 0; //当前偏移块
	int maxoffset = 0;     //最大偏移块
	string filename = T.table_name + "_tablereco";
	select_list.clear();  //清空存放记录的vector

	list<Condition>::iterator it;
	RBlock* bufblock = new RBlock(T.getTableName());
	int vi = 0;  //vector的第i个元素
	int i, len, ptroffset;
	int deleteNum = 0;  //删除的记录数 

	char* tmpAttr;
	int ivalue, tmpivalue;
	float fvalue, tmpfvalue;

	T.getTableInfo(); //获得表的信息

	//找到表中所有索引 用于b+树的删除
	list<IndexInfo> indexlist;
	IndexInfo idxinfo;
	for (int num = 0; num < T.attr_num; num++){
		idxinfo.indexname = T.getAttrIndex(T.getAttr(num).attr_name);
		idxinfo.attr_size = T.getAttr(num).attr_len;
		idxinfo.maxKeyNum = (4096 - 4 - INDEX_BLOCK_INFO) / (4 + idxinfo.attr_size);

		indexlist.push_back(idxinfo);
	}

	int sign = *(offsetlist.end() - 1);
	offsetlist.pop_back(); //删除尾元素
	if (sign == 0 || sign == 1) {
		//查询条件原本就为空 or 查询条件不为空，但没有任何索引信息
		//从文件中顺序查找所有满足条件的记录
		maxoffset = totalBlockNum(filename);  //文件内最大块数
		//遍历每个块中的每条记录，判断条件
		for (offsetNow = 0; offsetNow < maxoffset; offsetNow++){
			offsetNow = bufblock->getBlock(T.table_name + "_tablereco", offsetNow, 0);
			bufblock->ptr = &(bufblock->buffer[0]);
			bufblock->is_Changed = 0;
			for (len = 0; len < BLOCKSIZE; len += T.record_len){
				//从缓冲区中读出每条记录
				bufblock->ptr = &bufblock->buffer[len];
				ptroffset = 0;
				if (*(bufblock->ptr) == '1'){
					//该条记录存在
					if (!conditionlist.empty()){
						//若有查询条件，则判断是否满足所有条件
						for (it = conditionlist.begin(); it != conditionlist.end(); it++){
							ptroffset = 1;  //从该条记录的首位开始查找属性
							for (i = 0; i < T.attr_num; i++){
								if (T.attrs[i].attr_name == it->compare_attr) break;
								ptroffset += T.attrs[i].attr_len;  //找到第i个属性的偏移位置
							}//找到对应属性
							if (T.attrs[i].attr_type == CHAR){
								//属性为char类型
								tmpAttr = new char[T.attrs[i].attr_len + 1];
								memset(tmpAttr, 0, T.attrs[i].attr_len + 1);
								memcpy(tmpAttr, bufblock->ptr + ptroffset, T.attrs[i].attr_len);  //将该条属性的值存在tmpAttr中
								if (it->compare_type == "="){
									if (tmpAttr != it->compare_value) break;
								}
								else if (it->compare_type == "<>"){
									if (tmpAttr == it->compare_value) break;
								}
								else if (it->compare_type == ">"){
									if (tmpAttr <= it->compare_value) break;
								}
								else if (it->compare_type == ">="){
									if (tmpAttr < it->compare_value) break;
								}
								else if (it->compare_type == "<"){
									if (tmpAttr >= it->compare_value) break;
								}
								else if (it->compare_type == "<="){
									if (tmpAttr > it->compare_value) break;
								}
							}
							else if (T.attrs[i].attr_type == INT){
								memcpy(&tmpivalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								ivalue = atoi(it->compare_value.c_str());
								if (it->compare_type == "="){
									if (tmpivalue != ivalue) break;
								}
								if (it->compare_type == "<>"){
									if (tmpivalue == ivalue) break;
								}
								if (it->compare_type == ">"){
									if (tmpivalue <= ivalue) break;
								}
								if (it->compare_type == ">="){
									if (tmpivalue > ivalue) break;
								}
								if (it->compare_type == "<"){
									if (tmpivalue >= ivalue) break;
								}
								if (it->compare_type == "<="){
									if (tmpivalue > ivalue) break;
								}
							}
							else if (T.attrs[i].attr_type == FLOAT){
								memcpy(&tmpfvalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								fvalue = atof(it->compare_value.c_str());
								if (it->compare_type == "="){
									if (tmpfvalue != fvalue) break;
								}
								if (it->compare_type == "<>"){
									if (tmpfvalue == fvalue) break;
								}
								if (it->compare_type == ">"){
									if (tmpfvalue <= fvalue) break;
								}
								if (it->compare_type == ">="){
									if (tmpfvalue > fvalue) break;
								}
								if (it->compare_type == "<"){
									if (tmpfvalue >= fvalue) break;
								}
								if (it->compare_type == "<="){
									if (tmpfvalue > fvalue) break;
								}
							}
						}
					}//若有查询条件
					if (conditionlist.empty() || it == conditionlist.end()){
						//若没有查询条件/或者满足所有的查询条件
						//则将此条记录删除，并将结果写入缓冲块，当读完一块时回写
						*bufblock->ptr = '0';  //第一位标记删除标志
						deleteNum++;
						bufblock->is_Changed = 1; //缓冲块被改变，需要回写
					}
				}//该条记录存在
				if (bufblock->is_Changed){
					//若需要回写
					bufblock->writeBackBlock(filename.c_str(), offsetNow);  //将这块buffer写回文件
				}
			}
		}
	}

	else if (sign == 2){
		//查询条件不为空，有索引信息但没有符合条件的任何记录
		return 0;
	}

	else if (sign == 3){
		//正常情况
		INT32 blockAddr = 0, offset = 0;
		vector<int>::iterator offsetIt;
		for (offsetIt = offsetlist.begin(); offsetIt != offsetlist.end(); offsetIt++){
			//遍历offsetlist
			PopPosition((*offsetIt), T.record_len, blockAddr, offset);
			bufblock->getBlock(T.table_name + "_tablereco", blockAddr, 0);
			bufblock->ptr = &bufblock->buffer[offset];
			ptroffset = 0;
			if (*(bufblock->ptr) == '1'){
				//该条记录存在
				if (!conditionlist.empty()){
					//若有查询条件，则判断是否满足所有条件
					for (it = conditionlist.begin(); it != conditionlist.end(); it++){
						ptroffset = 1;  //从该条记录的首位开始查找属性
						for (i = 0; i < T.attr_num; i++){
							if (T.attrs[i].attr_name == it->compare_attr) break;
							ptroffset += T.attrs[i].attr_len;  //找到第i个属性的偏移位置
						}//找到对应属性
						if (T.attrs[i].attr_type == CHAR){
							//属性为char类型
							tmpAttr = new char[T.attrs[i].attr_len + 1];
							memset(tmpAttr, 0, T.attrs[i].attr_len + 1);
							memcpy(tmpAttr, bufblock->ptr + ptroffset, T.attrs[i].attr_len);  //将该条属性的值存在tmpAttr中
							if (it->compare_type == "="){
								if (tmpAttr != it->compare_value) break;
							}
							else if (it->compare_type == "<>"){
								if (tmpAttr == it->compare_value) break;
							}
							else if (it->compare_type == ">"){
								if (tmpAttr <= it->compare_value) break;
							}
							else if (it->compare_type == ">="){
								if (tmpAttr < it->compare_value) break;
							}
							else if (it->compare_type == "<"){
								if (tmpAttr >= it->compare_value) break;
							}
							else if (it->compare_type == "<="){
								if (tmpAttr > it->compare_value) break;
							}
						}
						else if (T.attrs[i].attr_type == INT){
							memcpy(&tmpivalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
							ivalue = atoi(it->compare_value.c_str());
							if (it->compare_type == "="){
								if (tmpivalue != ivalue) break;
							}
							if (it->compare_type == "<>"){
								if (tmpivalue == ivalue) break;
							}
							if (it->compare_type == ">"){
								if (tmpivalue <= ivalue) break;
							}
							if (it->compare_type == ">="){
								if (tmpivalue > ivalue) break;
							}
							if (it->compare_type == "<"){
								if (tmpivalue >= ivalue) break;
							}
							if (it->compare_type == "<="){
								if (tmpivalue > ivalue) break;
							}
						}
						else if (T.attrs[i].attr_type == FLOAT){
							memcpy(&tmpfvalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
							fvalue = atof(it->compare_value.c_str());
							if (it->compare_type == "="){
								if (tmpfvalue != fvalue) break;
							}
							if (it->compare_type == "<>"){
								if (tmpfvalue == fvalue) break;
							}
							if (it->compare_type == ">"){
								if (tmpfvalue <= fvalue) break;
							}
							if (it->compare_type == ">="){
								if (tmpfvalue > fvalue) break;
							}
							if (it->compare_type == "<"){
								if (tmpfvalue >= fvalue) break;
							}
							if (it->compare_type == "<="){
								if (tmpfvalue > fvalue) break;
							}
						}
					}
				}//若有查询条件
				if (conditionlist.empty() || it == conditionlist.end()){
					//若没有查询条件/或者满足所有的查询条件
					//则将此条记录删除，并将结果写入缓冲块，当读完一块时回写
					*bufblock->ptr = '0';  //第一位标记删除标志
					deleteNum++;
					bufblock->is_Changed = 1; //缓冲块被改变，需要回写
					//将此条记录从B+树中删除
					int j;
					ptroffset = 1;
					list<IndexInfo>::iterator li;
					for (j = 0, li = indexlist.begin(); j < T.attr_num, li != indexlist.end(); j++, li++){
						if (li->indexname != ""){
							//有索引
							if (T.attrs[j].attr_type == CHAR){
								//属性为char类型
								tmpAttr = new char[T.attrs[j].attr_len + 1];
								memset(tmpAttr, 0, T.attrs[j].attr_len + 1);
								memcpy(tmpAttr, bufblock->ptr + ptroffset, T.attrs[j].attr_len);
								string tmp = tmpAttr;
								Delete_Key_From_Index(*li, tmp);
								delete[] tmpAttr;
							}
							else if (T.attrs[j].attr_type == INT){
								memcpy(&tmpivalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								Delete_Key_From_Index(*li, tmpivalue);
							}
							else if (T.attrs[j].attr_type == FLOAT){
								memcpy(&tmpfvalue, bufblock->ptr + ptroffset, 4);  //将该条属性的值存在tmpivalue中
								Delete_Key_From_Index(*li, tmpfvalue);
							}
						}
						ptroffset += T.attrs[j].attr_len;
					}
				}
			}//该条记录存在
			if (bufblock->is_Changed){
				//若需要回写
				bufblock->writeBackBlock(filename.c_str(), blockAddr);  //将这块buffer写回文件
			}
		}
	}
	return deleteNum;
}
vector<TupleIndex_number> TupleList_IDX;  //全局

void Tuple::FetchAll(int AttrId)
{
	T.getTableInfo();

	int offsetblock, offsetNow, maxoffset;
	ifstream infile;
	string filename = T.table_name + "_tablereco";

	RBlock* bufblock = new RBlock(T.getTableName());
	maxoffset = totalBlockNum(filename);  //文件中已有块数
	if (maxoffset == 0){
		//文件为空，还未写进任何记录
		return;
	}

	int i, len;
	char *tmpAttr; //临时存放待比较的属性值
	int delta = 0;
	for (int j = 0; j < AttrId; j++){
		delta += T.attrs[j].attr_len;
	}
	for (offsetNow = 0; offsetNow < maxoffset; offsetNow++){
		offsetblock = bufblock->getBlock(T.table_name + "_tablereco", offsetNow, 0);  //找到第offsetNow块并读进缓冲块
		bufblock->ptr = &(bufblock->buffer[0]);
		for (len = 0; len < BLOCKSIZE; len += T.record_len){
			//从缓冲区中读出每条记录，并将每个属性的值与待插入记录比较。
			bufblock->ptr = &bufblock->buffer[len];
			if (*(bufblock->ptr) == '1'){
				//该条记录存在
				bufblock->ptr++; //指针后移一位
				int sum = 0;
				for (i = 0; i < AttrId; i++){
					sum += T.attrs[i].attr_len;
				}//for (i = 0; i < T.attr_num; i++)
				char tmpAttr[256];
				memcpy(tmpAttr, bufblock->ptr + delta, T.attrs[AttrId].attr_len);
				TupleIndex tpl;
				PushPosition(offsetNow, len, tmpAttr, T.attrs[AttrId].attr_type, T.record_len);
			}//if:该位置处有记录存在
		}//for (len = 0; len < BLOCKSIZE; len += T.record_len)
	}//for (offsetNow = 0; offsetNow < maxoffset; offsetNow++)
	//cout << "Fetch All" << TupleList_IDX.size() << endl;
}

void PushPosition(INT32 BlockAddr, INT32 Offset, char* key, int type, INT32 recordLength)
{
	TupleIndex_number tuple;
	INT32 position;
	int maxKeySize = (4096 - 4) / recordLength;
	position = maxKeySize*BlockAddr + Offset / recordLength;
	tuple.position = position;
	tuple.type = type;
	char buffer[256];
	if (type == INT){
		int intvalue;
		memcpy(&intvalue,key,4);
		tuple.key = "";
		tuple.intkey = intvalue;
		tuple.floatkey = 0;
	}
	else if (type == FLOAT){
		float floatvalue;
		memcpy(&floatvalue, key, 4);
		tuple.key = "";
		tuple.intkey = 0;
		tuple.floatkey = floatvalue;
	}
	else{
		tuple.key = key;
		tuple.intkey = 0;
		tuple.floatkey = 0;
	}

	TupleList_IDX.push_back(tuple);
}

void PopPosition(INT32 position, INT32 recordLength, INT32 &BlockAddr, INT32 &Offset)
{
	int maxKeySize = (4096 - 4) / recordLength;
	BlockAddr = position / maxKeySize;
	Offset = (position%maxKeySize)*recordLength;
}

INT32 PushPosition(INT32 BlockAddr, INT32 Offset, INT32 recordLength)
{
	INT32 position;
	int maxKeySize = (4096 - 4) / recordLength;
	position = maxKeySize*BlockAddr + Offset/recordLength;
	return position;
}