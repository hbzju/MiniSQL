#include "BPlus_Tree.h"

INT32 Left_First_Node;
INT32 FirstEmptyBlockAddr=1;

node::node()
{
	block = NULL;
	BlockAddr = 0;
	maxsize = 0;
	keynum = 0;
	parent = 0;
	type = 0;
	parent = NULL;
	extra = 0;
}

node::node(IndexInfo idx, INT32 offset, node* parent)
{
	block = Block::getBlock(idx.indexname, offset,0);
	char *values = block->buffer;
	BlockAddr = offset;
	memcpy(&type, values, 4);
	memcpy(&keynum, values + 4, 4);
	//cout << values[5] << endl;
	//cout << keynum << ' ' << type << ' ' <<BlockAddr << endl;
	if (IsRoot()){
		this->parent = NULL;
		extra = -1;
		memcpy(&Left_First_Node, block->buffer + 8, 4);
		FirstEmptyBlockAddr = (Left_First_Node >> 16) & 0x0000FFFF;
		Left_First_Node = Left_First_Node & 0x0000FFFF;
		if (FirstEmptyBlockAddr == 0)FirstEmptyBlockAddr = 1;
		//cout << FirstEmptyBlockAddr << ' ' << Left_First_Node << endl;
	}
	else{
		this->parent = parent;
		memcpy(&extra, block->buffer + 8, 4);
	}
	maxsize = idx.maxKeyNum;
	index = idx;
	//cout << keynum << ' ' << type << ' ' << maxsize << ' ' << BlockAddr << endl;
}

bool node::IsLeaf()
{
	if (type<2)return true;
	else return false;
}

bool node::IsFull()
{
	if (keynum >= maxsize)return true;
	else return false;
}

bool node::IsHalfFull()
{
	int hf;
	if (IsLeaf()){
		hf = maxsize / 2;
		if (maxsize % 2)hf++;
	}
	else{
		hf = (maxsize + 1) / 2;
		if ((maxsize + 1) % 2 == 0)hf--;
		// up_bound(n/2) -1
	}
	if (keynum <= hf)return true;
	else return false;
}

bool node::IsRoot()
{
	if (type == 0 || type == 3)return true;
	else return false;
}

INT32 node::Find_Real_Brother(INT32 blk_ofst, bool &flag)
{
	int position;
	if (flag){
		position = blk_ofst + index.attr_size;
	}
	else{
		//cout << "00A0D0AWIR214" << endl;
		position = blk_ofst - 8 - index.attr_size;
	}
	INT32 sibling;
	memcpy(&sibling, block->buffer + position, 4);
	return sibling;
}