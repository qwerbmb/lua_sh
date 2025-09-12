#include <string.h>
#include "lua.h"
#include "structure.h"
#include "lstring.h"


/// @brief 把一个key插入到全局桶，不判重，string会加前缀
/// @param h 全局桶的起始地址
/// @param hashsize 全局hash大小
/// @param cntof 当前用到了哪
/// @param head 头节点
/// @param key 键
/// @param ktype 键的类型
/// @param value 值(实际上全局hash不需要值，所以填0)
/// @param kq key存放的位置
/// @param cntkq kq用到了哪
/// @return 插入到的桶编号
int addGH(hash_bucket* h,int hashsize,int* cntof,int* head,
            const void* key,int ktype,int value,
            void* kq,int* cntkq){
    int ksize=getValSize(key,ktype);
    int hash=djb2_hash(key,ksize)%hashsize;
    int pos=*cntof;
    hash_bucket* fin=h+pos;
    *cntof=*cntof+1;
    fin->next_bucket=head[hash];
    head[hash]=pos;
    if(ktype==STRING){
        //string在前面存一个前缀，前缀不参与查找
        //使用(Tstring*)(ptr-strpre)获取TString*
        TString* ts=(TString*)(kq+*cntkq);
        // ts->hash=pos;
        
        if(ksize <= LUAI_MAXSHORTLEN){
            ts->shrlen = cast_byte(ksize) - 1;
            //lua的shr/lnglen都是不含\0的
            ts->u.hnext=NULL;
            ts->tt=LUA_VSHRSTR;
            ts->extra = 0;
            ts->isShare = 1;
            ts->hash=luaS_hash((const char*)key,ksize-1,998244353);
        }
        else{
            ts->shrlen = 1;
            ts->u.lnglen = ksize - 1;
            ts->tt=LUA_VLNGSTR;
            ts->extra = 1;
            ts->isShare = 1;
            ts->hash=luaS_hash((const char*)key,ksize-1,998244353);
            //LNGSTR的extra标记表示已有hash
        }
        *cntkq=*cntkq+strpre;
    }
    fin->value1=value;
    fin->kvalue=*cntkq;
    fin->ktype=ktype;
    memcpy(kq+*cntkq,key,ksize);
    *cntkq=*cntkq+ksize;
    return pos;
}

/// @brief 插入一个hash表项，key如果在全局表不存在则还会插入全局表
/// @param h 要插入的hash表
/// @param hashsize hash大小
/// @param cntof 用到了哪里
/// @param head 头节点
/// @param key 键
/// @param ktype 键类型
/// @param value 要存入的值
/// @param gh 要查询的全局hash
/// @param ghsize 全局hash大小
/// @param gcntof 全局hash用到了哪里
/// @param ghead 全局hash的头节点
/// @param kq 数据存放的位置
/// @param cntkq kq用到了哪里
/// @return 插入到的桶编号(不是全局)
int addNH(hash_bucket* h,int hashsize,int* cntof,int* head,
            const void* key,int ktype,int value,
            hash_bucket* gh,int ghsize,int* gcntof,int* ghead,
            void* kq,int* cntkq){
    int gpos=queryH(gh,ghsize,ghead,key,ktype,kq);
    if(gpos==0){
        gpos=addGH(gh,ghsize,gcntof,ghead,key,ktype,0,kq,cntkq);
    }
    int ksize=getValSize(key,ktype);
    int hash=djb2_hash(key,ksize)%hashsize;
    int pos=*cntof;
    hash_bucket* fin=h+pos;
    *cntof=*cntof+1;
    fin->next_bucket=head[hash];
    head[hash]=pos;
    fin->value1=value;
    fin->kvalue=gh[gpos].kvalue;
    fin->ktype=ktype;
    return pos;
}


/// @brief 在给定hash表中查询一个key
/// @param h 要查询的hash表
/// @param hashsize 头节点数量
/// @param head 头节点
/// @param key 键
/// @param ktype key的类型
/// @param kq key存放的位置
/// @return 桶编号，or 0
int queryH(const hash_bucket* h,int hashsize,const int* head,
            const void* key,int ktype,
            const void* kq){
    int ksize=getValSize(key,ktype);
    int hash=djb2_hash(key,ksize)%hashsize;
    int i=head[hash];
    while(i!=0){
        const void* ptr=kq+h[i].kvalue;
        if(ksize == getValSize(ptr,h[i].ktype) && memcmp(key,ptr,ksize)==0){
            return i;
        }
        i=h[i].next_bucket;
    }
    return 0;
}

/*
两个hash:
1.全局hash
key=str

2.点的出边hash
key=str,value=int=边的编号
*/
