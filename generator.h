#ifndef __generator
#define __generator

#include "structure.h"


struct string_intern {
    const void* data;//指向字符串内容(可以直接转char*访问)
    int pos;//在data或eData中的偏移量
    int len;//字符串长度(其实就是strlen((char*)data))
    int type;//类型
    struct string_intern* next;
};

struct bData{
    int n;
    int* head;
    int tot;
    struct edge* e;
    struct node* nd;
    char* data;
    char* eData;
    struct hash_bucket* hData;
    int cnt;//data
    int cnt2;//eData
    int cnt3;//hData
    int hDataSize;//hash总大小
    int curSiz1;
    int curSiz2;
    int curSiz3;
    
    struct string_intern** value_intern_table;
    int value_intern_size;
    
    struct string_intern** edge_intern_table;
    int edge_intern_size;
};

struct bData* initPointer(int n);

//把val,type赋值给pos这个节点；也就是叶子节点
void addNode(struct bData* global,int pos,const void* val,int type);

//添加一条边,也就是key
void add(struct bData* global,int u,int v,const void* val,int type);

//把建立好的global数据写入文件
//要在外面关描述符
void writeFile(struct bData* global,int fd);

//遍历一个建立好的树，统计child数据
int dfs(struct bData* global,int pos);

#endif