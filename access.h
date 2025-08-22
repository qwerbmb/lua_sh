#ifndef __accessor
#define __accessor

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

#include "structure.h"
#include "lock.h"
#include "lmem.h"

//访问器
typedef struct accessor{
    void* base;
    int n;
    int* head;
    int tot;
    struct edge* e;
    struct node* nd;
    char* data;
    char* eData;
    struct hash_bucket* hData;
    int size;
    int fd;
    int count;
    int isShare;
}accessor;


//返回key对应的边的编号
int findEdgeA(struct accessor* acs, int pos, const void* val, int siz,int ktype);

//返回边对应的节点编号
static inline int getEChildA(struct accessor* acs,int pos,int eNum){
    if(pos==-1){
        //用一下这个变量，不然有警告
    }
    return acs->e[eNum].v;
}

//直接子节点数量
static inline int getChildNumA(struct accessor* acs,int pos){
    return acs->nd[pos].childNum;
}

//所有子节点数量
static inline int getAllChildNumA(struct accessor* acs,int pos){
    return acs->nd[pos].allChildNum;
}

//深度。深度定义为从这一点出发到子树内叶子节点的最长距离
static inline int getDepthA(struct accessor* acs,int pos){
    return acs->nd[pos].depth;
}

//对于叶子节点返回存储的值，否则NULL
static inline void* getValA(struct accessor* acs,int pos){
    return pos>=0 && pos<acs->n ? acs->data+acs->nd[pos].vpos : NULL;
}

//对于叶子节点，返回存储的值的类型，否则0
static inline int getValTypeA(struct accessor* acs,int pos){
    return acs->nd[pos].valueType;
}

//是否是叶子节点
static inline int isLeafA(struct accessor* acs,int pos){
    return acs->nd[pos].childNum==0;
}

//返回编号对应的边的值(也就是key)，用于pairs
static inline const void* getEdgeA(struct accessor* acs,int eNum){
    return acs->eData+acs->e[eNum].w;
}

//
static inline int getEdgeTypeA(struct accessor* acs,int eNum){
    return acs->e[eNum].valuetype;
}

//获取一个节点的一条出边，用于pairs
static inline int getIterStartA(struct accessor* acs,int pos){
    return acs->head[pos];
}

//返回边的下一条边，用于pairs
static inline int getNextEdgeA(struct accessor* acs,int eNum){
    return acs->e[eNum].next;
}

// 子树转table
void build_full_tableA(lua_State* L, struct accessor* acs,int pos);

//从文件产生一个访问器
struct accessor* getAccessorFromFile(lua_State *L,const char* path);

//把文件映射到共享内存，产生一个访问器
struct accessor* getAccessorFromShare(lua_State *L,const char* path);


//修改引用计数，如果变成0会释放acs
void countSA(lua_State *L,struct accessor* acs,int num);

#endif