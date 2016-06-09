//
//  BPlus_Tree.hpp
//  minisql
//
//  Created by 王皓波 on 16/5/22.
//  Copyright © 2016年 王皓波. All rights reserved.
//

#ifndef BPlus_Tree_hpp
#define BPlus_Tree_hpp

#include "Block.h"
#include "minisql.h"
#include "vector"

class Conflict_Error{};
class NoKeyDeleted_Error{};

//#define INT32 unsigned int
#define MAX_STR_LENGTH 256

const int const_number[] = { 0, 1, 2, 3, 4 };
const int LEFT_FLAG = 1, RIGHT_FLAG = 2;
extern INT32 Left_First_Node;


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
    //type:0-叶结点&根结点 1-叶结点 2-内部结点 3-仅仅是根结点
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
    void Split_Node(T key, int blk_ofst, INT32 position);//分裂叶结点
    template<class T>
    void Split_Internal_Node(T key, int blk_ofst, INT32 position);//分离内部结点
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
    ~node(){
        //UnPin(block);
    }
};

void getElement(int *key, char *addr, int size);
void setElement(char *addr, int *key, int size);
void getElement(double *key, char *addr, int size);
void setElement(char *addr, double *key, int size);
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
        getElement(&key, block->record+INDEX_BLOCK_INFO+4, index.attr_size);
    }
    else{
        INT32 First_Ptr;
        memcpy(&First_Ptr, block->record+INDEX_BLOCK_INFO, 4);
        node MinSon(index,First_Ptr,this);
        MinSon.getMinmumdDesc(key);
    }
}

template<class T>
void node::Insert_Key(T key, INT32 position)
{
    char *values = block->record;
    int blk_ofst = Find_Position(key);//找到块中key应在位置的位移
    if (blk_ofst!=keynum*(index.attr_size+4)+INDEX_BLOCK_INFO+4){
        T Element;
        getElement(&Element, block->record+blk_ofst, index.attr_size);
        if (Element == key){
            Conflict_Error ce;
            throw ce;
        }
        //存在重复的关键字，不符合单值单属性的定义
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
        node son_node(index, son, this);//得到对应的子块
        son_node.Insert_Key(key, position);//向子块递归插入，直到遇到叶结点
    }
}

template<class T>
int node::Find_Position(T key)//二分法更快
{
    T Element;
    char *values = block->record + INDEX_BLOCK_INFO + 4;
    //让此指针指向第一个数据点
    for (int i = 0; i<keynum; i++){
        getElement(&Element, values, index.attr_size);
        if (key<=Element)return int(values - block->record);//返回已经造成的偏移量
        values += index.attr_size + 4;
    }
    return int(values - block->record);//说明比所有key都大
}

template<class T>
void node::Split_Node(T key, int blk_ofst, INT32 position)
{
    if (block->record == NULL)
    {
        block = getBlock(index.indexname.c_str(), BlockAddr);
    }
    int left_keynum, right_keynum;
    left_keynum=maxsize/2;
    if(maxsize%2)left_keynum++;
    right_keynum = maxsize + 1 - left_keynum;
    int left_size, right_size;
    left_size = left_keynum*(4 + index.attr_size);
    right_size = right_keynum*(4 + index.attr_size);
    int size=index.attr_size;
    if (IsRoot())//根结点被分裂
    {
        Block *left, *right;
        left = getBlock(index.indexname.c_str());
        right = getBlock(index.indexname.c_str());
        //在文件中获得2个新的空白的block，这里不是生成新的root
        //因为root应当一直被放在文件的offset为0即开头的位置
        memcpy(left->record, &const_number[1], 4);
        memcpy(right->record, &const_number[1], 4);//type
        memcpy(left->record + 8, &RIGHT_FLAG, 4);
        memcpy(right->record + 8, &LEFT_FLAG, 4);//extra,暂时没什么用，除了root
        //更新新的块的块头信息
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)//如果key应在的位置不在左侧
        {
            memcpy(left->record + 4, &left_keynum, 4);
            memcpy(right->record + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(left->record+INDEX_BLOCK_INFO, block->record + INDEX_BLOCK_INFO, left_size);
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->BlockAddr,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else
        {
            memcpy(left->record + 4, &(--left_keynum), 4);
            memcpy(right->record + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size-size-4,right_size+4);
            //要往后挪一个指针
            memcpy(left->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO,left_size-size);
            node lt(index,left->BlockAddr,parent);
            lt.Insert_Key_To_Node(lt.Find_Position(key),position,key);
        }
        int last_ptr=left_keynum*(4+size);
        memcpy(left->record+INDEX_BLOCK_INFO+last_ptr,&(right->BlockAddr) , 4);
        //至此叶结点处理完成，需要向上插入一个标记key
        T right_first_key;
        getElement(&right_first_key,right->record+INDEX_BLOCK_INFO+4,size);
        //分裂完成后，要取出右侧结点的第一个key作为root中唯一的key
        memcpy(block->record + 4, &const_number[1], 4);//keynum变成1
        memcpy(block->record, &const_number[3], 4);//更新type,变成了内部根结点
        memcpy(block->record + INDEX_BLOCK_INFO, &(left->BlockAddr), 4);
        setElement(block->record + INDEX_BLOCK_INFO + 4, &right_first_key, index.attr_size);
        memcpy(block->record + INDEX_BLOCK_INFO + 4 + index.attr_size, &(right->BlockAddr), 4);
        //依次向根结点存入左侧结点指针、左侧结点的第一个数据、右侧结点指针
    }
    else//如果不是根叶结点被分裂，说明是叶结点，需要向上插入这个key值
    {
        Block *right;
        right = getBlock(index.indexname.c_str());
        memcpy(block->record + 8, &RIGHT_FLAG, 4);
        memcpy(right->record, &const_number[1], 4);
        memcpy(right->record + 8, &LEFT_FLAG, 4);
        //更新两个块的块头信息,extra先随意了……
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)
        {
            memcpy(block->record + 4, &left_keynum, 4);
            memcpy(right->record + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->BlockAddr,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else
        {
            memcpy(block->record + 4, &(--left_keynum), 4);
            memcpy(right->record + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size-4-size,right_size+4);
            node lt(index,BlockAddr,parent);
            lt.Insert_Key_To_Node(Find_Position(key),position,key);
        }
        int last_ptr = INDEX_BLOCK_INFO + left_size;
        memcpy(block->record + last_ptr, &(right->BlockAddr), 4);
        //左结点的最后一个指针要更新为右结点的块号
        T right_first_key;
        getElement(&right_first_key, right->record + INDEX_BLOCK_INFO + 4, index.attr_size);
        INT32 insert_position=parent->Find_Position(right_first_key);
        if (parent->IsFull())
        {
            parent->Split_Internal_Node(right_first_key, insert_position, right->BlockAddr);
        }
        else{//如果父结点没满，直接插入就好
            parent->Insert_Key_To_Node(insert_position, right->BlockAddr, right_first_key);
        }
        
        if (blk_ofst == INDEX_BLOCK_INFO + 4&&!IsRoot()){
            T Element;
            getElement(&Element,block->record+blk_ofst,index.attr_size);
            parent->Update_First_Key(Element);
            //如果插入的结点是左侧结点的第一个，则向上更新父结点的第一个key
            //这一步在插入右侧第一个数据之后进行，不影响，因为右侧数据肯定比左侧数据大
        }
    }
}

template<class T>
void node::Split_Internal_Node(T key, int blk_ofst, INT32 position)
{
    if (block->record == NULL)
    {
        block = getBlock(index.indexname.c_str(), BlockAddr);
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
    if (IsRoot())//根结点被分裂
    {
        Block *left, *right;
        left = getBlock(index.indexname.c_str());
        right = getBlock(index.indexname.c_str());
        //在文件中获得2个新的空白的block，这里不是生成新的root
        //因为root应当一直被放在文件的offset为0即开头的位置
        memcpy(left->record, &const_number[2], 4);
        memcpy(right->record, &const_number[2], 4);//type
        memcpy(left->record + 8, &RIGHT_FLAG, 4);
        memcpy(right->record + 8, &LEFT_FLAG, 4);//extra,暂时没什么用，除了root
        //更新新的块的块头信息，先不更新keynum
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)//如果key应在的位置不在左侧
        {
            memcpy(left->record + 4, &left_keynum, 4);
            memcpy(right->record + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(left->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO,left_size+4);
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size+4+size,right_size-size);
            node rt(index,right->BlockAddr,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else{//key在左边
            memcpy(left->record + 4, &(--left_keynum), 4);
            memcpy(right->record + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size,right_size+4);
            //有一个key会被直接抛弃
            memcpy(left->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO,left_size-size);
            node lt(index,left->BlockAddr,parent);
            lt.Insert_Key_To_Node(lt.Find_Position(key),position,key);
        }
        //至此子结点处理完成，需要向上插入一个标记key
        T right_first_key;
        node rt(index,right->BlockAddr,parent);
        rt.getMinmumdDesc(right_first_key);
        //分裂完成后，要取出右侧结点的第一个key作为root中唯一的key
        memcpy(block->record + 4, &const_number[1], 4);//keynum变成1
        memcpy(block->record + INDEX_BLOCK_INFO, &(left->BlockAddr), 4);
        setElement(block->record + INDEX_BLOCK_INFO + 4, &right_first_key, index.attr_size);
        memcpy(block->record + INDEX_BLOCK_INFO + 4 + index.attr_size, &(right->BlockAddr), 4);
        //依次向根结点存入左侧结点指针、左侧结点的第一个数据、右侧结点指针
    }
    else//如果不是根结点被分裂，说明是叶结点，需要向上插入这个key值
    {
        Block *right;
        right = getBlock(index.indexname);
        //int last_ptr_position = INDEX_BLOCK_INFO + left_size;
        memcpy(block->record + 8, &RIGHT_FLAG, 4);
        memcpy(right->record, &const_number[2], 4);
        memcpy(right->record + 8, &LEFT_FLAG, 4);
        //更新两个块的块头信息,extra先随意
        if (left_size <= blk_ofst - 4 - INDEX_BLOCK_INFO)//如果key应在的位置不在左侧
        {
            memcpy(block->record + 4, &left_keynum, 4);
            memcpy(right->record + 4, &(--right_keynum), 4);//keynum
            right_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+INDEX_BLOCK_INFO+left_size,right_size-size);
            node rt(index,right->BlockAddr,parent);
            rt.Insert_Key_To_Node(rt.Find_Position(key),position,key);
        }
        else{//key在左边
            memcpy(block->record + 4, &(--left_keynum), 4);
            memcpy(right->record + 4, &right_keynum, 4);//keynum
            left_keynum++;
            memcpy(right->record+INDEX_BLOCK_INFO,block->record+left_size+INDEX_BLOCK_INFO,right_size+4);
            //有一个key会被直接抛弃
            node lt(index,BlockAddr,parent);
            lt.Insert_Key_To_Node(Find_Position(key),position,key);
        }
        //接下来让右侧结点的第一个数据向上插入
        T right_first_key;
        node rt(index,right->BlockAddr,parent);
        rt.getMinmumdDesc(right_first_key);
        INT32 insert_position=parent->Find_Position(right_first_key);
        if (parent->IsFull())
        {
            parent->Split_Internal_Node(right_first_key, insert_position, right->BlockAddr);
        }
        else{//如果父结点没满，直接插入就好
            parent->Insert_Key_To_Node(insert_position, right->BlockAddr, right_first_key);
        }
        
        if (blk_ofst == INDEX_BLOCK_INFO + 4&&!IsRoot()){
            T Element;
            getElement(&Element,block->record+blk_ofst,index.attr_size);
            parent->Update_First_Key(Element);
            //如果插入的结点是左侧结点的第一个，则向上更新父结点的第一个key
        }
    }
}

template<class T>
void node::Insert_Key_To_Node(int blk_ofst, INT32 ptr, T key)
{
    int copy_length = keynum*(4 + index.attr_size) + INDEX_BLOCK_INFO - blk_ofst + 8;
    char *values = block->record;
    if(IsLeaf())memcpy(values + blk_ofst + index.attr_size, values + blk_ofst - 4, copy_length);
    else memcpy(values+blk_ofst+index.attr_size+4,values+blk_ofst,copy_length-4);
    //插入部分后面的元素向后移动ptr+attr_size的长度
    /*if(!IsLeaf()&&blk_ofst==INDEX_BLOCK_INFO+4)
    {
        INT32 First_Son;
        memcpy(&First_Son,block->record+INDEX_BLOCK_INFO,4);
        Block *temp=getBlock(index.indexname,First_Son);
        getElement(&key,temp->record+INDEX_BLOCK_INFO+4,index.attr_size);
        //如果是插入到内部结点的第一个位置，则要把原来的第一个指针的儿子拿出来往后挪
    }*/
    setElement(values + blk_ofst, &key, index.attr_size);
    //复制key的值到应该在的位置
    if(IsLeaf()){
        memcpy(values + blk_ofst - 4, &ptr, 4);
    }
    else{
        memcpy(values + blk_ofst + index.attr_size, &ptr, 4);
        //可能被向上插入的只有右侧的结点，所以每个内部结点的第一个指针是不会变的
    }
    //复制key对应的表中的块偏移+块内偏移
    keynum++;
    memcpy(values + 4, &keynum, 4);
    //由于keynum已经更改，需要重新写入，而type和parent属性没有
    if (blk_ofst == INDEX_BLOCK_INFO + 4&&parent!=NULL){
        parent->Update_First_Key(key);//如果这个key是叶结点上的第一个关键字，则需要向上更新
    }
}

template<class T>
void node::Update_First_Key(T key)
{
    //if(this==NULL)return;
    if (block->record == NULL){
        block = getBlock(index.indexname, BlockAddr);
    }
    int ofst = Find_Position(key);
    int tail=keynum*(index.attr_size+4)+4+INDEX_BLOCK_INFO;
    if(ofst==tail)return;
    //如果发现要更新的key是在最后或者是在内部结点的最开始，那样的话应该放弃更新
    setElement(block->record + ofst, &key, index.attr_size);
    //只需要更新结点的上一层就可以了
}

template<class T>
Tuple_Addr node::Find_Key(T key)
{
    int ofst = Find_Position(key);
    T Element;
    getElement(&Element, block->record + ofst, index.attr_size);
    //先得到当前结点中key应在的位置的key
    if (IsLeaf())
    {
        //如果是叶结点，先取出当前结点中key应在的位置的值
        INT32 tbl_addr;
        memcpy(&tbl_addr, block->record + ofst - 4, 4);
        if (Element != key){//在叶结点没有找到相应的key，说明查找失败
            Tuple_Addr TA(BlockAddr, ofst - 4, tbl_addr, false);
            return TA;
        }
        else//找到了这个key
        {
            Tuple_Addr TA(BlockAddr, ofst - 4, tbl_addr, true);
            return TA;
        }
    }
    else{//如果不是叶结点，则递归地向下寻找
        INT32 son_addr;
        if(Element>key)
        {
            memcpy(&son_addr, block->record + ofst - 4, 4);
        }
        else
        {
            memcpy(&son_addr, block->record + ofst +index.attr_size, 4);
        }
        //得到子结点地址
        node son(index, son_addr, this);
        return son.Find_Key(key);
    }
}

template<class T>
void node::Find_Less_Than_Key(T key, bool equal, vector<Tuple_Addr> &vec)
{
    Tuple_Addr TA = Find_Key(key);
    INT32 ptr = Left_First_Node;
    T Element;
    INT32 position;
    while (1)
    {
        node temp(index, ptr, NULL);
        if (temp.getBlockAddr() == TA.getIBA())break;
        int n = temp.getKeynum();
        int ofst = INDEX_BLOCK_INFO;
        char *values = temp.getBlock_Ptr()->record;
        for (int i = 0; i<n; i++){
            memcpy(&position, values + ofst, 4);
            //不管是哪个结点，index内保存的attr_size信息都是一样的
            //上面两行获取每一个position的值,key的值没有必要获取
            Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
            vec.push_back(tuple);
            ofst += 4 + index.attr_size;
        }
        memcpy(&ptr, values + ofst, 4);
    }
    node temp(index, ptr, NULL);
    int ofst = INDEX_BLOCK_INFO;
    char *values = temp.getBlock_Ptr()->record;
    while (1)
    {
        memcpy(&position, values + ofst, 4);
        getElement(&Element, values + ofst + 4, index.attr_size);
        //不管是哪个结点，index内保存的attr_size信息都是一样的
        //上面两行获取每一个key和position的值
        if (Element >= key)break;
        Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
        vec.push_back(tuple);
        ofst += 4 + index.attr_size;
    }
    
    if (TA.Get_Key_Exist() && equal)
    {
        Tuple_Addr tuple(TA.getIBA(), TA.getIO(), TA.getTable_Addr(), true);
        vec.push_back(tuple);
    }
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
    while (1)
    {
        node temp(index, ptr, NULL);
        if (temp.IsRoot())break;
        //从Find_key的结点到
        int n = temp.getKeynum();
        int ofst = INDEX_BLOCK_INFO;
        char *values = temp.getBlock_Ptr()->record;
        for (int i = 0; i<n; i++){
            memcpy(&position, values + ofst, 4);
            if(ptr==TA.getIBA()&&ofst<=TA.getIO())break;
            //不管是哪个结点，index内保存的attr_size信息都是一样的
            //上面两行获取每一个position的值,key的值没有必要获取
            Tuple_Addr tuple(temp.getBlockAddr(), ofst, position, true);
            vec.push_back(tuple);
            ofst += 4 + index.attr_size;
        }
        memcpy(&ptr, values + ofst, 4);
    }
}

template<class T>
void node::Delete_Key(T key)
{
    int blk_ofst = Find_Position(key);//找到块中key应在位置的位移
    T Element;
    getElement(&Element,block->record+blk_ofst,index.attr_size);
    if(IsLeaf())
    {
        if(Element!=key){
            NoKeyDeleted_Error nkde;
            throw nkde;
        }
        //如果在叶结点没有找到key，则说明删除是失败的，没有这样的key
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
        //先删除关键字，然后再恢复
        Recover_From_Half_Full(key);
    }
    else//It's not a leaf node ,so we should find the position and 
    {
        INT32 son;
        int last_pos=INDEX_BLOCK_INFO+keynum*(index.attr_size+4)+4;
        if(key<Element||blk_ofst==last_pos)
        {
            memcpy(&son, block->record + blk_ofst - 4, 4);
        }
        else
        {
            memcpy(&son, block->record + blk_ofst +index.attr_size, 4);
        }
        node son_node(index, son, this);//得到对应的子块
        son_node.Delete_Key(key);//向子块递归删除，直到遇到叶结点
    }
}

template<class T>
void node::Delete_Key_From_Node(int blk_ofst,T key)
{
    INT32 src = blk_ofst + index.attr_size;
    INT32 dest = blk_ofst - 4;
    INT32 length = INDEX_BLOCK_INFO + keynum*(index.attr_size + 4) + 4 - src;//12+4+7+4+7+4 -12+4+7
    T Element;
    getElement(&Element, block->record+INDEX_BLOCK_INFO+4, index.attr_size);
    if(!IsLeaf())memcpy(block->record+dest+4,block->record+src+4,length-4);
    else memcpy(block->record+dest,block->record+src,length);
    keynum--;
    memcpy(block->record+4,&keynum,4);
    //不要忘记维护keynum
    if(blk_ofst==INDEX_BLOCK_INFO+4&&!IsRoot()&&IsLeaf())//如果删除了最开头的key，需要向上update，内部结点不需要
    {
        T Element;
        getElement(&Element, block->record+blk_ofst, index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
        //取出删除后的第一个数据并向上更新
    }
}

template<class T>
void node::Recover_From_Half_Full(T key)//仅仅针对叶结点
{
    //这里采取的策略是，找到自己的亲兄弟，也就是同一个父亲的最近的结点
    //默认是右侧结点，如果已经是父亲节点的最右侧结点那就找到左侧结点
    //不管亲兄弟是怎么样的，都执行下一步操作
    //注意this一定是half full的，不然不会进这个函数
    INT32 blk_ofst = parent->Find_Position(key);
    bool flag=true;//flag用来判定兄弟是右结点还是左结点，true为右
    T temp;
    getElement(&temp,parent->getBlock_Ptr()->record+blk_ofst,index.attr_size);
    int last_pos=INDEX_BLOCK_INFO+parent->getKeynum()*(index.attr_size+4)+4;
    if(temp==key)blk_ofst+=4+index.attr_size;
    if(blk_ofst==last_pos)flag=false;
    INT32 sib_position = parent->Find_Real_Brother(blk_ofst,flag);
    //以上默认父亲结点被锁定，不会被回收，其实比较危险
    node sibling(index,sib_position,parent);

    if(sibling.IsHalfFull()){
        if(flag)Merge_Siblings<T>(&sibling);
        else sibling.Merge_Siblings<T>(this);
    }
    else if(flag)//从右边开头取数据
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib,keynum_this;
        memcpy(&keynum_sib,sib->record+4,4);
        memcpy(&ptr,sib->record+INDEX_BLOCK_INFO,4);
        getElement(&Element,sib->record+INDEX_BLOCK_INFO+4,index.attr_size);
        //依次得到sib的keynum／第一个key的ptr和值
        memcpy(&keynum_this,block->record+4,4);
        //得到this的keynum
        int last_ptr;
        int ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(&last_ptr,block->record+ofst,4);
        //取出this的最后一个指针，因为要往后挪
        memcpy(block->record+ofst,&ptr,4);
        setElement(block->record+ofst+4,&Element,index.attr_size);
        //向当前结点存入从sibling中取出的key和ptr
        keynum_sib--;
        keynum_this++;
        memcpy(sib->record+4,&keynum_sib,4);
        memcpy(block->record+4,&keynum_this,4);
        //更新keynum的值
        ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(block->record+ofst,&last_ptr,4);
        //将最后一个指针重新写回当前结点
        int sib_copy_length=keynum_sib*(4+index.attr_size)+4;
        memcpy(sib->record+INDEX_BLOCK_INFO,sib->record+INDEX_BLOCK_INFO+4+index.attr_size,sib_copy_length);
        //将兄弟结点的数据向前挪动4+index.attr_size
        
        getElement(&Element,sib->record+INDEX_BLOCK_INFO+4,index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
        //必须向上更新兄弟的第一个键值,注意它肯定不是root
    }
    else//从左边末尾取数据
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib;
        memcpy(&keynum_sib,sib->record+4,4);
        int ofst=(keynum_sib-1)*(4+index.attr_size)+INDEX_BLOCK_INFO;
        memcpy(&ptr,sib->record+ofst,4);
        getElement(&Element,sib->record+ofst+4,index.attr_size);
        //取出sib的最后一个key和ptr
        memcpy(sib->record+ofst,sib->record+ofst+index.attr_size+4,4);
        //将sib的最后一个指针挪到前面去
        keynum_sib--;
        memcpy(sib->record+4,&keynum_sib,4);
        //不要忘记sib的keynum要减掉
        //至此sibling的指针和key都已经取出，数据移动也已经完毕

        //下面开始更新当前结点
        int keynum_this;
        memcpy(&keynum_this,block->record+4,4); 
        //得到this的keynum
        int this_copy_length=keynum_this*(4+index.attr_size)+4;
        memcpy(block->record+INDEX_BLOCK_INFO+4+index.attr_size,block->record+INDEX_BLOCK_INFO,this_copy_length);
        //把当前结点的数据往后移动4+index.attr_size
        memcpy(block->record+INDEX_BLOCK_INFO,&ptr,4);
        setElement(block->record+INDEX_BLOCK_INFO+4,&Element,index.attr_size);
        //把从sib中得到的末尾的数据添加到当前结点的开头
        keynum_this++;
        memcpy(block->record+4,&keynum_this,4); 

        getElement(&Element,block->record+INDEX_BLOCK_INFO+4,index.attr_size);
        parent->Update_First_Key_For_Delete(Element);
        //必须更新当前结点的第一个键值，同样不会是root
    }
    //上述可以简化，因为其实两个结点的keynum都是存在node中的
    //这里没有更新node，虽然确实也不用更新……
}

template<class T>
void node::Merge_Siblings(node *sibling)
{
    Block *sib=sibling->getBlock_Ptr();
    T Element;
    getElement(&Element,sib->record+INDEX_BLOCK_INFO+4,index.attr_size);
    INT32 copy_length=sibling->getKeynum()*(4+index.attr_size)+4;
    //复制sibling结点的所有数据
    INT32 dest=INDEX_BLOCK_INFO+keynum*(4+index.attr_size);
    INT32 src=INDEX_BLOCK_INFO;
    memcpy(block->record+dest,sib->record+src,copy_length);
    keynum=keynum+sibling->getKeynum();//总和不会变
    memcpy(block->record+4,&keynum,4);
    //接下去要删掉父亲结点中，有关sib的key和ptr
    INT32 ofst_in_parent=parent->Find_Position(Element);
    if((!parent->IsRoot())&&parent->IsHalfFull())//如果父亲结点不是root而且是半满的
    {
        parent->Delete_Key_From_Node(ofst_in_parent,Element);
        parent->Recover_Internal_From_Half_Full(Element);
    }
    else if(parent->IsRoot()&&parent->getKeynum()<=1)//父亲是root而且keynum只剩下1个
    {
        int length=(keynum+sibling->getKeynum())*(4+index.attr_size)+4+INDEX_BLOCK_INFO;
        memcpy(parent->getBlock_Ptr()->record,block->record,length);
        int root_type=IsLeaf()?0:3;
        memcpy(parent->getBlock_Ptr()->record,&root_type, 4);
        //让合并后的结点成为新的root即可
        /*------------------------------------------------------buffer do-------------------------------------------------*/
        Release(block);
        //此时this的数据全都是无效的，可以被释放了
        /*------------------------------------------------------buffer do-------------------------------------------------*/        
    }
    else
    {
        parent->Delete_Key_From_Node(ofst_in_parent,Element);
    }

    /*------------------------------------------------------buffer do-------------------------------------------------*/
    Release(sib);
    //此时sib的数据全都是无效的，可以被释放了
    /*------------------------------------------------------buffer do-------------------------------------------------*/
}

template<class T>
void node::Recover_Internal_From_Half_Full(T key)
{
    INT32 blk_ofst = parent->Find_Position(key);
    bool flag=true;//flag用来判定兄弟是右结点还是左结点，true为右
    T temp;
    getElement(&temp,parent->getBlock_Ptr()->record+blk_ofst,index.attr_size);
    int last_pos=INDEX_BLOCK_INFO+parent->getKeynum()*(index.attr_size+4)+4;
    if(temp==key)blk_ofst+=4+index.attr_size;
    if(blk_ofst==last_pos)flag=false;
    INT32 sib_position = parent->Find_Real_Brother(blk_ofst,flag);
    //以上默认父亲结点被锁定，不会被回收，其实比较危险
    node sibling(index,sib_position,parent);
    int size=index.attr_size;

    if(sibling.IsHalfFull()){
        if(flag)Merge_Internal_Siblings<T>(&sibling);
        else sibling.Merge_Internal_Siblings<T>(this);
    }
    else if(flag)//右边开头取出数据
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib,keynum_this;
        memcpy(&keynum_sib,sib->record+4,4);
        memcpy(&ptr,sib->record+INDEX_BLOCK_INFO,4);
        sibling.getMinmumdDesc(Element);
        //依次得到sib的keynum／第一个key的ptr和值
        keynum_this=keynum;
        int ofst=keynum_this*(4+index.attr_size)+INDEX_BLOCK_INFO+4;
        memcpy(block->record+ofst+size,&ptr,4);
        setElement(block->record+ofst,&Element,size);
        //向当前结点存入从sibling中取出的key和ptr
        keynum_sib--;
        keynum_this++;
        memcpy(sib->record+4,&keynum_sib,4);
        memcpy(block->record+4,&keynum_this,4);
        //更新keynum的值
        int sib_copy_length=keynum_sib*(4+index.attr_size)+4;
        memcpy(sib->record+INDEX_BLOCK_INFO,sib->record+INDEX_BLOCK_INFO+4+size,sib_copy_length);
        //将兄弟结点的数据向前挪动4+index.attr_size,第一个key被牺牲了
        
        sibling.getMinmumdDesc(Element);
        parent->Update_First_Key_For_Delete(Element);
        //必须向上更新兄弟的第一个键值,注意它肯定不是root
    }
    else//从左边末尾取数据
    {
        Block *sib=sibling.getBlock_Ptr();
        INT32 ptr;
        T Element;
        int keynum_sib;
        memcpy(&keynum_sib,sib->record+4,4);
        int ofst=(keynum_sib-1)*(4+index.attr_size)+INDEX_BLOCK_INFO+4;
        memcpy(&ptr,sib->record+ofst+size,4);
        //取出sib的最后一个ptr,最后一个key被牺牲
        getMinmumdDesc(Element);
        //得到当前结点的第一个指针对应的Element
        keynum_sib--;
        memcpy(sib->record+4,&keynum_sib,4);
        //不要忘记sib的keynum要减掉
        //至此sibling的指针和key都已经取出，数据移动也已经完毕

        //下面开始更新当前结点
        int keynum_this=keynum;
        int this_copy_length=keynum_this*(4+index.attr_size);
        memcpy(block->record+INDEX_BLOCK_INFO+4+index.attr_size,block->record+INDEX_BLOCK_INFO,this_copy_length+4);
        //把当前结点的数据往后移动4+index.attr_size
        memcpy(block->record+INDEX_BLOCK_INFO,&ptr,4);
        setElement(block->record+INDEX_BLOCK_INFO+4,&Element,size);
        //把从sib中得到的末尾的数据添加到当前结点的开头
        keynum_this++;
        memcpy(block->record+4,&keynum_this,4);
        
        getMinmumdDesc(Element);
        parent->Update_First_Key_For_Delete(Element);
        //必须向上更新兄弟的第一个键值,注意它肯定不是root
    }    
}

template<class T>
void node::Merge_Internal_Siblings(node *sibling)
{
    Block *sib=sibling->getBlock_Ptr();
    int keynum_sib=sibling->getKeynum();
    //因为只需要取出最后一个儿子的第一个数据，所以直接getblock省去不必要的麻烦
    T Element;
    sibling->getMinmumdDesc(Element);
    int last_pos=keynum*(4+index.attr_size)+4+INDEX_BLOCK_INFO;
    setElement(block->record+last_pos,&Element,index.attr_size);
    //直接把这个数据插入到当前结点的最后一个指针的后面
    //这个儿子不再是父结点的最后一个儿子，而是合并后的父结点的中间那个
    INT32 dest=last_pos+index.attr_size;
    INT32 src=INDEX_BLOCK_INFO;
    INT32 copy_length=keynum_sib*(4+index.attr_size)+4;
    //复制sibling结点的所有数据
    memcpy(block->record+dest,sib->record+src,copy_length);
    keynum=keynum+keynum_sib+1;
    memcpy(block->record+4,&keynum,4);
    //更新keynum

    INT32 ofst_in_parent=parent->Find_Position(Element);
    T temp;
    getElement(&temp,parent->getBlock_Ptr()->record+ofst_in_parent,index.attr_size);
    int lp=INDEX_BLOCK_INFO+parent->getKeynum()*(index.attr_size+4)+4;
    if(temp==Element||ofst_in_parent==lp)ofst_in_parent-=4+index.attr_size;
    if((!parent->IsRoot())&&parent->IsHalfFull())//如果父亲结点不是root而且是半满的
    {
        parent->Delete_Key_From_Node(ofst_in_parent,Element);
        T First_Key;
        getElement(&First_Key, parent->getBlock_Ptr()->record+INDEX_BLOCK_INFO+4, index.attr_size);
        if(Element<First_Key)Element=First_Key;
        parent->Recover_Internal_From_Half_Full(Element);
    }
    else if(parent->IsRoot()&&parent->getKeynum()<=1)//父亲是root而且keynum只剩下1个
    {
        int length=(keynum+keynum_sib)*(4+index.attr_size)+4+INDEX_BLOCK_INFO;
        memcpy(parent->getBlock_Ptr()->record,block->record,length);
        int root_type=3;
        memcpy(parent->getBlock_Ptr()->record,&root_type, 4);
        //让合并后的结点成为新的root即可
        /*------------------------------------------------------buffer do-------------------------------------------------*/
        Release(block);
        //此时this的数据全都是无效的，可以被释放了
        /*------------------------------------------------------buffer do-------------------------------------------------*/        
    }
    else
    {
        parent->Delete_Key_From_Node(ofst_in_parent,Element);
    }

    /*------------------------------------------------------buffer do-------------------------------------------------*/
    Release(sib);
    //此时sib的数据全都是无效的，可以被释放了
    /*------------------------------------------------------buffer do-------------------------------------------------*/    
}

template<class T>
void node::Update_First_Key_For_Delete(T key)
{
    int ofst = Find_Position(key);
    int head=4+INDEX_BLOCK_INFO;
    if(ofst==head)return;
    //如果发现要更新的key是在最后或者是在内部结点的最开始，那样的话应该放弃更新
    setElement(block->record + ofst-4-index.attr_size, &key, index.attr_size);
    //只需要更新结点的上一层就可以了
}

#endif /* BPlus_Tree_hpp */









