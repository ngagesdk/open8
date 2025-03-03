local t = 0

function _init()
    fillp(0b1010010110100101)
end

function _update()
    t += 0.01
end

function _draw()
    cls()

    for i = 0, 15 do
        local x = 64 + cos(t + i * 0.2) * 40
        local y = 64 + sin(t + i * 0.2) * 40
        local size = 10 + sin(t + i * 0.5) * 5
        rectfill(x - size, y - size, x + size, y + size, i + 1)
    end

    for i = 0, 15 do
        local x = 64 + cos(t + i * 0.3) * 30
        local y = 64 + sin(t + i * 0.3) * 30
        local radius = 5 + sin(t + i * 0.7) * 3
        circfill(x, y, radius, i + 1)
    end
end
