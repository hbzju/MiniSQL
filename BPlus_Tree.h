#ifndef BPlus_Tree_hpp
#define BPlus_Tree_hpp

#include "Interpreter.h"
#include "minisql.h"
#include "vector"

class NoKeyDeleted_Error{};

//#define INT32 unsigned int
#define MAX_STR_LENGTH 256

const int const_number[] = { 0, 1, 2, 3, 4 };
const int LEFT_FLAG = 1, RIGHT_FLAG = 2;
extern INT32 Left_First_Node;
extern INT32 FirstEmptyBlockAddr;

class node
{
protected:
    Block *block;
    INT32 BlockAddr;
    INT32 maxsize;
    INT32 keynum;
    node* parent;
    INT32 type;
    INT32 extra;
    IndexInfo index;
public:
    node(IndexInfo idx, INT32 offset, node* parent);
    node();
    INT32 getMaxSize(){ return maxsize; }
    INT32 getType(){ return type; }
    node* getParent(){ return parent; }
    INT32 getBlockAddr(){ return BlockAddr; }
    INT32 getKeynum(){ return keynum; }
    Block *getBlock_Ptr(){ return block; }
    void setMaxSize(INT32 maxsize){ this->maxsize = maxsize; }
    void setType(INT32 type){ this->type = type; }
    void setParent(node* parent){ this->parent = parent; }
    void setBlockAddr(INT32 addr){ BlockAddr = addr; }
    void setKeynum(int kn){ keynum = kn; }
    void setBlock(Block *blk){ block = blk; }
    bool IsLeaf();
    bool IsFull();
    bool IsHalfFull();
    bool IsRoot();
    template<class T>
    void Insert_Key(T key, INT32 offset);
    template<class T>
    int Find_Position(T key);
    template<class T>
    void Split_Node(T key, int blk_ofst, INT32 position);
    template<class T>
    void Split_Internal_Node(T key, int blk_ofst, INT32 position);
    template<class T>
    void Insert_Key_To_Node(int blk_ofst, INT32 ptr, T key);
    template<class T>
    void Update_First_Key(T key);
    template<class T>
    Tuple_Addr Find_Key(T key);
    template<class T>
    void Find_Less_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec);
    template<class T>
    void Find_Larger_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec);
    template<class T>
    void Delete_Key(T key);
    template<class T>
    void Delete_Key_From_Node(int blk_ofst, T key);
    template<class T>
    void Recover_From_Half_Full(T key);
    INT32 Find_Real_Brother(INT32 blk_ofst,bool &flag);
    template<class T>
    void Merge_Siblings(node *sibling);
    template<class T>
    void Merge_Internal_Siblings(node *sibling);
    template<class T>
    void getMinmumdDesc(T& key);
    template<class T>
    void Recover_Internal_From_Half_Full(T key);
    template<class T>
    void Update_First_Key_For_Delete(T key);
	void Update_Extra()
	{
		INT32 extraInfo = Left_First_Node + (FirstEmptyBlockAddr << 16);
		//cout << extraInfo << ' ' << Left_First_Node << ' ' << FirstEmptyBlockAddr << endl;
		memcpy(block->buffer + 8, &extraInfo, 4);
	}
    ~node(){
        //UnPin(block);
    }
};

void getElement(int *key, char *addr, int size);
void setElement(char *addr, int *key, int size);
void getElement(float *key, char *addr, int size);
void setElement(char *addr, float *key, int size);
void getElement(string *key, char *addr, int size);
void setElement(char *addr, string *key, int size);

inline void getElement(int *key, char *addr, int size)
{
	memcpy(key, addr, size);
}

inline void setElement(char *addr, int *key, int size)
{
	memcpy(addr, key, size);
}

inline void getElement(float *key, char *addr, int size)
{
	memcpy(key, addr, size);
}

inline void setElement(char *addr, float *key, int size)
{
	memcpy(addr, key, size);
}

inline void getElement(string *key, char *addr, int size)
{
	char temp_str[MAX_STR_LENGTH];
	memset(temp_str, 0, MAX_STR_LENGTH);
	memcpy(temp_str, addr, size);
	*key = temp_str;
}

inline void setElement(char *addr, string *key, int size)
{
	memcpy(addr, key->c_str(), size);
}

template<class T>
void node::getMinmumdDesc(T& key)
{
    if(IsLeaf()){
        getElement(&key, block->buffer+INDEX_BLOCK_INFO+4, index.attr_size);
    }
    else{
        INT32 First_Ptr;
        memcpy(&First_Ptr, block->buffer+INDEX_BLOCK_INFO, 4);
        node MinSon(index,First_Ptr,this);
        MinSon.getMinmumdDesc(key);
    }
}

template<class T>
void node::Insert_Key(T key, INT32 position)
{
    char *values = block->buffer;
    int blk_ofst = Find_Position(key);
    if (blk_ofst!=keynum*(index.attr_size+4)+INDEX_BLOCK_INFO+4){
        T Element;
        getElement(&Element, block->buffer+blk_ofst, index.attr_size);
        if (Element == key){
            Conflict_Error ce("");
            throw ce;
        }
    }
    if (IsLeaf() && !IsFull())//Is a leaf node and not full
    {
        Insert_Key_To_Node(blk_ofst, position, key);
    }
    else if (IsLeaf())//Is a leaf node and is full
    {
        Split_Node(key, blk_ofst, position);
    }
    else//It's not a leaf node ,so we should find the position and
    {
        INT32 son;
        memcpy(&son, values + blk_ofst - 4, 4);
        node son_node(index, son, this);
        son_node.Insert_Key(key, position);
    }
}

template<class T>
int node::Find_Position(T key)
{
	T Element;
	char *values = block->buffer + INDEX_BLOCK_INFO + 4;
	//cout << keynum <<' '<<index.indexname<< endl;
	for (INT32 i = 0; i < keynum; i++){
		getElement(&Element, values, index.attr_size);
		//cout << Element << "J" << endl;
		if (key <= Element)return int(values - block->buffer);
		values += index.attr_size + 4;
	}
	return int(values - block->buffer);
}

template<class T>
void node::Split_Node(T key, int blk_ofst, INT32 position)
{
    if (block->buffer == NULL)
    {
		block = Block::getBlock(index.indexname, BlockAddr, 0);
    }
    int left_keynum, right_keynum;
    left_keynum=maxsize/2;
    if(maxsize%2)left_keynum++;
    right_keynum = maxsize + 1 - left_keynum;
    int left_size, right_size;
    left_size = left_keynum*(4 + index.attr_size);
    right_size = right_keynum*(4 + index.attr_size);
    int size=index.attr_size;
    if (IsRoot())
    {
		Block *left, *right;
		left = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
		right = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
		
		//cout<< left->offset << ' ' << right->offset << endl;
        memcpy(left->buffer, &const_number[1], 4);
        memcpy(right->buffer, &const_number[1], 4);//type
        memcpy(left->buffer + 8, &RIGHT_FLAG, 4);
        memcpy(right->buffer + 8, &LEFT_FLAG, 4);
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)
        {
            memcpy(left->buffer + 4, &left_keynum, 4);
            memcpy(right->buffer + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(left->buffer+INDEX_BLOCK_INFO, block->buffer + INDEX_BLOCK_INFO, left_size);
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->offset,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else
        {
            memcpy(left->buffer + 4, &(--left_keynum), 4);
            memcpy(right->buffer + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size-size-4,right_size+4);
            memcpy(left->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO,left_size-size);
            node lt(index,left->offset,parent);
            lt.Insert_Key_To_Node(lt.Find_Position(key),position,key);
        }
        int last_ptr=left_keynum*(4+size);
        memcpy(left->buffer+INDEX_BLOCK_INFO+last_ptr,&(right->offset) , 4);
        T right_first_key;
        getElement(&right_first_key,right->buffer+INDEX_BLOCK_INFO+4,size);
		memset(block->buffer + 4,0,BLOCKSIZE);
		memcpy(block->buffer, &const_number[3], 4);
        memcpy(block->buffer + 4, &const_number[1], 4);
		Left_First_Node = left->offset;
        memcpy(block->buffer + INDEX_BLOCK_INFO, &(left->offset), 4);
        setElement(block->buffer + INDEX_BLOCK_INFO + 4, &right_first_key, index.attr_size);
        memcpy(block->buffer + INDEX_BLOCK_INFO + 4 + index.attr_size, &(right->offset), 4);
    }
    else
    {
        Block *right;
		right = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
        memcpy(block->buffer + 8, &RIGHT_FLAG, 4);
        memcpy(right->buffer, &const_number[1], 4);
        memcpy(right->buffer + 8, &LEFT_FLAG, 4);
      
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)
        {
            memcpy(block->buffer + 4, &left_keynum, 4);
            memcpy(right->buffer + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->offset,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else
        {
            memcpy(block->buffer + 4, &(--left_keynum), 4);
            memcpy(right->buffer + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size-4-size,right_size+4);
            node lt(index,BlockAddr,parent);
            lt.Insert_Key_To_Node(Find_Position(key),position,key);
        }
        int last_ptr = INDEX_BLOCK_INFO + left_size;
        memcpy(block->buffer + last_ptr, &(right->offset), 4);
        
        T right_first_key;
        getElement(&right_first_key, right->buffer + INDEX_BLOCK_INFO + 4, index.attr_size);
        INT32 insert_position=parent->Find_Position(right_first_key);
        if (parent->IsFull())
        {
            parent->Split_Internal_Node(right_first_key, insert_position, right->offset);
        }
        else{
            parent->Insert_Key_To_Node(insert_position, right->offset, right_first_key);
        }
        
        if (blk_ofst == INDEX_BLOCK_INFO + 4&&!IsRoot()){
            T Element;
            getElement(&Element,block->buffer+blk_ofst,index.attr_size);
            parent->Update_First_Key(Element);
           
        }
    }
}

template<class T>
void node::Split_Internal_Node(T key, int blk_ofst, INT32 position)
{
    if (block->buffer == NULL)
    {
        block = Block::getBlock(index.indexname, BlockAddr,0);
    }
    int left_keynum, right_keynum;
    left_keynum=(maxsize+1)/2;
    if((maxsize+1)%2==0)left_keynum--;
    // up_bound(n/2) -1
    right_keynum = maxsize - left_keynum;
    int left_size, right_size;
    left_size = left_keynum*(4 + index.attr_size);
    right_size = right_keynum*(4 + index.attr_size);
    int size=index.attr_size;
    if (IsRoot())
    {
		Block *left , *right;
		left = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
		right = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
        memcpy(left->buffer, &const_number[2], 4);
        memcpy(right->buffer, &const_number[2], 4);//type
        memcpy(left->buffer + 8, &RIGHT_FLAG, 4);
        memcpy(right->buffer + 8, &LEFT_FLAG, 4);

        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)
        {
            memcpy(left->buffer + 4, &left_keynum, 4);
            memcpy(right->buffer + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(left->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO,left_size+4);
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size+4+size,right_size-size);
            node rt(index,right->offset,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else{
            memcpy(left->buffer + 4, &(--left_keynum), 4);
            memcpy(right->buffer + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size,right_size+4);
            memcpy(left->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO,left_size-size);
            node lt(index,left->offset,parent);
            lt.Insert_Key_To_Node(lt.Find_Position(key),position,key);
        }
        T right_first_key;
        node rt(index,right->offset,parent);
        rt.getMinmumdDesc(right_first_key);
		memset(block->buffer, 0, BLOCKSIZE);
        memcpy(block->buffer + 4, &const_number[1], 4);
        memcpy(block->buffer + INDEX_BLOCK_INFO, &(left->offset), 4);
        setElement(block->buffer + INDEX_BLOCK_INFO + 4, &right_first_key, index.attr_size);
        memcpy(block->buffer + INDEX_BLOCK_INFO + 4 + index.attr_size, &(right->offset), 4);
    }
    else
    {
		Block *right;
		right = Block::getBlock(index.indexname, FirstEmptyBlockAddr, 1);
		FirstEmptyBlockAddr++;
        //int last_ptr_position = INDEX_BLOCK_INFO + left_size;
		
        memcpy(block->buffer + 8, &RIGHT_FLAG, 4);
        memcpy(right->buffer, &const_number[2], 4);
        memcpy(right->buffer + 8, &LEFT_FLAG, 4);
		if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)
        {
            memcpy(block->buffer + 4, &left_keynum, 4);
            memcpy(right->buffer + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->offset,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else{
            memcpy(block->buffer + 4, &(--left_keynum), 4);
            memcpy(right->buffer + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->buffer+INDEX_BLOCK_INFO,block->buffer+left_size+INDEX_BLOCK_INFO,right_size+4);
            node lt(index,BlockAddr,parent);
            lt.Insert_Key_To_Node(Find_Position(key),position,key);
        }
        T right_first_key;
        node rt(index,right->offset,parent);
        rt.getMinmumdDesc(right_first_key);
        INT32 insert_position=parent->Find_Position(right_first_key);
        if (parent->IsFull())
        {
            parent->Split_Internal_Node(right_first_key, insert_position, right->offset);
        }
        else{
            parent->Insert_Key_To_Node(insert_position, right->offset, right_first_key);
        }
        
        if (blk_ofst == INDEX_BLOCK_INFO + 4&&!IsRoot()){
            T Element;
            getElement(&Element,block->buffer+blk_ofst,index.attr_size);
            parent->Update_First_Key(Element);
        }
    }
}

template<class T>
void node::Insert_Key_To_Node(int blk_ofst, INT32 ptr, T key)
{
    int copy_length = keynum*(4 + index.attr_size) + INDEX_BLOCK_INFO - blk_ofst + 8;
    char *values = block->buffer;
    if(IsLeaf())memcpy(values + blk_ofst + index.attr_size, values + blk_ofst - 4, copy_length);
    else memcpy(values+blk_ofst+index.attr_size+4,values+blk_ofst,copy_length-4);

    setElement(values + blk_ofst, &key, index.attr_size);
    if(IsLeaf()){
        memcpy(values + blk_ofst - 4, &ptr, 4);
    }
    else{
        memcpy(values + blk_ofst + index.attr_size, &ptr, 4);

    }
    keynum++;
    memcpy(values + 4, &keynum, 4);
    if (blk_ofst == INDEX_BLOCK_INFO + 4&&parent!=NULL){
        parent->Update_First_Key(key);
    }
}

template<class T>
void node::Update_First_Key(T key)
{
    //if(this==NULL)return;
    if (block->buffer == NULL){
		block = Block::getBlock(index.indexname, BlockAddr, 0);
    }
    int ofst = Find_Position(key);
    int tail=keynum*(index.attr_size+4)+4+INDEX_BLOCK_INFO;
    if(ofst==tail)return;
    setElement(block->buffer + ofst, &key, index.attr_size);
}

template<class T>
Tuple_Addr node::Find_Key(T key)
{
    int ofst = Find_Position(key);
    T Element;
    if(ofst!=INDEX_BLOCK_INFO+4+keynum*(4+index.attr_size))
		getElement(&Element, block->buffer + ofst, index.attr_size);
	else Element = INFI;
	//cout << "El" << Element << endl;
	int tail = keynum*(4 + index.attr_size) - 4;
    if (IsLeaf())
    {
        INT32 tbl_addr;
        memcpy(&tbl_addr, block->buffer + ofst - 4, 4);
        if (Element != key||ofst==tail){
            Tuple_Addr TA(BlockAddr, ofst - 4, tbl_addr, false);
            return TA;
        }
        else
        {
            Tuple_Addr TA(BlockAddr, ofst - 4, tbl_addr, true);
            return TA;
        }
    }
    else{
        INT32 son_addr;
        if(Element>key)
        {
            memcpy(&son_addr, block->buffer + ofst - 4, 4);
        }
        else
        {
            memcpy(&son_addr, block->buffer + ofst +index.attr_size, 4);
        }
        node son(index, son_addr, this);
        return son.Find_Key(key);
    }
}

/*template<class T>
void node::Find_Less_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec)
{
    Tuple_Addr TA = Find_Key(key);
    INT32 ptr = Left_First_Node;
    T Element;
    INT32 position;
	int flag = false;
    while (1)
    {
        node temp(index, ptr, NULL);

        int n = temp.getKeynum();
        int ofst = INDEX_BLOCK_INFO;
        char *values = temp.getBlock_Ptr()->buffer;
        for (int i = 0; i<n; i++){
            memcpy(&position, values + ofst, 4);
			getElement(&Element, values + INDEX_BLOCK_INFO + ofst + 4, index.attr_size);
			cout << "Element" << Element << endl;
			if (Element < key){
				Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
				vec.push_back(tuple);
				ofst += 4 + index.attr_size;
			}
			else{
				flag = true;
				break;
			}
        }
		if (ptr==TA.getIBA()||flag)break;
        memcpy(&ptr, values + ofst, 4);
    }
    if (TA.Get_Key_Exist() && equal)
    {
        Tuple_Addr tuple(TA.getIBA(), TA.getIO(), TA.getTable_Addr(), true);
        vec.push_back(tuple);
    }
	//sort(vec.begin(), vec.end());
}*/

template<class T>
void node::Find_Less_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec)
{
	Tuple_Addr TA = Find_Key(key);
	INT32 ptr = Left_First_Node;
	T Element;
	INT32 position;
	int flag = false;
	while (1)
	{
		node temp(index, ptr, NULL);

		int n = temp.getKeynum();
		int ofst = INDEX_BLOCK_INFO;
		char *values = temp.getBlock_Ptr()->buffer;
		for (int i = 0; i<n; i++){
			memcpy(&position, values + ofst, 4);
			getElement(&Element, values + ofst + 4, index.attr_size);
			if (Element < key){
				Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
				vec.push_back(tuple);
				ofst += 4 + index.attr_size;
			}
			else{
				flag = true;
				break;
			}
		}
		if (temp.getBlockAddr() == TA.getIBA() || flag)break;
		memcpy(&ptr, values + ofst, 4);
	}
	if (TA.Get_Key_Exist() && equal)
	{
		Tuple_Addr tuple(TA.getIBA(), TA.getIO(), TA.getTable_Addr(), true);
		vec.push_back(tuple);
	}
	//sort(vec.begin(), vec.end());
}


template<class T>
void node::Find_Larger_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec)
{
    Tuple_Addr TA = Find_Key(key);
    if (TA.Get_Key_Exist() && equal)
    {
        Tuple_Addr tuple(TA.getIBA(), TA.getIO(), TA.getTable_Addr(), true);
        vec.push_back(tuple);
    }

    INT32 ptr = TA.getIBA();

    T Element;
    INT32 position;
	int flag=false;
	if (TA.getIBA()==0)flag=true;
    while (1)
    {
        node temp(index, ptr, NULL);
		if (temp.IsRoot() && flag == false)break;
        int n = temp.getKeynum();
        int ofst = INDEX_BLOCK_INFO;
        char *values = temp.getBlock_Ptr()->buffer;

		if (ptr == Left_First_Node){
			T MinElement;
			int position;
			memcpy(&position, values + INDEX_BLOCK_INFO, 4);
			getElement(&MinElement, values + INDEX_BLOCK_INFO + 4, index.attr_size);
			if (MinElement > key){
				Tuple_Addr tuple(Left_First_Node, INDEX_BLOCK_INFO + 4, position, true);
				vec.push_back(tuple);
			}
		}

        for (int i = 0; i<n; i++){
            memcpy(&position, values + ofst, 4);
			if (ptr == TA.getIBA() && ofst <= TA.getIO()){
				ofst += 4 + index.attr_size;
				continue;
			}
            Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
            vec.push_back(tuple);
            ofst += 4 + index.attr_size;
        }
        memcpy(&ptr, values + ofst, 4);
		if (temp.IsRoot() && flag == true)flag = false;
    }
}

template<class T>
void node::Delete_Key(T key)
{
    int blk_ofst = Find_Position(key);
    T Element;
    getElement(&Element,block->buffer+blk_ofst,index.attr_size);
    if(IsLeaf())
    {
        if(Element!=key){
            NoKeyDeleted_Error nkde;
            throw nkde;
        }
    }
    //cout<<"--------uuuuuu----"<<BlockAddr<<endl;
    if (IsLeaf() && (!IsHalfFull()||IsRoot()))//Is a leaf node and not half full, or is a leaf&&root node
    {
        Delete_Key_From_Node(blk_ofst, key);
    }
    else if (IsLeaf())//Is a leaf node and is half full
    {
        Delete_Key_From_Node(blk_ofst, key);
        getMinmumdDesc(key);
        Recover_From_Half_Full(key);
    }
    else//It's not a leaf node ,so we should find the position and 
    {
        INT32 son;
        int last_pos=INDEX_BLOCK_INFO+keynum*(index.attr_size+4)+4;
        if(key<Element||blk_ofst==last_pos)
        {
            memcpy(&son, block->buffer + blk_ofst - 4, 4);
        }
        else
        {
            memcpy(&son, block->buffer + blk_ofst +index.attr_size, 4);
        }
        node son_node(index, son, this);
        son_node.Delete_Key(key);
    }
}

template<class T>
void node::Delete_Key_From_Node(int blk_ofst,T key)
{
    INT32 src = blk_ofst + index.attr_size;
    INT32 dest = blk_ofst - 4;
    INT32 length = INDEX_BLOCK_INFO + keynum*(index.attr_size + 4) + 4 - src;//12+4+7+4+7+4 -12+4+7
    if(!IsLeaf())memcpy(block->buffer+dest+4,block->buffer+src+4,length-4);
    else memcpy(block->buffer+dest,block->buffer+src,length);
    keynum--;
    memcpy(block->buffer+4,&keynum,4);
    if(blk_ofst==INDEX_BLOCK_INFO+4&&!IsRoot()&&IsLeaf())
    {
        T Element;
        getElement(&Element, block->buffer+blk_ofst, index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
    }
}

template<class T>
void node::Recover_From_Half_Full(T key)
{
    INT32 blk_ofst = parent->Find_Position(key);
    bool flag=true;
    T temp;
    getElement(&temp,parent->getBlock_Ptr()->buffer+blk_ofst,index.attr_size);
    int last_pos=INDEX_BLOCK_INFO+parent->getKeynum()*(index.attr_size+4)+4;
    if(temp==key)blk_ofst+=4+index.attr_size;
    if(blk_ofst==last_pos)flag=false;
    INT32 sib_position = parent->Find_Real_Brother(blk_ofst,flag);
    node sibling(index,sib_position,parent);

    if(sibling.IsHalfFull()){
        if(flag)Merge_Siblings<T>(&sibling);
        else sibling.Merge_Siblings<T>(this);
    }
    else if(flag)
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib,keynum_this;
        memcpy(&keynum_sib,sib->buffer+4,4);
        memcpy(&ptr,sib->buffer+INDEX_BLOCK_INFO,4);
        getElement(&Element,sib->buffer+INDEX_BLOCK_INFO+4,index.attr_size);
        memcpy(&keynum_this,block->buffer+4,4);
        int last_ptr;
        int ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(&last_ptr,block->buffer+ofst,4);
        memcpy(block->buffer+ofst,&ptr,4);
        setElement(block->buffer+ofst+4,&Element,index.attr_size);
        keynum_sib--;
        keynum_this++;
        memcpy(sib->buffer+4,&keynum_sib,4);
        memcpy(block->buffer+4,&keynum_this,4);
        ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(block->buffer+ofst,&last_ptr,4);
        int sib_copy_length=keynum_sib*(4+index.attr_size)+4;
        memcpy(sib->buffer+INDEX_BLOCK_INFO,sib->buffer+INDEX_BLOCK_INFO+4+index.attr_size,sib_copy_length);
        
        getElement(&Element,sib->buffer+INDEX_BLOCK_INFO+4,index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
    }
    else
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib;
        memcpy(&keynum_sib,sib->buffer+4,4);
        int ofst=(keynum_sib-1)*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(&ptr,sib->buffer+ofst,4);
        getElement(&Element,sib->buffer+ofst+4,index.attr_size);
        memcpy(sib->buffer+ofst,sib->buffer+ofst+index.attr_size+4,4);
        keynum_sib--;
        memcpy(sib->buffer+4,&keynum_sib,4);
        int keynum_this;
        memcpy(&keynum_this,block->buffer+4,4); 
        int this_copy_length=keynum_this*(4+index.attr_size)+4;
        memcpy(block->buffer+INDEX_BLOCK_INFO+4+index.attr_size,block->buffer+INDEX_BLOCK_INFO,this_copy_length);
        memcpy(block->buffer+INDEX_BLOCK_INFO,&ptr,4);
        setElement(block->buffer+INDEX_BLOCK_INFO+4,&Element,index.attr_size);
        keynum_this++;
        memcpy(block->buffer+4,&keynum_this,4); 

        getElement(&Element,block->buffer+INDEX_BLOCK_INFO+4,index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
    }
}

template<class T>
void node::Merge_Siblings(node *sibling)
{
	Block *sib = sibling->getBlock_Ptr();
	T Element;
	getElement(&Element, sib->buffer + INDEX_BLOCK_INFO + 4, index.attr_size);
	INT32 copy_length = sibling->getKeynum()*(4 + index.attr_size) + 4;
	INT32 dest = INDEX_BLOCK_INFO + keynum*(4 + index.attr_size);
	INT32 src = INDEX_BLOCK_INFO;
	memcpy(block->buffer + dest, sib->buffer + src, copy_length);
	keynum = keynum + sibling->getKeynum();
	memcpy(block->buffer + 4, &keynum, 4);
	INT32 ofst_in_parent = parent->Find_Position(Element);
	if ((!parent->IsRoot()) && parent->IsHalfFull())
	{
		parent->Delete_Key_From_Node(ofst_in_parent, Element);
		parent->Recover_Internal_From_Half_Full(Element);
	}
	else if (parent->IsRoot() && parent->getKeynum() <= 1)
	{
		int length = (keynum + sibling->getKeynum())*(4 + index.attr_size) + 4 + INDEX_BLOCK_INFO;
		memcpy(parent->getBlock_Ptr()->buffer, block->buffer, length);
		int root_type = IsLeaf() ? 0 : 3;
		memcpy(parent->getBlock_Ptr()->buffer, &root_type, 4);
		Left_First_Node = 0;
		//让合并后的结点成为新的root即可
		/*------------------------------------------------------buffer do-------------------------------------------------*/
		memset(block->buffer, 0, BLOCKSIZE);
		//此时this的数据全都是无效的，可以被释放了
		/*------------------------------------------------------buffer do-------------------------------------------------*/
	}
	else
	{
		parent->Delete_Key_From_Node(ofst_in_parent, Element);
	}

	/*------------------------------------------------------buffer do-------------------------------------------------*/
	memset(sib->buffer, 0, BLOCKSIZE);
	//此时sib的数据全都是无效的，可以被释放了
	/*------------------------------------------------------buffer do-------------------------------------------------*/
}

template<class T>
void node::Recover_Internal_From_Half_Full(T key)
{
    INT32 blk_ofst = parent->Find_Position(key);
    bool flag=true;
    T temp;
    getElement(&temp,parent->getBlock_Ptr()->buffer+blk_ofst,index.attr_size);
    int last_pos=INDEX_BLOCK_INFO+parent->getKeynum()*(index.attr_size+4)+4;
    if(temp==key)blk_ofst+=4+index.attr_size;
    if(blk_ofst==last_pos)flag=false;
    INT32 sib_position = parent->Find_Real_Brother(blk_ofst,flag);
    node sibling(index,sib_position,parent);
    int size=index.attr_size;

    if(sibling.IsHalfFull()){
        if(flag)Merge_Internal_Siblings<T>(&sibling);
        else sibling.Merge_Internal_Siblings<T>(this);
    }
    else if(flag)
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib,keynum_this;
        memcpy(&keynum_sib,sib->buffer+4,4);
        memcpy(&ptr,sib->buffer+INDEX_BLOCK_INFO,4);
        sibling.getMinmumdDesc(Element);
        keynum_this=keynum;
        int ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO+4;
        memcpy(block->buffer+ofst+size,&ptr,4);
        setElement(block->buffer+ofst,&Element,size);
        keynum_sib--;
        keynum_this++;
        memcpy(sib->buffer+4,&keynum_sib,4);
        memcpy(block->buffer+4,&keynum_this,4);
        int sib_copy_length=keynum_sib*(4+index.attr_size)+4;
        memcpy(sib->buffer+INDEX_BLOCK_INFO,sib->buffer+INDEX_BLOCK_INFO+4+size,sib_copy_length);
        sibling.getMinmumdDesc(Element);
        parent->Update_First_Key_For_Delete(Element);
    }
    else
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib;
        memcpy(&keynum_sib,sib->buffer+4,4);
        int ofst=(keynum_sib-1)*(4+index.attr_size)+INDEX_BLOCK_INFO+4;
        memcpy(&ptr,sib->buffer+ofst+size,4);
        getMinmumdDesc(Element);
        keynum_sib--;
        memcpy(sib->buffer+4,&keynum_sib,4);
        int keynum_this=keynum;
        int this_copy_length=keynum_this*(4+index.attr_size);
        memcpy(block->buffer+INDEX_BLOCK_INFO+4+index.attr_size,block->buffer+INDEX_BLOCK_INFO,this_copy_length+4);
        memcpy(block->buffer+INDEX_BLOCK_INFO,&ptr,4);
        setElement(block->buffer+INDEX_BLOCK_INFO+4,&Element,size);
        keynum_this++;
        memcpy(block->buffer+4,&keynum_this,4);
        
        getMinmumdDesc(Element);
        parent->Update_First_Key_For_Delete(Element);
    }    
}

template<class T>
void node::Merge_Internal_Siblings(node *sibling)
{
	Block *sib = sibling->getBlock_Ptr();
	int keynum_sib = sibling->getKeynum();
	T Element;
	sibling->getMinmumdDesc(Element);
	int last_pos = keynum*(4 + index.attr_size) + 4 + INDEX_BLOCK_INFO;
	setElement(block->buffer + last_pos, &Element, index.attr_size);
	INT32 dest = last_pos + index.attr_size;
	INT32 src = INDEX_BLOCK_INFO;
	INT32 copy_length = keynum_sib*(4 + index.attr_size) + 4;
	memcpy(block->buffer + dest, sib->buffer + src, copy_length);
	keynum = keynum + keynum_sib + 1;
	memcpy(block->buffer + 4, &keynum, 4);

	INT32 ofst_in_parent = parent->Find_Position(Element);
	T temp;
	getElement(&temp, parent->getBlock_Ptr()->buffer + ofst_in_parent, index.attr_size);
	int lp = INDEX_BLOCK_INFO + parent->getKeynum()*(index.attr_size + 4) + 4;
	if (temp == Element || ofst_in_parent == lp)ofst_in_parent -= 4 + index.attr_size;
	if ((!parent->IsRoot()) && parent->IsHalfFull())
	{
		parent->Delete_Key_From_Node(ofst_in_parent, Element);
		T First_Key;
		getElement(&First_Key, parent->getBlock_Ptr()->buffer + INDEX_BLOCK_INFO + 4, index.attr_size);
		if (Element<First_Key)Element = First_Key;
		parent->Recover_Internal_From_Half_Full(Element);
	}
	else if (parent->IsRoot() && parent->getKeynum() <= 1)
	{
		int length = (keynum + keynum_sib)*(4 + index.attr_size) + 4 + INDEX_BLOCK_INFO;
		memcpy(parent->getBlock_Ptr()->buffer, block->buffer, length);
		int root_type = 3;
		memcpy(parent->getBlock_Ptr()->buffer, &root_type, 4);
		//让合并后的结点成为新的root即可
		/*------------------------------------------------------buffer do-------------------------------------------------*/
		memset(block->buffer, 0, BLOCKSIZE);
		//此时this的数据全都是无效的，可以被释放了
		/*------------------------------------------------------buffer do-------------------------------------------------*/
	}
	else
	{
		parent->Delete_Key_From_Node(ofst_in_parent, Element);
	}

	/*------------------------------------------------------buffer do-------------------------------------------------*/
	memset(sib->buffer, 0, BLOCKSIZE);
	//此时sib的数据全都是无效的，可以被释放了
	/*------------------------------------------------------buffer do-------------------------------------------------*/
}

template<class T>
void node::Update_First_Key_For_Delete(T key)
{
    int ofst = Find_Position(key);
    int head=4+INDEX_BLOCK_INFO;
    if(ofst==head)return;
    setElement(block->buffer + ofst-4-index.attr_size, &key, index.attr_size);
}

#endif /* BPlus_Tree_hpp */









