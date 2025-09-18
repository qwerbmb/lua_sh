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




#endif