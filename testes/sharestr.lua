
local tb = {}
for i=1,3 do
    tb["key_"..i]="value_"..i
end
local tbin={}
for i=1,3 do
    tbin["keyi_"..i]="valuei_"..i
end
tb["inner"]=tbin

local path="./data3.bin"
sharelib.buildFile(tb,path)
local sd=sharelib.Get(path,1)
for k,v in pairs(sd) do
    print(k,v,sd[k])
end

