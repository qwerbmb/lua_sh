dofile("data.lua")
local tb=GetTable() -- 需要转换并共享的table

local path="./data.bin"
sharelib.buildFile(tb,path) -- 把一个table转换为文件
local sd=sharelib.Get(path) -- 从一个文件读取sharedata

print(sd[1])
for k,v in pairs(sd) do
    print(k,v) -- 能够像table一样，通过index和pairs访问
end

能够把目录下格式类似./data.lua的所有lua文件里的table数据转为一个以路径为key的table，并存储到文件。

例如，当前目录下有：
./testDirSub/test111.lua
./test222.lua
./test_dir.lua(自身)
会生成如下table：
{
    ['./testDirSub/test111.lua'] = {数据1}
    ['./test222.lua'] = {数据2}
}