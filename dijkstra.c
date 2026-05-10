#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_NODES 15
#define INF INT_MAX

typedef struct {
    int numNodes;
    int adjMatrix[MAX_NODES][MAX_NODES];
} Graph;

void printPath(int parent[], int j) {
    if (parent[j] == -1) {
        printf("%d", j);
        return;
    }
    printPath(parent, parent[j]);
    printf(" -> %d", j);
}

void runDijkstra(Graph *g, int start, int end) {
    int dist[MAX_NODES];
    int parent[MAX_NODES];
    bool visited[MAX_NODES];

    for (int i = 0; i < g->numNodes; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = false;
    }

    dist[start] = 0;

    for (int count = 0; count < g->numNodes - 1; count++) {
        int u = -1;
        int minDist = INF;
        for (int v = 0; v < g->numNodes; v++) {
            if (!visited[v] && dist[v] <= minDist) {
                minDist = dist[v];
                u = v;
            }
        }
        if (u == -1 || dist[u] == INF) break;
        visited[u] = true;
        for (int v = 0; v < g->numNodes; v++) {
            if (!visited[v] && g->adjMatrix[u][v] != INF && 
                dist[u] + g->adjMatrix[u][v] < dist[v]) {
                dist[v] = dist[u] + g->adjMatrix[u][v];
                parent[v] = u;
            }
        }
    }

    if (dist[end] == INF) {
        printf("No path found\n");
    } else if (start == end) {
        printf("%d\n%d\n", start, start);
    } else {
        printPath(parent, end);
        printf("\n%d\n", dist[end]);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    Graph g;
    int startNode, endNode, numEdges;
    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;

    if (fscanf(file, "%d %d", &g.numNodes, &numEdges) != 2) {
        fclose(file);
        return 1;
    }

    for (int i = 0; i < g.numNodes; i++)
        for (int j = 0; j < g.numNodes; j++)
            g.adjMatrix[i][j] = (i == j) ? 0 : INF;

    for (int i = 0; i < numEdges; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) != 3) break;
        if (w < 0) {
            printf("Invalid input\n");
            fclose(file);
            return 1;
        }
        g.adjMatrix[u][v] = w;
    }

    fscanf(file, "%d %d", &startNode, &endNode);
    fclose(file);

    runDijkstra(&g, startNode, endNode);
    return 0;
}
