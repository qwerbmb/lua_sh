dofile("data.lua")
local tb=GetTable()


local function traverse(t1)
    for k,v in pairs(t1) do
        if type(v)=="table" or type(v)=="sharedata" then
            traverse(v)
        else
            local vv=v
            local tv=type(v)
        end
    end
end

local function ctime(action,...)
    local t1=os.clock()
    local ret=action(...)
    local t2=os.clock()
    return t2-t1,ret
end

local function ptime(action,...)
    local r1,r2=ctime(action,...)
    print(r1)
    return r2
end

local path="./data.bin"

ptime(sharelib.buildFile,tb,path)
local sd=ptime(sharelib.Get,path)
ptime(traverse,sd)
ptime(traverse,tb)
