-- dofile("data.lua")
-- local tb=GetTable()
-- local path="./data.bin"
-- sharelib.buildFile(tb,path)
-- local sd=sharelib.Get(path,1)

local path="./data.bin"
local sd=sharelib.Get(path,1)

local function traverse(t1)
    for k,v in pairs(t1) do
        if type(v)=="table" or type(v)=="sharedata" then
            traverse(v)
        else
            assert(t1[k]==v)
        end
    end
end

local function ptime(f,...)
    local ti=500
    local t1=os.clock()
    for i=1,ti do
        f(...)
    end
    local t2=os.clock()
    local ret=(t2-t1)/ti
    return ret
end

local t1=ptime(traverse,sd)


local ttb=sharelib.getData(sd)

local t2=ptime(traverse,ttb)
print("string都在shm中时，sharedata：",t1)

print("string都在shm中时，table：",t2)