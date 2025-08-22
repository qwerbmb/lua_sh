
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

#include "lock.h"
#include "access.h"
#include "structure.h"


//从一个指针建立访问器，指针指向的需要是writefile生成的文件
//fd用于文件锁
static struct accessor* getAccessor(lua_State* L,void* ptr,int fd){
    //struct accessor* acs=calloc(1,sizeof(struct accessor));
    struct accessor* acs=luaM_new(L,accessor);
    struct config* cfg=(struct config*)ptr;
    char* base=(char*)cfg;
    acs->base=base;
    acs->n=cfg->n;
    acs->head=(int*)(base+cfg->head);
    acs->tot=cfg->tot;
    acs->data=base+cfg->data;
    acs->eData=base+cfg->eData;
    acs->e=(struct edge*)(base+cfg->edge);
    acs->nd=(struct node*)(base+cfg->node);
    acs->hData=(struct hash_bucket*)(base+cfg->hData);
    acs->size=cfg->size;
    acs->fd=fd;
    acs->count=0;
    acs->isShare=0;
    return acs;
}


int findEdgeA(struct accessor* acs, int pos, const void* val, int siz,int ktype) {
    if (pos == -1) {
        return -1;
    }
    struct node* nd = &acs->nd[pos];
    int hash_size = nd->childNum + 1;
    if (hash_size > 0) {
        unsigned int hash = djb2_hash(val, siz);
        int bucket_idx = hash % hash_size;
        
        struct hash_bucket* buckets = acs->hData + nd->hpos;
        
        int current = bucket_idx;
        while (current != -1) {
            int edge_idx = buckets[current].edge_idx;
            if (edge_idx > 0) {
                int ww = acs->e[edge_idx].w;
                const char* eval = acs->eData + ww;
                
                if (ktype==acs->e[edge_idx].valuetype && memcmp(val, eval, siz) == 0) {
                    if(ktype!=STRING||strlen(eval)==strlen((const char*)val)){
                        return edge_idx;
                    }
                }
            }
            current = buckets[current].next_bucket;
        }
        return -1;
    }
    return -1;
    
}

// 子树转table
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
static void endShareA(lua_State* L,struct accessor* acs){
    // printf("unlock ret: %d\n",unlock(acs->fd));
    // printf("lock status: %d\n",getIsLocked(acs->fd));
    close(acs->fd);
    if(acs->isShare){
        //share时，base的内存是mmap分配的
        munmap(acs->base,acs->size);
        //统计到lua中
        global_State *g=G(L);
        g->GCdebt -= acs->size;
        // printf("minus size: %d\n",acs->size);
    }
    else{
        luaM_free_(L,acs->base,acs->size);
    }
    luaM_free_(L,acs,sizeof(accessor));
}

//引用计数
void countSA(lua_State *L,struct accessor* acs,int num){
    if(acs==NULL){
        return;
    }
    // printf("countSA = %d + %d \n",acs->count,num);
    acs->count+=num;
    if(acs->count==0){
        // printf("endshare\n");
        endShareA(L,acs);
    }
}

