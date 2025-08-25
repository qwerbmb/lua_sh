//数据结构的定义
#ifndef __structure
#define __structure
#include <string.h>
#include "lua.h"
struct edge{
    int v,w,next;
    //w:边值地址(edata)的偏移量
    int valuetype;
};


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

//根据子节点数量计算hash表大小
//目前直接设为max(cnt+1,4)
static inline int calculateHashSize(int childCount) {
    if (childCount <= 0) return 0;
    
    // int size = 1;
    // while (size < childCount * 2) {
    //     size *= 2;
    // }
    int size=childCount+1;
    return size < minhashsize ? minhashsize : size;
}

struct hash_bucket {
    int key_type;
    //lua的
    int edge_idx;
    int next_bucket;
};

struct node{
    int vpos;
    //节点对应值(data)的偏移量
    int valueType;
    
    int hpos;
    //hash表(hdata)的偏移量
    //int hash_size;    
    //桶数量现在总是设为childNum+1
    int depth;
    int childNum;
    int allChildNum;
};
struct config{
    int n;
    int head;
    int tot;
    int edge;
    int node;
    int data;
    int eData;
    int hData;
    int size;
    //以上数据在共享内存中起始位置的偏移量
};
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