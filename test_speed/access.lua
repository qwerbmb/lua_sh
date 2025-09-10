dofile("data.lua")
local tb=GetTable()


local function traverse(t1)
    for k,v in pairs(t1) do
        if type(v)=="table" or type(v)=="sharedata" then
            traverse(v)
        else
            assert(t1[k]==v)
        end
    end
end

local function ctime(action,...)
    local t1=os.clock()
    local ret={action(...)}
    local t2=os.clock()
    return t2-t1,table.unpack(ret)
end

local function ptime(action,...)
    local r1,ret=ctime(action,...)
    print(r1)
    return table.unpack(ret)
end

local path="./data.bin"

ptime(sharelib.buildFile,tb,path)
local sd=ptime(sharelib.Get,path)
ptime(traverse,sd)
ptime(traverse,tb)
