# OS Project - Milestones 1 to 6

CC = gcc
CFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# First milestone: run Dijkstra in terminal
milestone1:
	$(CC) dijkstra.c -o dijkstra

# Show the graph on screen
milestone2:
	$(CC) sim.c -o sim $(CFLAGS)

# Animate a single traveler
milestone3:
	$(CC) sim.c -o sim $(CFLAGS)

# Add multiple travelers with fork()
milestone4:
	$(CC) sim.c -o sim $(CFLAGS)

# Use pipes for communication between processes
milestone5:
	$(CC) sim.c -o sim $(CFLAGS)

# Use semaphores to control junction access
milestone6:
	$(CC) sim.c -o sim $(CFLAGS)

# Delete executable files
clean:
	rm -f dijkstra sim
