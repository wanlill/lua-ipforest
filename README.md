# Lua IP FOREST - COLLECTION OF RADIX IP TREES FOR LUA # 

## How to Build ##
make LUA_INCLUDE_DIR=/usr/include/luajit-2.0

## How to Install ##
make LUA_INCLUDE_DIR=/usr/include/luajit-2.0 install

## How to Test ##
make LUA_INCLUDE_DIR=/usr/include/luajit-2.0 test

## How to Use ##
local ipforest = ipforest.load("blacklist", "./blacklist.txt"))
print(ipforest.match("blacklist", "127.0.0.1")) -- yield true/false
print(ipforest.match("blacklist", "127.0.0.2")) -- yield true/false

## Format Supported ##
<pre>
#####################################################################
# be a wise man, no leading and trailing useless whitespace allowed #
#####################################################################

# host
13.13.13.13
127.0.0.1
127.0.0.2
127.0.0.3
192.168.1.3

# network
127.0.0.0/255.0.0.0
128.0.0.0/255.0.0.0
10.0.0.0/255.255.255.0
128.0.0.0/128.0.0.0
10.0.0.0/9

# ip range
1.2.3.4-9.0.3.188
11.11.11.13-128
14.14.14.14-20
</pre>
