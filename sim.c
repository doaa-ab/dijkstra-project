 #include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_NODES 15
#define INF 999999

typedef struct {
    int x, y;
} NodePos;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    int numNodes, numEdges;
    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;
    
    // Read N and M  
    fscanf(file, "%d %d", &numNodes, &numEdges);

    int adj[MAX_NODES][MAX_NODES];
    for(int i=0; i<MAX_NODES; i++) 
        for(int j=0; j<MAX_NODES; j++) adj[i][j] = (i==j) ? 0 : INF;

    // Read edges  
    for(int i=0; i<numEdges; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        adj[u][v] = w;
    }
    fclose(file);

    InitWindow(800, 600, "OS Project - Milestone 2");
    SetTargetFPS(60);

    // Arrange nodes in a circle for readability 
    NodePos positions[MAX_NODES];
    for (int i = 0; i < numNodes; i++) {
        positions[i].x = 400 + 200 * cos(i * 2 * PI / numNodes);
        positions[i].y = 300 + 200 * sin(i * 2 * PI / numNodes);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw Edges with arrows and weights  
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < numNodes; j++) {
                if (adj[i][j] != INF && i != j) {
                    DrawLine(positions[i].x, positions[i].y, positions[j].x, positions[j].y, GRAY);
                    DrawText(TextFormat("%d", adj[i][j]), (positions[i].x + positions[j].x)/2, (positions[i].y + positions[j].y)/2, 20, RED);
                }
            }
        }

        // Draw Nodes as circles with IDs  
        for (int i = 0; i < numNodes; i++) {
            DrawCircle(positions[i].x, positions[i].y, 25, BLUE);
            DrawText(TextFormat("%d", i), positions[i].x - 5, positions[i].y - 5, 20, WHITE);
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
