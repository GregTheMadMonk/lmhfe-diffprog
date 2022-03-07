#!/bin/env python3
import pyvista as vista
import argparse

def main():
    parser = argparse.ArgumentParser(description="A tool to generate VTK domain meshses using PyVista. Supports only rectangular/cuboid domains")
    parser.add_argument("-o", metavar="FILE", type=str, help="Output file name", required=True)
    parser.add_argument("-T", action="store_const", const=True, default=False, help="Use triangles or tetrahedrons for elements instead of rectangles/cuboids.")
    parser.add_argument("-Nx", type=int, default=None, help="Amount of grid cells along X axis", required=True)
    parser.add_argument("-Ny", type=int, default=None, help="Amount of grid cells along Y axis", required=True)
    parser.add_argument("-Nz", type=int, default=0,help="Amount of grid cells along Z axis", )
    parser.add_argument("-dx", type=float, default=None, help="Cell size along X axis", required=True)
    parser.add_argument("-dy", type=float, default=None, help="Cell size along Y axis", required=True)
    parser.add_argument("-dz", type=float, default=0, help="Cell size along Z axis", )
    args = parser.parse_args()

    output = vista.UniformGrid(dims=(args.Nx + 1, args.Ny + 1, args.Nz + 1), spacing=(args.dx, args.dy, args.dz))
    if args.T: output = output.triangulate()

    output.save(args.o)

if __name__ == "__main__":
    main()
