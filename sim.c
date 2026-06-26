// sim.c - OS Project (Milestones 2-7)
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

#define MAX_NODES     15
#define MAX_TRAVELERS 10
#define INF           999999
#define STEP_US       300000
#define WIN_W         900
#define WIN_H         650

typedef enum {
    MSG_WAITING   = 0,
    MSG_ARRIVED   = 1,
    MSG_DEPARTING = 2,
    MSG_FINISHED  = 3,
} MsgType;

typedef enum {
    SCHED_NONE = 0,
    SCHED_FCFS = 1,
    SCHED_SJF  = 2,
} SchedAlgo;

typedef struct {
    MsgType type;
    pid_t   pid;
    int     tindex;
    int     from_node;
    int     to_node;
    int     next_node;
    int     weight;
    int     job;
} Message;

typedef struct {
    sem_t node_sem[MAX_NODES];
    sem_t go_sem[MAX_TRAVELERS];
} SharedData;

static int g_n;
static int g_adj[MAX_NODES][MAX_NODES];
static SchedAlgo g_sched = SCHED_NONE;

static int sch_busy[MAX_NODES];
static int sch_q[MAX_NODES][MAX_TRAVELERS];
static int sch_qlen[MAX_NODES];
static int sch_job[MAX_TRAVELERS];
static int sch_seq[MAX_TRAVELERS];
static int sch_seqctr = 0;

static int dijkstra(int src, int dst, int *path) {
    int dist[MAX_NODES], prev[MAX_NODES], vis[MAX_NODES];
    for (int i = 0; i < g_n; i++) {
        dist[i] = INF; prev[i] = -1; vis[i] = 0;
    }
    dist[src] = 0;
    for (int k = 0; k < g_n; k++) {
        int u = -1;
        for (int j = 0; j < g_n; j++)
            if (!vis[j] && (u == -1 || dist[j] < dist[u])) u = j;
        if (u == -1 || dist[u] == INF) break;
        vis[u] = 1;
        for (int v = 0; v < g_n; v++) {
            if (g_adj[u][v] != INF && dist[u] + g_adj[u][v] < dist[v]) {
                dist[v] = dist[u] + g_adj[u][v]; prev[v] = u;
            }
        }
    }
    if (dist[dst] == INF) return 0;
    int tmp[MAX_NODES], cnt = 0, cur = dst;
    while (cur != -1) { tmp[cnt++] = cur; cur = prev[cur]; }
    for (int i = 0; i < cnt; i++) path[i] = tmp[cnt - 1 - i];
    return cnt;
}

static int remaining_cost(int *path, int psz, int i) {
    int rem = 0;
    for (int k = i; k < psz - 1; k++) rem += g_adj[path[k]][path[k + 1]];
    return rem;
}

static void child_run(int ti, int src, int dst, int wfd, SharedData *sh) {
    int path[MAX_NODES];
    int psz = dijkstra(src, dst, path);

    // --- TASK C: Path Logging ---
    printf("Traveler %d path: ", ti);
    for (int k = 0; k < psz; k++) printf("%d%s", path[k], (k == psz - 1) ? "" : " -> ");
    printf("\n");
    fflush(stdout);
    // ----------------------------

    Message m = {0};
    m.pid = getpid();
    m.tindex = ti;

    if (psz == 0) {
        m.type = MSG_FINISHED; m.from_node = -1; m.to_node = dst;
        write(wfd, &m, sizeof m); close(wfd); exit(0);
    }

    m.type = MSG_WAITING; m.from_node = -1; m.to_node = path[0];
    m.job = remaining_cost(path, psz, 0);
    m.next_node = (psz > 1) ? path[1] : -1;
    write(wfd, &m, sizeof m);

    if (g_sched == SCHED_NONE) sem_wait(&sh->node_sem[path[0]]);
    else sem_wait(&sh->go_sem[ti]);

    m.type = MSG_ARRIVED; m.to_node = path[0]; write(wfd, &m, sizeof m);
    sleep(1);

    for (int i = 0; i < psz - 1; i++) {
        int u = path[i], v = path[i + 1], w = g_adj[u][v];
        if (g_sched == SCHED_NONE) sem_post(&sh->node_sem[u]);
        m.type = MSG_DEPARTING; m.from_node = u; m.to_node = v; m.weight = w;
        write(wfd, &m, sizeof m);
        usleep((useconds_t)w * STEP_US);
        m.type = MSG_WAITING; m.from_node = u; m.to_node = v;
        m.job = remaining_cost(path, psz, i + 1);
        m.next_node = (i + 2 < psz) ? path[i + 2] : -1;
        write(wfd, &m, sizeof m);
        if (g_sched == SCHED_NONE) sem_wait(&sh->node_sem[v]);
        else sem_wait(&sh->go_sem[ti]);
        m.type = MSG_ARRIVED; m.to_node = v; write(wfd, &m, sizeof m);
        sleep(1);
    }

    if (g_sched == SCHED_NONE) sem_post(&sh->node_sem[path[psz - 1]]);
    m.type = MSG_FINISHED; m.from_node = path[psz - 1]; m.next_node = -1;
    write(wfd, &m, sizeof m);
    close(wfd); exit(0);
}

