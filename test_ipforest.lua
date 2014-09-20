#!/usr/bin/env luajit

require "lunit"

module( "test_ipforest", package.seeall, lunit.testcase )

local ipforest

function setup()
  ipforest = require("ipforest")
end

function teardown()
  ipforest = nil
end

function test_load()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_false(ipforest.load("whitelist", "./nonexist.txt"))
end

function test_match()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.match("blacklist", "127.0.0.1"))
  assert_true(ipforest.match("blacklist", "127.0.0.2"))
  assert_true(ipforest.match("blacklist", "127.0.0.3"))
  assert_true(ipforest.match("blacklist", "127.0.0.255"))
  assert_true(ipforest.match("blacklist", "128.0.0.255"))
  assert_true(ipforest.match("blacklist", "192.168.1.3"))
  assert_true(ipforest.match("blacklist", "192.168.1.2"))
  assert_true(ipforest.match("blacklist", "222.168.1.2"))
  assert_true(ipforest.match("blacklist", "10.127.1.2"))
  assert_true(ipforest.match("blacklist", "10.127.0.0"))
  assert_true(ipforest.match("blacklist", "13.13.13.13"))

  assert_false(ipforest.match("blacklist", "1.2.3.2"))
  assert_false(ipforest.match("blacklist", "1.2.3.3"))
  assert_true(ipforest.match("blacklist", "1.2.3.4"))
  assert_true(ipforest.match("blacklist", "1.2.3.5"))

  assert_true(ipforest.match("blacklist", "9.0.3.187"))
  assert_true(ipforest.match("blacklist", "9.0.3.188"))
  assert_false(ipforest.match("blacklist", "9.0.3.189"))
  assert_false(ipforest.match("blacklist", "9.0.3.190"))

  assert_false(ipforest.match("blacklist", "10.128.1.2"))

  assert_false(ipforest.match("blacklist", "11.11.11.12"))
  assert_true(ipforest.match("blacklist", "11.11.11.13"))
  assert_true(ipforest.match("blacklist", "11.11.11.14"))
  assert_true(ipforest.match("blacklist", "11.11.11.127"))
  assert_true(ipforest.match("blacklist", "11.11.11.128"))
  assert_false(ipforest.match("blacklist", "11.11.11.129"))

  assert_false(ipforest.match("blacklist", "14.14.14.13"))
  assert_true(ipforest.match("blacklist", "14.14.14.14"))
  assert_true(ipforest.match("blacklist", "14.14.14.15"))
  assert_true(ipforest.match("blacklist", "14.14.14.16"))
  assert_true(ipforest.match("blacklist", "14.14.14.17"))
  assert_true(ipforest.match("blacklist", "14.14.14.18"))
  assert_true(ipforest.match("blacklist", "14.14.14.19"))
  assert_true(ipforest.match("blacklist", "14.14.14.20"))
  assert_false(ipforest.match("blacklist", "14.14.14.21"))


  assert_false(ipforest.match("whitelist", "127.0.0.1"))
end

function test_append()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.append("blacklist", "8.8.8.22/24"));
  assert_true(ipforest.match("blacklist", "8.8.8.23"));
end

function test_reset()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.append("blacklist", "8.8.8.8/24"));
  assert_true(ipforest.reset("blacklist"))
  assert_false(ipforest.match("blacklist", "13.13.13.13"))
  assert_false(ipforest.match("blacklist", "8.8.8.9"))
end

function test_has()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.has("blacklist"))
  assert_false(ipforest.has("whitelist"))
end

function test_compact()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.compact("blacklist"))
  assert_false(ipforest.compact("whitelist"))
end

function test_free()
  assert_true(ipforest.load("blacklist", "./blacklist.txt"))
  assert_true(ipforest.free("blacklist"))
  assert_false(ipforest.free("whitelist"))
end
