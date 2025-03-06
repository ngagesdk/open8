-- Unit tests.
local failed_tests = 0

function assert_equal(actual, expected, test_name)
    if actual == expected then
        log(test_name .. " passed")
    else
        log(test_name .. " failed: expected " .. string.format("0x%04x", expected) .. " but got " .. string.format("0x%04x", actual))
        failed_tests += 1
    end
end

function run_tests()
    assert_equal(abs(5),                5, "abs(5)")
    assert_equal(abs(-5),               5, "abs(-5)")
    assert_equal(abs(0),                0, "abs(0)")
    assert_equal(abs(12345),        12345, "abs(12345)")
    assert_equal(abs(-12345),       12345, "abs(-12345)")
    assert_equal(abs(0x8000), 0x7fff.ffff, "abs(0x8000)")

    assert_equal(atan2(1, 0),    0,     "atan2(1, 0)")
    assert_equal(atan2(1, 1),    0.875, "atan2(1, 1)")
    assert_equal(atan2(0, 1),    0.75,  "atan2(0, 1)")
    assert_equal(atan2(-1, 1),   0.625, "atan2(-1, 1)")
    assert_equal(atan2(-1, 0),   0.5,   "atan2(-1, 0)")
    assert_equal(atan2(-1, -1),  0.375, "atan2(-1, -1)")
    assert_equal(atan2(0, -1),   0.25,  "atan2(0, -1)")
    assert_equal(atan2(1, -1),   0.125, "atan2(1, -1)")
    assert_equal(atan2(0, 0),    0.25,  "atan2(0, 0)")
    assert_equal(atan2(99, 99),  0.875, "atan2(99, 99)")

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
    assert_equal(cos(1),     0x0001.0000, "cos(1)")

    assert_equal(flr(5.9),   5, "flr(5.9)")
    assert_equal(flr(-5.2), -6, "flr(-5.2)")
    assert_equal(flr(7),     7, "flr(7)")
    assert_equal(flr(-7),   -7, "flr(-7)")

    assert_equal(max(1, 2), 2, "max(1, 2)")
    assert_equal(max(2, 1), 2, "max(2, 1)")

    assert_equal(mid(8, 2, 4),           4,   "mid(8, 2, 4)")
    assert_equal(mid(-3.5, -3.4, -3.6), -3.5, "mid(-3.5, -3.4, -3.6)")
    assert_equal(mid(6, 6, 8),           6,   "mid(6, 6, 8)")

    assert_equal(min(1, 2), 1, "min(1, 2)")
    assert_equal(min(2, 1), 1, "min(2, 1)")

    assert_equal(rotl(8, 3),         64,  "rotl(8, 3)")
    assert_equal(rotl(0.125, 3),       1, "rotl(0.125, 3)")
    assert_equal(rotl(-4096, 12), 0.0586, "rotl(-4096, 12)")
    assert_equal(rotl(1,3),            8, "rotl(1,3)")

    assert_equal(rotr(64, 3),          8, "rotr(64, 3)")
    assert_equal(rotr(1, 3),       0.125, "rotr(1, 3)")
    assert_equal(rotr(-4096,12),      15, "rotr(-4096,12)")
    assert_equal(rotr(0x8000, 1), 0x4000, "rotr(0x8000, 1)")

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

    assert_equal(sin(0),     0x0000.0000, "sin(0)")
    assert_equal(sin(0.125), 0xffff.4afb, "sin(0.125)")
    assert_equal(sin(0.25),  0xffff.0000, "sin(0.25)")
    assert_equal(sin(0.375), 0xffff.4afb, "sin(0.375)")
    assert_equal(sin(0.5),   0x0000.0000, "sin(0.5)")
    assert_equal(sin(0.625), 0x0000.b505, "sin(0.625)")
    assert_equal(sin(0.75),  0x0001.0000, "sin(0.75)")
    assert_equal(sin(0.875), 0x0000.b505, "sin(0.875)")
    assert_equal(sin(1),     0x0000.0000, "sin(1)")

    assert_equal(sgn(100),  1, "sgn(100)")
    assert_equal(sgn(0),    1, "sgn(0)")
    assert_equal(sgn(-14), -1, "sgn(-14)")

    assert_equal(sqrt(9),    3,      "sqrt(9)")
    assert_equal(sqrt(2),    1.4142, "sqrt(2)")
    assert_equal(sqrt(0.25), 0.5,    "sqrt(0.25)")
    assert_equal(sqrt(0),    0,      "sqrt(0)")
    assert_equal(sqrt(-1),   0,      "sqrt(-1)")

    log("srand(1)")
    srand(1)
    assert_equal(rnd(1.0),     0x0000.14b7, "rnd(1.0)")
    assert_equal(rnd(20.0),    0x0001.fd18, "rnd(20.0)")
    assert_equal(rnd(345.678), 0x00a8.1d42, "rnd(345.678)")

    log("srand(2)")
    srand(2)
    assert_equal(rnd(1.0),     0x0000.c665, "rnd(1.0)")
    assert_equal(rnd(20.0),    0x0004.5511, "rnd(20.0)")
    assert_equal(rnd(345.678), 0x00f1.c8e9, "rnd(345.678)")

    log("srand(3)")
    srand(3)
    assert_equal(rnd(1.0),     0x0000.7593, "rnd(1.0)")
    assert_equal(rnd(20.0),    0x0006.234b, "rnd(20.0)")
    assert_equal(rnd(345.678), 0x004f.6f4b, "rnd(345.678)")

    --[[
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
    --]]

    fillp(0b0011001111001100) -- 0x33CC
    assert_equal(peek2(0x5F31), 0x33CC, "peek2(0x5F31)")

    pset(10, 10, 8)
    assert_equal(pget(10, 10), 8, "pget(10, 10)")
    pset(10, 10, 0)
    assert_equal(pget(10, 10), 0, "pget(10, 10)")

    memset(0x6000, 0xab, 8)
    assert_equal(peek2(0x6000), 0xabab, "memset(0x6000, 0xab, 8)")
    memset(0x6002, 0xba, 8)
    assert_equal(peek2(0x6002), 0xbaba, "memset(0x6002, 0xba, 8)")

    memcpy(0x6000, 0x7000, 2)
    assert_equal(peek2(0x6000), peek2(0x7000), "memcpy(0x6000, 0x7000, 2)")
    assert_equal(peek2(0x7000), 0xabab,        "memcpy(0x6000, 0x7000, 2)")
    memcpy(0x6002, 0x7002, 2)
    assert_equal(peek2(0x6002), peek2(0x7002), "memcpy(0x6002, 0x7002, 2)")
    assert_equal(peek2(0x7002), 0xbaba,        "memcpy(0x6002, 0x7002, 2)")
end

run_tests()

return failed_tests
