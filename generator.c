//生成文件的部分



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "structure.h"
#include "generator.h"
#include "lobject.h"
#include "lstring.h"

/// @brief 初始化一个bData指针
/// @param n 节点个数
struct bData* initPointer(int n){
    struct bData* global=(struct bData*)calloc(1,sizeof(struct bData));
    global->n=n;
    
    global->heade=(int*)calloc(1,sizeof(int)*global->n);
    //这个head初始化为0，边是从1开始存储的，遍历到0表示结束
    global->e=(struct edge*)calloc(global->n,sizeof(struct edge));
    global->tot=0;
    
    global->nd=(struct node*)calloc(global->n,sizeof(struct node));

    int avg=64;//期望k,v平均长度,sizeof(TString)=32
    global->sData=(char*)calloc((global->n)*avg*2,sizeof(char));
    global->cnts=0;

    //链表合计长度=出边数量=n-1<n
    global->hData = (struct hash_bucket*)calloc(n, sizeof(struct hash_bucket));
    global->headh = (int*)calloc(global->n,sizeof(int));
    //memset(global->headh,0xFF,sizeof(int)*global->n);
    global->cnth=1;//初始化为1，遍历时可以直接判断!=0
    global->cnthh=0;

    //链表合计长度 <= 节点总数+边数 <=n*2-1 < n*2
    global->ghData = (struct hash_bucket*)calloc(n*2, sizeof(struct hash_bucket));
    global->headgh = (int*)calloc(global->n*2,sizeof(int));
    //memset(global->headh,0xFF,sizeof(int)*global->n);
    global->cntgh=1;
    return global;
}



//释放global，以及hash表
static void freeGlobal(struct bData* global){
    free(global->heade);
    free(global->e);
    free(global->nd);
    free(global->sData);
    free(global->hData);
    free(global->headh);
    free(global->ghData);
    free(global->headgh);
    free(global);
}

/// @brief 向全局表加入一个元素
/// @param global 
/// @param ptr 
/// @param ktype 
static inline int addHashGS(bData* global,const void* ptr,int ktype){

    bData* g=global;
    return addGH(g->ghData,g->n*2,&(g->cntgh),g->headgh,
        ptr,ktype,0,
        g->sData,&(g->cnts));
}

/// @brief 查询一个元素在全局hash表中的位置
/// @param global 
/// @param ptr 
/// @return 偏移量
static int queryGS(bData* global,const void* ptr,int ktype){
    int ret=queryH(global->ghData,global->n*2,global->headgh,
                    ptr,ktype,
                    global->sData);
    return ret;
}


/// @brief 向某个节点的hash表插入一个元素
/// @param global 
/// @param ptr 
/// @param ktype 
/// @param pos 
/// @param eNum 
/// @return 
static int addHashNS(bData* global,int pos,int eNum){
    bData* g=global;
    int cnum=g->nd[pos].childNum;
    struct edge* e=global->e;
    const void* ptr=global->sData+e[eNum].w;
    int ktype=e[eNum].valuetype;
    return addNH(g->hData,cnum,&(g->cnth),g->headh+g->nd[pos].hpos,
        ptr,ktype,eNum,
        g->ghData,g->n*2,&(g->cntgh),g->headgh,
        g->sData,&(g->cnts));
}

//为一个节点的出边建立hash表
static void buildHashTable(struct bData* global, int pos) {
    struct node* nd = &global->nd[pos];
    int childNum = nd->childNum;
    if (childNum <= 0) {
        nd->hpos = -1;
        return;
    }
    
    nd->hpos = global->cnthh;
    global->cnthh+=childNum;
    int* head = global->heade;
    struct edge* e=global->e;
    for(int i=head[pos];i!=0;i=e[i].next){
        addHashNS(global,pos,i);
    }
}




//把val,type赋值给pos这个节点；也就是叶子节点
void addNode(struct bData* global,int pos,const void* val,int type){
    struct node* nd=global->nd;
    
    int bkt=queryGS(global,val,type);
    if(bkt==0){
        bkt=addHashGS(global,val,type);
    }
    hash_bucket* h=global->ghData;
    nd[pos].vpos=h[bkt].kvalue;
    nd[pos].valueType=type;
    nd[pos].vs=h[bkt].ksvalue;
    nd[pos].hpos=-1;//没有出边
}

//添加一条边,也就是key
void add(struct bData* global,int u,int v,const void* val,int type){
    //printf("add %d %d\n",u,v);
    int tot=global->tot;
    struct edge* e=global->e;
    int* heade=global->heade;
    tot++;
    int bkt=queryGS(global,val,type);
    if(bkt==0){
        bkt=addHashGS(global,val,type);
    }
    hash_bucket* h=global->ghData;
    e[tot] = (struct edge){v,h[bkt].kvalue,heade[u],type,h[bkt].ksvalue};
    heade[u] = tot;
    global->tot = tot;
}

//从指针a处复制num个元素到addr处，并使addr增加写入的长度
#define WSZ(addr, a , num) do { \
    memcpy(addr, a, num*sizeof(*a)); \
    addr = (char*)addr + num*sizeof(*a); \
} while(0)

//从指针a处复制num个元素到addr+dev处，并使dev增加写入的长度
#define WMEM(addr,dev, a , num) do { \
    memcpy(addr+dev, a, num*sizeof(*a)); \
    dev = dev + num*sizeof(*a); \
} while(0)


//把建立好的global数据写入文件
//要在外面关描述符
void writeFile(struct bData* global,int fd){
    bData* g=global;
    int n = g->n;
    int* heade = g->heade;
    struct edge* e = g->e;
    
    node* nd = g->nd;
    char* sData = g->sData;
    int cnts = g->cnts;
    
    hash_bucket* hData = g->hData;
    int* headh = g->headh;
    int cnth = g->cnth;
    
    hash_bucket* ghData = g->ghData;
    int* headgh = g->headgh;
    int cntgh = g->cntgh;

    int fileSize = sizeof(struct config) +
                  n * sizeof(int) +          //heade
                  n * sizeof(struct edge) + //edge,边从1开始
                  n * sizeof(struct node) +   //node
                  cnts * sizeof(hash_bucket) +  //sData  
                  
                  cnth * sizeof(hash_bucket) +  //hData
                  n * sizeof(int) + //headh
                  cntgh * sizeof(hash_bucket) +  //ghData
                  n * 2 * sizeof(int) ;  //headgh
    
    void* base=malloc(fileSize);
    char* addr=(char*)base;
    struct config cfg={};
    int dev=sizeof(cfg);

    cfg.n=n;
    cfg.heade=dev;
    WMEM(addr,dev,heade,n);

    cfg.e=dev;
    WMEM(addr,dev,e,n);

    cfg.nd=dev;
    WMEM(addr,dev,nd,n);

    cfg.sData=dev;
    WMEM(addr,dev,sData,cnts);

    cfg.hData=dev;
    WMEM(addr,dev,hData,cnth);

    cfg.headh=dev;
    WMEM(addr,dev,headh,n);
    //这里是n-(叶子节点数量)，但是int多一些不关键

    cfg.ghData=dev;
    WMEM(addr,dev,ghData,cntgh);

    cfg.headgh=dev;
    WMEM(addr,dev,headgh,n*2);
    //这里一定是n*2

    cfg.size=fileSize;

    dev=0;
    WMEM(base,dev,&cfg,1);

    write(fd,base,fileSize);
    freeGlobal(global);
    free(base);
}

//遍历一个建立好的树，统计child数据
int dfs(struct bData* global,int pos){
    struct edge* e=global->e;
    int* head=global->heade;
    struct node* nd=global->nd;
    int sum=0;
    int allsum=0;
    nd[pos].depth=1;
    for(int i=head[pos];i!=0;i=e[i].next){
        int v=e[i].v;
        dfs(global,v);
        allsum=allsum+nd[v].allChildNum+1;
        sum=sum+1;
        nd[pos].depth=nd[pos].depth < nd[v].depth+1 ? nd[v].depth+1 : nd[pos].depth;
    }
    nd[pos].allChildNum=allsum;
    nd[pos].childNum=sum;
    
    buildHashTable(global, pos);
    
    return 0;
}