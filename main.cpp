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

//VASANT's Part
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
// added my part
// ──────────────────────────────────────────────
//  TSP
// ──────────────────────────────────────────────
static const int TN = 5;
static int dist[TN][TN] = {{0, 10, 15, 20, 25}, {10, 0, 35, 25, 17}, {15, 35, 0, 30, 28}, {20, 25, 30, 0, 22}, {25, 17, 28, 22, 0}};
static int tspMem[1 << TN][TN];
static int tspResult = 0;
static vector<int> tspTour;
static Vector2 tspNode[TN] = {{560, 290}, {700, 190}, {810, 340}, {710, 470}, {510, 450}};

static int tspDP(int mask, int pos)
{
    if (mask == (1 << TN) - 1)
        return dist[pos][0];
    if (tspMem[mask][pos] != -1)
        return tspMem[mask][pos];
    int ans = INT_MAX;
    for (int i = 0; i < TN; i++)
        if (!(mask & (1 << i)))
            ans = min(ans, dist[pos][i] + tspDP(mask | (1 << i), i));
    return tspMem[mask][pos] = ans;
}
static int tspBF()
{
    vector<int> nodes = {1, 2, 3, 4};
    int mn = INT_MAX;
    do
    {
        int cost = 0, k = 0;
        for (int n : nodes)
        {
            cost += dist[k][n];
            k = n;
        }
        cost += dist[k][0];
        mn = min(mn, cost);
    } while (next_permutation(nodes.begin(), nodes.end()));
    return mn;
}
static void solveTSP()
{
    if (chooseDP_L4)
    {
        memset(tspMem, -1, sizeof(tspMem));
        tspResult = tspDP(1, 0);
        tspTour.clear();
        tspTour.push_back(0);
        int mask = 1, pos = 0;
        while (mask != (1 << TN) - 1)
        {
            int best = -1, bc = INT_MAX;
            for (int i = 0; i < TN; i++)
                if (!(mask & (1 << i)))
                {
                    int c = dist[pos][i] + tspDP(mask | (1 << i), i);
                    if (c < bc)
                    {
                        bc = c;
                        best = i;
                    }
                }
            mask |= (1 << best);
            pos = best;
            tspTour.push_back(best);
        }
        tspTour.push_back(0);
        score += 200;
    }
    else
    {
        tspResult = tspBF();
        tspTour = {0, 1, 2, 3, 4, 0};
        score += 80;
    }
}
// ──────────────────────────────────────────────
//  DRAW HELPERS
// ──────────────────────────────────────────────
static void DrawBG(Color a, Color b)
{
    for (int y = 0; y < SH; y++)
    {
        float t = (float)y / SH;
        DrawLine(0, y, SW, y, {(uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t), (uint8_t)(a.b + (b.b - a.b) * t), 255});
    }
}
static void DC(const char *s, int y, int sz, Color c) { DrawText(s, (SW - MeasureText(s, sz)) / 2, y, sz, c); }
static void DrawHUD()
{
    DrawRectangle(0, 0, SW, 52, {8, 8, 18, 240});
    DrawLine(0, 52, SW, 52, {60, 60, 100, 255});
    DrawText("THE HEIST & ESCAPE LOOP", 18, 14, 22, RED);
    char buf[64];
    sprintf(buf, "Score: %04d", score);
    DrawText(buf, SW - MeasureText(buf, 22) - 18, 14, 22, GOLD);
}
static void Panel(int x, int y, int w, int h, Color bg, Color border)
{
    DrawRectangleRounded({(float)x, (float)y, (float)w, (float)h}, 0.07f, 6, bg);
    DrawRectangleRoundedLines({(float)x, (float)y, (float)w, (float)h}, 0.07f, 6, border);
}
static void DrawMaze()
{
    int T = 2;
    Color wc = {200, 200, 210, 255};
    for (int row = 0; row < ROWS; row++)
        for (int col = 0; col < COLS; col++)
        {
            int x = MX + col * CELL, y = MY + row * CELL, w = mazeW[row][col];
            if (!(w & 1))
                DrawRectangle(x, y, CELL, T, wc);
            if (!(w & 2))
                DrawRectangle(x, y + CELL - T, CELL, T, wc);
            if (!(w & 4))
                DrawRectangle(x + CELL - T, y, T, CELL, wc);
            if (!(w & 8))
                DrawRectangle(x, y, T, CELL, wc);
        }
    int ex = MX + (COLS - 1) * CELL, ey = MY + (ROWS - 1) * CELL;
    DrawRectangle(ex + 3, ey + 3, CELL - 6, CELL - 6, {0, 220, 80, 180});
    DrawText("EXIT", ex + 5, ey + CELL / 2 - 7, 12, {0, 0, 0, 200});
}

// Draw player — texture FITTED to cell, never overflow
static void DrawPlayer(int gx, int gy)
{
    int cx = MX + gx * CELL, cy = MY + gy * CELL, sz = CELL - 4;
    if (userTex.id)
    {
        DrawTexturePro(userTex, {0, 0, (float)userTex.width, (float)userTex.height},
                       {(float)(cx + 2), (float)(cy + 2), (float)sz, (float)sz}, {0, 0}, 0, WHITE);
    }
    else
    {
        DrawCircle(cx + CELL / 2, cy + CELL / 2, sz / 2, BLUE);
    }
}

// Draw guard — texture FITTED to cell, never overflow
static void DrawGuard(int gx, int gy)
{
    int cx = MX + gx * CELL, cy = MY + gy * CELL, sz = CELL - 4;
    if (guardTex.id)
    {
        DrawTexturePro(guardTex, {0, 0, (float)guardTex.width, (float)guardTex.height},
                       {(float)(cx + 2), (float)(cy + 2), (float)sz, (float)sz}, {0, 0}, 0, WHITE);
    }
    else
    {
        DrawCircle(cx + CELL / 2, cy + CELL / 2, sz / 2, {220, 40, 40, 255});
    }
    DrawRectangleLines(cx + 1, cy + 1, CELL - 2, CELL - 2, {255, 50, 50, 200});
}

// ──────────────────────────────────────────────
//  EXPLANATION SCREENS
//  These are the "WHY" cards shown after every choice
// ──────────────────────────────────────────────
static void DrawL1Explain()
{
    DrawBG({5, 5, 20, 255}, {20, 5, 40, 255});
    DrawHUD();
    Panel(28, 58, SW - 56, SH - 76, {14, 14, 35, 235}, {60, 60, 120, 255});
    DC("LEVEL 1 DEBRIEF:  Why Did Your Choice Score That?", 78, 24, GOLD);
    DrawLine(28, 112, SW - 28, 112, {60, 60, 100, 255});

    if (chooseDP_L1)
    {
        Panel(330, 122, 440, 58, {20, 80, 20, 240}, GREEN);
        DC("You chose DYNAMIC PROGRAMMING  =>  +200 pts", 142, 18, GREEN);
    }
    else
    {
        Panel(280, 122, 540, 58, {80, 30, 10, 240}, {255, 140, 0, 255});
        DC("You chose GREEDY ALGORITHM  =>  +80 pts  (suboptimal!)", 142, 18, ORANGE);
    }

    // Left: Greedy
    Panel(36, 196, 492, 470, {30, 15, 10, 220}, {255, 140, 0, 255});
    DrawText("GREEDY APPROACH", 52, 208, 18, {255, 160, 40, 255});
    DrawLine(52, 230, 520, 230, {100, 60, 20, 255});
    const char *gL[] = {
        "Idea: sort items by value/weight",
        "ratio, greedily pick the best.",
        "",
        "Time:  O(n log n)   Space: O(n)",
        "",
        "WHY ONLY 80 POINTS?",
        "Greedy is NOT always optimal",
        "for 0/1 Knapsack.",
        "The 0/1 means: take whole item",
        "or skip it. No splitting allowed.",
        "Example (our items, W=8):",
        "  Greedy picks Lockpick(ratio 2)",
        "  then Flashbang(ratio 1.5):",
        "  val=2+3=5 but misses better",
        "  combo: Flashbang+EMP=3+4=7!",
        "Greedy is optimal ONLY for the",
        "Fractional Knapsack variant.",
    };
    for (int i = 0; i < 17; i++)
        DrawText(gL[i], 54, 244 + i * 22, 14, (i == 5 || i == 9 || i == 19) ? (Color){255, 120, 60, 255} : WHITE);

    // Right: DP
    Panel(556, 196, SW - 594, 470, {10, 30, 10, 220}, GREEN);
    DrawText("DYNAMIC PROGRAMMING (0/1 Knapsack)", 572, 208, 17, {80, 255, 120, 255});
    DrawLine(572, 230, SW - 44, 230, {20, 80, 20, 255});
    const char *dL[] = {
        "Idea: build table dp[i][w]",
        "dp[i][w] = max value using first",
        "i items with capacity w.",
        "",
        "Time:  O(n*W)   Space: O(n*W)",
        "",
        "WHY 200 POINTS?",
        "DP explores ALL subsets via",
        "the table — ALWAYS optimal!",
        "Answer: dp[n][W]",
        "For our items: val=7 (optimal)",
    };
    for (int i = 0; i < 11; i++)
        DrawText(dL[i], 578, 244 + i * 22, 14, (i == 6 || i == 18) ? (Color){80, 255, 120, 255} : WHITE);

    DC("[SPACE]  Continue to Level 2:  BFS Maze", SH - 34, 19, {0, 210, 80, 255});
}

static void DrawL3Explain()
{
    DrawBG({15, 5, 5, 255}, {30, 5, 5, 255});
    DrawHUD();
    Panel(28, 58, SW - 56, SH - 76, {20, 10, 10, 235}, {120, 40, 40, 255});
    DC("LEVEL 3 DEBRIEF: Why Did Your Sort Choice Matter?", 78, 24, GOLD);
    DrawLine(28, 112, SW - 28, 112, {100, 40, 40, 255});

    if (chooseQuick_L3)
    {
        Panel(280, 122, 540, 58, {20, 70, 20, 240}, GREEN);
        DC("You chose QUICK SORT  =>  Slower guard, easier escape!", 142, 18, GREEN);
    }
    else
    {
        Panel(280, 122, 540, 58, {80, 20, 10, 240}, ORANGE);
        DC("You chose BUBBLE SORT  =>  Faster guard, harder escape!", 142, 18, ORANGE);
    }

    Panel(36, 196, 492, 462, {30, 10, 10, 220}, {255, 100, 60, 255});
    DrawText("BUBBLE SORT  O(N^2)", 52, 208, 18, ORANGE);
    DrawLine(52, 230, 520, 230, {100, 40, 20, 255});
    const char *bL[] = {
        "Compare adjacent pairs,",
        "swap if out of order. Repeat N times.",
        "",
        "Time:  O(N^2)   Space: O(1)",
        "",
        "For N=10 items:",
        "  Up to 45 comparisons+swaps.",
        "  Every swap = noise spike.",
        "",
        "WHY GUARD IS FASTER?",
        "More operations = higher noise.",
        "Guard move delay: 0.28s",
        "Very hard to escape!",
        "",
        "When is it OK to use?",
        "  Nearly sorted small arrays.",
        "  Teaching & visualization.",
        "  When N < 10 and simplicity",
        "  matters more than speed.",
    };
    for (int i = 0; i < 19; i++)
        DrawText(bL[i], 54, 244 + i * 22, 14, (i == 9 || i == 3) ? (Color){255, 120, 60, 255} : WHITE);

    Panel(556, 196, SW - 594, 462, {10, 25, 10, 220}, GREEN);
    DrawText("QUICK SORT  O(N log N)", 572, 208, 18, {80, 255, 100, 255});
    DrawLine(572, 230, SW - 44, 230, {20, 80, 20, 255});
    const char *qL[] = {
        "Choose pivot, partition array",
        "into < pivot and > pivot halves,",
        "recurse on each half.",
        "",
        "Time:  O(N log N) avg  O(N^2) worst",
        "Space: O(log N) stack",
        "",
        "For N=10 items:",
        "  ~33 comparisons on average.",
        "  Much quieter than Bubble!",
        "",
        "WHY GUARD IS SLOWER?",
        "Fewer ops = less noise generated.",
        "Guard move delay: 0.55s",
        "Easier to reach the exit!",
        "",
        "Used in: std::sort (most libs),",
        "C++ std::sort (introsort variant),",
        "real databases, OS schedulers.",
    };
    for (int i = 0; i < 19; i++)
        DrawText(qL[i], 578, 244 + i * 22, 14, (i == 11 || i == 4) ? (Color){80, 255, 100, 255} : WHITE);

    DC("[SPACE]  Continue to Level 4  TSP Laser Grid", SH - 34, 19, {0, 210, 80, 255});
}

static void DrawL4Explain()
{
    DrawBG({5, 5, 25, 255}, {5, 15, 45, 255});
    DrawHUD();
    Panel(28, 58, SW - 56, SH - 76, {12, 12, 35, 235}, {60, 80, 160, 255});
    DC("LEVEL 4 DEBRIEF: Why Does The TSP Algorithm Matter?", 78, 24, GOLD);
    DrawLine(28, 112, SW - 28, 112, {60, 80, 130, 255});

    char badge[120];
    sprintf(badge, "You chose: %s   Min Tour Cost: %d   %s",
            chooseDP_L4 ? "Held-Karp DP" : "Brute Force", tspResult, chooseDP_L4 ? "+200 pts" : "+80 pts");
    if (chooseDP_L4)
    {
        Panel(200, 122, SW - 400, 58, {20, 70, 20, 240}, GREEN);
        DC(badge, 142, 17, GREEN);
    }
    else
    {
        Panel(200, 122, SW - 400, 58, {70, 30, 10, 240}, ORANGE);
        DC(badge, 142, 17, ORANGE);
    }

    Panel(36, 196, 490, 462, {20, 15, 10, 220}, ORANGE);
    DrawText("BRUTE FORCE  O(N!)", 52, 208, 18, ORANGE);
    DrawLine(52, 230, 518, 230, {80, 50, 10, 255});
    const char *bfL[] = {
        "Try every possible order of cities.",
        "Track the minimum total cost.",
        "",
        "Time:  O(N!)  Space: O(N)",
        "",
        "How fast does it EXPLODE?",
        "  N=5 :        24 perms",
        "  N=10:   362,880 perms",
        "  N=15:   87 billion perms",
        "  N=20:   10^18 — impossible!",
        "  N=25:   10^25 perms",
        "",
        "WHY ONLY 80 POINTS?",
        "Works for tiny N only.",
        "For any real city map (N>20)",
        "brute force would take longer",
        "than the age of the universe!",
        "",
        "Use case: small N test oracle,",
        "verifying DP correctness only.",
    };
    for (int i = 0; i < 20; i++)
        DrawText(bfL[i], 54, 244 + i * 21, 14, (i == 5 || i == 12) ? (Color){255, 130, 50, 255} : WHITE);

    Panel(554, 196, SW - 590, 462, {10, 25, 15, 220}, GREEN);
    DrawText("HELD-KARP DP  O(2^N * N^2)", 570, 208, 17, {80, 255, 120, 255});
    DrawLine(570, 230, SW - 44, 230, {20, 80, 30, 255});
    const char *dpL[] = {
        "State: dp[mask][pos]",
        "mask = bitmask of visited cities",
        "pos  = current city index",
        "",
        "Time:  O(2^N * N^2)  Space: O(2^N*N)",
        "",
        "How much better than O(N!)?",
        "  N=5 :       80 states",
        "  N=10:    10,240 states",
        "  N=15:   491,520 states",
        "  N=20:  ~20 million states",
        "  N=25: ~838 million states",
        "",
        "WHY 200 POINTS?",
        "Feasible up to N~20 on modern",
        "hardware. Exact optimal answer.",
        "",
        "TSP is NP-Hard: no polynomial",
        "solution is known.",
        "Held-Karp is the best exact algo!",
    };
    for (int i = 0; i < 20; i++)
        DrawText(dpL[i], 572, 244 + i * 21, 14, (i == 6 || i == 13 || i == 17) ? (Color){80, 255, 120, 255} : WHITE);

    DC("[SPACE]  View Mission Accomplished", SH - 34, 19, {0, 210, 80, 255});
}

// ──────────────────────────────────────────────
//  INTRO CARDS (before each level)
// ──────────────────────────────────────────────
static void DrawL2Intro()
{
    DrawBG({4, 12, 4, 255}, {8, 24, 8, 255});
    DrawHUD();
    Panel(60, 65, SW - 120, SH - 110, {10, 20, 10, 230}, {40, 140, 40, 255});
    DC("LEVEL 2:  THE ENCRYPTED MAZE", 90, 28, GOLD);
    DrawLine(60, 130, SW - 60, 130, {40, 100, 40, 255});

    Panel(78, 148, 452, 472, {12, 25, 12, 220}, {60, 180, 60, 255});
    DrawText("BFS:  Breadth-First Search", 96, 162, 20, {80, 255, 120, 255});
    const char *bfs[] = {
        "Maze generated by Randomized DFS.",
        "Every path guaranteed solvable!",
        "",
        "BFS explores cells LAYER by LAYER",
        "from the start. It guarantees the",
        "SHORTEST path in any unweighted",
        "graph.",
        "",
        "Time: O(V+E)   Space: O(V)",
        "V=150 cells  E=open passages",
        "",
        "Press [H] to see BFS path live.",
        "",
        "In Level 3, the guard also uses",
        "BFS to chase you every step —",
        "that's how real game AI works!",
        "(Pac-Man ghosts, RPG enemies)",
    };
    for (int i = 0; i < 17; i++)
        DrawText(bfs[i], 96, 196 + i * 24, 16, (i == 8 || i == 3) ? (Color){80, 255, 120, 255} : WHITE);

    Panel(568, 148, SW - 636, 472, {12, 12, 30, 220}, {80, 120, 220, 255});
    DrawText("HOW TO NAVIGATE", 586, 162, 19, SKYBLUE);
    const char *ctrl[] = {
        "Arrow Keys:  Move player",
        "H :     Toggle BFS hint",
        "",
        "The BFS hint highlights the",
        "shortest path to exit in CYAN.",
        "",
        "You earn +2 pts per step.",
        "+100 pts for reaching exit.",
        "",
        "The maze is DIFFERENT every",
        "game run (random DFS seed).",
        "",
        "Tip: BFS always finds the",
        "shortest route. Use the hint",
        "to understand how BFS explores",
        "systematically level by level.",
        "",
        "Reach the GREEN exit cell!",
    };
    for (int i = 0; i < 18; i++)
        DrawText(ctrl[i], 586, 196 + i * 24, 15, WHITE);

    DC("[SPACE] Enter the Maze!", SH - 44, 22, {0, 220, 80, 255});
}

static void DrawL3Intro()
{
    DrawBG({18, 5, 5, 255}, {35, 8, 8, 255});
    DrawHUD();
    Panel(60, 65, SW - 120, SH - 110, {22, 10, 10, 230}, {140, 40, 40, 255});
    DC("LEVEL 3:  STEALTH MODE", 90, 28, GOLD);
    DrawLine(60, 130, SW - 60, 130, {100, 30, 30, 255});

    Panel(78, 148, 452, 472, {28, 10, 10, 220}, {200, 60, 60, 255});
    DrawText("THE SORTING CHOICE", 96, 162, 19, {255, 120, 80, 255});
    const char *sl[] = {
        "Same maze but a guard chases",
        "you using BFS every step.",
        "Every move, your noise array is",
        "sorted. The algorithm you pick",
        "directly controls guard speed:",
        "BUBBLE SORT O(N^2):",
        "  Many swaps = loud noise",
        "  Guard moves every 0.28s FAST",
        "  Very hard to escape!",
        "QUICK SORT O(N log N):",
        "  Few comparisons = quiet",
        "  Guard moves every 0.55s SLOW",
        "  Much easier to escape!",
        "Watch the noise bar + bar chart",
        "on the right panel while playing.",
        "If caught: [R] retry, [M] menu.",
    };
    for (int i = 0; i < 16; i++)
        DrawText(sl[i], 96, 196 + i * 23, 15, (i == 7 || i == 12) ? (Color){255, 150, 80, 255} : WHITE);

    Panel(568, 148, SW - 636, 472, {12, 12, 30, 220}, {80, 80, 200, 255});
    DrawText("SELECT YOUR SORT NOW", 586, 162, 18, SKYBLUE);
    const char *sc[] = {
        "Click a button below to choose",
        "your sort algorithm before",
        "starting Level 3.",
        "",
        "Your choice is shown with a",
        "checkmark (SELECTED) below.",
        "",
        "Reach the EXIT cell to escape.",
        "+150 pts + time bonus on escape.",
        "The sort choice teaches you:",
        "Algorithm complexity is not just",
        "theory — it directly affects",
        "real performance outcomes!",
        "A faster algorithm (QuickSort)",
        "generates less work = less noise",
        "= guard reacts slower to you.",
        "This is WHY we care about O(N)",
        "vs O(N^2) in real engineering.",
    };
    for (int i = 0; i < 18; i++)
        DrawText(sc[i], 586, 196 + i * 23, 14, WHITE);

    DC("[SPACE] Start Stealth Mode", SH - 44, 21, {220, 80, 0, 255});
}

static void DrawL4Intro()
{
    DrawBG({5, 5, 22, 255}, {5, 12, 40, 255});
    DrawHUD();
    Panel(60, 65, SW - 120, SH - 110, {10, 10, 30, 230}, {60, 60, 180, 255});
    DC("LEVEL 4 TSP LASER GRID", 90, 28, GOLD);
    DrawLine(60, 130, SW - 60, 130, {60, 60, 140, 255});

    Panel(78, 148, 452, 510, {12, 12, 30, 220}, {80, 80, 200, 255});
    DrawText("TRAVELLING SALESMAN PROBLEM", 96, 162, 17, SKYBLUE);
    const char *tl[] = {
        "Visit ALL 5 laser nodes exactly",
        "once and return to BASE.",
        "Minimize the total travel cost.",
        "",
        "This is NP-Hard — no known",
        "polynomial time solution!",
        "",
        "You choose HOW to solve it:",
        "",
        "BRUTE FORCE:",
        "  Try all (N-1)! orderings.",
        "  N=5 => 24 permutations OK",
        "  N=20 => 10^18 IMPOSSIBLE",
        "HELD-KARP DP:",
        "  Bitmask DP on subsets.",
        "  N=5 => 80 states  FAST",
        "  N=20 => 20M  still feasible",
        "See the full explanation AFTER",
        "you click your choice below!",
    };
    for (int i = 0; i < 19; i++)
        DrawText(tl[i], 96, 196 + i * 23, 14, (i == 4 || i == 9 || i == 14) ? (Color){100, 180, 255, 255} : WHITE);

    Panel(568, 148, SW - 636, 510, {10, 20, 10, 220}, {60, 180, 60, 255});
    DrawText("COMPLEXITY PREVIEW", 586, 162, 18, {80, 255, 120, 255});
    const char *tc[] = {
        "N cities — comparisons needed:",
        "",
        "N  | Brute Force O(N!)| Held-Karp",
        "5  |           24    |       80",
        "10 |      362,880    |   10,240",
        "15 | 87 billion     |  491,520",
        "20 | 10^18 KABOOM   | 20 million",
        "",
        "Held-Karp is EXPONENTIALLY",
        "better than brute force!",
        "Distance matrix (5 nodes):",
        "     B   A   B   C   D",
        "[0  10  15  20  25]",
        "[10   0  35  25  17]",
        "[15  35   0  30  28]",
        "[20  25  30   0  22]",
        "[25  17  28  22   0]",
        "DP ",
        "Brute Force",
    };
    for (int i = 0; i < 19; i++)
        DrawText(tc[i], 586, 196 + i * 23, 13, (i == 8 || i == 9 || i == 19 || i == 20) ? (Color){80, 255, 120, 255} : WHITE);

    DC("[SPACE]  Approach the Grid", SH - 44, 21, {80, 160, 255, 255});
}

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
            if (showHint)
            {
                char hb[32];
                sprintf(hb, "Path: %d steps", (int)hintPath.size());
                DrawText(hb, hx + 10, MY + 175, 13, SKYBLUE);
            }
            DrawText("BFS: O(V+E)", hx + 10, MY + 240, 13, {100, 200, 255, 255});
            DrawText("Shortest path", hx + 10, MY + 258, 13, {100, 200, 255, 255});
            DrawText("in any grid!", hx + 10, MY + 276, 13, {100, 200, 255, 255});
        }
        else if (gState == S_L3_INTRO)
        {
            DrawL3Intro();
            bool hB = CheckCollisionPointRec(mouse, {200.f, SH - 90, 220, 52});
            bool hQ = CheckCollisionPointRec(mouse, {SW - 420.f, SH - 90, 220, 52});
            Color cB = (!chooseQuick_L3) ? (Color){255, 140, 60, 255} : hB ? (Color){200, 90, 40, 255}
                                                                           : (Color){140, 60, 20, 255};
            Color cQ = (chooseQuick_L3) ? (Color){80, 220, 100, 255} : hQ ? (Color){50, 180, 70, 255}
                                                                          : (Color){30, 120, 50, 255};
            DrawRectangleRounded({200.f, SH - 90, 220, 52}, 0.25f, 6, cB);
            DrawText(!chooseQuick_L3 ? "BUBBLE  (selected)" : "BUBBLE SORT", 215, SH - 74, 17, WHITE);
            DrawRectangleRounded({SW - 420.f, SH - 90, 220, 52}, 0.25f, 6, cQ);
            DrawText(chooseQuick_L3 ? "QUICK  (selected)" : "QUICK SORT", SW - 410, SH - 74, 17, WHITE);
        }
        else if (gState == S_L3_PLAY)
        {
            DrawBG({14, 4, 4, 255}, {28, 6, 6, 255});
            DrawHUD();
            DC("LEVEL 3: STEALTH MODE", 56, 24, RED);
            DrawMaze();
            DrawPlayer((int)pPos.x, (int)pPos.y);
            DrawGuard((int)gPos.x, (int)gPos.y);
            int hx = MX + COLS * CELL + 8;
            Panel(hx, MY, SW - hx - 8, 490, {22, 8, 8, 210}, {160, 40, 40, 255});
            DrawText("Stealth HUD", hx + 10, MY + 8, 16, {255, 120, 80, 255});
            DrawText("Noise Level:", hx + 10, MY + 34, 13, WHITE);
            DrawRectangle(hx + 10, MY + 50, 180, 18, DARKGRAY);
            Color nc = noiseLevel > 70 ? RED : noiseLevel > 40 ? ORANGE
                                                               : GREEN;
            DrawRectangle(hx + 10, MY + 50, (int)(180 * noiseLevel / 100.f), 18, nc);
            DrawRectangleLines(hx + 10, MY + 50, 180, 18, WHITE);
            char nb[16];
            sprintf(nb, "%d%%", noiseLevel);
            DrawText(nb, hx + 88, MY + 51, 13, WHITE);
            DrawText("Noise array bars:", hx + 10, MY + 78, 13, LIGHTGRAY);
            for (int i = 0; i < (int)noiseArr.size(); i++)
            {
                int bh = (int)(noiseArr[i] * 0.52f);
                DrawRectangle(hx + 10 + i * 18, MY + 178 - bh, 14, bh, chooseQuick_L3 ? SKYBLUE : ORANGE);
            }
            DrawText(chooseQuick_L3 ? "QuickSort O(NlogN)" : "BubbleSort O(N^2)", hx + 10, MY + 188, 12, chooseQuick_L3 ? SKYBLUE : ORANGE);
            char tbuf[32];
            sprintf(tbuf, "Time: %.1fs", stealthTime);
            DrawText(tbuf, hx + 10, MY + 208, 13, WHITE);
            DrawText("Guard step delay:", hx + 10, MY + 228, 13, WHITE);
            DrawText(chooseQuick_L3 ? "0.55s (slow)" : "0.28s (FAST!)", hx + 10, MY + 246, 13, chooseQuick_L3 ? GREEN : RED);
            DrawText("[Arrow] Move", hx + 10, MY + 295, 13, LIGHTGRAY);
            DrawText("Reach GREEN exit!", hx + 10, MY + 315, 13, GREEN);
            DrawText("RED border = guard", hx + 10, MY + 350, 12, RED);
        }
        else if (gState == S_L3_CAUGHT)
        {
            DrawBG({30, 0, 0, 255}, {60, 0, 0, 255});
            DrawHUD();
            DC("CAUGHT BY THE GUARD!", SH / 2 - 90, 42, RED);
            DC("The BFS algorithm computed the shortest path to you.", SH / 2 - 30, 19, WHITE);
            if (!chooseQuick_L3)
                DC("Bubble Sort O(N^2) generated too much noise!", SH / 2 + 10, 18, ORANGE);
            else
                DC("Even Quick Sort couldn't save you this time.", SH / 2 + 10, 18, ORANGE);
            DC("Try Quick Sort: fewer operations = less noise = slower guard.", SH / 2 + 46, 17, LIGHTGRAY);
            DC("[R] Retry Level 3      [M] Main Menu", SH / 2 + 108, 22, YELLOW);
        }
        else if (gState == S_L3_EXPLAIN)
        {
            DrawL3Explain();
        }
        else if (gState == S_L4_INTRO)
        {
            DrawL4Intro();
        }
        else if (gState == S_L4_PLAY)
        {
            DrawBG({4, 4, 22, 255}, {4, 12, 40, 255});
            DrawHUD();
            DC("LEVEL 4: TSP LASER GRID", 58, 26, GOLD);
            for (int i = 0; i < TN; i++)
                for (int j = i + 1; j < TN; j++)
                {
                    DrawLineEx(tspNode[i], tspNode[j], 1.5f, {50, 50, 110, 180});
                    Vector2 mid = {(tspNode[i].x + tspNode[j].x) / 2, (tspNode[i].y + tspNode[j].y) / 2};
                    char db[8];
                    sprintf(db, "%d", dist[i][j]);
                    DrawText(db, (int)mid.x - 6, (int)mid.y - 7, 13, {140, 140, 200, 200});
                }
            const char *nn[] = {"BASE", "A", "B", "C", "D"};
            for (int i = 0; i < TN; i++)
            {
                DrawCircle((int)tspNode[i].x, (int)tspNode[i].y, 24, {22, 22, 70, 255});
                DrawCircleLines((int)tspNode[i].x, (int)tspNode[i].y, 24, SKYBLUE);
                DrawText(nn[i], (int)tspNode[i].x - 16, (int)tspNode[i].y - 8, 15, WHITE);
            }
            Panel(28, 83, 262, 490, {14, 14, 36, 220}, SKYBLUE);
            DrawText("TSP Problem", 42, 96, 17, SKYBLUE);
            DrawText("Visit all nodes once,", 42, 120, 14, WHITE);
            DrawText("return to BASE.", 42, 138, 14, WHITE);
            DrawText("Minimize total cost.", 42, 156, 14, WHITE);
            DrawText("Brute Force O(N!)", 42, 188, 14, ORANGE);
            DrawText("Held-Karp O(2^N*N^2)", 42, 208, 14, GREEN);
            DrawText("NP-Hard problem!", 42, 228, 14, YELLOW);
            DrawText("Distance Matrix:", 42, 262, 14, LIGHTGRAY);
            for (int i = 0; i < TN; i++)
            {
                char row[64] = "";
                int off = 0;
                for (int j = 0; j < TN; j++)
                    off += sprintf(row + off, "%3d ", dist[i][j]);
                DrawText(row, 42, 280 + i * 20, 13, {160, 180, 200, 255});
            }
            DrawText("Click a button below.", 42, 398, 13, GOLD);
            DrawText("Full explanation shown", 42, 416, 13, GOLD);
            DrawText("after your choice!", 42, 434, 13, GOLD);
            bool hBF = CheckCollisionPointRec(mouse, {120.f, 630, 240, 56});
            bool hDP = CheckCollisionPointRec(mouse, {SW - 360.f, 630, 240, 56});
            DrawRectangleRounded({120.f, 630, 240, 56}, 0.25f, 6, hBF ? (Color){255, 160, 60, 255} : (Color){170, 70, 10, 255});
            DrawText("BRUTE FORCE (+80)", 136, 647, 18, WHITE);
            DrawRectangleRounded({SW - 360.f, 630, 240, 56}, 0.25f, 6, hDP ? (Color){80, 255, 120, 255} : (Color){25, 140, 60, 255});
            DrawText("HELD-KARP DP (+200)", SW - 350, 647, 18, WHITE);
            DC("Select algorithm to bypass the laser grid", 598, 17, LIGHTGRAY);
        }
        else if (gState == S_L4_EXPLAIN)
        {
            DrawL4Explain();
        }
        else if (gState == S_WIN)
        {
            DrawBG({4, 18, 4, 255}, {6, 35, 6, 255});
            DrawHUD();
            float flash = (sinf(gTimer * 3.f) + 1.f) * 0.5f;
            DC("MISSION ACCOMPLISHED!", (int)(95), 46, {(uint8_t)(180 + 75 * flash), 220, 0, 255});
            DC("The vault is yours, Ghost. And now you know WHY.", 152, 22, WHITE);
            Panel(80, 192, SW - 160, 432, {8, 22, 8, 218}, GOLD);
            char fb[64];
            sprintf(fb, "FINAL SCORE: %d", score);
            DC(fb, 212, 32, GOLD);
            DrawLine(90, 256, SW - 90, 256, {60, 100, 60, 255});
            DrawText("Level", 100, 266, 16, LIGHTGRAY);
            DrawText("Your Choice", 295, 266, 16, LIGHTGRAY);
            DrawText("Key Lesson", 630, 266, 16, LIGHTGRAY);
            DrawLine(90, 288, SW - 90, 288, {60, 100, 60, 255});
            const char *rows[4][3] = {
                {"1 - Knapsack", chooseDP_L1 ? "DP (+200)" : "Greedy (+80)", "DP finds optimal; Greedy may miss 0/1 combinations"},
                {"2 - BFS Maze", "BFS pathfinding", "BFS guarantees shortest path in O(V+E)"},
                {"3 - Stealth", chooseQuick_L3 ? "QuickSort (+bonus)" : "BubbleSort", "O(NlogN) generates less noise than O(N^2)"},
                {"4 - TSP Laser", chooseDP_L4 ? "Held-Karp DP (+200)" : "Brute Force (+80)", "2^N*N^2 is VASTLY better than N! for large N"},
            };
            Color rc[4] = {SKYBLUE, GREEN, ORANGE, {180, 100, 255, 255}};
            for (int i = 0; i < 4; i++)
            {
                DrawText(rows[i][0], 100, 303 + i * 56, 16, rc[i]);
                DrawText(rows[i][1], 295, 303 + i * 56, 15, WHITE);
                DrawText(rows[i][2], 630, 303 + i * 56, 13, LIGHTGRAY);
            }
            DrawLine(90, 530, SW - 90, 530, {60, 100, 60, 255});
            DC("Algorithm complexity is not just theory.", 562, 16, {80, 220, 100, 255});
            DC("Every choice has a cost. Every cost has a reason. Now you know both.", 585, 15, WHITE);
            Panel(350, 620, SW - 700, 58, {10, 10, 10, 180}, {80, 80, 80, 255});
            DC("[M] Return to Menu", 640, 19, {140, 140, 160, 255});
        }

        EndDrawing();
    }
            
    

    if (userTex.id)
        UnloadTexture(userTex);
    if (guardTex.id)
        UnloadTexture(guardTex);
    CloseWindow();
    return 0;
}
