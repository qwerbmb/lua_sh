
# 总体设计思路

把table数据转化为一个树形结构，结构内的访问操作均通过偏移量进行。
将这个树形结构存储到文件，然后映射到共享内存给多进程访问。


# structure.c / structure.h

定义了结构所用到的一些基本结构体和函数，包括节点、边、数据类型(INTEGER,DOUBLE,STRING,BOOLEAN)、hash函数。

所有的数据都以char*存储，要通过偏移量获取数据还需要知道数据所占字节数。

# generator.c / generator.h
定义了把table转化为树形结构的函数。

使用一个bData结构体存储建立过程中的数据，具体可查看定义。

所有的实际数据都连续存放在char* sData中，存储时需要记录数据存储位置相对起始位置的偏移量，以及数据类型。由数据类型可以得到数据所占字节数。

边通过链表存储：


#### 树形结构如下：

以table自身为根节点；每条出边存储对应的key(的偏移量和类型，下同)，指向的子节点存储value。
所有的非叶子节点都是table，叶子节点存储具体数据。



#### 建立方式：

对于table的每一对key-value，给value建立一个节点并连一条指向它的边，边上存储key。如果value是table，则递归遍历；否则在该节点存储value。

hash表有两种：全局hash表和每个节点单独的hash表。

对于每个节点，将其出边的key加入该节点的hash表。
同时，对于所有的key和value，处理时会检测全局hash表中是否已有该值，没有则加入全局hash表中。


对于STRING类型，存储时实际存储进去的是一个lua的TString对象，但偏移量指向的仍然是字符串的起始位置。参与hash计算的也只有字符串部分。
可以通过#define strpre offsetof(TString, contents)这个宏，用字符串的起始地址-strpre，从一个字符串获取TString对象。
这个TString对象的hash存储的不再是实际的hash值，而是在hash表中的节点编号。作用后述。


全部处理完成后，把bData中的指针指向的数据复制到连续的内存中，在这片内存的起始位置存储一个config结构体用于记录每种区域的起始位置。把这片内存写入文件。

# access.c / access.h

定义了accessor结构体，用于单个lua虚拟机访问上述生成的文件。
定义了一些访问函数。通过accessor指针+当前节点编号pos，能够在树形结构上移动到新的pos，并查询相关数据。

创建accessor时，可以选择是否把文件映射到共享内存；然后把config对象中的偏移量重新计算为指针(即还原出bData)，并记录path，文件描述符等一些额外的信息到结构体中。
具体定义可以查看access.h。

每当创建一个accessor时，会插入到global_state中的accessor* acslist，表示当前进程连接到的所有共享内存。
然后计算该acs内所有TString对象在当前进程的hash值，存储在uint* hashval中。
当需要获取一个在shm中的Tstring类型对象(标记isShare=1)的hash值时，需要遍历acslist找到它的所属acs，通过hashval[hash]获取在当前进程的hash值。
这么做的原因是不同lua_state的seed是不同的，从而计算出的hash值不同，于是这个hash值不能存储在TString对象内。

每当试图生成一个新的短字符串时，除了在已有的stringtable检查之外，还需要遍历acslist，检查是否有内容相同的TString对象。当试图从acs中读出一个字符串时也是如此。
这样保证了lua原有的，直接靠地址判断短字符串相等的机制仍然能够正常工作。

accessor对象不在allgc链表上，在调用lua_close时才被释放。



# lsharedata.h

定义了创建和gc一个sharedata的函数。

# lock.h

实现了一些文件锁相关的内容。

虽然设计上shm本身是只读的，但在建立/覆盖文件时仍然需要检查锁。

# lobject.h

添加了sharedata这个基本数据类型以及相关的宏，枚举变量是LUA_TSHAREDATA = 9。

每个sharedata除了继承GCObject之外，还记录一个accessor指针、一个pos、一个用于pairs记录的变量。
通过acs和pos，能够在acs上查询数据(上述access.c实现的接口)。
当从sharedata上index到一个非叶子节点时，会返回一个新的sharedata。

sharedata对象在allgc链表上，被gc时不会释放自身的acs。

# lapi.c / lapi.h

添加了sharedata相关的接口，包括index，pairs，以及一些栈上操作。

其中index和pairs没有通过元方法实现，而是像原生table那样单独处理了。

# lsharelib.c

定义了注册到lua的c函数。
包括从文件读取acs并创建sharedata，把一个table转换为文件，获取锁的状态，sharedata转换为table等。

基本上是对上述access.h / generator.h中函数的封装。






方案：
acs再存一个sharedata* sdlist
每次试图生成新的sd时改为访问sdlist取指针
这样sd不需要gc，但是没法放在共享内存中
而且这一堆sd有大量的重复结构acs
或者可以改为
acs int int ...
然后指针指向int，地址-int*sizeof(int)-sizeof(acs*)就能得到acs
总之可以先试一下sdlist，工程量应该很小

