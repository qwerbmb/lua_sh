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
#include "lobject.h"

typedef unsigned int uint;

typedef struct accessor{
    void* base; //文件内容
    int n;  //节点总数
    int* head;  //存储树结构的头节点
    struct edge* e;  //边
    struct node* nd;  //节点
    char* sData;  //存储的数据；int,double也转为char*存储
    
    struct hash_bucket* hData;  //节点出边的hash
    int* headh;  //hdata的头节点；节点需要headh+nd[pos].hpos获取自身头节点起始位置

    hash_bucket* ghData;  //sdata的hash
    int* headgh;  //ghdata的头节点
    int cntgh;   //sdata的元素总数量，= ghdata的数量
    //以上是把config的偏移量转换回的指针

    int size;  //base大小
    int fd;  //打开base使用的文件描述符
    int count;  //引用计数
    int isShare;  //是否在shm
    struct accessor* next;  //当前进程加载的下一个acs
    uint* hashval;  //当前acs里所有str值的hash
    char* path;
}accessor;


int findEdgeA(struct accessor* acs, int pos, const void* val,int ktype);

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
    return pos>=0 && pos<acs->n ? acs->sData+acs->nd[pos].vpos : NULL;
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
    return acs->sData+acs->e[eNum].w;
}

//获取一条边的key的类型
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

void indexA(lua_State* L,const TValue* sd,TValue* key,StkId val);

void build_full_tableA(lua_State* L, struct accessor* acs,int pos);

struct accessor* getAccessorFromFile(lua_State *L,const char* path);

struct accessor* getAccessorFromShare(lua_State *L,const char* path);

void countSA(lua_State *L,struct accessor* acs,int num);

void endShareA(lua_State *L,struct accessor* acs);

#endif