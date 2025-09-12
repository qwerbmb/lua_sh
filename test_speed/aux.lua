
-- dofile("data.lua")
-- local tb=GetTable()
-- sharelib.buildFile(tb,"data.bin")

local htb=dofile("huge_data.lua")
sharelib.buildFile(htb,"huge_data.bin")

-- local stb={
--     ["a"]=1,
--     ["b"]={
--         ["c"]=2,
--         ["d"]=3
--     }
-- }
-- sharelib.buildFile(stb,"datas.bin")