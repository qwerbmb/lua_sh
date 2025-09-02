-- dofile("data.lua")
-- local tb=GetTable()
local path="./data.bin"
-- sharelib.buildFile(tb,path)
local sd=sharelib.Get(path,1)
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
local function iscont(t)
    return type(t)=="table" or type(t)=="sharedata"
end
local function tcmp(t1,t2)
    -- print(t1,t2)
    for k,v in pairs(t1) do
        if iscont(t1[k]) and iscont(v) and iscont(t2[k])then
            --print(k)
            local st=tcmp(v,t2[k])
            if st~="" then
                print(k,t1[k],t2[k],v)
                return k.."."..st
            end   
        elseif not equ(t1[k],t2[k],v) then
            print(k,t1[k],t2[k],v)
            return k
        end
        -- print("???")
    end
    return ""
end
local function cmp(t1,t2)
    local st=tcmp(t1,t2)
    if st=="" then
        -- print("success")
        return true
    end
    print(st)
    return false
end

local function ptime(f,...)
    local avg=0
    local ti=100
    for i=1,ti do
        local t1=os.clock()
        local r=f(...)
        local t2=os.clock()
        avg=avg+(t2-t1)
    end

    print(avg/ti)
    return r
end
dofile("data.lua")
local tb=GetTable()
ptime(cmp,tb,sd)
ptime(cmp,tb,tb)