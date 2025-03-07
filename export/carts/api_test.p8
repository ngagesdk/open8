function _init()
    pset(10, 10, 8) -- Set a red pixel
    local color = pget(10, 10) -- Get the color of the pixel
    if color == 8 then
        log("TEST PASSED")
    else
        log("TEST FAILED: "..color)
    end

    print()
end
