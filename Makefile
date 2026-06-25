# OS Project - Milestones 1 to 7
# Build system for Dijkstra graph simulation project

CC = gcc
CFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Milestone 1: terminal-based Dijkstra shortest path
milestone1:
	$(CC) dijkstra.c -o dijkstra

# Milestone 2: display graph visualization using raylib
milestone2:
	$(CC) sim.c -o sim $(CFLAGS)

# Milestone 3: animate single traveler on graph
milestone3:
	$(CC) sim.c -o sim $(CFLAGS)

# Milestone 4: multiple travelers using fork()
milestone4:
	$(CC) sim.c -o sim $(CFLAGS)

# Milestone 5: inter-process communication using pipes
milestone5:
	$(CC) sim.c -o sim $(CFLAGS)

# Milestone 6: synchronization using semaphores
milestone6:
	$(CC) sim.c -o sim $(CFLAGS)

# Milestone 7: scheduling algorithms (FCFS / SJF) selected at runtime
milestone7:
	$(CC) sim.c -o sim $(CFLAGS)

# Clean build files
clean:
	rm -f dijkstra sim
