#ifndef __shareLib
#define __shareLib

#include "access.h"
#include "lock.h"
#include "lobject.h"
#include "lmem.h"
#include "lgc.h"
#include "lua.h"


//free一个sharedata
static inline void luaR_free(lua_State *L, sharedata* sd){
    countSA(L,sd->acs,-1);
    luaM_free(L, sd);
}

//创建一个空的sharedata
static inline sharedata* luaR_new(lua_State *L){
    GCObject* o = luaC_newobj(L, LUA_VSHAREDATA, sizeof(sharedata));
    sharedata* sd = gco2sd(o);
    sd->acs = NULL;
    sd->pos = 0;
    return sd;
}

//创建一个sharedata
static inline sharedata* luaR_create(lua_State *L, accessor* acs,int pos){
    sharedata* sd2 = luaR_new(L);
    if(acs!=NULL){
        sd2->acs=acs;
        sd2->pos=pos;
        countSA(L,sd2->acs,1);
    }
    return sd2;
}

//以integer为key，返回出边编号
static inline int luaR_getEdgeI(lua_State *L, sharedata* sd, lua_Integer key){
    if(L){

    }
    return findEdgeA(sd->acs,sd->pos,&key,INTEGER);
}

//以double为key，返回出边编号
static inline int luaR_getEdgeD(lua_State *L, sharedata* sd, double key){
    if(L){
        
    }
    return findEdgeA(sd->acs,sd->pos,&key,DOUBLE);
}

//以string为key，返回出边编号
static inline int luaR_getEdgeS(lua_State *L, sharedata* sd, const char* key){
    if(L){
        
    }
    return findEdgeA(sd->acs,sd->pos,key,STRING);
}

//从出边获取子节点
static inline int luaR_getChild(lua_State *L, sharedata* sd, int edge){
    if(L){
        
    }
    return getEChildA(sd->acs,sd->pos,edge);
}

typedef struct {
    accessor* acs;
    int pos;
    int eNum;
}sdata_pstate;


//把一个sd转table
static inline void luaR_sdata2table(lua_State *L, sharedata* sd){
    if(L){

    }
    build_full_tableA(L,sd->acs,sd->pos);
}


#endif