//数据结构的定义
#ifndef __structure
#define __structure
#include <string.h>
#include "lua.h"
struct edge{
    int v;  //指向的子节点
    int w;  //边对应的key在sdata中的偏移量
    int next;  //树结构的下一条边
    int valuetype;  //边值类型
    
};

//从一个TString*的数据部分(contents)获取指向完整结构的指针
#define strpre offsetof(TString, contents)

/// @brief 根据值的类型获取对应大小，str包含\0
/// @param ptr 指向值的指针
/// @param type 类型
/// @return 
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

typedef struct hash_bucket {
    int value1;  //存储的值
    int kvalue;  //key所在位置(sdata)的偏移量
    int ktype;  //key的类型
    int next_bucket;  //链表
}hash_bucket;

struct node{
    int vpos;  //节点对应值(sdata)的偏移量
    int valueType;  //节点对应值的类型
    int hpos;  //head起始位置的偏移量,设定的head大小总是childcount
    int depth;  //深度，定义为到子树内节点的最长距离
    int childNum;  //直接子节点个数
    int allChildNum;  //所有子节点个数
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

#endif