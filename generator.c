//生成文件的部分
//todo:这里也应该都改用luaM



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "structure.h"
#include "generator.h"


struct bData* initPointer(int n){
    struct bData* global=(struct bData*)calloc(1,sizeof(struct bData));
    global->n=n;
    global->tot=0;
    global->head=(int*)calloc(1,sizeof(int)*global->n);
    global->e=(struct edge*)calloc(1,sizeof(struct edge)*global->n);
    global->nd=(struct node*)calloc(1,sizeof(struct node)*global->n);
    int avg=32;//期望k,v平均长度
    global->data=(char*)calloc(1,sizeof(char)*(global->n)*avg);
    global->eData=(char*)calloc(1,sizeof(char)*(global->n)*avg);
    
    //桶大小是n，至多溢出n-1次，故总大小至多为n*2
    //但是桶最小是4，所以最坏会有(n-1)*4
    int estimated_hash_size = n * 8;
    global->hData = (struct hash_bucket*)calloc(estimated_hash_size, sizeof(struct hash_bucket));
    global->hDataSize = estimated_hash_size;
    
    int intern_size = n > 8 ? n: 8;//最多n
    
    global->value_intern_table = (struct string_intern**)calloc(intern_size, sizeof(struct string_intern*));
    global->value_intern_size = intern_size;
    
    global->edge_intern_table = (struct string_intern**)calloc(intern_size, sizeof(struct string_intern*));
    global->edge_intern_size = intern_size;
    
    global->cnt=0;
    global->cnt2=0;
    global->cnt3=0;
    return global;
}

static int find_interned_string(struct string_intern** table, int table_size, const void* val, int len, int type) {
    if (type != STRING) {
        return -1;
    }
    
    unsigned int hash = djb2_hash((void*)val, len);
    int index = hash % table_size;
    
    struct string_intern* current = table[index];
    while (current != NULL) {
        if (current->type == type && current->len == len) {
            if (memcmp(current->data, val, len) == 0) {
                return current->pos;
            }
        }
        current = current->next;
    }
    
    return -1;  // 未找到
}

static void add_interned_string(struct string_intern** table, int table_size, const void* val, int len, int type, int pos) {
    if (type != STRING) {
        return;
    }
    
    unsigned int hash = djb2_hash((void*)val, len);
    int index = hash % table_size;
    
    struct string_intern* new_entry = (struct string_intern*)malloc(sizeof(struct string_intern));
    new_entry->data = val;
    new_entry->len = len;
    new_entry->type = type;
    new_entry->pos = pos;
    new_entry->next = table[index];
    table[index] = new_entry;
}

//释放global，以及hash表
static void freeGlobal(struct bData* global){
    free(global->head);
    free(global->e);
    free(global->data);
    free(global->eData);
    free(global->hData);
    
    //其实应该像上面那样放到一整块区域的
    for (int i = 0; i < global->value_intern_size; i++) {
        struct string_intern* current = global->value_intern_table[i];
        while (current != NULL) {
            struct string_intern* next = current->next;
            free(current);
            current = next;
        }
    }
    free(global->value_intern_table);
    
    for (int i = 0; i < global->edge_intern_size; i++) {
        struct string_intern* current = global->edge_intern_table[i];
        while (current != NULL) {
            struct string_intern* next = current->next;
            free(current);
            current = next;
        }
    }
    free(global->edge_intern_table);
    
    free(global);
}


//int ccount=0;
static void buildHashTable(struct bData* global, int node_pos) {
    struct node* nd = &global->nd[node_pos];
    int childNum = nd->childNum;
    
    if (childNum <= 0) {
        nd->hpos = -1;
        return;
    }
    
    int hash_size = calculateHashSize(childNum);
    nd->hpos = global->cnt3;
    
    for (int i = 0; i < hash_size; i++) {
        global->hData[global->cnt3 + i].edge_idx = 0;
        global->hData[global->cnt3 + i].next_bucket = -1;
    }
    
    struct edge* e = global->e;
    int* head = global->head;
    char* eData = global->eData;
    int overflow_idx = global->cnt3 + hash_size; //溢出区的起始位置
    
    for (int i = head[node_pos]; i != 0; i = e[i].next) {
        int edge_idx = i;
        int key_type = e[i].valuetype;
        
        const void* key = eData + e[i].w;
        int key_size = getValSize(key, key_type);
        
        unsigned int hash = djb2_hash(key, key_size);
        int bucket_idx = hash % hash_size;
        
        int table_pos = global->cnt3 + bucket_idx;
        
        if (global->hData[table_pos].edge_idx != 0) {
            // if(ccount%500==0){
            //    printf("collision:%d\n",ccount++);
            // }
            // else{
            //     ccount++;
            // }
            
            global->hData[overflow_idx].edge_idx = edge_idx;
            global->hData[overflow_idx].key_type = key_type;
            global->hData[overflow_idx].next_bucket = global->hData[table_pos].next_bucket;
            global->hData[table_pos].next_bucket = overflow_idx - global->cnt3;
            
            overflow_idx++;
        } else {
            global->hData[table_pos].edge_idx = edge_idx;
            global->hData[table_pos].key_type = key_type;
            global->hData[table_pos].next_bucket = -1;
        }
    }
    
    global->cnt3 = overflow_idx;
}

//把val,type赋值给pos这个节点；也就是叶子节点
void addNode(struct bData* global,int pos,const void* val,int type){
    struct node* nd=global->nd;
    char* data=global->data;
    nd[pos].valueType=type;

    int len = getValSize(val, type);
    int existing_pos = -1;
    
    if (type == STRING) {
        existing_pos = find_interned_string(global->value_intern_table, global->value_intern_size, val, len, type);
    }
    
    if (existing_pos != -1) {
        nd[pos].vpos = existing_pos;
    } else {
        int cnt = global->cnt;
        memcpy(data + cnt, val, len);
        nd[pos].vpos = cnt;
        
        if (type == STRING) {
            add_interned_string(global->value_intern_table, global->value_intern_size, 
                               data + cnt, len, type, cnt);
        }
        
        cnt += len;
        global->cnt = cnt;
    }
}

//添加一条边,也就是key
void add(struct bData* global,int u,int v,const void* val,int type){
    //printf("add %d %d\n",u,v);
    int tot=global->tot;
    struct edge* e=global->e;
    char* eData=global->eData;
    int* head=global->head;
    tot++;

    int len = 0;
    len=getValSize(val, type);

    int existing_pos = -1;
    
    if (type == STRING) {
        existing_pos = find_interned_string(global->edge_intern_table, global->edge_intern_size, val, len, type);
    }
    
    int edge_data_pos;
    
    if (existing_pos != -1) {
        edge_data_pos = existing_pos;
    } else {
        int cnt2 = global->cnt2;
        memcpy(eData + cnt2, val, len);
        
        if (type == STRING) {
            add_interned_string(global->edge_intern_table, global->edge_intern_size, 
                               eData + cnt2, len, type, cnt2);
        }
        
        edge_data_pos = cnt2;
        cnt2 += len;
        global->cnt2 = cnt2;
    }
    
    e[tot] = (struct edge){v, edge_data_pos, head[u], type};
    head[u] = tot;
    global->tot = tot;
}

#define WSZ(addr, a , num) do { \
    memcpy(addr, a, num*sizeof(*a)); \
    addr = (char*)addr + num*sizeof(*a); \
} while(0)

#define WMEM(addr,dev, a , num) do { \
    memcpy(addr+dev, a, num*sizeof(*a)); \
    dev = dev + num*sizeof(*a); \
} while(0)


//把建立好的global数据写入文件
//要在外面关描述符
void writeFile(struct bData* global,int fd){
    int n=global->n;
    struct edge* e=global->e;
    struct node* nd=global->nd;
    int* head=global->head;
    int tot=global->tot;
    char* data=global->data;
    char* eData=global->eData;
    struct hash_bucket* hData=global->hData;
    int cnt=global->cnt;
    int cnt2=global->cnt2;
    int cnt3=global->cnt3;

    int fileSize = sizeof(struct config) +
                  n * sizeof(int) +          //head
                  (tot+1) * sizeof(struct edge) + //edge,边从1开始
                  n * sizeof(struct node) +   //node
                  cnt +                       //node数据
                  cnt2 +                      //edge数据
                  cnt3 * sizeof(struct hash_bucket); //hash数据
    
    void* base=malloc(fileSize + 1024);
    char* addr=(char*)base;
    struct config cfg={0};
    int dev=sizeof(cfg);

    cfg.n=n;

    cfg.head=dev;
    WMEM(addr,dev,head,n);

    cfg.tot=tot;

    cfg.edge=dev;
    WMEM(addr,dev,e,(tot+1));//边从1开始

    cfg.node=dev;
    WMEM(addr,dev,nd,n);

    cfg.data=dev;
    WMEM(addr,dev,data,cnt);

    cfg.eData=dev;
    WMEM(addr,dev,eData,cnt2);

    cfg.hData=dev;
    WMEM(addr,dev,hData,cnt3);

    cfg.size=dev;

    int p=0;
    WMEM(base,p,&cfg,1);

    size_t size = dev;
    write(fd,base,size);
    freeGlobal(global);
    free(base);
}

//遍历一个建立好的树，统计child数据
int dfs(struct bData* global,int pos){
    struct edge* e=global->e;
    int* head=global->head;
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