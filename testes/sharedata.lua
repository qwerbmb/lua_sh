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


--测试访问
assert(sd.Attrs.Content.BothOil.TalentETips == [[T_Expedition_AttritETips28]],"access fail")


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
print(m1,m2,m3,"期望：t1>t2==t3>>t4")
assert(sharelib.getLock(path)==0,"lock4 fail")
