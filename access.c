
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lua.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"

#include "lock.h"
#include "access.h"
#include "structure.h"
static void setGlobalHash(lua_State* L,accessor* acs){
    global_State* g=G(L);
    uint* h=acs->hashval;
    int cntgh=acs->cntgh;
    for(int i = 0 ;i < cntgh;i++){
        const char* val=acs->sData+acs->ghData[i].kvalue;
        h[i]=luaS_hash(val,strlen(val),g->seed);
    }
}
/*
str->hash
str->acs->hashval[str->hash]
没法->acs
可能直接hash指针的值？
hashul((ul)str)



另一种方式：
str里再存一个自身偏移量，然后查找复杂度就是O(acs数量)

*/
// unsigned int getGlobalHash(lua_State* L,accessor* acs,unsigned int phash){
//     global_State* g=G(L);
// }

//从一个指针建立访问器，指针指向的需要是writefile生成的文件
//fd用于文件锁
static struct accessor* getAccessor(lua_State* L,void* ptr,int fd){
    //struct accessor* acs=calloc(1,sizeof(struct accessor));
    struct accessor* acs=luaM_new(L,accessor);
    struct config* cfg=(struct config*)ptr;
    char* base=(char*)cfg;
    acs->base=base;
    acs->n=cfg->n;
    acs->head=(int*)(base+cfg->heade);
    acs->e=(struct edge*)(base+cfg->e);
    acs->nd=(struct node*)(base+cfg->nd);
    acs->sData=base+cfg->sData;

    acs->hData=(struct hash_bucket*)(base+cfg->hData);
    acs->headh=(int*)(base+cfg->headh);
    acs->ghData=(struct hash_bucket*)(base+cfg->ghData);
    acs->headgh=(int*)(base+cfg->headgh);
    acs->cntgh=cfg->cntgh;
    acs->size=cfg->size;
    acs->fd=fd;
    acs->count=0;
    acs->isShare=0;
    
    acs->hashval=(uint*)luaM_malloc_(L,sizeof(size_t)*cfg->cntgh,0);
    setGlobalHash(L,acs);
    global_State* g=G(L);
    //添加到链表末尾，保证查找是按加入顺序
    acs->next=NULL;
    if(g->acslist==NULL){
        g->acslist=acs;
        
    }
    else{
        for(accessor* a=g->acslist;a!=NULL;a=a->next){
            if(a->next==NULL){
                a->next=acs;
            }
        }
    }
    // acs->next=g->acslist;
    // g->acslist=acs;

    return acs;
}


int findEdgeA(struct accessor* acs, int pos, const void* val,int ktype) {
    if (pos == -1) {
        return -1;
    }
    struct node* nd = &(acs->nd[pos]);
    int hash_size = nd->childNum;
    int bkt = queryH(acs->hData, hash_size, acs->headh + nd->hpos, val, ktype,acs->sData);
    if(bkt==0){
        return -1;
    }
    return acs->hData[bkt].value1;
    // struct edge* e = acs->e;
    // int* head = acs->head;
    
    // for (int i = head[pos]; i != 0; i = e[i].next) {
    //     if (i == e[i].next) {
    //         break;
    //     }
    //     int v = e[i].v, ww = e[i].w;
        
    //     const char* eval = acs->eData + ww;
        
    //     if (ktype==e[i].valuetype && memcmp(val, eval, siz) == 0) {
    //         if(ktype!=STRING||strlen(eval)==strlen((const char*)val)){
    //             return v;
    //         }
    //     }
    // }
    
    // return -1;
    
}

// 子树转table
//返回的table含共享成分
/*
如果要不含共享成分会有问题。如果存在str，不在strtable但是在某个shm
此时返回的table中的str和上述str地址不同，导致不能相等
也就是产生了同时在两个层级的str

我改了intern所以pushstring已经会在shm查找了

*/
void build_full_tableA(lua_State* L, struct accessor* acs,int pos) {
    int depth = getDepthA(acs,pos);
    //printf("%d %d\n",pos,depth);
    if (depth == 1) {
        // 叶子节点
        //printf("leaf %d\n",acs->nd[pos].valueType);
        switch(acs->nd[pos].valueType){
            case DOUBLE:
                lua_pushnumber(L, *(double*)getValA(acs,pos));
                break;
            case BOOLEAN:
                lua_pushboolean(L, *(int*)getValA(acs,pos));
                break;
            case STRING:
                lua_pushstring(L, (char*)getValA(acs,pos));
                break;
            case INTEGER:
                lua_pushinteger(L, *(lua_Integer*)getValA(acs,pos));
                break;
            default:
                break;

        }
    } else {
        // 非叶
        lua_createtable(L, 0, 0);
        for(int i=acs->head[pos];i!=0;i=acs->e[i].next){
            int v=acs->e[i].v;
            int ty=acs->e[i].valuetype;
            if(ty==STRING){
                const char* edge=(const char*)getEdgeA(acs,i);
                build_full_tableA(L, acs,v);
                lua_setfield(L, -2, edge);
            }
            else if(ty==DOUBLE){
                const double* edge = (const double*)getEdgeA(acs, i);
                lua_pushnumber(L, *edge);
                build_full_tableA(L, acs, v);
                lua_settable(L, -3);
            }
            else if(ty==INTEGER){
                const lua_Integer* edge = (const lua_Integer*)getEdgeA(acs, i);
                lua_pushinteger(L, *edge);
                build_full_tableA(L, acs, v);
                lua_settable(L, -3);
            }
        }
    }
}

struct accessor* getAccessorFromFile(lua_State *L,const char* path){
    //const char* path="testnc";
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        return NULL;
    }
    if(getRLock(fd) == -1){
        close(fd);
        return NULL;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        return NULL;
    }
    off_t file_size = sb.st_size;

    //char *content = malloc(file_size + 1);
    char* content=(char*)luaM_malloc_(L,file_size+1,0);
    read(fd, content, file_size);

    content[file_size] = '\0';
    struct accessor* acs=getAccessor(L,content,fd);
    return acs;
}

//从文件描述符获取一块等于文件大小的共享内存
static void* getShare(lua_State* L,int fd){
    //没办法，lua并没有提供类似luaM_mmap的东西
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        return NULL;
    }
    off_t file_size = sb.st_size;

    void *ptr = mmap(
        NULL,                 // 由系统选择映射地址
        file_size,            // 映射长度
        PROT_READ,            // 只读保护
        MAP_SHARED,           // 共享映射
        fd,                   // 文件描述符
        0                     // 文件偏移量(从开头开始)
    );
    
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    //统计到lua中
    global_State *g=G(L);
    g->GCdebt += file_size;
    // printf("alloc share size: %ld\n",file_size);
    return ptr;
}
struct accessor* getAccessorFromShare(lua_State *L,const char* path){
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }
    getRLock(fd);
    void* ptr=getShare(L,fd);
    accessor* acs=getAccessor(L,ptr,fd);
    acs->isShare=1;
    return acs;
}

//释放acs指针，以及可能存在的共享内存
void endShareA(lua_State* L,struct accessor* acs){
    // printf("unlock ret: %d\n",unlock(acs->fd));
    // printf("lock status: %d\n",getIsLocked(acs->fd));

    global_State* g=G(L);
    accessor* a=g->acslist;
    if(a==acs){
        g->acslist=NULL;
        //todo:把acslist改成双向链表
    }
    while(a!=NULL){
        if(a->next==acs){
            a->next=acs->next;
            break;
        }
        a=a->next;
    }
    luaM_free_(L,acs->hashval,sizeof(uint)*acs->cntgh);

    close(acs->fd);
    //flock关闭文件描述符时自动释放
    if(acs->isShare){
        //share时，base的内存是mmap分配的
        munmap(acs->base,acs->size);
        //统计到lua中
        global_State *gg=G(L);
        gg->GCdebt -= acs->size;
        // printf("minus size: %d\n",acs->size);        
    }
    else{
        luaM_free_(L,acs->base,acs->size);
    }

    luaM_free_(L,acs,sizeof(accessor));
    // printf("free acs\n");
}

//引用计数
void countSA(lua_State *L,struct accessor* acs,int num){
    if(acs==NULL){
        return;
    }
    if(L && num){

    }
    // printf("countSA = %d + %d \n",acs->count,num);
    //暂时不用引用计数了，改为close时释放
    // acs->count+=num;
    // if(acs->count==0){
    //     // printf("endshare\n");
    //     endShareA(L,acs);
    // }
}

