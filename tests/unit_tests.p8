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

    assert_max_deviation(sin(0),      0,      0.0001, "sin(0)")
    assert_max_deviation(sin(0.125), -0.7071, 0.0001, "sin(0.125)")
    assert_max_deviation(sin(0.25),  -1,      0.0001, "sin(0.25)")
    assert_max_deviation(sin(0.375), -0.7071, 0.0001, "sin(0.375)")
    assert_max_deviation(sin(0.5),    0,      0.0001, "sin(0.5)")
    assert_max_deviation(sin(0.625),  0.7071, 0.0001, "sin(0.625)")
    assert_max_deviation(sin(0.75),   1,      0.0001, "sin(0.75)")
    assert_max_deviation(sin(0.875),  0.7071, 0.0001, "sin(0.875)")
    assert_max_deviation(sin(1),      0,      0.0001, "sin(1)")

    assert_equal(1, 0, "1 == 0")
end

run_tests()

return failed_tests
