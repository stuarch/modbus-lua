# modbus-lua
Lua bindings for libmodbus.

## Installation
```
$ sudo apt-get install liblua5.1-dev lua5.1 libmodbus-dev
$ make
$ sudo make install
```

## Examples
```
modbus=require("modbus")
--<host>,port
dev=modbus.new("192.168.1.1",502)

dev:connect()
dev:slave(2)

addr={}
for i in 1,6 do
    table.insert(addr, i)
end

--query all the value of the address in the list.
value_table=dev:read(addr)

for k,v in pairs(value_table) do
    print(k, v)
end

--query address 7 and 8
print(dev:mread(7,2))

--write value 0 in address 9
dev:write(9,0)

dev:close()
```
