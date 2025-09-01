//数据结构的定义
#ifndef __structure
#define __structure
#include <string.h>
#include "lua.h"
struct edge{
    int v,w,next;
    //w:边值地址(edata)的偏移量
    int valuetype;
    //valuetype:边值类型
    int ws;
    //ws:在valuetype=STRING的情况下，value作为TString的偏移量
};
#define strpre offsetof(TString, contents)

static inline int getValSize(const void* ptr, int type) {
    switch(type) {
        case INTEGER:
            return sizeof(lua_Integer);
        case DOUBLE:
            return sizeof(double);
        case STRING:
            return strlen((const char*)ptr) + 1;
        case BOOLEAN:
            return sizeof(int);
        default:
            return 0;
    }
}


//hash函数
static inline unsigned int djb2_hash(const void* key, int size) {
    unsigned long hash = 5381;
    const unsigned char* str = (const unsigned char*)key;
    int c;
    for(int i=0;i<size;i++){
        c = *str++;
        hash = ((hash << 5) + hash) + c;

    }
    return (unsigned int)hash;
}

static const int minhashsize=4;

typedef struct hash_bucket {
    
    int value1;//值


    int kvalue;//key所在位置的偏移量
    int ksvalue;//如果这是一个string，这是TString的偏移量
    int ktype;//key的type
    int next_bucket;//链表
}hash_bucket;

struct node{
    
    int vpos;
    //节点对应值(sdata)的偏移量
    int valueType;
    //节点对应值的类型
    int vs;
    //在valueType=STRING的情况下，value作为TString的偏移量
    int hpos;
    //head起始位置的偏移量,设定的head大小总是childcount
    int depth;//深度，定义为到子树内节点的最长距离
    int childNum;//直接子节点个数
    int allChildNum;//所有子节点个数
};
typedef struct config{
    int n;
    int heade;
    int e;
    
    int nd;
    int sData;
    
    int hData;
    int headh;
    
    int ghData;
    int headgh;
    int cntgh;
    int size;
    //以上数据在共享内存中起始位置的偏移量
}config;

int addGH(hash_bucket* h,int hashsize,int* cntof,int* head,
            const void* key,int ktype,int value,
            void* kq,int* cntkq);

int queryH(const hash_bucket* h,int hashsize,const int* head,
            const void* key,int ktype,
            const void* kq);

int addNH(hash_bucket* h,int hashsize,int* cntof,int* head,
            const void* key,int ktype,int value,
            hash_bucket* gh,int ghsize,int* gcntof,int* ghead,
            void* kq,int* cntkq);
/*
config
head
edge
node
data
eData
hData

*/

#endif