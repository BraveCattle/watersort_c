#include <stdbool.h>
#include "raylib.h"
#include "utils.h"

int frame = 0;
int screenWidth = 700;
int screenHeight = 600;
// int screenWidth = 1600;
// int screenHeight = 900;

int selectedTube = -1;
int TUBE_NUM    = 5;
int TUBE_THICKNESS  = 5;
float TUBE_WIDTH    = 80.0;
float TUBE_HEIGHT   = 300.0;
float WATER_PERCENT = 0.9; // relative to TUBE_HEIGHT
float targetAngle[] = { 90.0, 86.0, 77.0, 65.0, 45.0 };

float HEIGHT_SELECT = 15.0;
float HEIGHT_POUR   = 15.0;
int FRAME_SELECT    = 10;
int FRAME_MOVE      = 30;
int FRAME_POUR      = 90;
float animationList[MAX_TUBE_NUM][MAX_FRAME_NUM][ANIMATION_INFO_LENGTH];
int animationIdx[MAX_TUBE_NUM];

int countWater(Tube tube){
    int waterTotal = 0;
    for(int i = 0; i < MAX_TUBE_WATER; i++){
        // printf("counting water: idx: %d, empty? %d\n", i, emptyColor(tube.contains[i]));
        if(emptyColor(tube.contains[i])) break;
        waterTotal++;
    }
    return waterTotal;
}

int isPourLeft(float angle){
    return (((int)angle)%360+360)%360 > 180;
}

bool sameColor(Color x, Color y){
    return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a;
}

bool emptyColor(Color c){
    return sameColor(c, BLANK);
}

bool insideTube(Vector2 pos, Tube tube){
    float rad = tube.angle*PI/180.0;
    float radius = tube.rect.width/2.0;
    Vector2 topLeft     = (Vector2){ tube.rect.x, tube.rect.y },
            topRight    = (Vector2){ tube.rect.x+tube.rect.width*cos(rad), tube.rect.y+tube.rect.width*sin(rad) },
            bottomLeft  = (Vector2){ tube.rect.x-tube.rect.height*sin(rad), tube.rect.y+tube.rect.height*cos(rad) },
            bottomRight = (Vector2){ tube.rect.x+tube.rect.width*cos(rad)-tube.rect.height*sin(rad),
                                     tube.rect.y+tube.rect.width*sin(rad)+tube.rect.height*cos(rad) };
    Vector2 semiCircleCenter = (Vector2){ bottomLeft.x+radius*cos(rad), bottomLeft.y+radius*sin(rad) };
    
    if(CheckCollisionPointCircle(pos, semiCircleCenter, radius)) return true;
    if(CheckCollisionPointTriangle(pos, topLeft, bottomLeft, bottomRight)) return true;
    if(CheckCollisionPointTriangle(pos, bottomRight, topRight, topLeft)) return true;
    return false;
}

void copyAnimation(float* dst, float src[ANIMATION_INFO_LENGTH]){
    for(int i = 0; i < ANIMATION_INFO_LENGTH; i++) dst[i] = src[i];
}

void printTubeInfo(Tube tube, int idx){
    printf("Tube %d info:\n", idx);
    printf("<-- rect: (%.0f, %.0f, %.0f, %.0f) -->\n", tube.rect.x, tube.rect.y, tube.rect.width, tube.rect.height);
    int waterTotal = countWater(tube);
    printf("<-- Angle: %f, water level: %d, Animation stage: %d -->\n", tube.angle, waterTotal, tube.animationStage);

    // printf("Colors(bottom to top):\n");
    // for(int i = 0; i < waterTotal; i++)
    //     printf("(%u, %u, %u, %u)%c", tube.contains[i].r, tube.contains[i].g, tube.contains[i].b, tube.contains[i].a, ",\n"[i == waterTotal-1]);
    printf("\n");
}

void printAnimationInfo(float info[ANIMATION_INFO_LENGTH], int idx){
    printf("Animation Info idx: %d\n", idx);
    printf("Position: (%.1f, %.1f), angle: %.2f, stage: %.0f, pouring to: %.0f, pour count: %.0f\n",
            info[RECT_X],
            info[RECT_Y],
            info[ANGLE],
            info[ANIMATION_STAGE],
            info[POURING_TO],
            info[POUR_COUNT]);
}

float waterLevelAnimation(float pourProcessRatio, int waterTotal){
    // choose a non-linear function to make the water level change more natural
    // pourProcessRatio range: [0, 1]
    // output range: [0, 1]
    return powf(pourProcessRatio, 8-waterTotal);
}

float getPouredAmount(Tube* tubes, int idx){
    // get the amount of water units poured to tube idx at this frame
    float amount = 0;
    for(int i = 0; i < TUBE_NUM; i++){
        if(animationIdx[i] > 0 && tubes[i].animationStage == POURING &&
           animationList[i][animationIdx[i]][POURING_TO] == idx){
            int c0 = countWater(tubes[i]), pourCnt = (int)animationList[i][animationIdx[i]][POUR_COUNT];
            float pouringProcessRatio = (fabs(tubes[i].angle)-targetAngle[c0])/(targetAngle[c0-pourCnt]-targetAngle[c0]);
            amount += pouringProcessRatio*pourCnt;
        }
    }
    return amount;
}

void initTube(Tube* tube, Rectangle rect, float angle, Color tubeColors[MAX_TUBE_WATER]){
    (*tube).rect = rect;
    (*tube).angle = angle;
    for(int i = 0; i < MAX_TUBE_WATER; i++)
        (*tube).contains[i] = tubeColors[i];
    (*tube).animationStage = STILL;
    // (*tube).animationStage = POURING;
}

void initTubes(Tube* tubes){
    initTube(&tubes[0], (Rectangle){ 100.0, 150.0, TUBE_WIDTH, TUBE_HEIGHT }, 0.0, (Color[]){ BLUE, RED, BLUE, GREEN });
    initTube(&tubes[1], (Rectangle){ 200.0, 150.0, TUBE_WIDTH, TUBE_HEIGHT }, 0.0, (Color[]){ GREEN, RED, RED, BLUE });
    initTube(&tubes[2], (Rectangle){ 300.0, 150.0, TUBE_WIDTH, TUBE_HEIGHT }, 0.0, (Color[]){ GREEN, BLUE, GREEN, RED });
    for(int i = 3; i < TUBE_NUM; i++)
        initTube(&tubes[i], (Rectangle){ 100.0*(i+1), 150.0, TUBE_WIDTH, TUBE_HEIGHT }, 0.0, (Color[]){ BLANK, BLANK, BLANK, BLANK });
}

void initGame(Tube* tubes){
    // init animation settings
    memset(animationList, 0, sizeof(animationList));
    memset(animationIdx, 0, sizeof(animationIdx));
    for(int i = 0; i < TUBE_NUM; i++)
        for(int j = 1; j < MAX_FRAME_NUM; j++)
            animationList[i][j][POURING_TO] = -1;
    // init tubes
    initTubes(tubes);
}

void drawWater(Tube* tubes, int idx){
    // tube info
    int s = 1-2*isPourLeft(tubes[idx].angle); // pour left -> -1, pour right -> 1
    float rad = fabs(tubes[idx].angle*PI/180.0);
    float radius = tubes[idx].rect.width/2.0-TUBE_THICKNESS;
    float waterSurfaceLength = (tubes[idx].rect.width-TUBE_THICKNESS*2)/cos(rad);
    int waterTotal = countWater(tubes[idx]);
    int pourCnt = animationList[idx][animationIdx[idx]][POUR_COUNT];
    // if(waterTotal == 0) return; // 
    if(waterTotal < 0){
        printf("Error: Total water in tube %d < 0", idx);
        exit(-1);
    }

    // bottom water ratio relative to other water colors
    float eachWaterHeight = 1.0*(radius+TUBE_HEIGHT)*WATER_PERCENT/MAX_TUBE_WATER; // to ensure each water has same height
    float bottomWaterRatioBegin = (1.0*TUBE_HEIGHT*WATER_PERCENT-eachWaterHeight*(MAX_TUBE_WATER-1))/eachWaterHeight;
    float bottomWaterRatioEnd = 1.5;
    // printf("begin ratio: %f, end ratio: %f\n", bottomWaterRatioBegin, bottomWaterRatioEnd);

    // drawing info
    float lineDensity = 1.0;    // pixel
    float waveAmplitude = 2.0;  // pixel
    float waveFrequency = 150.0/screenWidth;
    float frameAnimationConstant = 0.1;

    // ratios for determining point positions
    float pourProcessRatio = fabs(tubes[idx].angle)/targetAngle[waterTotal-pourCnt];
    pourProcessRatio = waterLevelAnimation(pourProcessRatio, waterTotal); // from 0 (vertical) to 1 (Reaching the target angle)
    float bottomWaterRatio = (1-pourProcessRatio)*bottomWaterRatioBegin + pourProcessRatio*bottomWaterRatioEnd;
    float curMaxWaterPosRatio = WATER_PERCENT*(bottomWaterRatio+waterTotal-1)/(bottomWaterRatio+MAX_TUBE_WATER-1); // for still case only
    curMaxWaterPosRatio += pourProcessRatio*(1-curMaxWaterPosRatio); // make curMaxWaterPosRatio universal to tilt cases
    // printTubeInfo(tubes[idx], idx);
    // printf("pour process ratio: %f\n", pourProcessRatio);
    // printf("bottom water ratio: %f\n", bottomWaterRatio);
    // printf("current max water position ratio: %f\n", curMaxWaterPosRatio);

    // corner coordinates for inner side
    Vector2 topLeft     = (Vector2){ tubes[idx].rect.x+TUBE_THICKNESS*cos(rad), tubes[idx].rect.y+TUBE_THICKNESS*sin(rad)*s },
            topRight    = (Vector2){ tubes[idx].rect.x+(tubes[idx].rect.width-TUBE_THICKNESS)*cos(rad),
                                     tubes[idx].rect.y+(tubes[idx].rect.width-TUBE_THICKNESS)*sin(rad)*s },
            bottomLeft  = (Vector2){ tubes[idx].rect.x+TUBE_THICKNESS*cos(rad)-tubes[idx].rect.height*sin(rad)*s,
                                     tubes[idx].rect.y+TUBE_THICKNESS*sin(rad)*s+tubes[idx].rect.height*cos(rad) },
            bottomRight = (Vector2){ tubes[idx].rect.x+(tubes[idx].rect.width-TUBE_THICKNESS)*cos(rad)-tubes[idx].rect.height*sin(rad)*s,
                                     tubes[idx].rect.y+(tubes[idx].rect.width-TUBE_THICKNESS)*sin(rad)*s+tubes[idx].rect.height*cos(rad) };
    Vector2 semiCircleCenter = (Vector2){ bottomLeft.x+radius*cos(rad), bottomLeft.y+radius*sin(rad)*s };

    Vector2 bottom      = s < 1 ? bottomLeft  : bottomRight,
            top         = s < 1 ? topLeft     : topRight,
            bottomOppo  = s < 1 ? bottomRight : bottomLeft,
            topOppo     = s < 1 ? topRight    : topLeft;
    Vector2 curMaxWaterPos = (Vector2){ bottom.x*(1-curMaxWaterPosRatio)+top.x*curMaxWaterPosRatio,
                                        bottom.y*(1-curMaxWaterPosRatio)+top.y*curMaxWaterPosRatio };
    Vector2 fullWaterPos = (Vector2){ bottom.x*(1-WATER_PERCENT)+top.x*WATER_PERCENT,
                                      bottom.y*(1-WATER_PERCENT)+top.y*WATER_PERCENT };
    // DrawCircleV(fullWaterPos, 10, ORANGE);

    // add plot for water fall
    if(tubes[idx].animationStage == POURING){
        int to = animationList[idx][animationIdx[idx]][POURING_TO];
        int ptWaterCnt = countWater(tubes[to]); // pouring to water count
        float waterLevelRatio = (bottomWaterRatioBegin+(float)ptWaterCnt-1.0)/(bottomWaterRatioBegin+(float)(MAX_TUBE_WATER-1));

        float fullWaterLevel_y = (tubes[to].rect.y+tubes[to].rect.height)*(1-WATER_PERCENT) + tubes[to].rect.y*(WATER_PERCENT);
        float curWaterLevel_y = ptWaterCnt == 0 ? tubes[to].rect.y+tubes[to].rect.height+radius
                                : fullWaterLevel_y*waterLevelRatio+(tubes[to].rect.y+tubes[to].rect.height)*(1-waterLevelRatio);

        Vector2 pourPos = s > 0 ? topRight : topLeft;
        float waterHeight = curWaterLevel_y-pourPos.y;
        // printf("water height: %f\n", waterHeight);

        // draw falling water column
        DrawRectangleV(pourPos, (Vector2){ TUBE_THICKNESS, waterHeight }, tubes[idx].contains[waterTotal-1]);
    }

    // check if this tube is being poured
    int beingPoured = 0;
    Color pouredCol = BLANK;
    for(int i = 0; i < TUBE_NUM; i++){
        if(animationIdx[i] > 0 && tubes[i].animationStage == POURING && (int)animationList[i][animationIdx[i]][POURING_TO] == idx){
            if(beingPoured == 0){
                beingPoured = 1;
                pouredCol = tubes[i].contains[countWater(tubes[i])-1];
            } else if(!sameColor(tubes[i].contains[countWater(tubes[i])-1], pouredCol)){
                printf("Error: Inconsistent poured color at tube: %d", idx);
                exit(-1);
            }
        }
    }

    if(beingPoured){
        // printf("Tube %d is being poured\n", idx);
        float pourAmount = getPouredAmount(tubes, idx);
        float waterLowLevelRatio = (bottomWaterRatio+(float)waterTotal-1.0)/(bottomWaterRatio+(float)(MAX_TUBE_WATER-1));
        // printf("pour amount: %f\n", pourAmount);

        float lowPos_y;
        Vector2 waterPos;
        if(waterTotal == 0){ // waterLowLevelRatio < 0
            float waterRatio = pourAmount/MAX_TUBE_WATER;
            // printf("water ratio: %f\n", waterRatio);
            lowPos_y = bottom.y+radius;
            waterPos = (Vector2){ topLeft.x, (bottom.y+radius)*(1-waterRatio)+fullWaterPos.y*waterRatio };
        } else {
            float ptWaterLevelRatio = (bottomWaterRatio+waterTotal-1+pourAmount)/(bottomWaterRatio+(float)(MAX_TUBE_WATER-1));
            lowPos_y = fullWaterPos.y*waterLowLevelRatio+bottom.y*(1-waterLowLevelRatio);
            waterPos = (Vector2){ topLeft.x, fullWaterPos.y*ptWaterLevelRatio+bottom.y*(1-ptWaterLevelRatio) };
        }
        // DrawCircleV(waterPos, 10, BLACK);
        // draw poured water from the water fall and the corresponding wave
        if(waterPos.y < semiCircleCenter.y){ // if above semi circle
            // wave
            for(float pixel_x = waterPos.x; pixel_x < waterPos.x+2*radius; pixel_x += lineDensity){
                float waveHeight = waveAmplitude*sin(pixel_x*waveFrequency+frame*frameAnimationConstant);
                Vector2 waveStart = (Vector2){ pixel_x, waterPos.y-waveAmplitude+waveHeight },
                        waveEnd   = (Vector2){ pixel_x, waterPos.y+1.0 };
                DrawLineV(waveStart, waveEnd, pouredCol);
            }
            // water fill
            if(waterTotal == 0){
                DrawRectangleV((Vector2){ waterPos.x, waterPos.y }, (Vector2){ radius*2, bottom.y-waterPos.y }, pouredCol);
                DrawCircleSector(semiCircleCenter, radius, 0.0, 180.0, 1, pouredCol);
            } else {
                DrawRectangleV((Vector2){ waterPos.x, waterPos.y }, (Vector2){ radius*2, lowPos_y-waterPos.y }, pouredCol);
            }
        } else {
            // wave
            float pixel_start = waterPos.x+radius-sqrtf(radius*radius-(semiCircleCenter.y-waterPos.y)*(semiCircleCenter.y-waterPos.y));
            float pixel_end   = waterPos.x+radius+sqrtf(radius*radius-(semiCircleCenter.y-waterPos.y)*(semiCircleCenter.y-waterPos.y));
            for(float pixel_x = pixel_start; pixel_x < pixel_end; pixel_x += lineDensity){
                float waveHeight = waveAmplitude*sin(pixel_x*waveFrequency+frame*frameAnimationConstant);
                Vector2 waveStart = (Vector2){ pixel_x, waterPos.y-waveAmplitude+waveHeight },
                        waveEnd   = (Vector2){ pixel_x, waterPos.y+1.0 };
                DrawLineV(waveStart, waveEnd, pouredCol);
                // DrawCircleV(waveStart, 5, ORANGE);
                // DrawCircleV(waveEnd, 5, PURPLE);
            }
            // water fill
            for(int pixel_y = waterPos.y; pixel_y < semiCircleCenter.y+radius; pixel_y += lineDensity){
                float pixel_start = waterPos.x+radius-sqrtf(radius*radius-(semiCircleCenter.y-pixel_y)*(semiCircleCenter.y-pixel_y));
                float pixel_end   = waterPos.x+radius+sqrtf(radius*radius-(semiCircleCenter.y-pixel_y)*(semiCircleCenter.y-pixel_y));
                DrawLineV((Vector2){ pixel_start, pixel_y },
                          (Vector2){ pixel_end, pixel_y }, pouredCol);
            }
        }
    }

    for(int j = waterTotal-1; j >= 0; j -= (pourCnt && j == waterTotal-1) ? pourCnt : 1){
        Color col = tubes[idx].contains[j];
        Vector2 startPos, endPos;
        float startRatio, endRatio;

        if(tubes[idx].animationStage == POURING){
            // pouring process ratio from 0 (just reach start pouring angle) to 1 (pouring done)
            float pouringProcessRatio = (fabs(tubes[idx].angle)-targetAngle[waterTotal])
                                        /(targetAngle[waterTotal-pourCnt]-targetAngle[waterTotal]);
            float pourAmount = pourCnt*pouringProcessRatio;
            if(j == waterTotal-1){ // for the top water
                startRatio = 1;
                endRatio = (bottomWaterRatio+(float)j-pourCnt)/(bottomWaterRatio+(float)(waterTotal-1)-pourAmount);
            }
            else {
                startRatio = (bottomWaterRatio+(float)j)/(bottomWaterRatio+(float)(waterTotal-1)-pourAmount);
                endRatio = (bottomWaterRatio+(float)j-1)/(bottomWaterRatio+(float)(waterTotal-1)-pourAmount);
            }
        } else if(tubes[idx].animationStage == MOVE_BACK){
            float moveBackProcessRatio = 1-(fabs(tubes[idx].angle)/targetAngle[waterTotal]);
            if(j == waterTotal-1){ // for the top water
                startRatio = 1;
                endRatio = (bottomWaterRatio+(float)j-1)/(bottomWaterRatio+(float)(waterTotal-1));
            }
            else {
                startRatio = (bottomWaterRatio+(float)j)/(bottomWaterRatio+(float)(waterTotal-1));
                endRatio = (bottomWaterRatio+(float)j-1)/(bottomWaterRatio+(float)(waterTotal-1));
            }
        } else if(tubes[idx].animationStage == MOVE_TO){
            if(j == waterTotal-1){ // for the top water
                startRatio = (bottomWaterRatio+(float)j)/(bottomWaterRatio+(float)(waterTotal-1));
                endRatio = (bottomWaterRatio+(float)j-pourCnt)/(bottomWaterRatio+(float)(waterTotal-1));
            } else {
                startRatio = (bottomWaterRatio+(float)j)/(bottomWaterRatio+(float)(waterTotal-1));
                endRatio = (bottomWaterRatio+(float)j-1)/(bottomWaterRatio+(float)(waterTotal-1));
            }
        } else {
            // for vertical cases (STILL, SELECT_PRE, SELECT_DONE, SELECT_RECOVER)
            startRatio = (bottomWaterRatio+(float)j)/(bottomWaterRatio+(float)(waterTotal-1));
            endRatio = (bottomWaterRatio+(float)j-1)/(bottomWaterRatio+(float)(waterTotal-1));
        }

        if(pourCnt == waterTotal) endRatio = 0;
        // printf("start ratio: %f, end ratio: %f\n", startRatio, endRatio);
        startPos = (Vector2){ curMaxWaterPos.x*startRatio+bottom.x*(1-startRatio),
                              curMaxWaterPos.y*startRatio+bottom.y*(1-startRatio) };
        endPos = (Vector2){ curMaxWaterPos.x*endRatio+bottom.x*(1-endRatio),
                            curMaxWaterPos.y*endRatio+bottom.y*(1-endRatio) };
        // DrawCircleV(startPos, 5, BLACK);
        // DrawCircleV(endPos, 5, PINK);
        if(startPos.y < top.y) startPos = top;

        // add water fluctuation for the top color
        if(j == waterTotal-1 && (tubes[idx].animationStage == MOVE_TO || tubes[idx].animationStage == POURING)){
            if(waterTotal == 1 || (waterTotal > 1 && endPos.y > startPos.y)){
                // printf("Drawing wave\n");
                float pixel_start, waveHeight;
                if(startPos.y < bottomOppo.y){
                    pixel_start = startPos.x-s*waterSurfaceLength;
                } else {
                    pixel_start = semiCircleCenter.x-s*sqrtf(radius*radius-(startPos.y-semiCircleCenter.y)*(startPos.y-semiCircleCenter.y));
                }
                for(float pixel_x = pixel_start; s*pixel_x < s*startPos.x; pixel_x += s*lineDensity){
                    waveHeight = waveAmplitude*sin(pixel_x*waveFrequency+frame*frameAnimationConstant);
                    DrawLineV((Vector2){ pixel_x, startPos.y-waveAmplitude+waveHeight },
                              (Vector2){ pixel_x, startPos.y+1.0 }, col);
                }
            }
        }
        
        // main part of the water
        if(j == 0 || (pourCnt > 0 && j == pourCnt-1)){ // the bottom water
            if(startPos.y < bottomOppo.y){
                Vector2 v1 = s > 0 ? (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y } : startPos;
                Vector2 v2 = s > 0 ? startPos : (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y };
                DrawTriangle(v1, bottomOppo, v2, col);
                v1 = s < 0 ? (Vector2){ bottomOppo.x+s*waterSurfaceLength, bottomOppo.y } : bottomOppo;
                v2 = s < 0 ? bottomOppo : (Vector2){ bottomOppo.x+s*waterSurfaceLength, bottomOppo.y };
                DrawTriangle(v1, v2, startPos, col);
            }
            for(float pixel_y = startPos.y; pixel_y < bottom.y; pixel_y += lineDensity){
                float pixel_x1 = semiCircleCenter.x-s*sqrtf(radius*radius-(pixel_y-semiCircleCenter.y)*(pixel_y-semiCircleCenter.y));
                float pixel_x2 = startPos.x+(startPos.x-bottom.x)/(startPos.y-bottom.y)*(pixel_y-startPos.y);
                DrawLineV((Vector2){ pixel_x1, pixel_y }, (Vector2){ pixel_x2, pixel_y }, col);
            }
            for(float pixel_y = bottom.y; pixel_y < semiCircleCenter.y+radius; pixel_y += lineDensity){
                float pixel_x1 = semiCircleCenter.x-sqrtf(radius*radius-(pixel_y-semiCircleCenter.y)*(pixel_y-semiCircleCenter.y));
                float pixel_x2 = semiCircleCenter.x+sqrtf(radius*radius-(pixel_y-semiCircleCenter.y)*(pixel_y-semiCircleCenter.y));
                DrawLineV((Vector2){ pixel_x1, pixel_y }, (Vector2){ pixel_x2, pixel_y }, col);
            }
        } else {
            // DrawCircleV(endPos, 5, ORANGE);
            if(endPos.y < bottomOppo.y){
                Vector2 v1 = s > 0 ? (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y } : startPos;
                Vector2 v2 = s > 0 ? startPos : (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y };
                DrawTriangle(v1, endPos, v2, col);
                v1 = s > 0 ? (Vector2){ endPos.x-s*waterSurfaceLength, endPos.y } : endPos;
                v2 = s > 0 ? endPos : (Vector2){ endPos.x-s*waterSurfaceLength, endPos.y };
                DrawTriangle((Vector2){ startPos.x-s*waterSurfaceLength, startPos.y }, v1, v2, col);
            } else { // endPos.y > bottomOppo.y
                if(startPos.y < bottomOppo.y){
                    Vector2 v1 = s > 0 ? (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y } : startPos;
                    Vector2 v2 = s > 0 ? startPos : (Vector2){ startPos.x-s*waterSurfaceLength, startPos.y };
                    DrawTriangle(v1, bottomOppo, v2, col);
                    v1 = s < 0 ? (Vector2){ bottomOppo.x+s*waterSurfaceLength, bottomOppo.y } : bottomOppo;
                    v2 = s < 0 ? bottomOppo : (Vector2){ bottomOppo.x+s*waterSurfaceLength, bottomOppo.y };
                    DrawTriangle(v1, v2, startPos, col);
                }
                for(float pixel_y = startPos.y; pixel_y < endPos.y; pixel_y += lineDensity){
                    float pixel_x1 = semiCircleCenter.x-s*sqrtf(radius*radius-(pixel_y-semiCircleCenter.y)*(pixel_y-semiCircleCenter.y));
                    float pixel_x2 = startPos.x+(startPos.x-endPos.x)/(startPos.y-endPos.y)*(pixel_y-startPos.y);
                    DrawLineV((Vector2){ pixel_x1, pixel_y }, (Vector2){ pixel_x2, pixel_y }, col);
                }
            }
        }
    }
}

void drawTubeWall(Tube* tubes, int idx){
    DrawRectanglePro((Rectangle){ tubes[idx].rect.x, tubes[idx].rect.y, TUBE_THICKNESS, tubes[idx].rect.height },
                     (Vector2){ 0.0, 0.0 }, tubes[idx].angle, TUBE_WALL_COLOR);
    DrawRectanglePro((Rectangle){ tubes[idx].rect.x+(tubes[idx].rect.width-TUBE_THICKNESS)*cos(tubes[idx].angle*PI/180.0),
                                  tubes[idx].rect.y+(tubes[idx].rect.width-TUBE_THICKNESS)*sin(tubes[idx].angle*PI/180.0),
                                  TUBE_THICKNESS,
                                  tubes[idx].rect.height },
                     (Vector2){ 0.0, 0.0 }, tubes[idx].angle, TUBE_WALL_COLOR);
    DrawRing((Vector2){ tubes[idx].rect.x+tubes[idx].rect.width/2*cos(tubes[idx].angle*PI/180.0)-tubes[idx].rect.height*sin(tubes[idx].angle*PI/180.0),
                        tubes[idx].rect.y+tubes[idx].rect.width/2*sin(tubes[idx].angle*PI/180.0)+tubes[idx].rect.height*cos(tubes[idx].angle*PI/180.0) },
             tubes[idx].rect.width/2, tubes[idx].rect.width/2-TUBE_THICKNESS,
             tubes[idx].angle, tubes[idx].angle+180.0, 1, TUBE_WALL_COLOR);
}

void drawTubes(Tube* tubes){
    // plot still tubes
    for(int i = 0; i < TUBE_NUM; i++){
        if(animationIdx[i] == 0){
            drawWater(tubes, i);
            drawTubeWall(tubes, i);
        }
    }
    // plot moving tubes
    for(int i = 0; i < TUBE_NUM; i++){
        if(animationIdx[i] > 0){
            drawWater(tubes, i);
            drawTubeWall(tubes, i);
        }
    }
}

void selectTube(Tube* tubes, int tubeIdx){
    // if(animationIdx[tubeIdx] > 0) return;
    selectedTube = tubeIdx;
    // add move up animation
    int idx = FRAME_SELECT;
    float offset = HEIGHT_SELECT/FRAME_SELECT;
    animationIdx[tubeIdx] = FRAME_SELECT;
    for(int i = 1; i < FRAME_SELECT; i++){
        animationList[tubeIdx][idx][RECT_X] = tubes[tubeIdx].rect.x;
        animationList[tubeIdx][idx][RECT_Y] = tubes[tubeIdx].rect.y-i*offset;
        animationList[tubeIdx][idx][ANGLE] = 0.0;
        animationList[tubeIdx][idx][ANIMATION_STAGE] = SELECT_PRE;
        animationList[tubeIdx][idx][POURING_TO] = -1;
        animationList[tubeIdx][idx][POUR_COUNT] = 0;
        idx--;
    }
    animationList[tubeIdx][1][RECT_X] = tubes[tubeIdx].rect.x;
    animationList[tubeIdx][1][RECT_Y] = tubes[tubeIdx].rect.y-FRAME_SELECT*offset;
    animationList[tubeIdx][1][ANGLE] = 0.0;
    animationList[tubeIdx][1][ANIMATION_STAGE] = SELECT_DONE;
    animationList[tubeIdx][1][POURING_TO] = -1;
    animationList[tubeIdx][1][POUR_COUNT] = 0;
    // for(int i = 1; i <= animationIdx[tubeIdx]; i++)
    //     printAnimationInfo(animationList[tubeIdx][i]);
}

void deselectTube(Tube* tubes, int tubeIdx){
    // if(animationIdx[tubeIdx] > 0) return;
    float offset = HEIGHT_SELECT/FRAME_SELECT;
    int idx = FRAME_SELECT-animationIdx[tubeIdx];
    // printf("In deselect:\n");
    memset(animationList[tubeIdx], 0, sizeof(animationList[tubeIdx]));
    animationIdx[tubeIdx] = idx;
    for(int i = idx; i > 1; i--){
        animationList[tubeIdx][i][RECT_X] = tubes[tubeIdx].rect.x;
        animationList[tubeIdx][i][RECT_Y] = tubes[tubeIdx].rect.y+(idx-i+1)*offset;
        animationList[tubeIdx][i][ANGLE] = 0.0;
        animationList[tubeIdx][i][ANIMATION_STAGE] = SELECT_PRE;
    }
    animationList[tubeIdx][1][RECT_X] = tubes[tubeIdx].rect.x;
    animationList[tubeIdx][1][RECT_Y] = tubes[tubeIdx].rect.y+idx*offset;
    animationList[tubeIdx][1][ANGLE] = 0.0;
    animationList[tubeIdx][1][ANIMATION_STAGE] = STILL;
    // for(int i = 1; i <= idx; i++)
    //     printAnimationInfo(animationList[tubeIdx][i]);
}

bool checkPour(Tube* tubes, int from, int to){
    // printf("checking pour:\n");
    // printTubeInfo(tubes[from], from);
    // printTubeInfo(tubes[to], to);
    if(tubes[from].animationStage != SELECT_PRE && tubes[from].animationStage != SELECT_DONE)
        return false;
    if(tubes[to].animationStage != STILL)
        return false;
    // if other tubes are pouring, check the pouring condition
    int curWaterTotal = countWater(tubes[to]), pourWaterTotal = 0;
    for(int i = 0; i < TUBE_NUM; i++){
        if(i == from || i == to) continue;
        if(animationIdx[i] > 0 && tubes[i].animationStage == POURING && animationList[i][animationIdx[i]][POURING_TO] == to)
            pourWaterTotal += (int)animationList[i][animationIdx[i]][POUR_COUNT];
    }
    int pourCnt = countWater(tubes[from]);
    if(curWaterTotal == MAX_TUBE_WATER || pourCnt == 0)
        return false;
    if(curWaterTotal > 0 && !sameColor(tubes[from].contains[pourCnt-1], tubes[to].contains[curWaterTotal-1]))
        return false;
    return curWaterTotal+pourWaterTotal < MAX_TUBE_WATER;
}

void pour(Tube* tubes, int from, int to){
    // if(animationIdx[from] > 0) return;
    int c1 = countWater(tubes[from]), c2 = countWater(tubes[to]);
    float tarX, tarY, fullX, fullY, tarAngle, fullAngle;
    int pourCnt = 0;
    if(c2 == 0){
        for(int i = c1-2; i >= 0 && sameColor(tubes[from].contains[i], tubes[from].contains[c1-1]); i--)
            pourCnt++;
        pourCnt++;
    } else {
        for(int i = c1-1; i >= 0 && sameColor(tubes[from].contains[i], tubes[to].contains[c2-1]); i--)
            pourCnt++;
    }
    pourCnt = min(pourCnt, MAX_TUBE_WATER-c2);
    bool pourRight = tubes[from].rect.x < tubes[to].rect.x;
    if(pourRight){ // pour right
        tarAngle = targetAngle[c1-pourCnt];
        tarX = tubes[to].rect.x+tubes[to].rect.width/2.0-tubes[from].rect.width*cos(tarAngle*PI/180.0);
        tarY = tubes[to].rect.y-HEIGHT_POUR-tubes[from].rect.width*sin(tarAngle*PI/180.0);
        fullAngle = targetAngle[c1];
        fullX = tubes[to].rect.x+tubes[to].rect.width/2.0-tubes[from].rect.width*cos(fullAngle*PI/180.0);
        fullY = tubes[to].rect.y-HEIGHT_POUR-tubes[from].rect.width*sin(fullAngle*PI/180.0);
    } else {
        tarAngle = -targetAngle[c1-pourCnt];
        tarX = tubes[to].rect.x+tubes[to].rect.width/2.0;
        tarY = tubes[to].rect.y-HEIGHT_POUR;
        fullAngle = -targetAngle[c1];
        fullX = tubes[to].rect.x+tubes[to].rect.width/2.0;
        fullY = tubes[to].rect.y-HEIGHT_POUR;
    }
    // printf("Pouring Animation info:\n");
    // printf("from %d, to %d, pour count: %d, target angle: %f, full angle: %f\n", from, to, pourCnt, tarAngle, fullAngle);
    // push the pouring animation to animation list
    
    int idx = FRAME_MOVE*2+FRAME_POUR+1;
    animationIdx[from] = idx;
    
    // add pouring information
    for(int i = 1; i <= animationIdx[from]; i++){
        animationList[from][i][POURING_TO] = to;
        animationList[from][i][POUR_COUNT] = pourCnt;
    }
    
    // move animation to reach the top of target tube
    for(int i = 0; i < FRAME_MOVE; i++){
        animationList[from][idx][RECT_X] = tubes[from].rect.x+i*(fullX-tubes[from].rect.x)/FRAME_MOVE;
        animationList[from][idx][RECT_Y] = tubes[from].rect.y+i*(fullY-tubes[from].rect.y)/FRAME_MOVE;
        animationList[from][idx][ANGLE] = tubes[from].angle+i*(fullAngle-tubes[from].angle)/FRAME_MOVE;
        animationList[from][idx][ANIMATION_STAGE] = MOVE_TO;
        idx--;
    }

    // pouring animation
    for(int i = 0; i < FRAME_POUR; i++){
        animationList[from][idx][ANGLE] = fullAngle+i*(tarAngle-fullAngle)/FRAME_POUR;
        animationList[from][idx][ANIMATION_STAGE] = POURING;
        if(pourRight){
            float topRightX = animationList[from][idx+1][RECT_X]+tubes[from].rect.width*cos(animationList[from][idx+1][ANGLE]*PI/180.0),
                  topRightY = animationList[from][idx+1][RECT_Y]+tubes[from].rect.width*sin(animationList[from][idx+1][ANGLE]*PI/180.0);
            animationList[from][idx][RECT_X] = topRightX-tubes[from].rect.width*cos(animationList[from][idx][ANGLE]*PI/180.0);
            animationList[from][idx][RECT_Y] = topRightY-tubes[from].rect.width*sin(animationList[from][idx][ANGLE]*PI/180.0);
        } else {
            animationList[from][idx][RECT_X] = animationList[from][idx+1][RECT_X];
            animationList[from][idx][RECT_Y] = animationList[from][idx+1][RECT_Y];
        }
        idx--;
    }

    // move back animation
    for(int i = 0; i < FRAME_MOVE; i++){
        animationList[from][idx][RECT_X] = tarX-i*(tarX-tubes[from].rect.x)/FRAME_MOVE;
        animationList[from][idx][RECT_Y] = tarY-i*(tarY-tubes[from].rect.y)/FRAME_MOVE;
        animationList[from][idx][ANGLE] = tarAngle-i*(tarAngle-tubes[from].angle)/FRAME_MOVE;
        animationList[from][idx][ANIMATION_STAGE] = MOVE_BACK;
        if(i > 0) animationList[from][idx][POUR_COUNT] = 0; // when POURING stage done, POUR_COUNT is set to 0
        idx--;
    }
    if(idx != 1){
        printf("Error: wrong animation index %d\n", idx);
        exit(0);
    }
    
    animationList[from][idx][RECT_X] = tubes[from].rect.x;
    animationList[from][idx][RECT_Y] = tubes[from].rect.y+HEIGHT_SELECT;
    animationList[from][idx][ANGLE] = 0.0;
    animationList[from][idx][ANIMATION_STAGE] = STILL;
    animationList[from][idx][POURING_TO] = -1;
    animationList[from][idx][POUR_COUNT] = 0.0;
    
    // printf("animation length: %d\n", animationIdx[from]);
    // for(int i = animationIdx[from]; i >= 1; i--){
    //     printAnimationInfo(animationList[from][i], i);
    // }
    // exit(0);
}

void updateTubes(Tube* tubes){
    int idx;
    for(int i = 0; i < TUBE_NUM; i++){
        idx = animationIdx[i];
        if(idx == 0) continue;
        // printf("idx: %d, stage: %.0f, last stage: %.0f\n", idx, animationList[i][idx][ANIMATION_STAGE], animationList[i][1+idx][ANIMATION_STAGE]);

        // update actual water amount when pouring stage complete
        if(animationList[i][idx][ANIMATION_STAGE] == MOVE_BACK && animationList[i][idx+1][ANIMATION_STAGE] == POURING){
            // printf("updating water count!\n");
            // exit(0);
            int to = animationList[i][idx][POURING_TO];
            int c1 = countWater(tubes[i]), c2 = countWater(tubes[to]);
            for(int j = 0; j < animationList[i][idx][POUR_COUNT]; j++){
                tubes[to].contains[c2+j] = tubes[i].contains[c1-j-1];
                tubes[i].contains[c1-j-1] = BLANK;
            }
        }
        tubes[i].rect.x = animationList[i][idx][RECT_X];
        tubes[i].rect.y = animationList[i][idx][RECT_Y];
        tubes[i].angle = animationList[i][idx][ANGLE];
        tubes[i].animationStage = animationList[i][idx][ANIMATION_STAGE];
        // memset(animationList[i][idx], 0, sizeof(animationList[i][idx]));
        animationIdx[i]--;
    }
}

bool gameEnd(Tube* tubes){
    for(int i = 0; i < TUBE_NUM; i++)
        if(animationIdx[i] > 0) return false; // finish all the animation
    for(int i = 0; i < TUBE_NUM; i++){
        if(countWater(tubes[i]) == MAX_TUBE_WATER){
            for(int j = 1; j < MAX_TUBE_WATER; j++)
                if(!sameColor(tubes[i].contains[j], tubes[i].contains[j-1])) return false;
        } else if(countWater(tubes[i]) != 0) return false;
    }
    return true;
}

bool pouredTo(Tube* tubes, int idx){
    for(int i = 0; i < TUBE_NUM; i++){
        if(i == idx) continue;
        for(int j = 1; j <= animationIdx[i]; j++)
            if(animationList[i][j][POURING_TO] == idx) return true;
    }
    return false;
}