# dijkstra-project

Graph and Dijkstra simulation project

**Students:** Malek Dibs · Doaa Abdeen · Ibrahim Hirbawi

---

## Project Description

Simulates movement on a directed weighted graph through six milestones, building from a terminal Dijkstra solver up to a fully synchronized multi-traveler GUI.

---

## Milestone 1 – Directed Graph + Dijkstra Algorithm

**Features:** Read graph from file · Dijkstra shortest path · Print path & weight · Handle disconnected graphs · Reject negative weights

```bash
make milestone1
./dijkstra input.txt
```

---

## Milestone 2 – Graph GUI Visualization

**Features:** raylib window · Draw nodes, edges, weights, direction arrows · Static display

```bash
make milestone2
./sim input.txt
```

---

## Milestone 3 – Movement Animation

**Features:** Animate traveler along shortest path · Play/Stop button · Wait at nodes · Speed proportional to edge weight · Arrival message

```bash
make milestone3
./sim input.txt
```

---

## Milestone 4 – Multiple Travelers with `fork()`

**Features:** Extended input format with a `# travelers` section · Parent computes each path then `fork()`s one child per traveler · Children print `[PID] started` and sleep · Parent manages raylib and animates all travelers simultaneously · Each traveler shown in a distinct color · Parent waits for all children before exiting

```bash
make milestone4
./sim input_multi.txt
```

---

## Milestone 5 – Children Compute Own Routes (IPC via Pipes)

**Features:** Each child computes its own Dijkstra path · Sends position updates to parent via a dedicated unnamed pipe (one per child) · Parent reads messages with `O_NONBLOCK`, updates GUI, and prints the log:

```
[PID=1021] arrived at node 0 | next node: 2
[PID=1022] arrived at node 2 | next node: 1
...
[PID=1021] finished
```

**IPC method chosen: unnamed pipes**
Pipes are simple, unidirectional, and kernel-buffered. With `O_NONBLOCK` on the read end the parent can drain all pending messages each frame without blocking the raylib loop. One pipe per child eliminates multiplexing complexity.

```bash
make milestone5
./sim input_multi.txt
```

---

## Milestone 6 – Synchronized Junctions

**Features:** At most **one traveler inside any node at a time** · Travelers waiting outside a full junction are shown as a semi-transparent "W" marker offset around the node · No starvation (POSIX semaphores are fair on Linux) · Order of entry is non-deterministic (intentional per spec)

**Synchronization mechanism: POSIX semaphores in shared anonymous memory**

One `sem_t` per node is stored in a `mmap(MAP_SHARED|MAP_ANONYMOUS)` region so child processes inherit the same memory mapping. Each semaphore is initialized to 1 (binary semaphore / mutex). Protocol per traveler:

1. `sem_wait(node_sem[v])` – acquire node v before entering
2. `sem_post(node_sem[u])` – release previous node u after acquiring v

This release-before-acquire ordering prevents deadlock: a traveler never holds two semaphores simultaneously while waiting for a third.

Named semaphores (`sem_open`) were considered but rejected because they require explicit `sem_unlink` cleanup; the shared-memory approach is automatically reclaimed when the process exits.

```bash
make milestone6
./sim input_multi.txt
```

---

## Input File Format

**Single traveler (milestones 1-3):**
```
<numNodes> <numEdges>
<u> <v> <weight>
...
<src> <dst>
```

**Multiple travelers (milestones 4-6):**
```
<numNodes> <numEdges>
<u> <v> <weight>
...
# travelers
<N>
<src1> <dst1>
<src2> <dst2>
...
```

---

## Makefile Targets

| Target        | Description                              |
|---------------|------------------------------------------|
| `make milestone1` | Build `dijkstra` (terminal Dijkstra) |
| `make milestone2` | Build `sim` (static GUI)             |
| `make milestone3` | Build `sim` (animation)              |
| `make milestone4` | Build `sim` (multi-traveler, fork)   |
| `make milestone5` | Build `sim` (IPC pipes)              |
| `make milestone6` | Build `sim` (synchronized junctions) |
| `make clean`      | Remove compiled binaries             |

---

## Libraries Used

- Standard C Libraries
- [raylib](https://www.raylib.com/) – GUI / animation
- POSIX: `fork`, `pipe`, `mmap`, `sem_init` (shared semaphores)

---

## Notes

- Maximum nodes: 15 · Maximum travelers: 10
- Negative edge weights are invalid (rejected in milestone 1)
- Children never call raylib functions (fork happens before `InitWindow`)
- No memory leaks · Compatible with Linux
