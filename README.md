# dijkstra-project

Graph and Dijkstra simulation project

**Students:** Malek Dibs · Doaa Abdeen · Ibrahim Hirbawi · Mayar AbuArafeh

---

## Project Description

Simulates movement on a directed weighted graph through seven milestones, building from a terminal Dijkstra solver up to a fully synchronized multi-traveler GUI with selectable scheduling algorithms.

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
[PID=1021] arrived at node 0 | next 2
[PID=1022] arrived at node 2 | next 1
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

**Features:** At most **one traveler inside any node at a time** · Travelers waiting outside a full junction are shown as a semi-transparent "W" marker offset around the node · Order of entry is decided by the kernel (non-deterministic, per spec)

**Synchronization mechanism: POSIX semaphores in shared anonymous memory**

One `sem_t` per node is stored in a `mmap(MAP_SHARED|MAP_ANONYMOUS)` region so child processes inherit the same memory mapping. Each semaphore is initialized to 1 (binary semaphore / mutex). Each traveler **releases its current node before acquiring the next** one:

1. `sem_post(node_sem[u])` – release the node we are leaving
2. travel along the edge (holding no node)
3. `sem_wait(node_sem[v])` – acquire the next node before entering it

Because a traveler never holds two node semaphores at the same time, there is no hold-and-wait condition and therefore **no deadlock**, even on a cyclic graph.

Named semaphores (`sem_open`) were considered but rejected because they require explicit `sem_unlink` cleanup; the shared-memory approach is automatically reclaimed when the process exits.

```bash
make milestone6
./sim input_multi.txt
```

---

## Milestone 7 – Scheduling Algorithms (FCFS / SJF)

**Goal:** Replace the kernel's non-deterministic junction-entry order from Milestone 6 with an explicit scheduling policy. When several travelers wait at the same junction, the **parent** decides who enters next.

**Two algorithms implemented (selected at runtime, no code change):**

- **FCFS** (First Come First Served) – the traveler whose request arrived earliest enters first.
- **SJF** (Shortest Job First) – the traveler with the smallest **remaining cost to its destination** (sum of remaining edge weights) enters first; ties are broken by arrival order.

**How it works:**
- The kernel no longer picks the waiter. Instead, when a child wants to enter a node it sends a `MSG_WAITING` request (carrying its remaining-cost "job size") through its pipe and then blocks on a **personal "go" semaphore** (`go_sem[i]`, one per traveler, stored in the same shared-memory region).
- The parent keeps a **waiting queue per node**. When a node becomes free (a traveler departs or finishes), the parent selects the next traveler according to the chosen algorithm, marks the node occupied, and wakes exactly that child with `sem_post(go_sem[winner])`.
- This guarantees **one traveler per node** (the parent grants a node only when it is free) and stays **deadlock-free** (a traveler still releases its node before requesting the next).
- The GUI shows the active algorithm in the top-right corner (`Scheduler: FCFS` / `Scheduler: SJF`).

**Input format:** unchanged — Milestone 7 reuses the multi-traveler format. The SJF "job size" is derived from the graph itself, so no extra fields are required.

```bash
make milestone7
./sim -schd fcfs input_multi.txt    # run with First Come First Served
./sim -schd sjf  input_multi.txt    # run with Shortest Job First
```

### Comparison: effect on waiting times

Using the same input, the two policies behave differently whenever travelers contend for the same junction:

- **FCFS** is fair and **starvation-free**: every waiting traveler eventually advances in arrival order. However, a traveler with a short remaining route can get stuck behind one with a long route, which raises the **average** waiting time across all travelers.
- **SJF** lets travelers with the shortest remaining route through first, which **lowers the average waiting time** and gets more travelers to their destination sooner. The trade-off is possible **starvation**: a traveler with a long remaining route can be repeatedly overtaken as long as shorter ones keep arriving at the junction.

Example for three travelers waiting at the same junction — T0 (remaining cost 10) arrived first, T1 (3) second, T2 (7) third:

```
FCFS entry order: T0, T1, T2   (by arrival)
SJF  entry order: T1, T2, T0   (by smallest remaining cost)
```

Under SJF the two shorter travelers finish their waits sooner, while T0 (the longest job) is served last.

---

## Input File Format

**Single traveler (milestones 1-3):**
```
<numNodes> <numEdges>
<u> <v> <weight>
...
<src> <dst>
```

**Multiple travelers (milestones 4-7):**
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

| Target            | Description                                  |
|-------------------|----------------------------------------------|
| `make milestone1` | Build `dijkstra` (terminal Dijkstra)         |
| `make milestone2` | Build `sim` (static GUI)                     |
| `make milestone3` | Build `sim` (animation)                      |
| `make milestone4` | Build `sim` (multi-traveler, fork)           |
| `make milestone5` | Build `sim` (IPC pipes)                      |
| `make milestone6` | Build `sim` (synchronized junctions)         |
| `make milestone7` | Build `sim` (scheduling: FCFS / SJF)         |
| `make clean`      | Remove compiled binaries                     |

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