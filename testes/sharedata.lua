-- for k,v in pairs(_G) do
--     print(k,v)
-- end

dofile("data.lua")
local tb=GetTable()

-- local ast=function(...)
--     for k,v in pairs({...}) do
--         print(k,v)
--     end
--     assert(...)
-- end
-- assert=ast

local path="./data.bin"
--测试build
assert(sharelib.buildFile(tb,path)==0,"build1 fail")
assert(sharelib.buildFile(tb,path,1)==1,"build2 fail")
assert(sharelib.buildFile(tb,path,0)==0,"build3 fail")

--测试get
local sd=sharelib.Get(path,1)
assert(sd~=nil,"get fail")


--测试index和pairs
local function equ(...)
    local arg={...}
    local a1=arg[1]
    for k,v in pairs(arg) do
        if a1~=v then
            return false
        end
    end
    return true
end
local function tcmp(t1,t2)
    for k,v in pairs(t1) do
        if type(v)=="table" or type(v)=="sharedata" then
            local st=tcmp(v,t2[k])
            if st~="" then
                --print(k,t1[k],t2[k],v)
                return k.."."..st
            end   
        elseif not equ(t1[k],t2[k],v) then
            print(k,t1[k],t2[k],v)
            return k
        end
    end
    return ""
end
local function cmp(t1,t2)
    local st=tcmp(t1,t2)
    if st=="" then
        return true
    end
    return false
end
assert(cmp(sd,tb),"access fail")
assert(cmp(tb,sd),"access2 fail")
local sdtb=sharelib.getData(sd)
assert(cmp(sdtb,sd),"access3 fail")
assert(cmp(sd,sdtb),"access4 fail")


--测试gc和lock
assert(sharelib.getLock(path)==1,"lock1 fail")
local sd2=sd.Attrs

collectgarbage("collect")
local m1=collectgarbage("count")

sd=nil
collectgarbage("collect")
local m2=collectgarbage("count")

assert(sharelib.getLock(path)==1,"lock2 fail")
sd2=nil
collectgarbage("collect")
local m3=collectgarbage("count")
print(m1,m2,m3,"期望：m1==m2>>m3")
assert(sharelib.getLock(path)==0,"lock4 fail")
