
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "access.h"
#include "generator.h"

//参数：string path,(int type=0)
//从path读取文件，type=1会同时映射到共享内存
static int l_shareLib_Get(lua_State* L) {
    int num=lua_gettop(L);
    if(num!=1&&num!=2){
        lua_pushnil(L);
        return 1;
    }
    if(lua_type(L, 1) != LUA_TSTRING){
        lua_pushnil(L);
        return 1;
    }
    if(num == 2){
        luaL_checkinteger(L, 2);
        
    }
    else{
        lua_pushinteger(L, 0);
    }
    lua_createsdata(L);
    return 1;
}

//参数：table tb
//遍历一个table，获取节点总数，用于确定空间大小
static int getTableSize(lua_State *L) {
    int ret=1;
    int index=-2;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        int valueType = lua_type(L, -1);
        switch (valueType) {
            case LUA_TTABLE:
                //printf("(table):\n");
                ret+=getTableSize(L);
                break;
            default:
                ret++;
                break;
        }
        
        lua_pop(L, 1);
    }
    return ret;
}


//遍历table，把数据存入gptr
static void traverse_table(lua_State *L,struct bData* gptr,int* tot) {
    int pos=*tot;
    *tot=pos+1;
    int index=-2;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        //key
        //double,int,str
        if (lua_type(L, -2) == LUA_TSTRING) {
            //printf("%d %d",pos,*tot);
            add(gptr,pos,*tot,lua_tostring(L,-2),STRING);
        } 
        else if(lua_type(L, -2) ==  LUA_TNUMBER ){
            if(lua_isinteger(L,-2)){
                lua_Integer val=lua_tointeger(L,-2);
                add(gptr,pos,*tot,&val,INTEGER);
            }
            else{
                double val=lua_tonumber(L,-2);
                add(gptr,pos,*tot,&val,DOUBLE);
            }
        }
        else{
            //key只能是string,number,integer
            luaL_error(L, "key type error : %s",luaL_typename(L,-2));
        }
        
        //value
        int valueType = lua_type(L, -1);
        switch (valueType) {
            case LUA_TNUMBER:
                if(lua_isinteger(L,-1)){
                    lua_Integer val=lua_tointeger(L,-1);
                    addNode(gptr,*tot,&val,INTEGER);
                    (*tot)++;
                }
                else{
                    double val=lua_tonumber(L,-1);
                    addNode(gptr,*tot,&val,DOUBLE);
                    (*tot)++;
                }
                break;
            case LUA_TSTRING:
                addNode(gptr,*tot,lua_tostring(L,-1),STRING);
                //printf("add string : %s\n",lua_tostring(L,-1));
                (*tot)++;
                break;
            case LUA_TTABLE:
                traverse_table(L, gptr,tot);
                break;
            case LUA_TBOOLEAN:
                ;
                int val=lua_toboolean(L,-1);
                addNode(gptr,*tot,&val,BOOLEAN);
                (*tot)++;
                break;
            default:
                //不应该有其他类型，对于配置文件来说
                //userdata的话，应该转table
                luaL_error(L, "value type error : %s",luaL_typename(L,-1));
                break;
        }
        
        lua_pop(L, 1);//pop掉value，key给next获取下一个
    }
}

//参数：table tb,string path,(int type)
//table写入到path；type=0覆盖，type=1仅当文件不存在才写
//返回值：-1，失败；0，写入了文件；1，文件已存在
static int l_shareLib_buildFile(lua_State* L) {
    if(lua_type(L, 1) != LUA_TTABLE || lua_type(L,2) != LUA_TSTRING || lua_gettop(L) > 3){
        lua_pushinteger(L, -1);
        return 1;
    }
    int type;
    if(lua_isinteger(L,3)){
        type = lua_tointeger(L,3);
    }
    else{
        type = 0;
    }
    

    const char* p = lua_tostring(L,2);
    char* path = (char*) malloc(strlen(p)+1);
    strcpy(path,p);
    lua_pop(L,lua_gettop(L)-1);//留下table
    /*
    返回-1表示不正常的失败
    返回0表示build了文件
    type:
    =0 写，并且覆盖(默认行为)
    此时等待直到拿到写锁

    =1 仅当文件不存在时才写
    存在则返回1


    */
    int fd;
    
    if(type == 0){
        fd = open(path,O_RDWR | O_CREAT | O_TRUNC,0644);
        if(fd == -1){
            lua_pushinteger(L, -1);
            return 1;
        }
        if(getWLock(fd) == -1){
            lua_pushinteger(L, -1);
            return 1;
        }
    }
    else if(type == 1){
        fd = open(path,O_RDWR | O_CREAT | O_EXCL,0644);
        if(fd == -1){
            if(errno == EEXIST){
                lua_pushinteger(L, 1);
            }
            else{
                lua_pushinteger(L, -1);
            }
            
            return 1;
        }
        if(getWLock(fd) == -1){
            lua_pushinteger(L, -1);
            return 1;
        }
    }
    else{
        lua_pushinteger(L, -1);
        return 1;
    }
    //成功打开了文件，开始build
    int n=getTableSize(L);
    struct bData* gptr=initPointer(n);
    int tot=0;
    traverse_table(L,gptr,&tot);
    dfs(gptr,0);
    writeFile(gptr,fd);
    unlock(fd);
    close(fd);
    lua_pushinteger(L, 0);
    return 1;
}

//用于连接路径
//path..'/'..name
static char* pathLink(const char* path,const char* name){
    char* newpath=(char*) malloc(strlen(path)+strlen(name)+2);
    // -2: / \0
    strcpy(newpath,path);
    strcat(newpath,"/");
    strcat(newpath,name);
    return newpath;
}


//检测是否是lua文件
//也可以去掉前缀/后缀，或者替换
//但是dofile不需要这些所以没有参数
static char* pathTr(const char* path){

    if(strcmp(path+strlen(path)-4,".lua") != 0){
        //printf("path: %s\n",path);
        return NULL;
    }
    /*
    path=./abc/def/ghj.lua
    */    
    int plen=0;
    int hlen=0;
    char* newpath = (char*)malloc(strlen(path)-hlen-plen+1);
    memcpy(newpath,path+plen,strlen(path)-hlen-plen);
    newpath[strlen(path)-plen-hlen] = '\0';


    // for(int i = 0;i < strlen(newpath);i++){
    //     if(newpath[i] == '/'){
    //         newpath[i] = '.';
    //     }
    // }
    return newpath;
}

//遍历目录，key=value=路径
static void traverse_dir(lua_State *L,const char* path) {
    DIR* dir = opendir(path);
    if(dir == NULL) {
        printf("Error opening directory: %s\n", path);
        return;
    }
    struct dirent* d;
    while ((d = readdir(dir)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }
        char* newpath = pathLink(path,d->d_name);
        struct stat st;
        int result = stat(newpath,&st);
        if(result != 0){
            printf("Error stat: %s\n", newpath);
            continue;
        }
        if(S_ISDIR(st.st_mode)){
            //是目录，递归下去
            traverse_dir(L,newpath);
        }
        else if (S_ISREG(st.st_mode)){
            //是文件，添加到table
            char* ret = pathTr(newpath);
            if(ret != NULL){
                lua_pushstring(L,ret);
                lua_setfield(L,-2,newpath);
            }
            free(ret);
        }
        free(newpath);
    }
}

//参数:string path
//遍历目录，返回table={key=value=目录下的lua文件路径}
static int l_shareLib_buildDir(lua_State* L) {
    //lua的require
    if(lua_type(L,-1) != LUA_TSTRING){
        lua_pushnil(L);
        return 1;
    }
    const char* p = lua_tostring(L,-1);
    lua_newtable(L);
    //返回格式：{path=path,...}
    traverse_dir(L,p);
    return 1;
}

//参数：string path
//检测一个文件上是否有锁
//返回：
// nil=打开文件失败；-1=获取锁失败 
// 0=没锁；1=有读锁；2=有写锁
static int l_shareLib_getLock(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int fd = open(path, O_RDWR);
    if(fd == -1){
        lua_pushnil(L);
    }
    else{
        lua_pushinteger(L, getIsLocked(fd));
    }
    close(fd);
    return 1;
}

//参数：sharedata sd
//返回sd对应的table
static int l_shareTB_getData(lua_State* L){
    if(!lua_issharedata(L,1)){
        luaL_error(L,"not a sharedata");
    }
    lua_sdata2table(L);
    return 1;
}



static const luaL_Reg shareLib_funcs[] = {
    {"Get", l_shareLib_Get},
    {"buildFile", l_shareLib_buildFile},
    {"buildDir", l_shareLib_buildDir},
    {"getLock", l_shareLib_getLock},
    {"getData", l_shareTB_getData},
    {NULL, NULL}
};


LUAMOD_API int luaopen_shareLib (lua_State *L) {
    luaL_newlib(L, shareLib_funcs);
    return 1;
}

