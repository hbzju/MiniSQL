#include "IndexManager.h"
#include "Interpreter.h"

bool Create_Index_Init(IndexInfo index)
{
	Block *block;
	block=Block::getBlock(index.indexname, 0,1);
	if (block == NULL)return false;
	char *init_info = block->buffer;
	int type = 0;
	int keynum = 0;
	int parent = 0;
	memset(init_info, 0, BLOCKSIZE);
	memcpy(init_info, &type, 4);//root with no son
	memcpy(init_info + 4, &keynum, 4);//store key number of each block
	memcpy(init_info + 8, &parent, 4);
	return true;
}
extern vector<TupleIndex_number> TupleList_IDX;

/*void Create_Index(IndexInfo index)
{
	Create_Index_Init(index);
	for (int i = 0; i<int(TupleList_IDX.size()); i++){
		node root(index, 0, 0);
		if (TupleList_IDX[i].type == INT){
			root.Insert_Key(TupleList_IDX[i].intkey, TupleList_IDX[i].position);
			//cout << TupleList_IDX[i].intkey << 'P' << TupleList_IDX[i].position << endl;
		}
		else if (TupleList_IDX[i].type == FLOAT){
			root.Insert_Key(TupleList_IDX[i].floatkey, TupleList_IDX[i].position);
		}
		else{
			//char temp[256];
			//memcpy(temp, TupleList_IDX[i].key.c_str(), TupleList_IDX[i].key.length() + 1);
			//string key = temp;
			root.Insert_Key(TupleList_IDX[i].key, TupleList_IDX[i].position);
		}
	}
}*/
void Create_Index(IndexInfo index)
{
	Create_Index_Init(index);
	for (int i = 0; i<int(TupleList_IDX.size()); i++){
		node root(index, 0, 0);
		if (TupleList_IDX[i].type == INT){
			root.Insert_Key(TupleList_IDX[i].intkey, TupleList_IDX[i].position);
			//cout << TupleList_IDX[i].intkey << 'P' << TupleList_IDX[i].position << endl;
		}
		else if (TupleList_IDX[i].type == FLOAT){
			root.Insert_Key(TupleList_IDX[i].floatkey, TupleList_IDX[i].position);
		}
		else{
			//char temp[256];
			//memcpy(temp, TupleList_IDX[i].key.c_str(), TupleList_IDX[i].key.length() + 1);
			//string key = temp;
			root.Insert_Key(TupleList_IDX[i].key, TupleList_IDX[i].position);
		}
		root.Update_Extra();
	}
	int listSize = TupleList_IDX.size();
	for (int i = 0; i < listSize; i++){
		TupleList_IDX.pop_back();
	}
}


void Insert_Key_To_Index(IndexInfo index, TupleIndex tuple)
{
	node root(index, 0, 0);
	//try{
		if (tuple.type == INT){
			int key;
			key=atoi(tuple.key.c_str());
			root.Insert_Key(key, tuple.position);
		}
		else if (tuple.type == FLOAT){
			float key;
			key = atof(tuple.key.c_str());
			root.Insert_Key(key, tuple.position);
		}
		else{
			char temp[256];
			memcpy(&temp, tuple.key.c_str(), tuple.key.length() + 1);
			string key = temp;
			root.Insert_Key(key, tuple.position);
		}
		root.Update_Extra();
		//更新extra信息，保证第一个空块和最左侧节点的值被记录
	//}
	/*catch (...){
		cout << "Insert Error!" << endl;
	}*/
}

