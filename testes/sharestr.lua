
local tb = {}
for i=1,10 do
    tb["key_"..i]="value_"..i
end
local path="./data3.bin"
sharelib.buildFile(tb,path)
local sd=sharelib.Get(path,1)
print(sharelib.getLock(path))
print("----------------")
for k,v in pairs(sd) do
    print(k,v,k=="key_7")
end
sd=nil
collectgarbage("collect")

-- print("hello")

-- local a=0
-- local function outer()
--     local a=a
--     a=1
--     local function test()
--         print("test"..a)
--     end
--     return test
-- end
-- local t=outer()
-- print(a)
-- t()