#!/usr/bin/python3
from sympy import symbols, Eq, solve

v1x,v1y,v1z = symbols("v1x v1y v1z")
v2x,v2y,v2z = symbols("v2x v2y v2z")
v3x,v3y,v3z = symbols("v3x v3y v3z")
px,py,pz = symbols("px py pz")
u,v = symbols("u v")

eq1 = Eq(px,(1-u-v)*v1x + u*v2x + v*v3x)
eq2 = Eq(py,(1-u-v)*v1y + u*v2y + v*v3y)

solution = solve((eq1, eq2), (u,v))

print(solution)

