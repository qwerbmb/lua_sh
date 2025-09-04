#ifndef __generator
#define __generator

#include "structure.h"


typedef struct bData{
    int n;//节点总数

    int* heade;//树的结构
    struct edge* e;//存放边。从1开始，其他所有都从0开始。
    int tot;//当前e用到的位置

    struct node* nd;//存放点

    char* sData;//所有的数据,string以TString存储
    int cnts;//data用到了哪
    
    struct hash_bucket* hData;//每个节点分别的边hash
    int* headh;//每个节点分别的边hash的头节点
    int cnth;//hData用到了哪
    int cnthh;//headh用到了哪

    
    struct hash_bucket* ghData;//所有的string的hash
    int* headgh;//所有的string的hash的头节点
    int cntgh;//ghData用到了哪
}bData;

struct bData* initPointer(int n);

void addNode(struct bData* global,int pos,const void* val,int type);

void add(struct bData* global,int u,int v,const void* val,int type);

void writeFile(struct bData* global,int fd);

int dfs(struct bData* global,int pos);

#endif