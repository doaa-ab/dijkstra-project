# Makefile for Operating Systems Project - Milestones 1, 2, 3
CC = gcc
CFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Target for Milestone 1 (Dijkstra Logic)
milestone1:
	$(CC) dijkstra.c -o dijkstra

# Target for Milestone 2 (Static GUI)
milestone2:
	$(CC) sim.c -o sim $(CFLAGS)

# Target for Milestone 3 (Animation - Your part)
milestone3:
	$(CC) sim.c -o sim $(CFLAGS)

# Clean build files
clean:
	rm -f dijkstra sim
