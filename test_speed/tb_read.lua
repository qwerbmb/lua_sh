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

while(true) do
    traverse(tb)
end
