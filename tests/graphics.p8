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

circfill(64, 64, 32, 8)
pset(2, 2, 8)

return failed_tests
