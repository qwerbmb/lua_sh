local path="./huge_data.bin"
local sd=sharelib.Get(path,1)

local function traverse(t1)
    for k,v in pairs(t1) do
        if type(v)=="table" or type(v)=="sharedata" then
            traverse(v)
        else
            assert(t1[k]==v)
        end
    end
end
traverse(sd)
while(true) do
    
    -- collectgarbage("collect")
end


