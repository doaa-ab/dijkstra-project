# Makefile for Operating Systems Project - Milestones 1-6
CC     = gcc
CFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# ── Milestone 1 : Dijkstra logic (terminal only) ──────────────────────
milestone1:
	$(CC) dijkstra.c -o dijkstra

# ── Milestone 2 : Static graph GUI ───────────────────────────────────
milestone2:
	$(CC) sim.c -o sim $(CFLAGS)

# ── Milestone 3 : Animated single traveler ───────────────────────────
milestone3:
	$(CC) sim.c -o sim $(CFLAGS)

# ── Milestone 4 : Multiple travelers with fork() ─────────────────────
milestone4:
	$(CC) sim.c -o sim $(CFLAGS)

# ── Milestone 5 : Children compute own routes, IPC via pipes ─────────
milestone5:
	$(CC) sim.c -o sim $(CFLAGS)

# ── Milestone 6 : Synchronized junctions (semaphores) ────────────────
milestone6:
	$(CC) sim.c -o sim $(CFLAGS)

# ── Remove compiled binaries ─────────────────────────────────────────
clean:
	rm -f dijkstra sim
