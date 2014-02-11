#!/usr/bin/python
#
# very rough demo to test out ghost cells
# run with mpirun 
#
from dolfin import *
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
import matplotlib as mpl

if(MPI.size(mpi_comm_world()) == 1):
    print "Only works with MPI"
    quit()

# parameters["mesh_partitioner"] = "ParMETIS"
M = UnitSquareMesh(25, 25)
shared_vertices = M.topology().shared_entities(0).keys()

x,y = M.coordinates().transpose()

cell_ownership = M.data().array("ghost_owner", M.topology().dim())
process_number = MPI.rank(M.mpi_comm())
vmask_array = M.data().array("ghost_mask", 0) == 0
vmask_array_g = M.data().array("ghost_mask", 0) == 1

cells_store=[]
colors=[]
cmap=['#ffc0c0', '#c0ffc0', '#ffffc0', '#c0c0ff', '#c0ffff', '#808080', '#408080','#404040']

idx = 0
for c in cells(M):
    xc=[]
    yc=[]
    for v in vertices(c):
        xc.append(v.point().x())
        yc.append(v.point().y())
    cells_store.append(zip(xc,yc))
    
    colors.append(cmap[cell_ownership[idx]])
    idx += 1

fig, ax = plt.subplots()

# Make the collection and add it to the plot.
coll = PolyCollection(cells_store, facecolors=colors, edgecolors='black')
ax.add_collection(coll)

plt.plot(x[vmask_array], y[vmask_array], marker='o', color='red', linestyle='none')
plt.plot(x[shared_vertices], y[shared_vertices], marker='o', color='green', linestyle='none')
plt.plot(x[vmask_array_g], y[vmask_array_g], marker='o', color='blue', linestyle='none')
plt.xlim((-0.1,1.1))
plt.ylim((-0.1,1.1))

plt.show()

