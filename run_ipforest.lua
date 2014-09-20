#!/usr/bin/env luajit

local ipforest = require("ipforest")
ipforest.load("blacklist", "./blacklist.txt")
ipforest.match("blacklist", "127.0.0.1")
ipforest.match("blacklist", "127.0.0.2")
ipforest.match("blacklist", "127.0.0.3")
print(ipforest.match("blacklist", "127.0.0.255"))
ipforest.match("whitelist", "127.0.0.1")
