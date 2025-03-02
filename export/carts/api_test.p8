-- waves demo
-- by zep

r=64

function _draw()
	local color = 1
	cls()
		for y=-r,r,3 do
			for x=-r,r,2 do
				local dist=sqrt(x*x+y*y)
				z=cos(dist/40-t())*6
				pset(r+x,r+y-z,color)
			color+=1
			if color>15 then color=1 end
		end
	end
end
