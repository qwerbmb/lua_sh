local tl=0
local function pmem()
    local s1=collectgarbage("count")
    print(s1-tl)
    tl=s1
end

pmem()
local tb=dofile("hch_data.lua")

-- dofile("data.lua")
-- local tb=GetTable()
collectgarbage("collect")
pmem()
tb=nil
collectgarbage("collect")

pmem()
print(collectgarbage("count"))
-- while(true) do
    
--     -- collectgarbage("collect")
-- end
