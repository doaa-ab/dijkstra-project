#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_NODES 15   // the graph can have at most 15 nodes
#define INF INT_MAX    // INF means there is no road between two nodes

// this holds our graph
// adjMatrix[i][j] is the distance from node i to node j
// if there is no road, we put INF
typedef struct {
    int numNodes;
    int adjMatrix[MAX_NODES][MAX_NODES];
} Graph;

// this function prints the path from start to node j
// it calls itself going backwards until it reaches the start
void printPath(int parent[], int j) {
    if (parent[j] == -1) {
        printf("%d", j); // we reached the start node, print it
        return;
    }
    printPath(parent, parent[j]); // go back one step
    printf(" -> %d", j);          // then print this node
}

// this is the main function that finds the shortest path
// it goes from 'start' to 'end' using Dijkstra's algorithm
void runDijkstra(Graph *g, int start, int end) {
    int dist[MAX_NODES];     // shortest distance we found so far to each node
    int parent[MAX_NODES];   
    bool visited[MAX_NODES]; // did we already finalize this node

    // at the beginning we know nothing, so set everything to worst case
    for (int i = 0; i < g->numNodes; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = false;
    }

    dist[start] = 0; // distance from start to itself is 0

    // we repeat this for every node in the graph
    for (int count = 0; count < g->numNodes - 1; count++) {

        // find the closest node we haven't visited yet
        int u = -1;
        int minDist = INF;
        for (int v = 0; v < g->numNodes; v++) {
            if (!visited[v] && dist[v] <= minDist) {
                minDist = dist[v];
                u = v;
            }
        }

        // if we can't find any reachable node, stop
        if (u == -1 || dist[u] == INF) break;

        visited[u] = true; // we are done with this node

        // check all neighbors of u
        // if going through u gives a shorter path, update it
        for (int v = 0; v < g->numNodes; v++) {
            if (!visited[v] && g->adjMatrix[u][v] != INF && 
                dist[u] + g->adjMatrix[u][v] < dist[v]) {
                dist[v] = dist[u] + g->adjMatrix[u][v]; // found a shorter way
                parent[v] = u; // remember we came from u
            }
        }
    }

    // print the result
    if (dist[end] == INF) {
        printf("No path found\n");        // we never reached the end node
    } else if (start == end) {
        printf("%d\n%d\n", start, start); // start and end are the same node
    } else {
        printPath(parent, end);           // print the path we found
        printf("\n%d\n", dist[end]);      // print the total distance
    }
}

int main(int argc, char *argv[]) {
    // make sure the user gave us a file to read
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    Graph g;
    int startNode, endNode, numEdges;

    // open the file
    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;

    // read how many nodes and edges the graph has
    if (fscanf(file, "%d %d", &g.numNodes, &numEdges) != 2) {
        fclose(file);
        return 1;
    }

    // set up the matrix — 0 if same node, INF if no road between them
    for (int i = 0; i < g.numNodes; i++)
        for (int j = 0; j < g.numNodes; j++)
            g.adjMatrix[i][j] = (i == j) ? 0 : INF;

    // read each road (from u to v with distance w) and save it
    for (int i = 0; i < numEdges; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) != 3) break;

        // negative distances don't make sense for this algorithm
        if (w < 0) {
            printf("Invalid input\n");
            fclose(file);
            return 1;
        }
        g.adjMatrix[u][v] = w;
    }

    // read where we start and where we want to go
    fscanf(file, "%d %d", &startNode, &endNode);
    fclose(file);

    // now find and print the shortest path
    runDijkstra(&g, startNode, endNode);
    return 0;
}
