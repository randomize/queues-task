#!/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

a = np.loadtxt("./bench_0.txt");

fig = plt.figure()
ax = fig.add_subplot(1,1,1)

# major ticks every 20, minor ticks every 5
n = len(a)
major_ticks = np.arange(0, n, 70)
minor_ticks = np.arange(0, n, 7)

ax.set_xticks(major_ticks)
ax.set_xticks(minor_ticks, minor=True)
# ax.set_yticks(major_ticks)
# ax.set_yticks(minor_ticks, minor=True)

# and a corresponding grid

ax.grid(which='both')

# or if you want differnet settings for the grids:
ax.grid(which='minor', alpha=0.2)
ax.grid(which='major', alpha=0.5)

lines = plt.plot(a)
plt.setp(lines, color='r', linewidth=0.4)

plt.show()
