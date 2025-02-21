-- Unit tests.
local failed_tests = 0

function assert_equal(actual, expected, test_name)
    if actual == expected then
        log(test_name .. " passed")
    else
        log(test_name .. " failed: expected " .. expected .. " but got " .. actual)
        failed_tests += 1
    end
end

function run_tests()
    assert_equal(abs(5),          5, "abs(5)")
    assert_equal(abs(-5),         5, "abs(-5)")
    assert_equal(abs(0),          0, "abs(0)")
    assert_equal(abs(12345),  12345, "abs(12345)")
    assert_equal(abs(-12345), 12345, "abs(-12345)")
    assert_equal(abs(0x8000), 32768, "abs(0x8000)")

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

    assert_equal(cos(0),      1,      "cos(0)")
    assert_equal(cos(0.125),  0.7071, "cos(0.125)")
    assert_equal(cos(0.25),   0,      "cos(0.25)")
    assert_equal(cos(0.375), -0.7071, "cos(0.375)")
    assert_equal(cos(0.5),   -1,      "cos(0.5)")
    assert_equal(cos(0.625), -0.7071, "cos(0.625)")
    assert_equal(cos(0.75),   0,      "cos(0.75)")
    assert_equal(cos(0.875),  0.7071, "cos(0.875)")
    assert_equal(cos(1),      1,      "cos(1)")

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

    assert_equal(sin(0),      0,      "sin(0)")
    assert_equal(sin(0.125), -0.7071, "sin(0.125)")
    assert_equal(sin(0.25),  -1,      "sin(0.25)")
    assert_equal(sin(0.375), -0.7071, "sin(0.375)")
    assert_equal(sin(0.5),    0,      "sin(0.5)")
    assert_equal(sin(0.625),  0.7071, "sin(0.625)")
    assert_equal(sin(0.75),   1,      "sin(0.75)")
    assert_equal(sin(0.875),  0.7071, "sin(0.875)")
    assert_equal(sin(1),      0,      "sin(1)")

    assert_equal(sgn(100),  1, "sgn(100)")
    assert_equal(sgn(0),    1, "sgn(0)")
    assert_equal(sgn(-14), -1, "sgn(-14)")

    assert_equal(sqrt(9),    3,      "sqrt(9)")
    assert_equal(sqrt(2),    1.4142, "sqrt(2)")
    assert_equal(sqrt(0.25), 0.5,    "sqrt(0.25)")
    assert_equal(sqrt(0),    0,      "sqrt(0)")
    assert_equal(sqrt(-1),   0,      "sqrt(-1)")

    --[[
    print("srand(1)")
    srand(1)
    assert_equal(rnd(1.0),     0.2488,   "rnd(1.0)")
    assert_equal(rnd(20.0),    6.4254,   "rnd(20.0)")
    assert_equal(rnd(345.678), 283.1279, "rnd(345.678)")

    print("srand(2)")
    srand(2)
    assert_equal(rnd(1.0),      0.2491, "rnd(1.0)")
    assert_equal(rnd(20.0),    18.9634, "rnd(20.0)")
    assert_equal(rnd(345.678), 96.2164, "rnd(345.678)")

    print("srand(3)")
    srand(3)
    assert_equal(rnd(1.0),     0.2492, "rnd(1.0)")
    assert_equal(rnd(20.0),    5.2325, "rnd(20.0)")
    assert_equal(rnd(345.678), 2.7607, "rnd(345.678)")

    print("num_list = {17, 42, 89, 5.43, 63, 28.43, 91.42, 34, 76, 50}")
    print("srand(4)")
    srand(4)
    local num_list = {17, 42, 89, 5.43, 63, 28.43, 91.42, 34, 76, 50}
    assert_equal(rnd(num_list), 76, "rnd(num_list)")
    print("srand(5)")
    srand(5)
    assert_equal(rnd(num_list), 42, "rnd(num_list)")
    --]]

    fillp(0b0011001111001100) -- 0x33CC
    assert_equal(peek2(0x5F31), 0x33CC, "peek2(0x5F31)")
end

run_tests()

return failed_tests
