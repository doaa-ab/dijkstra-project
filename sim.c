#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_NODES 15
#define INF 999999

typedef struct {
    int x, y;
} NodePos;

// Global variables for pathfinding
int path[MAX_NODES];
int pathSize = 0;

// Simple Dijkstra implementation to find the shortest path
void find_path(int adj[MAX_NODES][MAX_NODES], int n, int startNode, int endNode) {
    int dist[MAX_NODES], prev[MAX_NODES], visited[MAX_NODES] = {0};
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[startNode] = 0;

    for (int i = 0; i < n; i++) {
        int u = -1;
        for (int j = 0; j < n; j++)
            if (!visited[j] && (u == -1 || dist[j] < dist[u])) u = j;
        if (dist[u] == INF) break;
        visited[u] = 1;
        for (int v = 0; v < n; v++) {
            if (adj[u][v] != INF && dist[u] + adj[u][v] < dist[v]) {
                dist[v] = dist[u] + adj[u][v];
                prev[v] = u;
            }
        }
    }

    // Reconstruct path
    if (dist[endNode] == INF) return;
    int temp[MAX_NODES], count = 0, curr = endNode;
    while (curr != -1) { temp[count++] = curr; curr = prev[curr]; }
    for (int i = 0; i < count; i++) path[i] = temp[count - 1 - i];
    pathSize = count;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    int numNodes, numEdges;
    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;
    
    fscanf(file, "%d %d", &numNodes, &numEdges);

    int adj[MAX_NODES][MAX_NODES];
    for(int i=0; i<MAX_NODES; i++) 
        for(int j=0; j<MAX_NODES; j++) adj[i][j] = (i==j) ? 0 : INF;

    for(int i=0; i<numEdges; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        if (u < MAX_NODES && v < MAX_NODES) adj[u][v] = w;
    }

    int src, dst;
    fscanf(file, "%d %d", &src, &dst); // Read source and destination for Dijkstra 
    fclose(file);

    // Calculate Dijkstra path
    find_path(adj, numNodes, src, dst);

    InitWindow(800, 600, "OS Project - Milestone 3");
    SetTargetFPS(60);

    NodePos positions[MAX_NODES];
    for (int i = 0; i < numNodes; i++) {
        positions[i].x = 400 + 200 * cos(i * 2 * PI / numNodes);
        positions[i].y = 300 + 200 * sin(i * 2 * PI / numNodes);
    }

    // Animation state variables
    bool isPlaying = false;
    float timer = 0.0f;
    int currentIdx = 0;
    int currentJump = 0;
    Vector2 entityPos = (pathSize > 0) ? (Vector2){positions[path[0]].x, positions[path[0]].y} : (Vector2){0,0};
    bool reachedEnd = false;

    while (!WindowShouldClose()) {
        // Logic Update Section 
        if (isPlaying && !reachedEnd && pathSize > 1) {
            timer += GetFrameTime();
            
            int u = path[currentIdx];
            int v = path[currentIdx + 1];
            int w = adj[u][v];

            float waitTime = (currentIdx > 0 && currentJump == 0) ? 1.0f : 0.0f; // Wait 1s at nodes [cite: 80]

            if (timer >= (waitTime + 0.3f)) { // 300ms per jump 
                currentJump++;
                timer = 0.0f; 

                // Linear interpolation for jumps 
                entityPos.x = positions[u].x + (positions[v].x - positions[u].x) * ((float)currentJump / w);
                entityPos.y = positions[u].y + (positions[v].y - positions[u].y) * ((float)currentJump / w);

                if (currentJump >= w) {
                    currentIdx++;
                    currentJump = 0;
                    if (currentIdx >= pathSize - 1) reachedEnd = true;
                }
            }
        }

        // Button interaction logic 
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle btn = { 10, 10, 100, 40 };
            if (CheckCollisionPointRec(GetMousePosition(), btn)) isPlaying = !isPlaying;
        }

        // Drawing Section 
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw Edges 
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < numNodes; j++) {
                if (adj[i][j] != INF && i != j) {
                    DrawLine(positions[i].x, positions[i].y, positions[j].x, positions[j].y, LIGHTGRAY);
                    DrawText(TextFormat("%d", adj[i][j]), (positions[i].x + positions[j].x)/2, (positions[i].y + positions[j].y)/2, 15, DARKGRAY);
                }
            }
        }

        // Draw Nodes 
        for (int i = 0; i < numNodes; i++) {
            DrawCircle(positions[i].x, positions[i].y, 25, (i == src || i == dst) ? GOLD : BLUE);
            DrawText(TextFormat("%d", i), positions[i].x - 5, positions[i].y - 5, 20, WHITE);
        }

        // Draw Moving Entity 
        if (pathSize > 0) {
            DrawCircleV(entityPos, 15, RED);
        }

        // Draw UI Elements 
        DrawRectangle(10, 10, 100, 40, DARKGRAY);
        DrawText(isPlaying ? "STOP" : "PLAY", 30, 20, 20, WHITE);

        if (reachedEnd) {
            DrawRectangle(250, 250, 300, 100, LIGHTGRAY);
            DrawText("TARGET REACHED!", 270, 285, 25, DARKGREEN); 
        }

        if (pathSize == 0) {
            DrawText("NO PATH FOUND", 10, 60, 20, MAROON); 
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
