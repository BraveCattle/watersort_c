#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"
#include "utils.h"

int main(void){
    InitWindow(screenWidth, screenHeight, "Watersort");
    SetTargetFPS(60);
    Tube* tubes = malloc(TUBE_NUM * sizeof(Tube));
    initGame(tubes);
    Texture2D backgroundImage = LoadTexture("assets/background.png");

    int keyPressed = 0, clickedTube = -1;
    Vector2 mousePos;

    while (!WindowShouldClose()){
        if(GetScreenWidth() > screenWidth || GetScreenHeight() > screenHeight) {
            SetWindowSize(screenWidth, screenHeight);
        }
        // printf("current frame: %d\n", frame);
        // printf("%d: Mouse positions at (%lf, %lf)!", ++frame, mouse_pos.x, mouse_pos.y);
        // TraceLog(LOG_INFO, "%d: Mouse positions at (%lf, %lf)!", ++frame, mouse_pos.x, mouse_pos.y);
        // printf("%d\n", gameEnd(tubes));
        if(!gameEnd(tubes)){
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                mousePos = GetMousePosition();
                for(int i = 0; i < TUBE_NUM; i++)
                    if(insideTube(mousePos, tubes[i])){
                        clickedTube = i;
                        // printf("Pressed tube: %d\n", i);
                    }
            }
            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
                if(clickedTube != -1){
                    if(insideTube(mousePos, tubes[clickedTube])){
                        printf("Clicked tube: %d\n", clickedTube);
                        if(selectedTube == -1){
                            if(countWater(tubes[clickedTube]) > 0){
                                if(tubes[clickedTube].animationStage == STILL && !pouredTo(tubes, clickedTube)){
                                    printf("Selected tube: %d\n", clickedTube);
                                    selectTube(tubes, clickedTube);
                                }
                            }
                        } else if(selectedTube == clickedTube){ // clicked selected tube
                            printf("Deselected tube: %d\n", selectedTube);
                            deselectTube(tubes, selectedTube);
                            selectedTube = -1;
                        } else { // clicked other tubes
                            // if(checkPour(tubes, selectedTube, clickedTube) && !pouredTo(tubes, clickedTube)){
                            if(checkPour(tubes, selectedTube, clickedTube)){
                                pour(tubes, selectedTube, clickedTube);
                            }
                            else {
                                deselectTube(tubes, selectedTube);
                            }
                            selectedTube = -1;
                        }
                    }
                }
                clickedTube = -1;
            }
            updateTubes(tubes);

            BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            DrawTexture(backgroundImage, 0, 0, WHITE);
            DrawText("Click on tubes to select!", 250, 500, 20, TUBE_WALL_COLOR);
            // DrawText("Congrats! You created your first window!", 190, 200, 20, BACKGROUND_COLOR);
            drawTubes(tubes);
            EndDrawing();
        } else {
            BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            DrawTexture(backgroundImage, 0, 0, WHITE);
            DrawText("You win!", 190, 200, 80, TUBE_WALL_COLOR);
            // drawTubes(tubes);
            EndDrawing();
        }
        ++frame;
        
        // printf("clicking tube: %d\n", clickedTube);
        // printf("selected tube: %d\n", selectedTube);
    }
    CloseWindow();
    return 0;
}