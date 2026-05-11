# dijkstra-project
Graph and Dijkstra simulation project

Students:
- Malek Dibs
- Doaa Abdeen
- Ibrahim Hirbawi

========================================
Project Description
========================================

This project simulates movement on a directed weighted graph.

The program:
1. Reads a graph from a text file.
2. Finds the shortest path using Dijkstra’s Algorithm.
3. Displays the graph using raylib GUI.
4. Animates an entity moving along the shortest path.

========================================
Milestone 1
Directed Graph + Dijkstra Algorithm
========================================

Features:
- Read graph from file
- Store graph in memory
- Calculate shortest path using Dijkstra
- Print path and total weight
- Handle disconnected graphs
- Handle invalid negative weights

Compile:
make milestone1

Run:
./dijkstra input.txt
========================================
Milestone 2
Graph GUI Visualization
========================================

Features:
- Display graph using raylib
- Draw nodes and edges
- Display edge weights
- Display arrows for directions
- Static graph visualization

Compile:
make milestone2

Run:
./sim input.txt

========================================
Milestone 3
Movement Animation
========================================

Features:
- Animate movement on shortest path
- Play / Stop button
- Waiting at nodes
- Edge movement based on edge weight
- Arrival message at destination
- Smooth animation

Compile:
make milestone3

Run:
./sim input.txt

========================================
Input File Format
========================================


Makefile Targets
========================================

make milestone1
make milestone2
make milestone3
make clean

========================================
Libraries Used
========================================

- Standard C Libraries
- raylib

========================================
Notes
========================================

- Maximum number of vertices: 15
- No memory leaks
- Negative weights are invalid
- Compatible with Linux environment

========================================
GitHub Tags
========================================

milestone1
milestone2
milestone3

========================================
