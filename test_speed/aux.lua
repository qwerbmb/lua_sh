
-- dofile("data.lua")
-- local tb=GetTable()
-- sharelib.buildFile(tb,"data.bin")


-- sharelib.buildFile(htb,"huge_data.bin")

-- local stb={
--     ["a"]=1,
--     ["b"]={
--         ["c"]=2,
--         ["d"]=3
--     }
-- }
-- sharelib.buildFile(stb,"datas.bin")

-- 生成指定数量的 table 并写入 lua 文件

local count = 52428      -- 你想要的 key 数量
local filename = "hch_data.lua"  -- 输出文件名

local file = io.open(filename, "w")
file:write("return {\n")

for i = 1, count do
    -- file:write(string.format("    [%d] = \"xxxxxxxxxxxxx%d\",\n", i, i))
    file:write(string.format("    [%d] = \"%d\",\n", i, i))
end

file:write("}\n")
file:close()

print("生成完成: " .. filename)