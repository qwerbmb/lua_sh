
从`Lua5.4.3` fork而来。  
增加了`sharedata`到基本数据结构中。  
`sharedata`可以存储在共享内存中，只读，其他使用方式和`table`相同，实现了 `__index` 和 `__pairs`  

测试：`./testes/sharedata.lua`  

```
sharelib.buildFile(table:tb,string:path,integer:mode = 0)
把tb存储到路径path处的文件中。
mode = 0 ：覆盖原本文件
mode = 1 ：如果文件存在，则不做操作

返回值：
-1，失败
0，成功写入文件
1，文件已存在





sharelib.Get(string:path,integer:mode = 0)
从路径path加载一个上述文件到sharedata。
mode = 0: 不使用共享内存
mode = 1: 使用共享内存
共享内存不需要手动释放，由gc管理。

返回值：
一个sharedata对象，失败返回nil





sharelib.getLock(string:path)
返回路径path的文件当前锁的状态。

返回值：
nil = 打开文件失败
-1 = 获取锁失败 
0 = 没有锁
1 = 有读锁
2 = 有写锁





sharelib.getData(sharedata:data)
从一个sharedata对象生成一个table对象。

返回值：
一个table对象
```

