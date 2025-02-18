-- Unit tests.
local failed_tests = 0

function assert_equal(actual, expected, test_name)
    if actual == expected then
        print(test_name .. " passed")
    else
        print(test_name .. " failed: expected " .. expected .. " but got " .. actual)
        failed_tests += 1
    end
end

function assert_max_deviation(actual, expected, deviation, test_name)
    if abs(actual - expected) <= deviation then
        print(test_name .. " passed")
    else
        print(test_name .. " failed: expected " .. expected .. " but got " .. actual)
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

    assert_max_deviation(cos(0),      1,      0.0001, "cos(0)")
    assert_max_deviation(cos(0.125),  0.7071, 0.0001, "cos(0.125)")
    assert_max_deviation(cos(0.25),   0,      0.0001, "cos(0.25)")
    assert_max_deviation(cos(0.375), -0.7071, 0.0001, "cos(0.375)")
    assert_max_deviation(cos(0.5),   -1,      0.0001, "cos(0.5)")
    assert_max_deviation(cos(0.625), -0.7071, 0.0001, "cos(0.625)")
    assert_max_deviation(cos(0.75),   0,      0.0001, "cos(0.75)")
    assert_max_deviation(cos(0.875),  0.7071, 0.0001, "cos(0.875)")
    assert_max_deviation(cos(1),      1,      0.0001, "cos(1)")

    assert_equal(max(1, 2), 2, "max(1, 2)")
    assert_equal(max(2, 1), 2, "max(2, 1)")

    assert_equal(mid(8, 2, 4),           4,   "mid(8, 2, 4)")
    assert_equal(mid(-3.5, -3.4, -3.6), -3.5, "mid(-3.5, -3.4, -3.6)")
    --assert_equal(mid(6, 6, 8),           6    "mid(6, 6, 8)")

    assert_equal(min(1, 2), 1, "min(1, 2)")
    assert_equal(min(2, 1), 1, "min(2, 1)")

    assert_max_deviation(sin(0),      0,      0.0001, "sin(0)")
    assert_max_deviation(sin(0.125), -0.7071, 0.0001, "sin(0.125)")
    assert_max_deviation(sin(0.25),  -1,      0.0001, "sin(0.25)")
    assert_max_deviation(sin(0.375), -0.7071, 0.0001, "sin(0.375)")
    assert_max_deviation(sin(0.5),    0,      0.0001, "sin(0.5)")
    assert_max_deviation(sin(0.625),  0.7071, 0.0001, "sin(0.625)")
    assert_max_deviation(sin(0.75),   1,      0.0001, "sin(0.75)")
    assert_max_deviation(sin(0.875),  0.7071, 0.0001, "sin(0.875)")
    assert_max_deviation(sin(1),      0,      0.0001, "sin(1)")

    assert_equal(sgn(100),  1, "sgn(100)")
    assert_equal(sgn(0),    1, "sgn(0)")
    assert_equal(sgn(-14), -1, "sgn(-14)")

    assert_equal(sqrt(9),    3,      "sqrt(9)")
    assert_equal(sqrt(2),    1.4142, "sqrt(2)")
    assert_equal(sqrt(0.25), 0.5,    "sqrt(0.25)")
    assert_equal(sqrt(0),    0,      "sqrt(0)")
    assert_equal(sqrt(-1),   0,      "sqrt(-1)")
end

run_tests()

return failed_tests
