--[[ @file tests.lua

A portable PICO-8 emulator written in C.

Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
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

-- Graphics.
function test_graphics()
    cls()

	for i = 15, 0, -1 do
        fillp(0x0000)
        circ(64, 64, 2 + (i * 2), i)
        fillp(0x51BF)
        circfill(0, 0, 2 + (i * 2), i)
    end

    fillp("\x80")
    rectfill(64, 0, 67, 3, 15)
    fillp("\x81")
    rectfill(68, 0, 71, 3, 14)
    fillp("\x82")
    rectfill(72, 0, 75, 3, 13)
    fillp("\x83")
    rectfill(76, 0, 79, 3, 12)
    fillp("\x84")
    rectfill(80, 0, 83, 3, 11)
    fillp("\x85")
    rectfill(84, 0, 87, 3, 10)
    fillp("\x86")
    rectfill(88, 0, 91, 3, 9)
    fillp("\x87")
    rectfill(92, 0, 95, 3, 8)

    fillp(0x0000)
    pset(64, 64, 142)
    rect(31, 31, 98, 98, 140)

    rectfill(32, 32, 40, 40, 8)
    rectfill(89, 32, 97, 40, 9)
    rectfill(32, 89, 40, 97, 10)
    rectfill(89, 89, 97, 97, 11)

    ovalfill(105, 105, 127, 115, 13)
    oval(107, 107, 125, 113, 15)
    line(110, 110, 122, 110, 8)

    local crc = crc32(0x6000, 0x2000);
    assert_equal(0x440c2f00, crc, "Graphics, CRC")
end

-- Math.
function test_math()
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

    -- Integer-input tests: exercise the 32-bit fast path.
    -- Perfect squares produce exact whole-number results.
    assert_equal(sqrt(1),   1,  "sqrt(1)")
    assert_equal(sqrt(4),   2,  "sqrt(4)")
    assert_equal(sqrt(100), 10, "sqrt(100)")
    -- Non-perfect-square integer: hex literal gives the exact fix32_t value.
    -- floor(sqrt(3) * 65536) = 113511 = 0x1BB67
    assert_equal(sqrt(3), 0x0001.bb67, "sqrt(3)")
    -- floor(sqrt(8) * 65536) = 185363 = 0x2D413
    assert_equal(sqrt(8), 0x0002.d413, "sqrt(8)")

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
end

-- Memory.
function test_memory()
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
end

-- Operators.
function test_operators()
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
end

-- P8SCII.
function test_p8scii()
    cls(1)
    for i = 16, 255 do
        print(chr(i), (i%16)*8, flr(i/16)*8, 7)
    end
    local crc = crc32(0x6000, 0x2000);
    assert_equal(0x561d4500, crc, "P8SCII, (16-255) CRC")
end

-- Tables.
function test_tables()
    assert_equal(#"Hello world.", 12, "#\"Hello world.\" == 12")

    local tbl = {0, 1, 1, 2, 3, 5, 8, 13}
    log("tbl = {0, 1, 1, 2, 3, 5, 8, 13}")
    assert_equal(tbl[5], 3, "tbl[5] == 3")
    assert_equal(tbl[6], 5, "tbl[6] == 5")
    assert_equal(tbl[7], 8, "tbl[7] == 8")

    tbl = {}
    log("tbl = {}")
    add(tbl, 0x11)
    add(tbl, 0x22)
    add(tbl, 0x33)
    log("add(tbl, 0x11)")
    log("add(tbl, 0x22)")
    log("add(tbl, 0x33)")
    assert_equal(tbl[1], 0x11, "tbl[1] == 0x11")
    assert_equal(tbl[2], 0x22, "tbl[2] == 0x22")
    assert_equal(tbl[3], 0x33, "tbl[3] == 0x33")
    add(tbl, 0x44, 3)
    log("add(tbl, 0x44, 3)")
    assert_equal(tbl[1], 0x11, "tbl[1] == 0x11")
    assert_equal(tbl[2], 0x22, "tbl[2] == 0x22")
    assert_equal(tbl[3], 0x44, "tbl[3] == 0x44")
    del(tbl, 0x22)
    log("del(tbl, 0x22)")
    assert_equal(tbl[1], 0x11, "tbl[1] == 0x11")
    assert_equal(tbl[2], 0x44, "tbl[2] == 0x44")

    tbl = {}
    tbl = { 1, 3, 5 }
    local tmp = 1 + 3 + 5
    function foreach_test(n)
        tmp -= n
    end

    foreach(tbl, foreach_test)
    assert_equal(tmp, 0, "foreach(tbl, foreach_test)")

    tbl = {}
    tbl = { 1, 2, 3, 4, nil, 5, 6, 7, 8, 9, 10 }
    local result = {}

    for v in all(tbl) do
        add(result, v)
    end

    -- Expected table should skip the nil value.
    local expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

    -- Check that result has the same length.
    assert_equal(#result, #expected, "test_all_length")

    -- Check that all elements are the same.
    for i = 1, #expected do
        assert_equal(result[i], expected[i], "test_all_value_" .. i)
    end

    tbl = {}
    tbl = {1,2,3,4}

    -- Mutate the table while iterating with all().
    -- The loop should see the original values (1,2,3,4) and not the mutated value (2).
    n = 1;
    for v in all(tbl) do
        if v==2 then
            del(tbl,2)
        end
        assert_equal(v, n, "all mutation: value " .. v)
        n += 1
    end

    tbl = {}

    -- inext and ipairs tests.
    log("Testing inext() and ipairs()...")

    -- Basic ipairs iteration over simple table.
    tbl = {10, 20, 30}
    local sum = 0
    local count = 0
    for i, v in ipairs(tbl) do
        sum += v
        count += 1
        assert_equal(tbl[i], v, "ipairs: tbl[" .. i .. "] == " .. v)
    end
    assert_equal(sum, 60, "ipairs: sum of {10, 20, 30} == 60")
    assert_equal(count, 3, "ipairs: iterated 3 times")

    -- ipairs with sparse table (skips nil values in iteration).
    tbl = {1, 2, nil, 4, 5}
    local values = {}
    for i, v in ipairs(tbl) do
        add(values, v)
    end
    -- ipairs stops at first nil, so only {1, 2} are iterated.
    assert_equal(#values, 2, "ipairs: sparse table stops at first nil")
    assert_equal(values[1], 1, "ipairs: sparse table first value")
    assert_equal(values[2], 2, "ipairs: sparse table second value")

    -- ipairs with empty table.
    tbl = {}
    count = 0
    for i, v in ipairs(tbl) do
        count += 1
    end
    assert_equal(count, 0, "ipairs: empty table iteration count == 0")

    -- Direct inext usage: manual iteration.
    tbl = {100, 200, 300}
    local i, v = inext(tbl, nil)
    assert_equal(i, 1, "inext(tbl, nil): first index == 1")
    assert_equal(v, 100, "inext(tbl, nil): first value == 100")

    i, v = inext(tbl, i)
    assert_equal(i, 2, "inext(tbl, 1): second index == 2")
    assert_equal(v, 200, "inext(tbl, 1): second value == 200")

    i, v = inext(tbl, i)
    assert_equal(i, 3, "inext(tbl, 2): third index == 3")
    assert_equal(v, 300, "inext(tbl, 2): third value == 300")

    -- inext returns nothing when exhausted.
    local next_i, next_v = inext(tbl, i)
    assert_equal(next_i, nil, "inext(tbl, 3): exhausted returns nil index")
    assert_equal(next_v, nil, "inext(tbl, 3): exhausted returns nil value")

    -- inext with default index (starting from 0).
    tbl = {5}
    i, v = inext(tbl)
    assert_equal(i, 1, "inext(tbl): default index starts from 0, finds index 1")
    assert_equal(v, 5, "inext(tbl): default index finds first value")

    -- inext skips nil values in sparse table.
    tbl = {1, nil, 3, nil, 5}
    i, v = inext(tbl, 0)
    assert_equal(i, 1, "inext sparse: first index == 1")
    assert_equal(v, 1, "inext sparse: first value == 1")

    -- inext stops at first nil, does not continue searching.
    i, v = inext(tbl, i)
    assert_equal(i, nil, "inext sparse: stops at first nil, returns nil index")
    assert_equal(v, nil, "inext sparse: stops at first nil, returns nil value")

    -- ipairs and inext produce same iteration order.
    tbl = {7, 8, 9, 10}
    local ipairs_values = {}
    for i, v in ipairs(tbl) do
        add(ipairs_values, v)
    end

    local inext_values = {}
    i = 0
    while true do
        i, v = inext(tbl, i)
        if i == nil then break end
        add(inext_values, v)
    end

    assert_equal(#ipairs_values, #inext_values, "ipairs vs inext: same iteration count")
    for idx = 1, #ipairs_values do
        assert_equal(ipairs_values[idx], inext_values[idx], 
            "ipairs vs inext: value at position " .. idx .. " matches")
    end

    log("inext() and ipairs() tests passed")
end

-- Palette transparency (palt).
function test_palt()
    -- palt() reset: color 0 transparent, all others opaque.
    palt()
    assert_equal(peek(0x5f00), 0x10, "palt() reset: color 0 transparent")
    assert_equal(peek(0x5f01), 0x01, "palt() reset: color 1 opaque")
    assert_equal(peek(0x5f0f), 0x0f, "palt() reset: color 15 opaque")

    -- palt(col, false): make a color opaque.
    palt(0, false)
    assert_equal(peek(0x5f00), 0x00, "palt(0, false): color 0 opaque")

    -- palt(col, true): make a color transparent.
    palt(1, true)
    assert_equal(peek(0x5f01), 0x11, "palt(1, true): color 1 transparent")

    -- palt(mask): bit 0 (LSB) = color 15, bit 15 (MSB) = color 0.
    palt(0x8001)
    assert_equal(band(peek(0x5f00), 0x10), 0x10, "palt(0x8001): color 0 transparent")
    assert_equal(band(peek(0x5f0f), 0x10), 0x10, "palt(0x8001): color 15 transparent")
    assert_equal(band(peek(0x5f01), 0x10), 0x00, "palt(0x8001): color 1 opaque")

    -- palt() reset to restore defaults.
    palt()
    assert_equal(band(peek(0x5f00), 0x10), 0x10, "palt() re-reset: color 0 transparent")
    assert_equal(band(peek(0x5f01), 0x10), 0x00, "palt() re-reset: color 1 opaque")
end

-- Map get/set and drawing (mget, mset, map).
function test_map()
    -- mset/mget: rows 0-31 (stored at 0x2000).
    mset(0, 0, 42)
    assert_equal(mget(0, 0), 42, "mset/mget(0, 0, 42)")

    mset(127, 31, 5)
    assert_equal(mget(127, 31), 5, "mset/mget(127, 31, 5)")

    -- mset/mget: rows 32-63 (stored at 0x1000, shared region).
    mset(0, 32, 7)
    assert_equal(mget(0, 32), 7, "mset/mget(0, 32, 7)")

    mset(127, 63, 9)
    assert_equal(mget(127, 63), 9, "mset/mget(127, 63, 9)")

    -- Verify mset writes to the correct RAM address.
    -- Row 5, col 10 -> 0x2000 + 5*128 + 10 = 0x228a.
    mset(10, 5, 33)
    assert_equal(peek(0x228a), 33, "mset(10, 5, 33): direct memory check")

    -- map() drawing: fill sprite 1 with solid color 7, place at map cell (0,0).
    -- Sprite 1: sprite_x_base = 4, sprite_y_base = 0.
    -- Each row of 8 pixels occupies 4 bytes at address: row*64 + 4.
    for dy = 0, 7 do
        memset(dy * 64 + 4, 0x77, 4)
    end
    mset(0, 0, 1)
    cls()
    palt()
    map(0, 0, 0, 0, 1, 1)
    assert_equal(pget(0, 0), 7, "map() draws sprite 1 at screen (0,0)")
    assert_equal(pget(7, 7), 7, "map() draws sprite 1 at screen (7,7)")

    -- Sprite 0 at a map cell must not draw (transparent cell).
    mset(0, 0, 0)
    cls()
    rectfill(0, 0, 7, 7, 5)
    map(0, 0, 0, 0, 1, 1)
    assert_equal(pget(0, 0), 5, "map() skips sprite 0 (transparent cell)")

    -- Layer filter: sprite with matching flag is drawn.
    poke(0x3000 + 1, 0x01)
    mset(0, 0, 1)
    cls()
    map(0, 0, 0, 0, 1, 1, 0x01)
    assert_equal(pget(0, 0), 7, "map() layer=1 draws sprite with flag bit 0")

    -- Layer filter: sprite without matching flag is skipped.
    poke(0x3000 + 1, 0x00)
    cls()
    rectfill(0, 0, 7, 7, 5)
    map(0, 0, 0, 0, 1, 1, 0x01)
    assert_equal(pget(0, 0), 5, "map() layer=1 skips sprite without flag")

    -- Clean up.
    palt()
    mset(0, 0, 0)
    mset(127, 31, 0)
    mset(0, 32, 0)
    mset(127, 63, 0)
    mset(10, 5, 0)
    poke(0x3000 + 1, 0x00)
    for dy = 0, 7 do
        memset(dy * 64 + 4, 0x00, 4)
    end
end

function test_pal_sget()
    -- sget: write a known nibble to spritesheet RAM and read it back.
    -- Sprite 0, pixel (0,0) is at address 0x0000, low nibble.
    poke(0x0000, 0x5a) -- low nibble = 0xa (10), high nibble = 0x5
    assert_equal(sget(0, 0), 10, "sget(0,0) low nibble")
    assert_equal(sget(1, 0), 5,  "sget(1,0) high nibble")

    -- pal() reset: draw palette back to identity, color 0 transparent.
    pal(0, 3)           -- remap color 0 -> 3 in draw palette
    assert_equal(band(peek(0x5f00), 0x0f), 3, "pal(0,3): draw remap applied")
    pal()               -- reset
    assert_equal(band(peek(0x5f00), 0x0f), 0,    "pal() reset: color 0 identity")
    assert_equal(band(peek(0x5f00), 0x10), 0x10, "pal() reset: color 0 transparent")
    assert_equal(band(peek(0x5f01), 0x0f), 1,    "pal() reset: color 1 identity")

    -- pal(c0,c1,1): display palette remap at 0x5f10.
    pal(2, 7, 1)
    assert_equal(band(peek(0x5f12), 0x0f), 7, "pal(2,7,1): display remap at 0x5f12")
    pal()               -- reset
    assert_equal(band(peek(0x5f12), 0x0f), 2, "pal() reset: display 0x5f12 identity")

    -- draw remap: pal(7,col) causes spr to write col instead of 7.
    for dy = 0, 7 do memset(dy * 64, 0x77, 4) end  -- sprite 0 = solid color 7
    mset(0, 0, 0)
    cls()
    pal(7, 9)           -- remap 7 -> 9
    palt(0, false)      -- draw color 0 opaque so sprite 0 isn't skipped
    spr(0, 0, 0)
    pal()               -- reset
    palt()              -- reset
    assert_equal(pget(0, 0), 9, "pal(7,9): spr draws remapped color 9")

    -- Clean up spritesheet pixel.
    poke(0x0000, 0x00)
    for dy = 0, 7 do memset(dy * 64, 0x00, 4) end
end

-- mapdraw is an alias for map; verify it produces identical output.
function test_mapdraw()
    -- Fill sprite 2 with solid color 6.
    -- Sprite 2: sprite_x_base = (2 & 0xF) << 2 = 8, sprite_y_base = 0.
    -- Each row of 8 pixels occupies bytes [8..11] at address: row*64 + 8.
    for dy = 0, 7 do
        memset(dy * 64 + 8, 0x66, 4)
    end
    mset(0, 0, 2)
    cls()
    palt()
    mapdraw(0, 0, 0, 0, 1, 1)
    assert_equal(pget(0, 0), 6, "mapdraw() draws sprite at screen (0,0)")
    assert_equal(pget(7, 7), 6, "mapdraw() draws sprite at screen (7,7)")

    -- Sprite 0 skipped (transparent cell).
    mset(0, 0, 0)
    cls()
    rectfill(0, 0, 7, 7, 3)
    mapdraw(0, 0, 0, 0, 1, 1)
    assert_equal(pget(0, 0), 3, "mapdraw() skips sprite 0")

    -- Screen offset: draw map cell at screen position (8, 8).
    mset(0, 0, 2)
    cls()
    mapdraw(0, 0, 8, 8, 1, 1)
    assert_equal(pget(8, 8),  6, "mapdraw() respects sx/sy offset")
    assert_equal(pget(0, 0),  0, "mapdraw() does not draw at (0,0) when offset to (8,8)")

    -- Layer filter: sprite with matching flag is drawn.
    poke(0x3000 + 2, 0x02)
    mset(0, 0, 2)
    cls()
    mapdraw(0, 0, 0, 0, 1, 1, 0x02)
    assert_equal(pget(0, 0), 6, "mapdraw() layer=2 draws sprite with flag bit 1")

    -- Layer filter: sprite without matching flag is skipped.
    poke(0x3000 + 2, 0x00)
    cls()
    rectfill(0, 0, 7, 7, 3)
    mapdraw(0, 0, 0, 0, 1, 1, 0x02)
    assert_equal(pget(0, 0), 3, "mapdraw() layer=2 skips sprite without flag")

    -- Clean up.
    palt()
    mset(0, 0, 0)
    poke(0x3000 + 2, 0x00)
    for dy = 0, 7 do
        memset(dy * 64 + 8, 0x00, 4)
    end
end

function test_sspr()
    -- Set up a 4x4 block at sprite-sheet pixel (0,0): solid color 5.
    -- 4 pixels wide = 2 bytes per row, 4 rows.
    for row = 0, 3 do
        memset(row * 64, 0x55, 2)
    end

    -- 1:1 copy: sspr(0,0,4,4, 0,0) should draw color 5 at screen (0,0).
    cls()
    palt()
    sspr(0, 0, 4, 4, 0, 0)
    assert_equal(pget(0, 0), 5, "sspr() 1:1 top-left pixel")
    assert_equal(pget(3, 3), 5, "sspr() 1:1 bottom-right pixel")
    assert_equal(pget(4, 0), 0, "sspr() 1:1 does not bleed right")

    -- Screen offset: draw at (8, 8).
    cls()
    sspr(0, 0, 4, 4, 8, 8)
    assert_equal(pget(8,  8), 5, "sspr() dx/dy offset top-left")
    assert_equal(pget(11, 11), 5, "sspr() dx/dy offset bottom-right")
    assert_equal(pget(0,  0), 0, "sspr() dx/dy offset: (0,0) untouched")

    -- Scale up 2x: sspr(0,0,4,4, 0,0, 8,8) -> 8x8 destination.
    cls()
    sspr(0, 0, 4, 4, 0, 0, 8, 8)
    assert_equal(pget(0, 0), 5, "sspr() 2x scale top-left")
    assert_equal(pget(7, 7), 5, "sspr() 2x scale bottom-right")
    assert_equal(pget(8, 0), 0, "sspr() 2x scale does not bleed right")

    -- flip_x: source has color 5 in left half, color 6 in right half.
    -- 8 pixels wide = 4 bytes per row, left nibbles = 5, right nibbles = 6.
    for row = 0, 3 do
        memset(row * 64,     0x55, 2)  -- pixels 0-3: color 5
        memset(row * 64 + 2, 0x66, 2)  -- pixels 4-7: color 6
    end
    cls()
    sspr(0, 0, 8, 4, 0, 0, 8, 4, true, false)
    assert_equal(pget(0, 0), 6, "sspr() flip_x: left edge maps to right source")
    assert_equal(pget(7, 0), 5, "sspr() flip_x: right edge maps to left source")

    -- flip_y: source has color 5 in top half, color 6 in bottom half.
    for row = 0, 1 do memset(row * 64, 0x55, 4) end  -- rows 0-1: color 5
    for row = 2, 3 do memset(row * 64, 0x66, 4) end  -- rows 2-3: color 6
    cls()
    sspr(0, 0, 8, 4, 0, 0, 8, 4, false, true)
    assert_equal(pget(0, 0), 6, "sspr() flip_y: top row maps to bottom source")
    assert_equal(pget(0, 3), 5, "sspr() flip_y: bottom row maps to top source")

    -- Transparency: color 0 in sprite sheet should be skipped.
    for row = 0, 3 do memset(row * 64, 0x00, 2) end  -- all pixels = color 0
    cls()
    rectfill(0, 0, 3, 3, 3)
    sspr(0, 0, 4, 4, 0, 0)
    assert_equal(pget(0, 0), 3, "sspr() transparent color 0 not drawn")

    -- Palette remap: pal(5, 9) redirects color 5 to 9.
    for row = 0, 3 do memset(row * 64, 0x55, 2) end
    cls()
    pal(5, 9)
    sspr(0, 0, 4, 4, 0, 0)
    pal()
    assert_equal(pget(0, 0), 9, "sspr() draw palette remap applied")

    -- Negative dw: same as flip_x + draw at dx+dw (backside spin effect).
    -- Source: left 4 pixels = color 5, right 4 pixels = color 6.
    for row = 0, 3 do
        memset(row * 64,     0x55, 2)
        memset(row * 64 + 2, 0x66, 2)
    end
    cls()
    -- sspr(0,0,8,4, 8,0, -8,4): draws 8 pixels ending at x=7 (dx+dw..dx-1),
    -- flipped horizontally. Screen x=0 should see source x=7 (color 6).
    sspr(0, 0, 8, 4, 8, 0, -8, 4)
    assert_equal(pget(0, 0), 6, "sspr() negative dw: left edge maps to right source")
    assert_equal(pget(7, 0), 5, "sspr() negative dw: right edge maps to left source")

    -- Negative dh: same as flip_y + draw at dy+dh.
    for row = 0, 1 do memset(row * 64, 0x55, 4) end  -- rows 0-1: color 5
    for row = 2, 3 do memset(row * 64, 0x66, 4) end  -- rows 2-3: color 6
    cls()
    sspr(0, 0, 8, 4, 0, 4, 8, -4)
    assert_equal(pget(0, 0), 6, "sspr() negative dh: top row maps to bottom source")
    assert_equal(pget(0, 3), 5, "sspr() negative dh: bottom row maps to top source")

    -- Clean up spritesheet.
    for row = 0, 3 do memset(row * 64, 0x00, 4) end
    palt()
end

function test_spr()
    -- Helper: fill sprite n (8x8) with a solid color c.
    -- sprite_x_base = (n % 16) * 4 bytes, sprite_y_base = (n \ 16) * 512 bytes.
    -- Each row is 64 bytes wide; 8 pixels = 4 bytes per row.
    local function fill_sprite(n, c)
        local xb = (n % 16) * 4
        local yb = (n \ 16) * 512
        local byte = bor(c, shl(c, 4))
        for row = 0, 7 do
            memset(yb + row * 64 + xb, byte, 4)
        end
    end

    local function clear_sprite(n)
        local xb = (n % 16) * 4
        local yb = (n \ 16) * 512
        for row = 0, 7 do
            memset(yb + row * 64 + xb, 0x00, 4)
        end
    end

    -- Basic draw: sprite 1 filled with color 7 draws at (0,0).
    fill_sprite(1, 7)
    cls()
    palt()
    spr(1, 0, 0)
    assert_equal(pget(0, 0), 7, "spr() top-left pixel")
    assert_equal(pget(7, 7), 7, "spr() bottom-right pixel")
    assert_equal(pget(8, 0), 0, "spr() does not bleed right")

    -- Screen offset: sprite drawn at (10, 20).
    cls()
    spr(1, 10, 20)
    assert_equal(pget(10, 20), 7, "spr() dx/dy offset top-left")
    assert_equal(pget(17, 27), 7, "spr() dx/dy offset bottom-right")
    assert_equal(pget(0,  0),  0, "spr() dx/dy: (0,0) untouched")

    -- Default w/h is 1x1 (8x8 pixels).
    cls()
    spr(1, 0, 0)
    assert_equal(pget(7, 7), 7, "spr() default w=1 h=1 covers 8x8")

    -- w=2: draws two sprites wide (16 pixels).
    fill_sprite(2, 6)
    cls()
    spr(1, 0, 0, 2, 1)
    assert_equal(pget(0,  0), 7, "spr() w=2: left sprite drawn")
    assert_equal(pget(8,  0), 6, "spr() w=2: right sprite drawn")
    assert_equal(pget(16, 0), 0, "spr() w=2: no bleed past 16px")

    -- h=2: draws two sprites tall (16 pixels).
    fill_sprite(17, 5)  -- sprite 17 = row 1, col 1 (same column as sprite 1)
    cls()
    spr(1, 0, 0, 1, 2)
    assert_equal(pget(0, 0),  7, "spr() h=2: top sprite drawn")
    assert_equal(pget(0, 8),  5, "spr() h=2: bottom sprite drawn")
    assert_equal(pget(0, 16), 0, "spr() h=2: no bleed past 16px")

    -- Color 0 transparent by default.
    fill_sprite(3, 0)
    cls()
    rectfill(0, 0, 7, 7, 4)
    spr(3, 0, 0)
    assert_equal(pget(0, 0), 4, "spr() color 0 transparent by default")

    -- palt(0, false): color 0 becomes opaque.
    cls()
    rectfill(0, 0, 7, 7, 4)
    palt(0, false)
    spr(3, 0, 0)
    palt()
    assert_equal(pget(0, 0), 0, "spr() palt(0,false): color 0 drawn")

    -- flip_x: sprite 1 has color 7 left half, color 6 right half.
    -- Set left 4 pixels (bytes 0-1) = 7, right 4 pixels (bytes 2-3) = 6.
    local xb1 = band(1, 0xF) * 4
    for row = 0, 7 do
        memset(row * 64 + xb1,     0x77, 2)
        memset(row * 64 + xb1 + 2, 0x66, 2)
    end
    cls()
    spr(1, 0, 0, 1, 1, true, false)
    assert_equal(pget(0, 0), 6, "spr() flip_x: left edge maps to right source")
    assert_equal(pget(7, 0), 7, "spr() flip_x: right edge maps to left source")

    -- flip_y: sprite 1 top half color 7, bottom half color 6.
    for row = 0, 3 do memset(row * 64 + xb1, 0x77, 4) end
    for row = 4, 7 do memset(row * 64 + xb1, 0x66, 4) end
    cls()
    spr(1, 0, 0, 1, 1, false, true)
    assert_equal(pget(0, 0), 6, "spr() flip_y: top row maps to bottom source")
    assert_equal(pget(0, 7), 7, "spr() flip_y: bottom row maps to top source")

    -- Palette remap: pal(7, 9) causes color 7 to be drawn as 9.
    fill_sprite(1, 7)
    cls()
    pal(7, 9)
    spr(1, 0, 0)
    pal()
    assert_equal(pget(0, 0), 9, "spr() draw palette remap applied")

    -- Screen clipping: sprite partially off the left edge.
    cls()
    spr(1, -4, 0)
    assert_equal(pget(0, 0), 7, "spr() clip left: visible pixels drawn")
    assert_equal(pget(3, 0), 7, "spr() clip left: last visible pixel drawn")
    -- Sprite is 8 wide starting at x=-4, so cols 4..7 of sprite land at x=0..3.

    -- Screen clipping: sprite fully off-screen to the right.
    cls()
    rectfill(0, 0, 7, 7, 4)
    spr(1, 128, 0)
    assert_equal(pget(0, 0), 4, "spr() fully off right: nothing drawn")

    -- Sprite 0: spr(0,...) draws from spritesheet normally like any other sprite.
    fill_sprite(0, 6)
    cls()
    palt(0, false)
    spr(0, 0, 0)
    palt()
    assert_equal(pget(0, 0), 6, "spr(0): sprite 0 draws from spritesheet normally")

    -- Clean up.
    clear_sprite(0)
    clear_sprite(1)
    clear_sprite(2)
    clear_sprite(17)
    clear_sprite(3)
    palt()
    pal()
end

-- Input: btn.
function test_btn()
    -- No buttons pressed: btn() bitfield == 0.
    poke(0x5f4c, 0x00)
    assert_equal(btn(), 0, "btn() no buttons")

    -- Single button: btn(b) returns true when bit b is set.
    for b = 0, 5 do
        poke(0x5f4c, shl(1, b))
        assert_true(btn(b) == true, "btn(" .. b .. ") pressed")
        assert_true(btn(b) == true, "btn(" .. b .. ",0) pressed p0")
    end

    -- btn() bitfield reflects all pressed buttons.
    poke(0x5f4c, 0x3f)
    assert_equal(btn(), 0x3f, "btn() all 6 buttons bitfield")

    -- btn(b) false when bit is clear.
    poke(0x5f4c, 0x00)
    assert_true(btn(0) == false, "btn(0) not pressed")
    assert_true(btn(5) == false, "btn(5) not pressed")

    -- Out-of-range button index returns false.
    assert_true(btn(6)  == false, "btn(6) out of range")
    assert_true(btn(-1) == false, "btn(-1) out of range")

    -- Player 1: poke 0x5f4d, read with player=1.
    poke(0x5f4c, 0x00)
    poke(0x5f4d, 0x01)
    assert_true(btn(0, 1) == true,  "btn(0,1) p1 pressed")
    assert_true(btn(0, 0) == false, "btn(0,0) p0 not pressed")

    -- Clean up.
    poke(0x5f4c, 0x00)
    poke(0x5f4d, 0x00)
end

-- Input: btnp.
function test_btnp()
    -- btnp(b): true only on frame 1 (first press).
    set_btnp_frames(0, 0)
    assert_true(btnp(0) == false, "btnp(0) frame 0: not triggered")

    set_btnp_frames(0, 1)
    assert_true(btnp(0) == true,  "btnp(0) frame 1: first press")

    set_btnp_frames(0, 2)
    assert_true(btnp(0) == false, "btnp(0) frame 2: not triggered")

    -- btnp repeat: triggers again at frame 16 (15+1*4-3... i.e. f>15 and (f-15)%4==0).
    set_btnp_frames(0, 15)
    assert_true(btnp(0) == false, "btnp(0) frame 15: not triggered")
    set_btnp_frames(0, 16)
    assert_true(btnp(0) == false, "btnp(0) frame 16: not triggered")
    set_btnp_frames(0, 19)
    assert_true(btnp(0) == true,  "btnp(0) frame 19: repeat (15+4)")
    set_btnp_frames(0, 23)
    assert_true(btnp(0) == true,  "btnp(0) frame 23: repeat (15+8)")
    set_btnp_frames(0, 20)
    assert_true(btnp(0) == false, "btnp(0) frame 20: not on repeat boundary")

    -- btnp(b): not triggered when frames==0 for all buttons.
    for b = 0, 5 do
        set_btnp_frames(b, 0)
    end
    assert_equal(btnp(), 0, "btnp() no buttons held")

    -- btnp() bitfield: set frame 1 on button 2 and 4.
    set_btnp_frames(2, 1)
    set_btnp_frames(4, 1)
    local expected = bor(shl(1, 2), shl(1, 4))
    assert_equal(btnp(), expected, "btnp() bitfield buttons 2 and 4")

    -- Out-of-range returns false.
    assert_true(btnp(6)  == false, "btnp(6) out of range")
    assert_true(btnp(-1) == false, "btnp(-1) out of range")

    -- Player 1: set frame count for p1 button 3.
    set_btnp_frames(3, 1, 1)
    assert_true(btnp(3, 1) == true,  "btnp(3,1) p1 first press")
    assert_true(btnp(3, 0) == false, "btnp(3,0) p0 not pressed")

    -- Clean up.
    for b = 0, 5 do
        set_btnp_frames(b, 0)
        set_btnp_frames(b, 0, 1)
    end
end

-- Sprite flags (fget).
function test_fget()
    -- Set known flags for sprite 10.
    poke(0x3000 + 10, 0x00)
    assert_equal(fget(10),    0,     "fget(10) all flags clear")
    assert_true(fget(10, 0) == false, "fget(10, 0) flag 0 clear")

    -- Set flag 0 (bit 0 = 1).
    poke(0x3000 + 10, 0x01)
    assert_equal(fget(10),    1,     "fget(10) flag 0 set")
    assert_true(fget(10, 0) == true,  "fget(10, 0) flag 0 set")
    assert_true(fget(10, 1) == false, "fget(10, 1) flag 1 clear")

    -- Set flags 0 and 7 (bit 0 + bit 7 = 0x81 = 129).
    poke(0x3000 + 10, 0x81)
    assert_equal(fget(10),    0x81,  "fget(10) flags 0 and 7 set")
    assert_true(fget(10, 0) == true,  "fget(10, 0) set")
    assert_true(fget(10, 7) == true,  "fget(10, 7) set")
    assert_true(fget(10, 3) == false, "fget(10, 3) clear")

    -- All flags set.
    poke(0x3000 + 10, 0xff)
    assert_equal(fget(10), 0xff, "fget(10) all flags set")
    for f = 0, 7 do
        assert_true(fget(10, f) == true, "fget(10, " .. f .. ") all set")
    end

    -- Clean up.
    poke(0x3000 + 10, 0x00)
end

function run_tests()
    init_crc32()

    test_arithmetic_operations()
    test_relational_operations()
    test_logical_operations()
    test_control_flow()
    test_graphics()
    test_math()
    test_memory()
    test_operators()
    test_p8scii()
    test_tables()
    test_palt()
    test_map()
    test_mapdraw()
    test_pal_sget()
    test_sspr()
    test_spr()
    test_btn()
    test_btnp()
    test_fget()
end

run_tests()

return failed_tests
