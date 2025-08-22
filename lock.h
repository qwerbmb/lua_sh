#ifndef __lock
#define __lock

#include <sys/file.h>
#include <errno.h>

/*
LOCK_SH	共享锁（读锁）
LOCK_EX	排他锁（写锁）
LOCK_UN	解锁
LOCK_NB	非阻塞模式（与上述选项组合使用）
*/

static inline int getRLock(int fd){
    // printf("Rlock %d\n",fd);
    return flock(fd,LOCK_SH);
}
static inline int getNRLock(int fd){
    // printf("NRLock %d\n",fd);
    return flock(fd,LOCK_SH|LOCK_NB);
}
static inline int getWLock(int fd){
    // printf("Wlock %d\n",fd);
    return flock(fd,LOCK_EX);
}
static inline int getNWLock(int fd){
    // printf("NWlock %d\n",fd);
    return flock(fd,LOCK_EX|LOCK_NB);
}
static inline int unlock(int fd){
    // printf("unlock %d\n",fd);
    return flock(fd,LOCK_UN);
}
//-1:非正常退出 0:没有 1:有读锁 2:有写锁
static inline int getIsLocked(int fd){
    if(getNRLock(fd)==-1){
        if(errno == EWOULDBLOCK){
            //读加不上去说明有人在写
            return 2;
        }
        return -1;
    }
    unlock(fd);
    if(getNWLock(fd)==-1){
        if(errno == EWOULDBLOCK){
            //如果没人在写，写加不上去说明有人在读
            return 1;
        }
        return -1;
    }
    unlock(fd);
    return 0;
}

#endif