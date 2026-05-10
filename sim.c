#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_NODES 15   // max 15 nodes
#define INF 999999     // no road = INF

// x and y position of a node on screen
typedef struct {
    int x, y;
} NodePos;

// the shortest path we found
int path[MAX_NODES];
int pathSize = 0; // how long the path is

// find the shortest path from startNode to endNode
void find_path(int adj[MAX_NODES][MAX_NODES], int n, int startNode, int endNode) {
    int dist[MAX_NODES], prev[MAX_NODES], visited[MAX_NODES] = {0};

    // start with everything unknown
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[startNode] = 0;

    for (int i = 0; i < n; i++) {
        // pick the closest node we haven't visited yet
        int u = -1;
        for (int j = 0; j < n; j++)
            if (!visited[j] && (u == -1 || dist[j] < dist[u])) u = j;

        if (dist[u] == INF) break; // no more reachable nodes
        visited[u] = 1;

        // if going through u is shorter, update the distance
        for (int v = 0; v < n; v++) {
            if (adj[u][v] != INF && dist[u] + adj[u][v] < dist[v]) {
                dist[v] = dist[u] + adj[u][v];
                prev[v] = u;
            }
        }
    }

    if (dist[endNode] == INF) return; // no path found

    // build the path backwards then flip it
    int temp[MAX_NODES], count = 0, curr = endNode;
    while (curr != -1) { temp[count++] = curr; curr = prev[curr]; }
    for (int i = 0; i < count; i++) path[i] = temp[count - 1 - i];
    pathSize = count;
}

int main(int argc, char *argv[]) {
    // need a file name to run
    if (argc < 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    int numNodes, numEdges;

    // open the file
    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;

    // read number of nodes and edges
    fscanf(file, "%d %d", &numNodes, &numEdges);

    // setup the matrix, no roads by default
    int adj[MAX_NODES][MAX_NODES];
    for(int i=0; i<MAX_NODES; i++) 
        for(int j=0; j<MAX_NODES; j++) adj[i][j] = (i==j) ? 0 : INF;

    // read each road and save it
    for(int i=0; i<numEdges; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        if (u < MAX_NODES && v < MAX_NODES) adj[u][v] = w;
    }

    // read start and end
    int src, dst;
    fscanf(file, "%d %d", &src, &dst);
    fclose(file);

    // find the path before opening the window
    find_path(adj, numNodes, src, dst);

    // open the window
    InitWindow(800, 600, "OS Project - Milestone 3");
    SetTargetFPS(60);

    // spread nodes in a circle on screen
    NodePos positions[MAX_NODES];
    for (int i = 0; i < numNodes; i++) {
        positions[i].x = 400 + 200 * cos(i * 2 * PI / numNodes);
        positions[i].y = 300 + 200 * sin(i * 2 * PI / numNodes);
    }

    // animation stuff
    bool isPlaying = false;
    float timer = 0.0f;
    int currentIdx = 0;  // which edge we are on right now
    int currentJump = 0; // how far along that edge we are
    Vector2 entityPos = (pathSize > 0) ? (Vector2){positions[path[0]].x, positions[path[0]].y} : (Vector2){0,0};
    bool reachedEnd = false;

    // main loop
    while (!WindowShouldClose()) {

        // move the red circle along the path
        if (isPlaying && !reachedEnd && pathSize > 1) {
            timer += GetFrameTime();

            int u = path[currentIdx];
            int v = path[currentIdx + 1];
            int w = adj[u][v]; // how many steps this edge takes

            // wait 1 second at each node before moving
            float waitTime = (currentIdx > 0 && currentJump == 0) ? 1.0f : 0.0f;

            // move one step every 300ms
            if (timer >= (waitTime + 0.3f)) {
                currentJump++;
                timer = 0.0f;

                // move the circle a little bit closer to the next node
                entityPos.x = positions[u].x + (positions[v].x - positions[u].x) * ((float)currentJump / w);
                entityPos.y = positions[u].y + (positions[v].y - positions[u].y) * ((float)currentJump / w);

                // done with this edge, go to the next one
                if (currentJump >= w) {
                    currentIdx++;
                    currentJump = 0;
                    if (currentIdx >= pathSize - 1) reachedEnd = true;
                }
            }
        }

        // click the button to play or stop
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle btn = { 10, 10, 100, 40 };
            if (CheckCollisionPointRec(GetMousePosition(), btn)) isPlaying = !isPlaying;
        }

        // draw everything
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // draw roads between nodes
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < numNodes; j++) {
                if (adj[i][j] != INF && i != j) {
                    DrawLine(positions[i].x, positions[i].y, positions[j].x, positions[j].y, LIGHTGRAY);
                    DrawText(TextFormat("%d", adj[i][j]), (positions[i].x + positions[j].x)/2, (positions[i].y + positions[j].y)/2, 15, DARKGRAY);
                }
            }
        }

        // draw the nodes, gold for start and end, blue for others
        for (int i = 0; i < numNodes; i++) {
            DrawCircle(positions[i].x, positions[i].y, 25, (i == src || i == dst) ? GOLD : BLUE);
            DrawText(TextFormat("%d", i), positions[i].x - 5, positions[i].y - 5, 20, WHITE);
        }

        // draw the moving red circle
        if (pathSize > 0) {
            DrawCircleV(entityPos, 15, RED);
        }

        // draw play/stop button
        DrawRectangle(10, 10, 100, 40, DARKGRAY);
        DrawText(isPlaying ? "STOP" : "PLAY", 30, 20, 20, WHITE);

        // show this when the circle reaches the end
        if (reachedEnd) {
            DrawRectangle(250, 250, 300, 100, LIGHTGRAY);
            DrawText("TARGET REACHED!", 270, 285, 25, DARKGREEN);
        }

        // show this if there is no path
        if (pathSize == 0) {
            DrawText("NO PATH FOUND", 10, 60, 20, MAROON);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
