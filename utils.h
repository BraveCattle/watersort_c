#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265358979323846
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define MAX_TUBE_WATER      4
#define MAX_TUBE_NUM        20
#define MAX_FRAME_NUM       240
#define ANIMATION_INFO_LENGTH 6
#define TUBE_WALL_COLOR     DARKBROWN
#define BACKGROUND_COLOR    DARKGRAY

// game related global variables
extern int frame;
extern int screenWidth;
extern int screenHeight;

// tube related global variables
extern int selectedTube;
extern int TUBE_NUM;
extern int TUBE_THICKNESS;
extern float TUBE_WIDTH;
extern float TUBE_HEIGHT;
extern float WATER_PERCENT;
extern float targetAngle[];

// animation related global variables
extern float HEIGHT_SELECT;
extern float HEIGHT_POUR;
extern int FRAME_SELECT; // # of frames for pulling up a tube
extern int FRAME_MOVE;
extern int FRAME_POUR; // # of frames for pouring water
extern float animationList[MAX_TUBE_NUM][MAX_FRAME_NUM][ANIMATION_INFO_LENGTH];
extern int animationIdx[MAX_TUBE_NUM];

typedef struct Tube {
    Rectangle rect;
    Color contains[MAX_TUBE_WATER];
    float angle;
    int animationStage;
}Tube;

typedef enum {
    STILL           = 0,
    SELECT_PRE      = 1,
    SELECT_DONE     = 2,
    SELECT_RECOVER  = 3,
    MOVE_TO         = 4,
    POURING         = 5, // when pouring, play waterfall animation
    MOVE_BACK       = 6
} TubeAnimationStage;

typedef enum {
    RECT_X          = 0,
    RECT_Y          = 1,
    ANGLE           = 2,
    ANIMATION_STAGE = 3,
    POURING_TO      = 4,
    POUR_COUNT      = 5
} AnimationListContent;

bool sameColor(Color x, Color y);
bool emptyColor(Color c);
int countWater(Tube tube);
bool insideTube(Vector2 pos, Tube tube);
void copyAnimation(float* dst, float src[ANIMATION_INFO_LENGTH]);

int isPourLeft(float angle);
float waterLevelAnimation(float pourProcessRatio, int waterTotal);
float getPouredAmount(Tube* tubes, int idx);

void printTubeInfo(Tube tube, int idx);
void printAnimationInfo(float info[ANIMATION_INFO_LENGTH], int idx);
void initTube(Tube* tube, Rectangle rect, float angle, Color tubeColors[MAX_TUBE_WATER]);
void initTubes(Tube* tubes);
void initGame(Tube* tubes);

void drawWater(Tube* tubes, int idx);
void drawTubes(Tube* tubes);

bool pouredTo(Tube* tubes, int idx);
bool gameEnd(Tube* tubes);
void selectTube(Tube* tubes, int tubeIdx);
void deselectTube(Tube* tubes, int tubeIdx);
bool checkPour(Tube* tubes, int from, int to);
void pour(Tube* tubes, int from, int to);
void updateTubes(Tube* tubes);

#endif // UTILS_H