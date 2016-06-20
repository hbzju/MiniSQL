#define _CRT_SECURE_NO_WARNINGS
#include "block.h"
#include "minisql.h"
#include <fstream> 
#include <vector>
#include <algorithm>
#include "BPlus_Tree.h"

Block* Block::block = NULL;
bool flag=true;

Block::Block()
{
	int i;
	if (flag){
		flag = false;
		Block::block = new Block[MAX_BLOCKS];
		for (i = 0; i < MAX_BLOCKS; i++){
			block[i].buffer = new char[BLOCKSIZE];
			block[i].ptr = NULL;
			block[i].name = "";
			block[i].offset = 0;
			block[i].is_changed = 0;
			block[i].in_use = 0;
			block[i].is_locked = 0;
			block[i].count = 0;
		}
	}
}

Block::~Block()
{
}

INT32 Block::find_lru_block()                                        //find the block which is least recently used
{
	int i;
	int max = 0;
	INT32 block_num = 0;
	for (i = 0; i < MAX_BLOCKS; i++)
	{
		if (block[i].count > max)
		{
			max = block[i].count;
			block_num = i;
		}
	}
	return block_num;
}

Block* Block::getBlock(string filename, INT32 offsetB, int is_want_empty) //if is_want_empty==1, get a empty block, else get the block with offsetB
{
	string filename_1;
	//cout << "getBlock: filename:" <<filename<<' '<< offsetB << endl;
	int i;
	INT32 num;
	int flag = 0;
	FILE* in;


	for (i = 0; i < MAX_BLOCKS; i++)
	{
		if (block[i].name == filename&&block[i].offset == offsetB)
		{
			update_count(i);
			return block + i;
		}
	}


	for (i = 0; i < MAX_BLOCKS; i++)
	{
		if (block[i].in_use == 0 && block[i].is_locked == 0)
		{
			break;
		}
	}

	if (i < MAX_BLOCKS)                                             //Exists empty block
	{
		num = i;
		memset(block[i].buffer, 0, 4096);
	}
	else
	{
		num = find_lru_block();                                     //LRU algorithm flush one block
		block[num].flush_block();
	}
	update_count(num);
	if (is_want_empty) {
		block[num].name = filename;
		block[num].offset = offsetB;
		block[num].is_changed = 1;
		return block + num;
	}

	block[num].name = filename;
	block[num].offset= offsetB;
	block[num].is_changed = 1;
	filename_1 = filename;
	if (in = fopen(filename_1.c_str(), "r"))
	{
		fseek(in, offsetB*BLOCKSIZE, 0);
		fread(block[num].buffer, BLOCKSIZE, 1, in);
		fclose(in);
	}
	return block+num;
}

void Block::flush_all_blocks()                                   //Write the data in the buffer to the file
{
	int i;
	for (i = 0; i < MAX_BLOCKS; i++)
	{
		if (block[i].is_changed == 0)continue;
		block[i].flush_block();
		memset(block[i].buffer,0,BLOCKSIZE);
	}
}

void Block::flush_block()                                        //Write the data of one block to the file
{
	fstream iofile;
	string filename;
	filename = name;
	//cout << "Flush : filename: " << name << offset << endl;
	if (is_changed)
	{
		iofile.open(filename.c_str(), ios::binary | ios::in | ios::out);
		if (iofile)
		{
			iofile.seekp(BLOCKSIZE*offset, ios::beg);
			iofile.write(buffer, BLOCKSIZE);
			iofile.close();
		}
	}
	name = "";
	offset = 0;
	is_changed = 0;
	in_use = 0;
	is_locked = 0;
	count = 0;
}

void Block::update_count(int num)                                //after using a block, update all blocks' count
{
	block[num].count = 0;
	block[num].in_use = 1;
	for (int i = 0; i < MAX_BLOCKS; i++)                        
	{
		if (block[i].in_use&&i != num&&block[i].is_locked == 0)
			block[i].count++;
	}
}

void Block::lock()                                               //Lock a buffer page, not allowed to be replaced out
{
	is_locked = 1;
}

void Block::change()                                             // let the block is changed
{
	is_changed = 1;
}

void Block::out_of_use()                                         //let the block is out of use
{
	in_use = 0;
}

void Block::Print()
{
	char *indexBlock = block[1].buffer;
	int type, keynum;
	memcpy(&type, indexBlock, 4);
	memcpy(&keynum, indexBlock + 4, 4);
	int ofst = 12;
	cout << type << ' ' << keynum << ' ' << endl;
	for (int i = 0; i < keynum; i++){
		int position;
		int key;
		memcpy(&position, indexBlock + ofst, 4);
		getElement(&key, indexBlock + ofst + 4, 4);
		ofst += 8;
		cout << position << ' ' << key << endl;
	}
	//验证插入模块是否正确
	int next;
	memcpy(&next, indexBlock + ofst, 4);
}