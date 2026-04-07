#include "raylib.h"
#include <bits/stdc++.h>
using namespace std;

// ──────────────────────────────────────────────
//  WINDOW & GRID
// ──────────────────────────────────────────────
static const int SW = 1100;
static const int SH = 750;
static const int COLS = 15;
static const int ROWS = 10;
static const int CELL = 52;
static const int MX = 55;
static const int MY = 58;

// ──────────────────────────────────────────────
//  GAME STATES
// ──────────────────────────────────────────────
enum State
{
    S_MENU,
    S_STORY,
    S_L1_PLAY,
    S_L1_EXPLAIN,
    S_L2_INTRO,
    S_L2_PLAY,
    S_L3_INTRO,
    S_L3_PLAY,
    S_L3_CAUGHT,
    S_L3_EXPLAIN,
    S_L4_INTRO,
    S_L4_PLAY,
    S_L4_EXPLAIN,
    S_WIN
};
State gState = S_MENU;

// ──────────────────────────────────────────────
//  GLOBALS
// ──────────────────────────────────────────────
int score = 0;
float gTimer = 0.f;
float stTimer = 0.f;

Texture2D userTex, guardTex;
bool chooseDP_L1 = true;
bool chooseQuick_L3 = true;
bool chooseDP_L4 = true;

//ANMOL's PARTT 

// ──────────────────────────────────────────────
//  KNAPSACK
// ──────────────────────────────────────────────
static const int KN = 5, KW = 8;
static int kwt[KN] = {2, 3, 4, 5, 1};
static int kval[KN] = {3, 4, 5, 6, 2};
static const char *kname[KN] = {"Flashbang", "EMP", "MediKit", "Grapple", "Lockpick"};
static int dpTable[KN + 1][KW + 1];
static int dpAniI = 1, dpAniW = 0;
static bool dpDone = false;

static void initKS()
{
    memset(dpTable, 0, sizeof(dpTable));
    dpAniI = 1;
    dpAniW = 0;
    dpDone = false;
}
static void stepDP()
{
    if (dpDone)
        return;
    int i = dpAniI, w = dpAniW;
    dpTable[i][w] = (kwt[i - 1] <= w) ? max(dpTable[i - 1][w], kval[i - 1] + dpTable[i - 1][w - kwt[i - 1]]) : dpTable[i - 1][w];
    if (++dpAniW > KW)
    {
        dpAniW = 0;
        if (++dpAniI > KN)
            dpDone = true;
    }
}
static int greedyVal()
{
    vector<pair<float, int>> r;
    for (int i = 0; i < KN; i++)
        r.push_back({(float)kval[i] / kwt[i], i});
    sort(r.rbegin(), r.rend());
    int rem = KW, v = 0;
    for (auto [ratio, i] : r)
        if (kwt[i] <= rem)
        {
            rem -= kwt[i];
            v += kval[i];
        }
    return v;
}

//VASANT's PART

// ──────────────────────────────────────────────
//  MAIN
// ──────────────────────────────────────────────
int main()
{

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "The Heist & Escape Loop — DAA Game | Team CodeX");
    SetTargetFPS(60);

    userTex = LoadTexture("user_photo.png");
    guardTex = LoadTexture("guard_photo.png");

    initMaze();
    initKS();

    bool showHint = false;
    vector<pair<int, int>> hintPath;
    bool l4Ready = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        gTimer += dt;
        stTimer += dt;
        Vector2 mouse = GetMousePosition();

        // ── UPDATE ──────────────────────
        switch (gState)
        {

        case S_MENU:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, {(SW - 220) / 2.f, 400, 220, 56}))
                {
                    gState = S_STORY;
                    stTimer = 0;
                }
            }
            break;

        case S_STORY:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L1_PLAY;
                stTimer = 0;
                initKS();
            }
            break;

        case S_L1_PLAY:
            if (!dpDone)
                stepDP();
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, {90.f, 680, 220, 50}))
                {
                    chooseDP_L1 = false;
                    score += 80;
                    gState = S_L1_EXPLAIN;
                    stTimer = 0;
                }
                if (CheckCollisionPointRec(mouse, {SW - 310.f, 680, 220, 50}))
                {
                    chooseDP_L1 = true;
                    score += 200;
                    gState = S_L1_EXPLAIN;
                    stTimer = 0;
                }
            }
            break;

        case S_L1_EXPLAIN:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L2_INTRO;
                stTimer = 0;
                initMaze();
                pPos = {0, 0};
                showHint = false;
            }
            break;

        case S_L2_INTRO:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L2_PLAY;
                stTimer = 0;
            }
            break;

        case S_L2_PLAY:
        {
            int px = (int)pPos.x, py = (int)pPos.y;
            if (IsKeyPressed(KEY_H))
            {
                hintPath = mazeBFS(px, py, COLS - 1, ROWS - 1);
                showHint = true;
            }
            bool mv = false;
            if (IsKeyPressed(KEY_RIGHT) && (mazeW[py][px] & 4) && px < COLS - 1)
            {
                pPos.x++;
                mv = true;
            }
            if (IsKeyPressed(KEY_LEFT) && (mazeW[py][px] & 8) && px > 0)
            {
                pPos.x--;
                mv = true;
            }
            if (IsKeyPressed(KEY_DOWN) && (mazeW[py][px] & 2) && py < ROWS - 1)
            {
                pPos.y++;
                mv = true;
            }
            if (IsKeyPressed(KEY_UP) && (mazeW[py][px] & 1) && py > 0)
            {
                pPos.y--;
                mv = true;
            }
            if (mv)
            {
                score += 2;
                showHint = false;
            }
            if ((int)pPos.x == COLS - 1 && (int)pPos.y == ROWS - 1)
            {
                score += 100;
                gState = S_L3_INTRO;
                stTimer = 0;
                pPos = {0, 0};
                gPos = {0, 0};
                guardTimer = 0;
                stealthTime = 0;
                noiseLevel = 0;
                noiseArr = {55, 12, 78, 34, 90, 23, 67, 45, 11, 88};
                bbI = 0;
                bbJ = 0;
                bbDone = false;
            }
            break;
        }

        case S_L3_INTRO:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, {200.f, SH - 90, 220, 52}))
                    chooseQuick_L3 = false;
                if (CheckCollisionPointRec(mouse, {SW - 420.f, SH - 90, 220, 52}))
                    chooseQuick_L3 = true;
            }
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L3_PLAY;
                stTimer = 0;
                stealthTime = 0;
            }
            break;

        case S_L3_PLAY:
        {
            stealthTime += dt;
            int px = (int)pPos.x, py = (int)pPos.y;
            if (IsKeyPressed(KEY_RIGHT) && (mazeW[py][px] & 4) && px < COLS - 1)
                pPos.x++;
            if (IsKeyPressed(KEY_LEFT) && (mazeW[py][px] & 8) && px > 0)
                pPos.x--;
            if (IsKeyPressed(KEY_DOWN) && (mazeW[py][px] & 2) && py < ROWS - 1)
                pPos.y++;
            if (IsKeyPressed(KEY_UP) && (mazeW[py][px] & 1) && py > 0)
                pPos.y--;
            if (chooseQuick_L3)
            {
                vector<int> tmp = noiseArr;
                qsort2(tmp, 0, (int)tmp.size() - 1);
                noiseArr = tmp;
                noiseLevel = max(0, noiseLevel - 2);
            }
            else
            {
                bubbleStep(noiseArr);
                noiseLevel = min(100, noiseLevel + 3);
            }
            float gDelay = chooseQuick_L3 ? 0.55f : 0.28f;

            // ⛔ Guard will NOT move for first 5 seconds
            if (stealthTime > 5.0f)
            {
                guardTimer += dt;

                if (guardTimer >= gDelay)
                {
                    guardTimer = 0;

                    auto path = mazeBFS((int)gPos.x, (int)gPos.y, (int)pPos.x, (int)pPos.y);

                    if (!path.empty())
                    {
                        gPos.x = path[0].first;
                        gPos.y = path[0].second;
                    }
                }
            }
            // ⛔ Can't be caught in first 5 sec
            if (stealthTime > 5.0f &&
                (int)gPos.x == (int)pPos.x &&
                (int)gPos.y == (int)pPos.y)
            {
                gState = S_L3_CAUGHT;
                stTimer = 0;
            }
            if ((int)pPos.x == COLS - 1 && (int)pPos.y == ROWS - 1)
            {
                score += 150 + max(0, 100 - (int)stealthTime);
                gState = S_L3_EXPLAIN;
                stTimer = 0;
            }
            break;
        }

        case S_L3_CAUGHT:
            if (IsKeyPressed(KEY_R))
            {
                pPos = {0, 0};
                gPos = {COLS - 2, 0};
                guardTimer = 0;
                stealthTime = 0;
                noiseLevel = 0;
                noiseArr = {55, 12, 78, 34, 90, 23, 67, 45, 11, 88};
                bbI = 0;
                bbJ = 0;
                bbDone = false;
                gState = S_L3_PLAY;
                stTimer = 0;
            }
            if (IsKeyPressed(KEY_M))
            {
                score = 0;
                gState = S_MENU;
            }
            break;

        case S_L3_EXPLAIN:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L4_INTRO;
                stTimer = 0;
                l4Ready = false;
            }
            break;

        case S_L4_INTRO:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L4_PLAY;
                stTimer = 0;
                l4Ready = true;
            }
            break;

        case S_L4_PLAY:
            if (l4Ready && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, {120.f, 630, 240, 56}))
                {
                    chooseDP_L4 = false;
                    solveTSP();
                    gState = S_L4_EXPLAIN;
                    stTimer = 0;
                }
                if (CheckCollisionPointRec(mouse, {SW - 360.f, 630, 240, 56}))
                {
                    chooseDP_L4 = true;
                    solveTSP();
                    gState = S_L4_EXPLAIN;
                    stTimer = 0;
                }
            }
            break;

        case S_L4_EXPLAIN:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_WIN;
                stTimer = 0;
            }
            break;

        case S_WIN:
            if (IsKeyPressed(KEY_M))
            {
                score = 0;
                gState = S_MENU;
            }
            break;

        default:
            break;
        }

        // ── DRAW ────────────────────────
        BeginDrawing();

        if (gState == S_MENU)
        {
            DrawBG({5, 5, 18, 255}, {20, 5, 40, 255});
            float p = 1.f + 0.04f * sinf(gTimer * 2.f);
            DC("THE HEIST", (int)(80), (int)(54 * p), RED);
            DC("& ESCAPE LOOP", (int)(148), (int)(46 * p - 2), {255, 80, 0, 255});
            DC("An Interactive DAA Learning Experience", 208, 18, {150, 150, 180, 255});
            if (userTex.id)
                DrawTexturePro(userTex, {0, 0, (float)userTex.width, (float)userTex.height}, {60, 285, 108, 108}, {0, 0}, 0, WHITE);
            if (guardTex.id)
                DrawTexturePro(guardTex, {0, 0, (float)guardTex.width, (float)guardTex.height}, {SW - 168.f, 285, 108, 108}, {0, 0}, 0, WHITE);
            bool hov = CheckCollisionPointRec(mouse, {(SW - 220) / 2.f, 400, 220, 56});
            DrawRectangleRounded({(SW - 220) / 2.f, 400, 220, 56}, 0.3f, 8, hov ? (Color){255, 210, 0, 255} : (Color){200, 40, 40, 255});
            DC("START MISSION", 414, 26, BLACK);
            DC("Team CodeX  DAA-IV-T049 | Graphic Era University, Dehradun", SH - 26, 13, {70, 70, 90, 255});
        }
        else if (gState == S_STORY)
        {
            DrawBG({8, 5, 22, 255}, {5, 18, 38, 255});
            DrawHUD();
            DC("MISSION BRIEFING", 72, 32, GOLD);
            Panel(80, 130, SW - 160, 480, {14, 14, 36, 230}, {55, 55, 110, 255});
            const char *st[] = {
                "You are GHOST:  elite hacker and data thief.",
                "",
                "Your target: The Secure Nexus Data Vault.",
                "Four security systems stand in your way.",
                "Each one is powered by a real DAA algorithm.",
                "",
                "Level 1  — Pack your loadout  (0/1 Knapsack: Greedy vs DP)",
                "Level 2  — Navigate the maze  (BFS pathfinding)",
                "Level 3  — Evade the guard    (Sorting + BFS guard AI)",
                "Level 4  — Bypass laser grid  (TSP: Held-Karp DP vs Brute Force)",
                "",
                "After EVERY choice you will see a full explanation of",
                "WHY that algorithm scored more or less.",
                "",
                "Better algorithm  =  more points  =  deeper understanding.",
                "No levels after Level 4. Just Mission Accomplished.",
            };
            for (int i = 0; i < 16; i++)
                DrawText(st[i], 105, 155 + i * 28, 17, (i == 0 || i == 6 || i == 7 || i == 8 || i == 9) ? (Color){GOLD.r, GOLD.g, GOLD.b, 255} : WHITE);
            DC("[SPACE]  Begin the Heist", SH - 50, 22, {0, 220, 80, 255});
        }
        else if (gState == S_L1_PLAY)
        {
            DrawBG({6, 6, 20, 255}, {16, 6, 32, 255});
            DrawHUD();
            DC("LEVEL 1:  PACK YOUR LOADOUT", 60, 26, GOLD);
            Panel(20, 80, 278, 524, {18, 18, 42, 230}, SKYBLUE);
            DrawText("Item Inventory", 34, 94, 17, SKYBLUE);
            DrawLine(34, 114, 290, 114, {40, 60, 100, 255});
            DrawText("Name       Wt  Val V/W", 34, 120, 14, LIGHTGRAY);
            for (int i = 0; i < KN; i++)
            {
                char buf[64];
                sprintf(buf, "%-10s %d   %d  %.1f", kname[i], kwt[i], kval[i], (float)kval[i] / kwt[i]);
                DrawText(buf, 34, 140 + i * 34, 15, WHITE);
            }
            char cb[32];
            sprintf(cb, "Capacity: %d kg", KW);
            DrawText(cb, 34, 330, 16, GOLD);
            int gv = greedyVal();
            DrawText("Greedy value:", 34, 365, 15, ORANGE);
            char gvb[32];
            sprintf(gvb, "%d", gv);
            DrawText(gvb, 34, 385, 22, ORANGE);
            DrawText("DP optimal:", 34, 422, 15, GREEN);
            char dvb[32];
            sprintf(dvb, "%d", dpTable[KN][KW]);
            DrawText(dvb, 34, 442, 22, GREEN);
            if (gv < dpTable[KN][KW])
                DrawText("Greedy missed\nthe optimal!", 34, 480, 14, RED);
            else
                DrawText("Both agree!", 34, 480, 14, GREEN);
            Panel(310, 80, SW - 332, 344, {12, 28, 12, 230}, {60, 180, 60, 255});
            DrawText("DP Table: dp[item][weight]  (animating...)", 322, 90, 14, {80, 220, 80, 255});
            for (int w = 0; w <= KW; w++)
            {
                char h[8];
                sprintf(h, "w=%d", w);
                DrawText(h, 328 + w * 46, 106, 11, LIGHTGRAY);
            }
            for (int i = 0; i <= KN; i++)
            {
                char rh[8];
                sprintf(rh, "i=%d", i);
                DrawText(rh, 312, 120 + i * 45, 11, LIGHTGRAY);
                for (int w = 0; w <= KW; w++)
                {
                    int tx = 326 + w * 46, ty = 113 + i * 45;
                    bool isA = (i == dpAniI && w == dpAniW), isPa = (i < dpAniI) || (i == dpAniI && w < dpAniW);
                    Color bg = isA ? YELLOW : isPa ? (Color){40, 160, 40, 255}
                                                   : (Color){26, 26, 52, 220};
                    DrawRectangle(tx, ty, 40, 30, bg);
                    DrawRectangleLines(tx, ty, 40, 30, DARKGRAY);
                    char v[8];
                    sprintf(v, "%d", dpTable[i][w]);
                    DrawText(v, tx + 7, ty + 7, 13, isA ? BLACK : WHITE);
                }
            }
            Panel(310, 438, SW - 332, 158, {20, 10, 30, 220}, {120, 80, 180, 255});
            DrawText("Complexity Info", 322, 448, 16, {180, 120, 255, 255});
            DrawText("DP:     O(n x W) time   O(n x W) space", 322, 470, 14, {80, 200, 80, 255});
            DrawText("Greedy: O(n log n) time  O(n) space", 322, 490, 14, ORANGE);
            DrawText("Key: Greedy FAILS for 0/1 knapsack — items can't be split!", 322, 514, 13, {255, 120, 80, 255});
            DrawText("Clicking either button shows you a full WHY explanation!", 322, 534, 13, GOLD);
            bool hG = CheckCollisionPointRec(mouse, {90.f, 680, 220, 50});
            bool hD = CheckCollisionPointRec(mouse, {SW - 310.f, 680, 220, 50});
            DrawRectangleRounded({90.f, 680, 220, 50}, 0.25f, 6, hG ? (Color){255, 130, 50, 255} : (Color){180, 60, 10, 255});
            DrawText("GREEDY  ", 105, 695, 17, WHITE);
            DrawRectangleRounded({SW - 310.f, 680, 220, 50}, 0.25f, 6, hD ? (Color){80, 255, 100, 255} : (Color){30, 150, 50, 255});
            DrawText("DYNAMIC PROG ", SW - 298, 695, 17, WHITE);
            DC("Which algorithm will you use to pack the loadout?", 650, 17, LIGHTGRAY);
        }
        else if (gState == S_L1_EXPLAIN)
        {
            DrawL1Explain();
        }
        else if (gState == S_L2_INTRO)
        {
            DrawL2Intro();
        }
        else if (gState == S_L2_PLAY)
        {
            DrawBG({4, 10, 4, 255}, {8, 20, 8, 255});
            DrawHUD();
            DC("LEVEL 2: THE ENCRYPTED MAZE", 56, 24, GOLD);
            DrawMaze();
            DrawPlayer((int)pPos.x, (int)pPos.y);
            if (showHint)
                for (auto [hx, hy] : hintPath)
                    DrawRectangle(MX + hx * CELL + CELL / 2 - 4, MY + hy * CELL + CELL / 2 - 4, 8, 8, {0, 200, 255, 170});
            int hx = MX + COLS * CELL + 8;
            Panel(hx, MY, SW - hx - 8, 400, {10, 18, 10, 210}, {50, 140, 50, 255});
            DrawText("BFS Info", hx + 10, MY + 10, 16, {80, 220, 80, 255});
            char pb[32];
            sprintf(pb, "Pos: (%d,%d)", (int)pPos.x, (int)pPos.y);
            DrawText(pb, hx + 10, MY + 34, 14, WHITE);
            char sb2[32];
            sprintf(sb2, "Score: %d", score);
            DrawText(sb2, hx + 10, MY + 54, 14, GOLD);
            DrawText("[Arrow] Move", hx + 10, MY + 80, 14, LIGHTGRAY);
            DrawText("[H] BFS Hint", hx + 10, MY + 100, 14, YELLOW);
            DrawText("Reach GREEN exit", hx + 10, MY + 128, 13, GREEN);
            DrawText("+2 pts per step", hx + 10, MY + 148, 13, GREEN);
            
          //BOOMIKA's PART

    if (userTex.id)
        UnloadTexture(userTex);
    if (guardTex.id)
        UnloadTexture(guardTex);
    CloseWindow();
    return 0;
}
