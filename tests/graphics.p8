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

for i = 15, 0, -1 do
    circfill(64, 64, 2 + (i * 2), i)
end
pset(64, 64, 142)
rect(31, 31, 98, 98, 140)
rectfill(32, 32, 40, 40, 8)
rectfill(89, 32, 97, 40, 9)
rectfill(32, 89, 40, 97, 10)
rectfill(89, 89, 97, 97, 11)


return failed_tests
