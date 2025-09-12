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

local function ctime(action,...)
    local t1=os.clock()
    local num=1000
    for i=1,num do
        action(...)
    end
    
    local t2=os.clock()
    return (t2-t1)/num
end

local function ptime(action,...)
    local r1=ctime(action,...)
    print(r1)
    return 
end


ptime(traverse,tb)