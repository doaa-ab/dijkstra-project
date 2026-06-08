/* sim.c – OS Project: Milestones 2-6
 *
 * Milestones 2-3 : single traveler, raylib animation (backward-compatible)
 * Milestone  4   : multiple travelers via fork(), parent manages GUI
 * Milestone  5   : children compute their own Dijkstra, send position
 *                  updates to parent through pipes (one pipe per child)
 * Milestone  6   : synchronized junctions — POSIX semaphores in shared
 *                  anonymous memory enforce mutual exclusion on every node
 *                  (at most one traveler inside a node at a time)
 *
 * IPC choice     : unnamed pipes  (one write-end per child → parent)
 * Sync choice    : sem_init(pshared=1) in mmap(MAP_SHARED|MAP_ANONYMOUS)
 *
 * Compile : gcc sim.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
 * Run     : ./sim <input_file>
 */

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

/* ═══════════════════════════════════════════════════════════════════════
   Constants
═══════════════════════════════════════════════════════════════════════ */
#define MAX_NODES     15
#define MAX_TRAVELERS 10
#define INF           999999
#define STEP_US       300000   /* 300 ms per weight unit (matches M3 speed) */
#define WIN_W         900
#define WIN_H         650

/* ═══════════════════════════════════════════════════════════════════════
   IPC message  (child → parent via pipe)
═══════════════════════════════════════════════════════════════════════ */
typedef enum {
    MSG_WAITING   = 0,  /* blocked outside a node, waiting for semaphore */
    MSG_ARRIVED   = 1,  /* entered a node (semaphore acquired)            */
    MSG_DEPARTING = 2,  /* left a node, now traveling along an edge       */
    MSG_FINISHED  = 3,  /* journey complete                               */
} MsgType;

typedef struct {
    MsgType type;
    pid_t   pid;
    int     from_node;   /* -1 when not applicable                        */
    int     to_node;     /* node being targeted / entered                 */
    int     next_node;   /* node after to_node  (-1 = none)               */
    int     weight;      /* edge weight for animation timing              */
} Message;

/* ═══════════════════════════════════════════════════════════════════════
   Shared memory – one binary semaphore per node (Milestone 6)
═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    sem_t node_sem[MAX_NODES];
} SharedData;

/* ═══════════════════════════════════════════════════════════════════════
   Graph  (written once in main, read-only after fork)
═══════════════════════════════════════════════════════════════════════ */
static int g_n;                          /* number of nodes   */
static int g_adj[MAX_NODES][MAX_NODES];  /* adjacency matrix  */

/* ═══════════════════════════════════════════════════════════════════════
   Dijkstra
═══════════════════════════════════════════════════════════════════════ */
static int dijkstra(int src, int dst, int *path) {
    int dist[MAX_NODES], prev[MAX_NODES], vis[MAX_NODES];
    for (int i = 0; i < g_n; i++) { dist[i]=INF; prev[i]=-1; vis[i]=0; }
    dist[src] = 0;
    for (int k = 0; k < g_n; k++) {
        int u = -1;
        for (int j = 0; j < g_n; j++)
            if (!vis[j] && (u == -1 || dist[j] < dist[u])) u = j;
        if (u == -1 || dist[u] == INF) break;
        vis[u] = 1;
        for (int v = 0; v < g_n; v++)
            if (g_adj[u][v] != INF && dist[u] + g_adj[u][v] < dist[v]) {
                dist[v] = dist[u] + g_adj[u][v]; prev[v] = u;
            }
    }
    if (dist[dst] == INF) return 0;
    int tmp[MAX_NODES], cnt=0, cur=dst;
    while (cur != -1) { tmp[cnt++] = cur; cur = prev[cur]; }
    for (int i = 0; i < cnt; i++) path[i] = tmp[cnt-1-i];
    return cnt;
}

/* ═══════════════════════════════════════════════════════════════════════
   Child process  (Milestones 5-6)
   – computes own Dijkstra path
   – acquires / releases per-node semaphores (milestone 6)
   – sends progress messages to parent via pipe
═══════════════════════════════════════════════════════════════════════ */
static void child_run(int src, int dst, int wfd, SharedData *sh) {
    int path[MAX_NODES];
    int psz = dijkstra(src, dst, path);
    Message m = {0};
    m.pid = getpid();

    /* No path found – notify parent and exit */
    if (psz == 0) {
        m.type = MSG_FINISHED; m.from_node = -1; m.to_node = dst;
        write(wfd, &m, sizeof m);
        close(wfd); exit(0);
    }

    /* ── Acquire starting node (critical section) ── */
    m.type = MSG_WAITING; m.from_node = -1; m.to_node = path[0];
    m.next_node = (psz > 1) ? path[1] : -1; m.weight = 0;
    write(wfd, &m, sizeof m);
    sem_wait(&sh->node_sem[path[0]]);

    m.type = MSG_ARRIVED; m.from_node = -1; m.to_node = path[0];
    m.next_node = (psz > 1) ? path[1] : -1; m.weight = 0;
    write(wfd, &m, sizeof m);

    /* ── Traverse each edge ── */
    for (int i = 0; i < psz - 1; i++) {
        int u   = path[i];
        int v   = path[i + 1];
        int w   = g_adj[u][v];
        int nxt = (i + 1 < psz - 1) ? path[i + 2] : -1;

        /* Leave u: release its semaphore, then travel the edge */
        sem_post(&sh->node_sem[u]);
        m.type = MSG_DEPARTING; m.from_node = u; m.to_node = v;
        m.next_node = nxt; m.weight = w;
        write(wfd, &m, sizeof m);

        /* Simulate edge traversal time */
        usleep((useconds_t)w * STEP_US);

        /* Arrive at v: wait for the node to be free (critical section) */
        m.type = MSG_WAITING; m.from_node = u; m.to_node = v;
        m.next_node = nxt; m.weight = 0;
        write(wfd, &m, sizeof m);
        sem_wait(&sh->node_sem[v]);

        /* Inside v */
        m.type = MSG_ARRIVED; m.from_node = u; m.to_node = v;
        m.next_node = nxt; m.weight = w;
        write(wfd, &m, sizeof m);
    }

    /* Release destination and finish */
    sem_post(&sh->node_sem[path[psz - 1]]);
    m.type = MSG_FINISHED; m.from_node = path[psz-1]; m.to_node = dst;
    m.next_node = -1; m.weight = 0;
    write(wfd, &m, sizeof m);
    close(wfd); exit(0);
}

/* ═══════════════════════════════════════════════════════════════════════
   Node screen positions  (circle layout)
═══════════════════════════════════════════════════════════════════════ */
typedef struct { int x, y; } NodePos;

static void make_positions(NodePos *pos) {
    double cx = WIN_W / 2.0, cy = WIN_H / 2.0;
    double rx = 220.0,       ry = 205.0;
    double start = -PI / 2.0;                   /* first node at top */
    for (int i = 0; i < g_n; i++) {
        double a = start + i * 2.0 * PI / g_n;
        pos[i].x = (int)(cx + rx * cos(a));
        pos[i].y = (int)(cy + ry * sin(a));
    }
}

/* ═══════════════════════════════════════════════════════════════════════
   Traveler colors
═══════════════════════════════════════════════════════════════════════ */
static Color TC[] = {
    RED,
    DARKGREEN,
    DARKBLUE,
    PURPLE,
    ORANGE,
    MAROON,
    {0, 170, 170, 255},
    {180, 110,  0, 255},
    PINK,
    VIOLET,
};

/* ═══════════════════════════════════════════════════════════════════════
   Single-traveler GUI  (Milestones 2-3 – backward compatible)
═══════════════════════════════════════════════════════════════════════ */
static void run_single(int src, int dst, NodePos *pos) {
    int path[MAX_NODES];
    int psz = dijkstra(src, dst, path);

    bool  playing = false;
    float timer   = 0.0f;
    int   ci = 0, cj = 0;
    bool  done = false;
    int   sn = (psz > 0) ? path[0] : src;
    Vector2 ep = { (float)pos[sn].x, (float)pos[sn].y };

    while (!WindowShouldClose()) {
        /* ── Animation update ── */
        if (playing && !done && psz > 1) {
            timer += GetFrameTime();
            int u = path[ci], v = path[ci+1], w = g_adj[u][v];
            float wait = (ci > 0 && cj == 0) ? 1.0f : 0.0f;
            if (timer >= wait + 0.3f) {
                cj++; timer = 0.0f;
                ep.x = pos[u].x + (pos[v].x - pos[u].x) * (float)cj / w;
                ep.y = pos[u].y + (pos[v].y - pos[u].y) * (float)cj / w;
                if (cj >= w) { ci++; cj = 0; }
                if (ci >= psz - 1) done = true;
            }
        }

        /* ── Button ── */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle btn = {10, 10, 100, 40};
            if (CheckCollisionPointRec(GetMousePosition(), btn))
                playing = !playing;
        }

        /* ── Draw ── */
        BeginDrawing(); ClearBackground(RAYWHITE);

        for (int i = 0; i < g_n; i++)
            for (int j = 0; j < g_n; j++)
                if (g_adj[i][j] != INF && i != j) {
                    DrawLine(pos[i].x, pos[i].y, pos[j].x, pos[j].y, LIGHTGRAY);
                    DrawText(TextFormat("%d", g_adj[i][j]),
                             (pos[i].x+pos[j].x)/2, (pos[i].y+pos[j].y)/2, 14, DARKGRAY);
                }

        for (int i = 0; i < g_n; i++) {
            Color nc = (i == src || i == dst) ? GOLD : BLUE;
            DrawCircle(pos[i].x, pos[i].y, 25, nc);
            DrawText(TextFormat("%d", i), pos[i].x-5, pos[i].y-5, 20, WHITE);
        }

        if (psz > 0) DrawCircleV(ep, 14, RED);

        DrawRectangle(10, 10, 100, 40, DARKGRAY);
        DrawText(playing ? "STOP" : "PLAY", 30, 20, 20, WHITE);

        if (done) {
            DrawRectangle(250, 250, 300, 80, LIGHTGRAY);
            DrawText("TARGET REACHED!", 263, 278, 22, DARKGREEN);
        }
        if (psz == 0) DrawText("NO PATH FOUND", 10, 60, 20, MAROON);

        EndDrawing();
    }
}

/* ═══════════════════════════════════════════════════════════════════════
   Traveler visual state  (parent side)
═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    pid_t pid;
    Color col;
    float vx, vy;           /* current visual position                  */
    int   cur;              /* logical current node                      */
    int   wait_for;         /* node we are waiting outside  (-1 = none) */
    bool  in_tr;            /* animating along an edge                  */
    int   tr_f, tr_t;       /* transit: from / to node                  */
    float tr_dur, tr_elap;  /* transit duration & elapsed time (sec)    */
    bool  started, finished;
    int   src, dst;
} TVis;

/* ═══════════════════════════════════════════════════════════════════════
   Multi-traveler GUI  (Milestones 4-6)
═══════════════════════════════════════════════════════════════════════ */
static void run_multi(int nt, int *src, int *dst,
                      int *rfds, pid_t *pids, NodePos *pos) {
    TVis tv[MAX_TRAVELERS];
    memset(tv, 0, sizeof tv);

    for (int i = 0; i < nt; i++) {
        tv[i].pid      = pids[i];
        tv[i].col      = TC[i % 10];
        tv[i].vx       = (float)pos[src[i]].x;
        tv[i].vy       = (float)pos[src[i]].y;
        tv[i].cur      = src[i];
        tv[i].wait_for = -1;
        tv[i].src      = src[i];
        tv[i].dst      = dst[i];
        fcntl(rfds[i], F_SETFL, O_NONBLOCK);
    }

    int done_cnt = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* ── Read all pending pipe messages ── */
        for (int i = 0; i < nt; i++) {
            if (tv[i].finished) continue;
            Message m;
            while (read(rfds[i], &m, sizeof m) == (ssize_t)sizeof m) {
                switch (m.type) {

                case MSG_WAITING: {
                    tv[i].wait_for = m.to_node;
                    tv[i].in_tr    = false;
                    /* place waiting marker around the target node,
                       offset by traveler index to avoid overlap        */
                    float ang = i * (2.0f * PI / nt);
                    tv[i].vx = pos[m.to_node].x + 44.0f * cosf(ang);
                    tv[i].vy = pos[m.to_node].y + 44.0f * sinf(ang);
                    break;
                }

                case MSG_ARRIVED:
                    tv[i].started   = true;
                    tv[i].cur       = m.to_node;
                    tv[i].vx        = (float)pos[m.to_node].x;
                    tv[i].vy        = (float)pos[m.to_node].y;
                    tv[i].wait_for  = -1;
                    tv[i].in_tr     = false;
                    if (m.next_node == -1)
                        printf("[PID=%d] arrived at node %d | DESTINATION\n",
                               m.pid, m.to_node);
                    else
                        printf("[PID=%d] arrived at node %d | next node: %d\n",
                               m.pid, m.to_node, m.next_node);
                    fflush(stdout);
                    break;

                case MSG_DEPARTING:
                    tv[i].in_tr   = true;
                    tv[i].wait_for = -1;
                    tv[i].tr_f    = m.from_node;
                    tv[i].tr_t    = m.to_node;
                    tv[i].tr_dur  = (m.weight > 0)
                                    ? m.weight * (STEP_US / 1000000.0f)
                                    : 0.001f;
                    tv[i].tr_elap = 0.0f;
                    break;

                case MSG_FINISHED:
                    tv[i].finished = true;
                    tv[i].vx = (float)pos[tv[i].cur].x;
                    tv[i].vy = (float)pos[tv[i].cur].y;
                    printf("[PID=%d] finished\n", m.pid);
                    fflush(stdout);
                    kill(m.pid, SIGTERM);   /* signal child to stop      */
                    done_cnt++;
                    break;
                }
            }
        }

        /* ── Animate edge traversal ── */
        for (int i = 0; i < nt; i++) {
            if (!tv[i].in_tr) continue;
            tv[i].tr_elap += dt;
            float t = tv[i].tr_elap / tv[i].tr_dur;
            if (t > 1.0f) t = 1.0f;
            tv[i].vx = pos[tv[i].tr_f].x +
                       (pos[tv[i].tr_t].x - pos[tv[i].tr_f].x) * t;
            tv[i].vy = pos[tv[i].tr_f].y +
                       (pos[tv[i].tr_t].y - pos[tv[i].tr_f].y) * t;
            if (t >= 1.0f) tv[i].in_tr = false;
        }

        /* ── Draw ── */
        BeginDrawing(); ClearBackground(RAYWHITE);

        /* Edges */
        for (int i = 0; i < g_n; i++)
            for (int j = 0; j < g_n; j++)
                if (g_adj[i][j] != INF && i != j) {
                    DrawLine(pos[i].x, pos[i].y, pos[j].x, pos[j].y, LIGHTGRAY);
                    DrawText(TextFormat("%d", g_adj[i][j]),
                             (pos[i].x+pos[j].x)/2,
                             (pos[i].y+pos[j].y)/2,
                             13, DARKGRAY);
                }

        /* Nodes – gold if occupied, sky-blue otherwise */
        for (int i = 0; i < g_n; i++) {
            bool occ = false;
            for (int t = 0; t < nt; t++)
                if (tv[t].started && !tv[t].finished &&
                    !tv[t].in_tr && tv[t].wait_for == -1 &&
                    tv[t].cur == i)
                    occ = true;
            DrawCircle(pos[i].x, pos[i].y, 25, occ ? GOLD : SKYBLUE);
            DrawText(TextFormat("%d", i),
                     pos[i].x - 5, pos[i].y - 5, 20, WHITE);
        }

        /* Travelers */
        for (int i = 0; i < nt; i++) {
            if (!tv[i].started || tv[i].finished) continue;
            int ix = (int)tv[i].vx, iy = (int)tv[i].vy;

            if (tv[i].wait_for >= 0) {
                /* Waiting marker: semi-transparent fill + outline + "W" */
                DrawCircle(ix, iy, 12, Fade(tv[i].col, 0.40f));
                DrawCircleLines(ix, iy, 12, tv[i].col);
                DrawText("W", ix - 4, iy - 7, 14, tv[i].col);
            } else {
                DrawCircle(ix, iy, 14, tv[i].col);
                DrawText(TextFormat("%d", i), ix - 5, iy - 7, 14, WHITE);
            }
        }

        /* Legend */
        int lh = 10 + nt * 20 + 6;
        DrawRectangle(4, 4, 198, lh, (Color){240, 240, 240, 210});
        for (int i = 0; i < nt; i++) {
            DrawRectangle(9, 9 + i*20, 14, 14, TC[i % 10]);
            const char *st =
                tv[i].finished     ? "done"    :
                tv[i].wait_for >= 0 ? "waiting" :
                tv[i].in_tr        ? "moving"  : "at node";
            DrawText(TextFormat("T%d: %d->%d [%s]",
                                i, tv[i].src, tv[i].dst, st),
                     27, 10 + i*20, 13, DARKGRAY);
        }

        if (done_cnt == nt) {
            int tw = MeasureText("ALL TRAVELERS FINISHED", 26);
            DrawText("ALL TRAVELERS FINISHED",
                     (WIN_W - tw) / 2, WIN_H / 2 - 16, 26, DARKGREEN);
        }
        EndDrawing();
    }

    /* Kill any children still running, then reap */
    for (int i = 0; i < nt; i++)
        if (!tv[i].finished) kill(pids[i], SIGTERM);
    for (int i = 0; i < nt; i++) {
        close(rfds[i]);
        waitpid(pids[i], NULL, 0);
    }
}

/* ═══════════════════════════════════════════════════════════════════════
   main
═══════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Usage: ./sim <file_name>\n"); return 1; }

    /* ── Read graph ── */
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("fopen"); return 1; }

    int numEdges;
    if (fscanf(f, "%d %d", &g_n, &numEdges) != 2) { fclose(f); return 1; }

    for (int i = 0; i < MAX_NODES; i++)
        for (int j = 0; j < MAX_NODES; j++)
            g_adj[i][j] = (i == j) ? 0 : INF;

    for (int k = 0; k < numEdges; k++) {
        int u, v, w;
        if (fscanf(f, "%d %d %d", &u, &v, &w) != 3) break;
        if (u < MAX_NODES && v < MAX_NODES) g_adj[u][v] = w;
    }

    /* ── Parse travelers section ──
       Supports two formats:
         (a) Single traveler  :  "src dst"   (milestones 2-3)
         (b) Multi traveler   :  "# travelers\nN\nsrc1 dst1\n..."
    ── */
    int nt = 0;
    int tsrc[MAX_TRAVELERS], tdst[MAX_TRAVELERS];
    char line[256];

    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') continue;

        if (*p == '#') {
            if (strstr(p, "traveler")) {
                if (fscanf(f, "%d", &nt) == 1 && nt > 0)
                    for (int i = 0; i < nt; i++)
                        fscanf(f, "%d %d", &tsrc[i], &tdst[i]);
            }
            continue;
        }

        /* Not a comment and no travelers yet → old "src dst" format */
        if (nt == 0) {
            int s, d;
            if (sscanf(p, "%d %d", &s, &d) == 2) {
                nt = 1; tsrc[0] = s; tdst[0] = d;
            }
        }
        break;
    }
    fclose(f);

    if (nt == 0) { printf("No travelers found in input.\n"); return 1; }

    /* Screen positions are computed before any fork/InitWindow */
    NodePos pos[MAX_NODES];
    make_positions(pos);

    /* ────────────────────────────────────────────────────────
       SINGLE TRAVELER  (Milestones 2-3)
    ──────────────────────────────────────────────────────── */
    if (nt == 1) {
        InitWindow(WIN_W, WIN_H, "OS Project – Graph Simulation");
        SetTargetFPS(60);
        run_single(tsrc[0], tdst[0], pos);
        CloseWindow();
        return 0;
    }

    /* ────────────────────────────────────────────────────────
       MULTI TRAVELER  (Milestones 4-6)
       Fork BEFORE InitWindow so children never touch raylib.
    ──────────────────────────────────────────────────────── */

    /* Shared anonymous memory for per-node semaphores */
    SharedData *sh = mmap(NULL, sizeof(SharedData),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sh == MAP_FAILED) { perror("mmap"); return 1; }

    for (int i = 0; i < g_n; i++)
        sem_init(&sh->node_sem[i], 1 /*shared*/, 1 /*initial=1*/);

    /* Create one pipe per traveler, then fork */
    int   rfds[MAX_TRAVELERS];
    pid_t pids[MAX_TRAVELERS];

    for (int i = 0; i < nt; i++) {
        int pfd[2];
        if (pipe(pfd) < 0) { perror("pipe"); return 1; }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }

        if (pid == 0) {
            /* ── Child process ── */
            close(pfd[0]);                              /* close read end */
            printf("[PID=%d] started\n", getpid());
            fflush(stdout);
            child_run(tsrc[i], tdst[i], pfd[1], sh);  /* never returns  */
        }

        /* ── Parent ── */
        close(pfd[1]);                                  /* close write end */
        rfds[i] = pfd[0];
        pids[i] = pid;
    }

    /* Parent opens the window after all forks */
    InitWindow(WIN_W, WIN_H, "OS Project – Synchronized Junctions");
    SetTargetFPS(60);

    run_multi(nt, tsrc, tdst, rfds, pids, pos);

    /* Cleanup shared semaphores */
    for (int i = 0; i < g_n; i++) sem_destroy(&sh->node_sem[i]);
    munmap(sh, sizeof(SharedData));

    CloseWindow();
    return 0;
}
