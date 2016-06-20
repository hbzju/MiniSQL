#ifndef __block_h__
#define __block_h__

#include <sstream>
#include <string>
#include <iostream>

#define MAX_BLOCKS 16
#define INT32 unsigned int

using namespace std;

class Block
{
public:
	char* buffer;				    //block's address
	char* ptr;                      //block's value
	string name;					//file's name
	INT32 offset;	                //the offset number of records corresponding the block
	int is_changed;			        //record the status of each block in the buffer. if the block is changed, it turn to 1,and it means that this is a dirty block, else it is equal to 0 (it used when writing back to disk)
	int in_use;			            //record the status of each block in the buffer. if the block is in use, it is equal to 1, else it equal to 0 
	int is_locked;                  //whether the block is locked
	int count;			            //record the use time of the block

	Block();
	~Block();
	static Block* getBlock(string filename, INT32 offsetB, int is_want_empty);   // read the specified data to the system buffer     
	static void flush_all_blocks();                                    //Write the data in the buffer to the file
	void lock();                                                       //Lock a buffer page, not allowed to be replaced out
	void change();                                                     //let the block is changed
	void out_of_use();                                                 //let the block is out of use 
	static void Print();

protected:
	static Block *block;
	static void update_count(int number);                               //after using a block, update all blocks' count
	static INT32 find_lru_block();                                      //find the block which is least recently used
	void flush_block();                                                 //Write the data of one block to the file
};

#endif