#!/bin/env python3
import pyvista as pv
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: preview.py MESH_FILE [LAYER_NAME]")
        return
    mesh = pv.read(sys.argv[1])
    if len(sys.argv) < 3:
        mesh.plot(scalars="cell_index")
    else:
        mesh.plot(scalars=sys.argv[2])

if __name__=="__main__":
    main()
