--[[ @file tests.lua

A portable PICO-8 emulator written in C.

Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
SPDX-License-Identifier: MIT

--]]

local failed_tests = 0

-- Test framework.
function assert_equal(actual, expected, test_name)
    if actual == expected then
        log(test_name .. " passed")
    else
        log(test_name .. " failed: expected " .. string.format("0x%04x", expected) .. " but got " .. string.format("0x%04x", actual))
        failed_tests += 1
    end
end

function assert_true(expression, test_name)
    if expression == true then
        log(test_name .. " passed")
    else
        log(test_name .. " failed")
        failed_tests += 1
    end
end

-- Arithmetic operations.
function test_arithmetic_operations()
    assert_equal(1 + 2, 3, "1 + 2")
    assert_equal(5 - 3, 2, "5 - 3")
    assert_equal(4 * 2, 8, "4 * 2")
    assert_equal(9 / 3, 3, "9 / 3")
    assert_equal(10 % 3, 1, "10 % 3")
    assert_equal(2 ^ 3, 8, "2 ^ 3")

    local n = 10
    log("n = 10")

    n += 15
    assert_equal(n, 25, "n += 15")

    n -= 7
    assert_equal(n, 18, "n -= 7")

    n *= 2
    assert_equal(n, 36, "n *= 2")

    n /= 3
    assert_equal(n, 12, "n /= 3")

    n %= 5
    assert_equal(n, 2, "n %= 5")
end

-- Relational operations.
function test_relational_operations()
    assert_true(3 == 3, "3 == 3")
    assert_true(3 ~= 4, "3 ~= 4")
    assert_true(3 < 4, "3 < 4")
    assert_true(4 > 3, "4 > 3")
    assert_true(3 <= 3, "3 <= 3")
    assert_true(4 >= 3, "4 >= 3")
end

-- Logical operations.
function test_logical_operations()
    assert_true(true and true, "true and true")
    assert_true(false or true, "false or true")
    assert_true(not false, "not false")
end

-- Control flow.
function test_control_flow()
    local x = 5
    if x == 5 then
        assert_true(true, "if-then")
    else
        assert_true(false, "if-then failed")
    end

    local count = 0
    while count < 3 do
        count = count + 1
    end
    assert_equal(count, 3, "while loop")

    count = 0
    repeat
        count = count + 1
    until count == 3
    assert_equal(count, 3, "repeat-until loop")

    local sum = 0
    for i = 1, 3 do
        sum = sum + i
    end
    assert_equal(sum, 6, "for loop")
end

function test_tables()
    local tbl = {0, 1, 1, 2, 3, 5, 8, 13}
    log("tbl = {0, 1, 1, 2, 3, 5, 8, 13}")
    --assert_equal(tbl[1], 0 "tbl[1]")
end

-- PICO-8 API.
function test_pico8_api()
    assert_equal(abs(5.000),  5.00,  "abs(5.000)")
    assert_equal(abs(-5.00),  5.00,  "abs(-5.00)")
    assert_equal(abs(0.000),  0.00,  "abs(0.000)")
    assert_equal(abs(12345.00), 12345.0, "abs(12345.00)")
    assert_equal(abs(-12345.0), 12345.0, "abs(-12345.0)")
    assert_equal(abs(0x8000), 0x7fff.ffff, "abs(0x8000)")

    assert_equal(atan2(1.00,  0.0), 0.000, "atan2(1.00,  0.0)")
    assert_equal(atan2(1.00,  1.0), 0.875, "atan2(1.00,  1.0)")
    assert_equal(atan2(0.00,  1.0), 0.750, "atan2(0.00,  1.0)")
    assert_equal(atan2(-1.0,  1.0), 0.625, "atan2(-1.0,  1.0)")
    assert_equal(atan2(-1.0,  0.0), 0.500, "atan2(-1.0,  0.0)")
    assert_equal(atan2(-1.0, -1.0), 0.375, "atan2(-1.0, -1.0)")
    assert_equal(atan2(0.00, -1.0), 0.250, "atan2(0.0,  -1.0)")
    assert_equal(atan2(1.00, -1.0), 0.125, "atan2(1.0,  -1.0)")
    assert_equal(atan2(0.00,  0.0), 0.250, "atan2(0.0,   0.0)")
    assert_equal(atan2(99.0, 99.0), 0.875, "atan2(99.0, 99.0)")

    assert_equal(band(0x1010, 0x1100), 0x1000, "band(0x1010, 0x1100)")
    assert_equal(band(0x0101, 0x1010), 0x0000, "band(0x0101, 0x1010)")
    assert_equal(band(0x1010, 0x1010), 0x1010, "band(0x1010, 0x1010)")
    assert_equal(band(0x1100, 0x1100), 0x1100, "band(0x1100, 0x1100)")

    assert_equal(bor(0x1010, 0x1100), 0x1110, "bor(0x1010, 0x1100)")
    assert_equal(bor(0x0101, 0x1010), 0x1111, "bor(0x0101, 0x1010)")
    assert_equal(bor(0x1010, 0x1010), 0x1010, "bor(0x1010, 0x1010)")
    assert_equal(bor(0x1100, 0x1100), 0x1100, "bor(0x1100, 0x1100)")

    assert_equal(bxor(0x1010, 0x1100), 0x0110, "bxor(0x1010, 0x1100)")
    assert_equal(bxor(0x0101, 0x1010), 0x1111, "bxor(0x0101, 0x1010)")
    assert_equal(bxor(0x1010, 0x1010), 0x0000, "bxor(0x1010, 0x1010)")
    assert_equal(bxor(0x1100, 0x1100), 0x0000, "bxor(0x1100, 0x1100)")

    assert_equal(bnot(0xff00), 0x00ff.ffff, "bnot(0xff00)")
    assert_equal(bnot(0x00ff), 0xff00.ffff, "bnot(0x00ff)")
    assert_equal(bnot(0x0000), 0xffff.ffff, "bnot(0x0000)")
    assert_equal(bnot(0xffff), 0x0000.ffff, "bnot(0xffff)")

    assert_equal(bnot(0x0000.ff00), 0xffff.00ff, "bnot(0x0000.ff00)")
    assert_equal(bnot(0x0000.00ff), 0xffff.ff00, "bnot(0x0000.00ff)")
    assert_equal(bnot(0xf0f0.f0f0), 0x0f0f.0f0f, "bnot(0xf0f0.f0f0)")
    assert_equal(bnot(0x0000.ffff), 0xffff.0000, "bnot(0x0000.ffff)")

    assert_equal(ceil(1.1),   2, "ceil(1.1)")
    assert_equal(ceil(-1.9), -1, "ceil(-1.9)")
    assert_equal(ceil(3),     3, "ceil(3)")

    assert_equal(cos(0),     0x0001.0000, "cos(0)")
    assert_equal(cos(0.125), 0x0000.b505, "cos(0.125)")
    assert_equal(cos(0.25),  0x0000.0000, "cos(0.25)")
    assert_equal(cos(0.375), 0xffff.4afb, "cos(0.375)")
    assert_equal(cos(0.5),   0xffff.0000, "cos(0.5)")
    assert_equal(cos(0.625), 0xffff.4afb, "cos(0.625)")
    assert_equal(cos(0.75),  0x0000.0000, "cos(0.75)")
    assert_equal(cos(0.875), 0x0000.b505, "cos(0.875)")
    assert_equal(cos(1), 0x0001.0000, "cos(1)")

    assert_equal(flr(5.9000),  5.0, "flr(5.9000)")
    assert_equal(flr(-5.200), -6.0, "flr(-5.200)")
    assert_equal(flr(7.0000),  7.0, "flr(7.0000)")
    assert_equal(flr(-7.000), -7.0, "flr(-7.000)")

    assert_equal(max(1, 2), 2, "max(1, 2)")
    assert_equal(max(2, 1), 2, "max(2, 1)")

    assert_equal(mid(8.00,  2.0,  4.0),  4.0, "mid(8.00,  2.0,  4.0)")
    assert_equal(mid(-3.5, -3.4, -3.6), -3.5, "mid(-3.5, -3.4, -3.6)")
    assert_equal(mid(6.00,  6.0,  8.0),  6.0, "mid(6.00,  6.0,  8.0)")

    assert_equal(min(1, 2), 1, "min(1, 2)")
    assert_equal(min(2, 1), 1, "min(2, 1)")

    assert_equal(rotl(8.000, 3.00), 64.0000, "rotl(8.000, 3.00)")
    assert_equal(rotl(0.125, 3.00),  1.0000, "rotl(0.125, 3.00)")
    assert_equal(rotl(-4096, 12.0),  0.0586, "rotl(-4096, 12.0)")
    assert_equal(rotl(1.000, 3.00),  8.0000, "rotl(1.000, 3.00)")

    assert_equal(rotr(64.00,  3.0),  8.000, "rotr(64.00,  3.0)")
    assert_equal(rotr(1.000,  3.0),  0.125, "rotr(1.000,  3.0)")
    assert_equal(rotr(-4096, 12.0), 15.000, "rotr(-4096, 12.0)")
    assert_equal(rotr(0x8000, 1.0), 0x4000, "rotr(0x8000, 1.0)")

    assert_equal(lshr(0b1010, 1), 0x0005.0000, "lshr(0b1010, 1)")
    assert_equal(lshr(0b1010, 2), 0x0002.8000, "lshr(0b1010, 2)")
    assert_equal(lshr(0b1010, 3), 0x0001.4000, "lshr(0b1010, 3)")
    assert_equal(lshr(0b1010, 4), 0x0000.a000, "lshr(0b1010, 4)")

    assert_equal(shl(0b1010, 1), 0b10100, "shl(0b1010, 1)")
    assert_equal(shl(0b1010, 2), 0b101000, "shl(0b1010, 2)")
    assert_equal(shl(0b1010, 3), 0b1010000, "shl(0b1010, 3)")
    assert_equal(shl(0b1010, 4), 0b10100000, "shl(0b1010, 4)")

    assert_equal(shr(0b1010, 1), 0x0005.0000, "shr(0b1010, 1)")
    assert_equal(shr(0b1010, 2), 0x0002.8000, "shr(0b1010, 2)")
    assert_equal(shr(0b1010, 3), 0x0001.4000, "shr(0b1010, 3)")
    assert_equal(shr(0b1010, 4), 0x0000.a000, "shr(0b1010, 4)")

    assert_equal(sin(0.000), 0x0000.0000, "sin(0.000)")
    assert_equal(sin(0.125), 0xffff.4afb, "sin(0.125)")
    assert_equal(sin(0.250), 0xffff.0000, "sin(0.250)")
    assert_equal(sin(0.375), 0xffff.4afb, "sin(0.375)")
    assert_equal(sin(0.500), 0x0000.0000, "sin(0.500)")
    assert_equal(sin(0.625), 0x0000.b505, "sin(0.625)")
    assert_equal(sin(0.750), 0x0001.0000, "sin(0.750)")
    assert_equal(sin(0.875), 0x0000.b505, "sin(0.875)")
    assert_equal(sin(1.000), 0x0000.0000, "sin(1.000)")

    assert_equal(sgn(100),  1, "sgn(100)")
    assert_equal(sgn(0.0),  1, "sgn(0.0)")
    assert_equal(sgn(-14), -1, "sgn(-14)")

    assert_equal(sqrt(9.00), 3.0000, "sqrt(9.00)")
    assert_equal(sqrt(2.00), 1.4142, "sqrt(2.00)")
    assert_equal(sqrt(0.25), 0.5000, "sqrt(0.25)")
    assert_equal(sqrt(0.00), 0.0000, "sqrt(0.00)")
    assert_equal(sqrt(-1.0), 0.0000, "sqrt(-1.0)")

    log("srand(1)")
    srand(1)
    assert_equal(rnd(1.00000), 0x0000.14b7, "rnd(1.00000)")
    assert_equal(rnd(20.0000), 0x0001.fd18, "rnd(20.0000)")
    assert_equal(rnd(345.678), 0x00a8.1d42, "rnd(345.678)")

    log("srand(2)")
    srand(2)
    assert_equal(rnd(1.00000), 0x0000.c665, "rnd(1.00000)")
    assert_equal(rnd(20.0000), 0x0004.5511, "rnd(20.0000)")
    assert_equal(rnd(345.678), 0x00f1.c8e9, "rnd(345.678)")

    log("srand(3)")
    srand(3)
    assert_equal(rnd(1.00000), 0x0000.7593, "rnd(1.00000)")
    assert_equal(rnd(20.0000), 0x0006.234b, "rnd(20.0000)")
    assert_equal(rnd(345.678), 0x004f.6f4b, "rnd(345.678)")

    log("num_list = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}")
    local num_list = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    log("srand(4)")
    srand(4)
    assert_equal(rnd(num_list), 6, "rnd(num_list)")
    assert_equal(rnd(num_list), 2, "rnd(num_list)")
    assert_equal(rnd(num_list), 5, "rnd(num_list)")
    assert_equal(rnd(num_list), 5, "rnd(num_list)")
    assert_equal(rnd(num_list), 5, "rnd(num_list)")
    assert_equal(rnd(num_list), 8, "rnd(num_list)")
    log("srand(5)")
    srand(5)
    assert_equal(rnd(num_list), 5,  "rnd(num_list)")
    assert_equal(rnd(num_list), 9,  "rnd(num_list)")
    assert_equal(rnd(num_list), 2,  "rnd(num_list)")
    assert_equal(rnd(num_list), 10, "rnd(num_list)")
    assert_equal(rnd(num_list), 2,  "rnd(num_list)")
    assert_equal(rnd(num_list), 8,  "rnd(num_list)")
    log("srand(6)")
    srand(6)
    assert_equal(rnd(num_list), 9, "rnd(num_list)")
    assert_equal(rnd(num_list), 9, "rnd(num_list)")
    assert_equal(rnd(num_list), 5, "rnd(num_list)")
    assert_equal(rnd(num_list), 4, "rnd(num_list)")
    assert_equal(rnd(num_list), 7, "rnd(num_list)")
    assert_equal(rnd(num_list), 3, "rnd(num_list)")

    fillp(0b0011001111001100) -- 0x33CC
    assert_equal(peek2(0x5F31), 0x33CC, "peek2(0x5F31)")

    pset(10, 10, 8)
    assert_equal(pget(10, 10), 8, "pget(10, 10)")
    pset(10, 10, 0)
    assert_equal(pget(10, 10), 0, "pget(10, 10)")

    memset(0x6000, 0xab, 2)
    assert_equal(peek2(0x6000), 0xabab, "memset(0x6000, 0xab, 8)")

    memcpy(0x6080, 0x6000, 2)
    assert_equal(peek2(0x6080), 0xabab, "memcpy(0x6080, 0x6000, 2)")

    memset(0x6000, 0, 0x1fc0)
    memset(0x6040, 0xf9, 0x1fc0)
    assert_equal(peek2(0x6040), 0xf9f9, "memset(0x6040, 0xf9, 0x1fc0)")
    memcpy(0x6000,0x6040,0x1fc0)
    assert_equal(peek2(0x6000), 0xf9f9, "memcpy(0x6000, 0x6040, 0x1fc0)")

    assert_equal(#"Hello world.", 12, "#\"Hello world.\"")
end

function run_tests()
    test_arithmetic_operations()
    test_relational_operations()
    test_logical_operations()
    test_control_flow()
    test_tables()
    test_pico8_api()
end

run_tests()

return failed_tests
