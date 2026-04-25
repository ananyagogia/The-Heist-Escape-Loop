#include "raylib.h"
#include <bits/stdc++.h>
using namespace std;

//  WINDOW & GRID

static const int SW = 1100;
static const int SH = 750;
static const int COLS = 15;
static const int ROWS = 10;
static const int CELL = 52;
static const int MX = 55;
static const int MY = 58;

//  GAME STATES

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
    S_L5_INTRO,
    S_L5_PLAY,
    S_L5_EXPLAIN,
    S_L6_INTRO,
    S_L6_PLAY,
    S_L6_CAUGHT,
    S_L6_EXPLAIN,
    S_WIN
};
State gState = S_MENU;

//  GLOBALS

int score = 0;
float gTimer = 0.f;
float stTimer = 0.f;

Texture2D userTex, guardTex;
bool chooseDP_L1 = true;
bool chooseQuick_L3 = true;
bool chooseDP_L4 = true;
bool chooseChaining_L5 = true; // true=Chaining, false=Open Addressing
bool chooseKMP_L6 = true;      // true=KMP, false=Naive

//  MAZE  (bitmask: bit0=N bit1=S bit2=E bit3=W)

static int mazeW[ROWS][COLS];
static bool mazeVis[ROWS][COLS];

static void carveMaze(int x, int y)
{
    mazeVis[y][x] = true;
    int dirs[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--)
        swap(dirs[i], dirs[GetRandomValue(0, i)]);
    int dx[4] = {0, 0, 1, -1};
    int dy[4] = {-1, 1, 0, 0};
    int myBit[4] = {1, 2, 4, 8};
    int nbBit[4] = {2, 1, 8, 4};
    for (int d : dirs)
    {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx >= 0 && ny >= 0 && nx < COLS && ny < ROWS && !mazeVis[ny][nx])
        {
            mazeW[y][x] |= myBit[d];
            mazeW[ny][nx] |= nbBit[d];
            carveMaze(nx, ny);
        }
    }
}
static void initMaze()
{
    memset(mazeW, 0, sizeof(mazeW));
    memset(mazeVis, 0, sizeof(mazeVis));
    carveMaze(0, 0);
}

//  BFS (wall-aware)

static vector<pair<int, int>> mazeBFS(int sx, int sy, int ex, int ey)
{
    static const int dx[4] = {0, 0, 1, -1};
    static const int dy[4] = {-1, 1, 0, 0};
    static const int bit[4] = {1, 2, 4, 8};
    bool vis[ROWS][COLS] = {};
    map<pair<int, int>, pair<int, int>> par;
    queue<pair<int, int>> q;
    q.push({sx, sy});
    vis[sy][sx] = true;
    while (!q.empty())
    {
        auto [cx, cy] = q.front();
        q.pop();
        if (cx == ex && cy == ey)
            break;
        for (int d = 0; d < 4; d++)
        {
            if (!(mazeW[cy][cx] & bit[d]))
                continue;
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx >= 0 && ny >= 0 && nx < COLS && ny < ROWS && !vis[ny][nx])
            {
                vis[ny][nx] = true;
                par[{nx, ny}] = {cx, cy};
                q.push({nx, ny});
            }
        }
    }
    vector<pair<int, int>> path;
    pair<int, int> cur = {ex, ey};
    while (cur != make_pair(sx, sy))
    {
        if (!par.count(cur))
            return {};
        path.push_back(cur);
        cur = par[cur];
    }
    reverse(path.begin(), path.end());
    return path;
}

//  PLAYER / GUARD

static Vector2 pPos = {0, 0};
static Vector2 gPos = {COLS - 2, 0};
static float guardTimer = 0.f;
static float stealthTime = 0.f;
static int noiseLevel = 0;
static vector<int> noiseArr = {55, 12, 78, 34, 90, 23, 67, 45, 11, 88};
static int bbI = 0, bbJ = 0;
static bool bbDone = false;

static void bubbleStep(vector<int> &a)
{
    if (bbDone)
        return;
    if (bbI >= (int)a.size())
    {
        bbDone = true;
        return;
    }
    if (bbJ < (int)a.size() - bbI - 1)
    {
        if (a[bbJ] > a[bbJ + 1])
            swap(a[bbJ], a[bbJ + 1]);
        bbJ++;
    }
    else
    {
        bbI++;
        bbJ = 0;
    }
}
static int qpart(vector<int> &a, int l, int r)
{
    int p = a[r], i = l - 1;
    for (int j = l; j < r; j++)
        if (a[j] < p)
        {
            i++;
            swap(a[i], a[j]);
        }
    swap(a[i + 1], a[r]);
    return i + 1;
}
static void qsort2(vector<int> &a, int l, int r)
{
    if (l < r)
    {
        int pi = qpart(a, l, r);
        qsort2(a, l, pi - 1);
        qsort2(a, pi + 1, r);
    }
}


//  KNAPSACK

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

//  TSP

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
            {
                if (!(mask & (1 << i)))
                {
                    int c = dist[pos][i] + tspDP(mask | (1 << i), i);
                    if (c < bc)
                    {
                        bc = c;
                        best = i;
                    }
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


//  LEVEL 5: FRACTIONAL KNAPSACK + HASHING

// Fractional Knapsack items: Liquid Gold and Data Dust
struct FracItem
{
    const char *name;
    float weight;
    float value;
    float ratio;
};

static const int FKN = 6;
static FracItem fkItems[FKN] = {
    {"Liq.Gold A", 3.0f, 9.0f, 3.0f},
    {"Liq.Gold B", 4.0f, 8.0f, 2.0f},
    {"Data Dust X", 2.0f, 6.0f, 3.0f},
    {"Data Dust Y", 5.0f, 7.5f, 1.5f},
    {"Liq.Gold C", 1.5f, 6.0f, 4.0f},
    {"Data Dust Z", 3.5f, 5.25f, 1.5f},
};
static const float FK_CAPACITY = 10.0f;
static float fkTaken[FKN]; // fraction taken per item
static float fkTotalVal = 0.f;
static bool fkDone = false;
static int fkAniStep = 0; // animation step
static float fkAniTimer = 0.f;

// sorted order indices for greedy display
static int fkSorted[FKN];

static void initFractKS()
{
    for (int i = 0; i < FKN; i++)
    {
        fkItems[i].ratio = fkItems[i].value / fkItems[i].weight;
        fkSorted[i] = i;
        fkTaken[i] = 0.f;
    }
    // sort indices by ratio descending
    sort(fkSorted, fkSorted + FKN, [](int a, int b)
         { return fkItems[a].ratio > fkItems[b].ratio; });
    fkTotalVal = 0.f;
    fkDone = false;
    fkAniStep = 0;
    fkAniTimer = 0.f;
}

static void stepFractKS()
{
    if (fkDone)
        return;
    // compute fractional knapsack fully
    float rem = FK_CAPACITY;
    fkTotalVal = 0.f;
    for (int i = 0; i < FKN; i++)
        fkTaken[i] = 0.f;
    for (int si = 0; si < fkAniStep && si < FKN; si++)
    {
        int idx = fkSorted[si];
        if (rem <= 0.f)
            break;
        float take = min(fkItems[idx].weight, rem);
        fkTaken[idx] = take / fkItems[idx].weight; // fraction
        fkTotalVal += fkTaken[idx] * fkItems[idx].value;
        rem -= take;
    }
    if (fkAniStep >= FKN)
        fkDone = true;
}

// Hash Simulation
static const int HASH_TABLE_SIZE = 7;
static vector<int> hashChainTable[HASH_TABLE_SIZE]; // chaining
static int hashOATable[HASH_TABLE_SIZE];            // open addressing (-1 = empty)
static int hashProbesChain[HASH_TABLE_SIZE];
static int hashProbesOA[HASH_TABLE_SIZE];
static int hashKeys[] = {14, 17, 21, 35, 7, 28, 42};
static const int HN = 7;
static int hashAniStep = 0;
static bool hashDone = false;
static int totalProbesChain = 0, totalProbesOA = 0;

static void initHashSim()
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        hashChainTable[i].clear();
        hashOATable[i] = -1;
        hashProbesChain[i] = 0;
        hashProbesOA[i] = 0;
    }
    hashAniStep = 0;
    hashDone = false;
    totalProbesChain = 0;
    totalProbesOA = 0;
}

static void stepHashSim()
{
    if (hashDone || hashAniStep >= HN)
    {
        hashDone = true;
        return;
    }
    int key = hashKeys[hashAniStep];
    int h = key % HASH_TABLE_SIZE;

    // Chaining: always 1 probe to find bucket
    hashChainTable[h].push_back(key);
    hashProbesChain[h]++;
    totalProbesChain += (int)hashChainTable[h].size(); // probes = chain length at insertion

    // Open Addressing (linear probing)
    int pos = h, probes = 1;
    while (hashOATable[pos] != -1)
    {
        pos = (pos + 1) % HASH_TABLE_SIZE;
        probes++;
    }
    hashOATable[pos] = key;
    hashProbesOA[pos] = probes;
    totalProbesOA += probes;

    hashAniStep++;
    if (hashAniStep >= HN)
        hashDone = true;
}

// L5 vault timer
static float vaultTimer = 0.f;
static const float VAULT_TIME = 60.f;
static bool l5HashPhase = false; // false=frac knapsack, true=hashing
static float l5AniTimer = 0.f;


//  LEVEL 6: DIJKSTRA GUARD + KMP

// Dijkstra on a small weighted graph (escape route)
// 8 nodes escape graph
// Node indices:
//   0=Start   1=Corridor  2=Stairs   3=Rooftop  4=Exit   (PLAYER PATH - top row)
//   5=Lobby   6=GrdRoom   7=Chkpoint              (GUARD ZONE - bottom row, side branches)
//
// Critical path (shortest): Start->Corridor->Stairs->Rooftop->Exit  cost=2+3+4+3=12
// Guard starts at GrdRoom(6), which is only reachable via Lobby(5) branch
// Guard cannot block the critical path at start
static const int EGN = 8;
static const char *egNames[EGN] = {"Start", "Corridor", "Stairs", "Rooftop", "Exit", "Lobby", "GrdRoom", "Chkpoint"};
//
//   Main escape route (top):  0 --2-- 1 --3-- 2 --4-- 3 --3-- 4(Exit)
//   Side branches:
//     Corridor(1) --5-- Lobby(5)
//     Lobby(5)    --4-- GrdRoom(6)   <-- guard starts here
//     Stairs(2)   --6-- Chkpoint(7)
//     Chkpoint(7) --5-- GrdRoom(6)
//     Rooftop(3)  --7-- Chkpoint(7)
//
static int egAdj[EGN][EGN] = {
    //  0   1   2   3   4   5   6   7
    {0, 2, 0, 0, 0, 0, 0, 0}, // 0 Start
    {2, 0, 3, 0, 0, 5, 0, 0}, // 1 Corridor
    {0, 3, 0, 4, 0, 0, 0, 6}, // 2 Stairs
    {0, 0, 4, 0, 3, 0, 0, 7}, // 3 Rooftop
    {0, 0, 0, 3, 0, 0, 0, 0}, // 4 Exit
    {0, 5, 0, 0, 0, 0, 4, 0}, // 5 Lobby
    {0, 0, 0, 0, 0, 4, 0, 5}, // 6 GrdRoom
    {0, 0, 6, 7, 0, 0, 5, 0}, // 7 Chkpoint
};

// Node positions - clean two-row layout
// Top row (escape route):   Start, Corridor, Stairs, Rooftop, Exit
// Bottom row (guard zone):  Lobby, GrdRoom, Chkpoint
static Vector2 egPos[EGN] = {
    {100, 480}, // 0 Start
    {270, 340}, // 1 Corridor
    {450, 220}, // 2 Stairs
    {620, 130}, // 3 Rooftop
    {760, 100}, // 4 Exit
    {270, 520}, // 5 Lobby       (below Corridor)
    {370, 580}, // 6 GrdRoom     (guard starts here, far from path)
    {560, 430}, // 7 Chkpoint    (side branch off Stairs/Rooftop)
};

static int dijkDist[EGN];
static int dijkPrev[EGN];
static bool dijkVis[EGN];
static vector<int> dijkPath; // shortest path from player node to exit
static int dijkPlayerNode = 0;
static bool dijkDone = false;

// Guard for L6 uses Dijkstra path to chase player
static int l6GuardNode = 6; // starts at GrdRoom (off the critical path)
static float l6GuardTimer = 0.f;
static float l6EscapeTime = 0.f;
static bool l6GuardActive = false;

static void runDijkstra(int src)
{
    for (int i = 0; i < EGN; i++)
    {
        dijkDist[i] = INT_MAX;
        dijkPrev[i] = -1;
        dijkVis[i] = false;
    }
    dijkDist[src] = 0;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
    pq.push({0, src});
    while (!pq.empty())
    {
        auto [d, u] = pq.top();
        pq.pop();
        if (dijkVis[u])
            continue;
        dijkVis[u] = true;
        for (int v = 0; v < EGN; v++)
        {
            if (egAdj[u][v] && !dijkVis[v] && dijkDist[u] + egAdj[u][v] < dijkDist[v])
            {
                dijkDist[v] = dijkDist[u] + egAdj[u][v];
                dijkPrev[v] = u;
                pq.push({dijkDist[v], v});
            }
        }
    }
    // reconstruct path from src to Exit(4)
    dijkPath.clear();
    int cur = 4;
    while (cur != -1)
    {
        dijkPath.push_back(cur);
        cur = dijkPrev[cur];
    }
    reverse(dijkPath.begin(), dijkPath.end());
    dijkDone = true;
}

// Guard Dijkstra path to player node
static vector<int> guardDijkPath;
static void guardDijkstra(int guardNode, int playerNode)
{
    int d[EGN];
    int prev[EGN];
    bool vis[EGN];
    for (int i = 0; i < EGN; i++)
    {
        d[i] = INT_MAX;
        prev[i] = -1;
        vis[i] = false;
    }
    d[guardNode] = 0;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
    pq.push({0, guardNode});
    while (!pq.empty())
    {
        auto [dd, u] = pq.top();
        pq.pop();
        if (vis[u])
            continue;
        vis[u] = true;
        for (int v = 0; v < EGN; v++)
        {
            if (egAdj[u][v] && !vis[v] && d[u] + egAdj[u][v] < d[v])
            {
                d[v] = d[u] + egAdj[u][v];
                prev[v] = u;
                pq.push({d[v], v});
            }
        }
    }
    guardDijkPath.clear();
    int cur = playerNode;
    while (cur != -1)
    {
        guardDijkPath.push_back(cur);
        cur = prev[cur];
    }
    reverse(guardDijkPath.begin(), guardDijkPath.end());
}

// KMP String Matching
static string kmpText = "XBADCABRACADABRA$#@ABRACADABRA!END";
static string kmpPat = "ABRACADABRA";
static int kmpFoundIdx = -1;
static int kmpNaiveOps = 0;
static int kmpKmpOps = 0;
static bool kmpSearchDone = false;
static vector<int> kmpFailure; // KMP failure function

static void buildKMPFailure(const string &pat, vector<int> &f)
{
    int m = (int)pat.size();
    f.assign(m, 0);
    int k = 0;
    for (int i = 1; i < m; i++)
    {
        while (k > 0 && pat[k] != pat[i])
            k = f[k - 1];
        if (pat[k] == pat[i])
            k++;
        f[i] = k;
    }
}

static void runKMP()
{
    kmpFailure.clear();
    buildKMPFailure(kmpPat, kmpFailure);
    int n = (int)kmpText.size(), m = (int)kmpPat.size();
    int q = 0;
    kmpKmpOps = 0;
    kmpFoundIdx = -1;
    for (int i = 0; i < n; i++)
    {
        kmpKmpOps++;
        while (q > 0 && kmpPat[q] != kmpText[i])
        {
            q = kmpFailure[q - 1];
            kmpKmpOps++;
        }
        if (kmpPat[q] == kmpText[i])
            q++;
        if (q == m)
        {
            kmpFoundIdx = i - m + 1;
            break;
        }
    }
    // Naive ops estimate
    kmpNaiveOps = 0;
    for (int i = 0; i <= n - m; i++)
    {
        for (int j = 0; j < m; j++)
        {
            kmpNaiveOps++;
            if (kmpText[i + j] != kmpPat[j])
                break;
        }
        if (kmpFoundIdx == i)
        {
        } // found
    }
    kmpSearchDone = true;
}

// L6 player node navigation
static int l6PlayerNode = 0; // starts at Floor1
static bool l6Won = false;   // reached Exit(7)
static bool l6Caught = false;
static float l6HighlightTimer = 0.f;

static void initL6()
{
    l6PlayerNode = 0;
    l6GuardNode = 6; // GrdRoom - off the critical path
    l6GuardTimer = 0.f;
    l6EscapeTime = 0.f;
    l6GuardActive = false;
    l6Won = false;
    l6Caught = false;
    kmpSearchDone = false;
    kmpFoundIdx = -1;
    dijkDone = false;
    dijkPath.clear();
    guardDijkPath.clear();
    l6HighlightTimer = 0.f;
    runDijkstra(l6PlayerNode);
    guardDijkstra(l6GuardNode, l6PlayerNode);
}

//  DRAW HELPERS

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

//  EXPLANATION SCREENS

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
    for (int i = 0; i < 15; i++)
        DrawText(bL[i], 54, 244 + i * 22, 14, (i == 5 || i == 3) ? (Color){255, 120, 60, 255} : WHITE);

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
        "WHY GUARD IS SLOWER?",
        "Fewer ops = less noise generated.",
        "Guard move delay: 0.55s",
        "Easier to reach the exit!",
        "",
        "Used in: std::sort (most libs),",
        "C++ introsort variant,",
        "real databases, OS schedulers.",
    };
    for (int i = 0; i < 15; i++)
        DrawText(qL[i], 578, 244 + i * 22, 14, (i == 7 || i == 4) ? (Color){80, 255, 100, 255} : WHITE);

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
    for (int i = 0; i < 19; i++)
        DrawText(bfL[i], 54, 244 + i * 21, 14, (i == 5 || i == 11) ? (Color){255, 130, 50, 255} : WHITE);

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
        "",
        "WHY 200 POINTS?",
        "Feasible up to N~20 on modern",
        "hardware. Exact optimal answer.",
        "",
        "TSP is NP-Hard: no polynomial",
        "solution is known.",
        "Held-Karp is the best exact algo!",
    };
    for (int i = 0; i < 19; i++)
        DrawText(dpL[i], 572, 244 + i * 21, 14, (i == 6 || i == 12 || i == 16) ? (Color){80, 255, 120, 255} : WHITE);

    DC("[SPACE]  Enter the Vault — Level 5", SH - 34, 19, {0, 210, 80, 255});
}

// ── LEVEL 5 INTRO ──
static void DrawL5Intro()
{
    DrawBG({10, 5, 2, 255}, {28, 14, 4, 255});
    DrawHUD();
    Panel(60, 65, SW - 120, SH - 110, {16, 10, 4, 230}, {200, 140, 20, 255});
    DC("LEVEL 5:  INSIDE THE VAULT", 90, 28, GOLD);
    DrawLine(60, 130, SW - 60, 130, {160, 100, 20, 255});

    Panel(78, 148, 460, 490, {14, 18, 6, 220}, {180, 200, 40, 255});
    DrawText("FRACTIONAL KNAPSACK (Greedy)", 96, 162, 17, {220, 240, 80, 255});
    const char *fk[] = {
        "Liquid Gold & Data Dust can",
        "be SPLIT — take any fraction!",
        "",
        "Strategy: Sort items by",
        "Value/Weight ratio (descending).",
        "Greedily fill bag until full.",
        "",
        "Time: O(n log n)   Space: O(n)",
        "",
        "WHY GREEDY WORKS HERE:",
        "Unlike 0/1 Knapsack, we can",
        "take partial amounts, so the",
        "greedy ratio approach is ALWAYS",
        "optimal for Fractional Knapsack!",
        "",
        "Watch the animation: items",
        "fill the vault bag one by one,",
        "last item split if needed.",
    };
    for (int i = 0; i < 18; i++)
        DrawText(fk[i], 96, 196 + i * 23, 14, (i == 9 || i == 7) ? (Color){220, 240, 80, 255} : WHITE);

    Panel(568, 148, SW - 636, 490, {10, 10, 24, 220}, {80, 100, 220, 255});
    DrawText("HASHING: Collision Resolution", 586, 162, 16, SKYBLUE);
    const char *hk[] = {
        "To download the master file,",
        "you must resolve hash collisions",
        "in a data stream of 7 keys.",
        "",
        "Hash function: key mod 7",
        "",
        "CHAINING:",
        "  Each bucket holds a linked list.",
        "  O(1) avg insert, O(1+alpha)",
        "  search. Never probes.",
        "",
        "OPEN ADDRESSING (Linear Probe):",
        "  Find next empty slot linearly.",
        "  Probes increase with load.",
        "  O(1/(1-alpha)) avg search.",
        "",
        "Compare total probes to see",
        "which method handles collisions",
        "more efficiently!",
    };
    for (int i = 0; i < 18; i++)
        DrawText(hk[i], 586, 196 + i * 23, 13, (i == 6 || i == 11) ? (Color){100, 200, 255, 255} : WHITE);

    DC("[SPACE]  Begin Vault Raid  (60 seconds!)", SH - 44, 22, {220, 180, 0, 255});
}

// ── LEVEL 5 EXPLAIN ──
static void DrawL5Explain()
{
    DrawBG({10, 8, 2, 255}, {20, 16, 4, 255});
    DrawHUD();
    Panel(28, 58, SW - 56, SH - 76, {14, 12, 4, 235}, {180, 140, 20, 255});
    DC("LEVEL 5 DEBRIEF: Fractional Knapsack + Hashing", 78, 24, GOLD);
    DrawLine(28, 112, SW - 28, 112, {140, 100, 20, 255});

    // Left: Fractional Knapsack
    Panel(36, 130, 490, 510, {20, 18, 4, 220}, {200, 180, 40, 255});
    DrawText("FRACTIONAL KNAPSACK (Greedy)", 52, 142, 16, {240, 220, 60, 255});
    DrawLine(52, 164, 518, 164, {120, 100, 20, 255});
    const char *fkL[] = {
        "Sorted by Value/Weight ratio:",
        "  Liq.Gold C  : ratio 4.0  BEST",
        "  Liq.Gold A  : ratio 3.0",
        "  Data Dust X : ratio 3.0",
        "  Liq.Gold B  : ratio 2.0",
        "  Data Dust Y : ratio 1.5",
        "  Data Dust Z : ratio 1.5  WORST",
        "",
        "Greedy fills capacity=10kg:",
        "  Take all Liq.Gold C  (1.5kg)",
        "  Take all Liq.Gold A  (3.0kg)",
        "  Take all Data Dust X (2.0kg)",
        "  Take all Liq.Gold B  (4.0kg)  FULL",
        "",
        "Total Value = 6+9+6+8 = 29.0",
        "",
        "WHY GREEDY IS OPTIMAL HERE:",
        "Fractional Knapsack allows splits.",
        "Taking highest ratio first =",
        "mathematically provable optimal.",
        "Proof: exchange argument shows",
        "any swap reduces total value.",
    };
    for (int i = 0; i < 22; i++)
        DrawText(fkL[i], 52, 178 + i * 20, 13, (i == 16 || i == 8) ? (Color){240, 220, 60, 255} : WHITE);

    // Right: Hashing
    Panel(554, 130, SW - 590, 510, {10, 10, 22, 220}, {80, 100, 220, 255});
    DrawText("CHAINING vs OPEN ADDRESSING", 570, 142, 15, SKYBLUE);
    DrawLine(570, 164, SW - 44, 164, {40, 60, 140, 255});

    char hb1[80], hb2[80];
    sprintf(hb1, "Total Probes (Chaining):         %d", totalProbesChain);
    sprintf(hb2, "Total Probes (Open Addressing):  %d", totalProbesOA);
    DrawText(hb1, 570, 178, 13, {100, 220, 255, 255});
    DrawText(hb2, 570, 196, 13, totalProbesOA > totalProbesChain ? (Color){255, 150, 60, 255} : GREEN);

    const char *hL[] = {
        "",
        "CHAINING:",
        "  Collisions -> linked list in bucket",
        "  Load factor alpha = n/m",
        "  Avg search: O(1 + alpha)",
        "  Never runs out of space",
        "  Extra memory for pointers",
        "",
        "OPEN ADDRESSING (Linear Probe):",
        "  Collision -> probe next slot",
        "  Clustering degrades performance",
        "  Avg search: O(1/(1-alpha))",
        "  alpha must stay < 1",
        "  Cache-friendly (contiguous)",
        "",
        "WHEN TO USE WHICH?",
        "  Chaining: unknown load, deletions",
        "  Open Addr: known load, speed critical",
        "  Real DBs use Robin Hood hashing",
        "  Python dicts use open addressing!",
    };
    for (int i = 0; i < 20; i++)
        DrawText(hL[i], 570, 214 + i * 20, 12, (i == 1 || i == 8 || i == 15) ? (Color){100, 200, 255, 255} : WHITE);

    DC("[SPACE]  The Alarm Sounds — Level 6: The Great Escape", SH - 34, 17, {0, 210, 80, 255});
}

// ── LEVEL 6 INTRO ──
static void DrawL6Intro()
{
    DrawBG({20, 0, 0, 255}, {40, 4, 4, 255});
    DrawHUD();
    Panel(60, 65, SW - 120, SH - 110, {22, 4, 4, 230}, {180, 20, 20, 255});
    DC("LEVEL 6:  THE GREAT ESCAPE", 90, 28, {255, 80, 80, 255});
    DrawLine(60, 130, SW - 60, 130, {140, 20, 20, 255});

    Panel(78, 148, 452, 490, {20, 6, 6, 220}, {200, 40, 40, 255});
    DrawText("DIJKSTRA'S ALGORITHM (Guards)", 96, 162, 16, {255, 120, 80, 255});
    const char *dl[] = {
        "Elite Guards spawn and use",
        "Dijkstra to find the absolute",
        "SHORTEST WEIGHTED PATH to you.",
        "",
        "Time: O(E + V log V)  (with heap)",
        "Space: O(V)",
        "",
        "Navigate 8-node escape graph:",
        "Floor1 -> Lobby -> Stairs -> Roof",
        "Use arrow keys to move between",
        "connected nodes.",
        "",
        "Guard recalculates Dijkstra path",
        "to YOUR position every 2 seconds.",
        "This is how game AI works!",
        "",
        "Hint: Dijkstra path (yellow) shows",
        "your shortest route to Exit.",
        "Guard path (red) shows their route.",
    };
    for (int i = 0; i < 19; i++)
        DrawText(dl[i], 96, 196 + i * 22, 13, (i == 4 || i == 12) ? (Color){255, 180, 80, 255} : WHITE);

    Panel(568, 148, SW - 636, 490, {10, 10, 22, 220}, {80, 80, 200, 255});
    DrawText("KMP STRING MATCHING", 586, 162, 18, SKYBLUE);
    const char *kl[] = {
        "The exit door requires a",
        "pattern match in a symbol stream.",
        "",
        "Text:    34-char data stream",
        "Pattern: ABRACADABRA (11 chars)",
        "",
        "NAIVE approach:",
        "  Try every position O(N*M)",
        "  Guards catch you — TOO SLOW!",
        "",
        "KMP (Knuth-Morris-Pratt):",
        "  Build failure function first.",
        "  Never re-scan matched chars.",
        "  Time: O(N+M)  ALWAYS!",
        "",
        "The failure function encodes",
        "pattern's self-overlap so we",
        "skip redundant comparisons.",
        "Choose KMP to open the door fast!",
    };
    for (int i = 0; i < 19; i++)
        DrawText(kl[i], 586, 196 + i * 22, 13, (i == 6 || i == 10 || i == 13) ? (Color){100, 200, 255, 255} : WHITE);

    DC("[SPACE]  Start The Escape!", SH - 44, 22, {255, 80, 0, 255});
}

// ── LEVEL 6 EXPLAIN ──
static void DrawL6Explain()
{
    DrawBG({18, 4, 4, 255}, {32, 6, 6, 255});
    DrawHUD();
    Panel(28, 58, SW - 56, SH - 76, {18, 6, 6, 235}, {160, 40, 40, 255});
    DC("LEVEL 6 DEBRIEF: Dijkstra + KMP", 78, 24, GOLD);
    DrawLine(28, 112, SW - 28, 112, {120, 40, 40, 255});

    if (chooseKMP_L6)
    {
        Panel(300, 122, SW - 600, 54, {20, 70, 20, 240}, GREEN);
        DC("You chose KMP  =>  Door opened fast!  +200 pts", 140, 18, GREEN);
    }
    else
    {
        Panel(240, 122, SW - 480, 54, {80, 30, 10, 240}, ORANGE);
        DC("You chose NAIVE MATCH  =>  Slow! Guard nearly caught you.  +80 pts", 140, 17, ORANGE);
    }

    // Left: Dijkstra
    Panel(36, 186, 490, 490, {16, 6, 6, 220}, {200, 60, 40, 255});
    DrawText("DIJKSTRA'S ALGORITHM", 52, 198, 17, {255, 130, 80, 255});
    DrawLine(52, 220, 518, 220, {100, 30, 20, 255});
    const char *dijL[] = {
        "Weighted shortest path algorithm.",
        "Uses a min-priority queue.",
        "",
        "Time:  O(E + V log V)  [min-heap]",
        "Space: O(V)",
        "",
        "How it works:",
        "  dist[src] = 0, all others = INF",
        "  Relax edges greedily by distance",
        "  Extract min-dist unvisited node",
        "  Update neighbors if shorter path",
        "",
        "WHY guards use it:",
        "  BFS only works for unweighted.",
        "  Dijkstra handles WEIGHTED edges.",
        "  Guards find TRUE shortest path.",
        "  Used in: GPS, routing protocols,",
        "  game AI, network path planning.",
        "",
        "Does NOT work with negative edges!",
        "Use Bellman-Ford for negatives.",
    };
    for (int i = 0; i < 21; i++)
        DrawText(dijL[i], 52, 234 + i * 19, 13, (i == 3 || i == 12 || i == 19) ? (Color){255, 160, 80, 255} : WHITE);

    // Right: KMP
    Panel(554, 186, SW - 590, 490, {10, 10, 22, 220}, {60, 80, 200, 255});
    DrawText("KMP STRING MATCHING", 570, 198, 17, SKYBLUE);
    DrawLine(570, 220, SW - 44, 220, {40, 60, 140, 255});

    char kb1[80], kb2[80], kb3[80];
    sprintf(kb1, "Naive operations:  %d", kmpNaiveOps);
    sprintf(kb2, "KMP   operations:  %d", kmpKmpOps);
    int saving = kmpNaiveOps - kmpKmpOps;
    sprintf(kb3, "KMP saved %d ops  (%.0f%% faster!)", saving, saving > 0 ? 100.f * saving / kmpNaiveOps : 0.f);

    DrawText(kb1, 570, 228, 14, ORANGE);
    DrawText(kb2, 570, 246, 14, {80, 220, 120, 255});
    DrawText(kb3, 570, 264, 13, GOLD);

    const char *kmpL[] = {
        "",
        "Failure Function (partial match):",
        "  Encodes longest proper prefix",
        "  that is also a suffix.",
        "  Built in O(M) time.",
        "",
        "Pattern: ABRACADABRA",
        "Failure: 0 0 0 1 0 1 0 1 2 3 4",
        "  'ABRA' at end matches 'ABRA'",
        "  at start — skip 4 chars!",
        "",
        "Naive: O(N*M) — re-scans chars",
        "KMP  : O(N+M) — never re-scans",
        "",
        "Used in: grep, text editors,",
        "bioinformatics (DNA search),",
        "network intrusion detection.",
        "",
        "N=34, M=11:",
    };
    for (int i = 0; i < 19; i++)
        DrawText(kmpL[i], 570, 284 + i * 19, 13, (i == 11 || i == 12 || i == 6) ? (Color){100, 200, 255, 255} : WHITE);

    char nLine[80];
    sprintf(nLine, "  Naive ~%d ops, KMP ~%d ops", kmpNaiveOps, kmpKmpOps);
    DrawText(nLine, 570, 642, 13, GOLD);

    DC("[SPACE]  View Final Mission Summary", SH - 34, 17, {0, 210, 80, 255});
}

//  INTRO CARDS

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
    };
    for (int i = 0; i < 10; i++)
        DrawText(tc[i], 586, 196 + i * 23, 13, (i == 8 || i == 9) ? (Color){80, 255, 120, 255} : WHITE);

    DC("[SPACE]  Approach the Grid", SH - 44, 21, {80, 160, 255, 255});
}

//  MAIN

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

        //Update
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
                gState = S_L5_INTRO;
                stTimer = 0;
                // Init L5
                initFractKS();
                initHashSim();
                vaultTimer = 0.f;
                l5HashPhase = false;
                l5AniTimer = 0.f;
            }
            break;

        // ── LEVEL 5 ──
        case S_L5_INTRO:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L5_PLAY;
                stTimer = 0;
                initFractKS();
                initHashSim();
                vaultTimer = 0.f;
                l5HashPhase = false;
                l5AniTimer = 0.f;
            }
            break;

        case S_L5_PLAY:
        {
            vaultTimer += dt;
            l5AniTimer += dt;

            if (!l5HashPhase)
            {
                // Fractional knapsack animation: advance step every 0.7s
                if (l5AniTimer >= 0.7f && fkAniStep < FKN)
                {
                    fkAniStep++;
                    stepFractKS();
                    l5AniTimer = 0.f;
                }
                if (fkAniStep >= FKN)
                {
                    fkDone = true;
                }
                // Move to hash phase when fk done or space
                if (fkDone || IsKeyPressed(KEY_SPACE))
                {
                    l5HashPhase = true;
                    l5AniTimer = 0.f;
                    // Run all frac ks immediately if skipped
                    if (!fkDone)
                    {
                        fkAniStep = FKN;
                        stepFractKS();
                        fkDone = true;
                    }
                }
            }
            else
            {
                // Hash animation: advance key every 0.6s
                if (l5AniTimer >= 0.6f && !hashDone)
                {
                    stepHashSim();
                    l5AniTimer = 0.f;
                }

                // Choice buttons for Chaining vs OA
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    if (CheckCollisionPointRec(mouse, {100.f, 690, 240, 46}))
                    {
                        chooseChaining_L5 = true;
                        score += hashDone ? 200 : 150;
                        if (!hashDone)
                        { // force complete
                            while (!hashDone)
                                stepHashSim();
                        }
                        gState = S_L5_EXPLAIN;
                        stTimer = 0;
                    }
                    if (CheckCollisionPointRec(mouse, {SW - 340.f, 690, 240, 46}))
                    {
                        chooseChaining_L5 = false;
                        score += 120;
                        if (!hashDone)
                        {
                            while (!hashDone)
                                stepHashSim();
                        }
                        gState = S_L5_EXPLAIN;
                        stTimer = 0;
                    }
                }
            }

            // Vault timer expired
            if (vaultTimer >= VAULT_TIME)
            {
                // force complete everything and proceed
                if (!fkDone)
                {
                    fkAniStep = FKN;
                    stepFractKS();
                    fkDone = true;
                }
                while (!hashDone)
                    stepHashSim();
                score += 100; // partial credit
                gState = S_L5_EXPLAIN;
                stTimer = 0;
            }
            break;
        }

        case S_L5_EXPLAIN:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L6_INTRO;
                stTimer = 0;
            }
            break;

        // ── LEVEL 6 ──
        case S_L6_INTRO:
            if (IsKeyPressed(KEY_SPACE))
            {
                gState = S_L6_PLAY;
                stTimer = 0;
                initL6();
            }
            break;

        case S_L6_PLAY:
        {
            l6EscapeTime += dt;
            l6HighlightTimer += dt;

            // Player navigation on escape graph (arrow keys = go forward along dijkstra path)
            // Use LEFT/RIGHT to cycle adjacent nodes, ENTER/SPACE to move
            static int l6Selected = -1;
            if (IsKeyPressed(KEY_SPACE) && !kmpSearchDone && l6PlayerNode == 4)
            {
                // Run KMP to get exit code (only at Exit node)
                runKMP();
                if (chooseKMP_L6)
                    score += 200;
                else
                    score += 80;
            }

            // Navigate: press number key 1-8 or arrow keys to move to adjacent node
            // Simplified: press arrow keys to move along shortest path
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN))
            {
                // Move forward along dijkstra path toward exit
                if (dijkPath.size() > 1)
                {
                    auto it = find(dijkPath.begin(), dijkPath.end(), l6PlayerNode);
                    if (it != dijkPath.end() && next(it) != dijkPath.end())
                    {
                        int nxt = *next(it);
                        if (egAdj[l6PlayerNode][nxt])
                        {
                            l6PlayerNode = nxt;
                            runDijkstra(l6PlayerNode);
                            // Recompute guard path
                            guardDijkstra(l6GuardNode, l6PlayerNode);
                        }
                    }
                }
            }
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP))
            {
                // Move backward (retreat)
                auto it = find(dijkPath.begin(), dijkPath.end(), l6PlayerNode);
                if (it != dijkPath.begin())
                {
                    int prv = *prev(it);
                    if (egAdj[l6PlayerNode][prv])
                    {
                        l6PlayerNode = prv;
                        runDijkstra(l6PlayerNode);
                        guardDijkstra(l6GuardNode, l6PlayerNode);
                    }
                }
            }

            // Guard moves every 2.5s using Dijkstra
            if (kmpSearchDone)
            { // guard activates after KMP solved
                l6GuardActive = true;
            }
            if (l6GuardActive)
            {
                l6GuardTimer += dt;
                if (l6GuardTimer >= 2.5f)
                {
                    l6GuardTimer = 0.f;
                    // Guard moves one step along its Dijkstra path to player
                    guardDijkstra(l6GuardNode, l6PlayerNode);
                    if (guardDijkPath.size() > 1)
                    {
                        l6GuardNode = guardDijkPath[1];
                    }
                }
            }

            // Check caught
            if (l6GuardNode == l6PlayerNode)
            {
                gState = S_L6_CAUGHT;
                stTimer = 0;
            }

            // Check won: player reached Exit (node 4) AND KMP done
            if (l6PlayerNode == 4 && kmpSearchDone)
            {
                score += 250 + max(0, 200 - (int)l6EscapeTime * 5);
                gState = S_L6_EXPLAIN;
                stTimer = 0;
            }

            // Allow direct proceed if KMP done and at exit
            if (l6PlayerNode == 4 && !kmpSearchDone)
            {
                // Prompt to press space for KMP
            }

            break;
        }

        case S_L6_CAUGHT:
            if (IsKeyPressed(KEY_R))
            {
                initL6();
                gState = S_L6_PLAY;
                stTimer = 0;
            }
            if (IsKeyPressed(KEY_M))
            {
                score = 0;
                gState = S_MENU;
            }
            break;

        case S_L6_EXPLAIN:
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

        // ── DRAW ───
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
            Panel(80, 130, SW - 160, 500, {14, 14, 36, 230}, {55, 55, 110, 255});
            const char *st[] = {
                "You are GHOST:  elite hacker and data thief.",
                "",
                "Your target: The Secure Nexus Data Vault.",
                "Six security systems stand in your way.",
                "Each one is powered by a real DAA algorithm.",
                "",
                "Level 1  — Pack your loadout     (0/1 Knapsack: Greedy vs DP)",
                "Level 2  — Navigate the maze     (BFS pathfinding)",
                "Level 3  — Evade the guard       (Sorting + BFS guard AI)",
                "Level 4  — Bypass laser grid     (TSP: Held-Karp DP vs Brute Force)",
                "Level 5  — Grab the payload      (Fractional Knapsack + Hashing)",
                "Level 6  — The Great Escape      (Dijkstra Guards + KMP Pattern Match)",
                "",
                "After EVERY choice you will see a full explanation of",
                "WHY that algorithm scored more or less.",
                "",
                "Better algorithm  =  more points  =  deeper understanding.",
            };
            for (int i = 0; i < 17; i++)
                DrawText(st[i], 105, 155 + i * 28, 17, (i == 0 || i == 6 || i == 7 || i == 8 || i == 9 || i == 10 || i == 11) ? (Color){GOLD.r, GOLD.g, GOLD.b, 255} : WHITE);
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

        // ── LEVEL 5 DRAW ──
        else if (gState == S_L5_INTRO)
        {
            DrawL5Intro();
        }
        else if (gState == S_L5_PLAY)
        {
            DrawBG({12, 8, 2, 255}, {24, 16, 4, 255});
            DrawHUD();

            // Timer bar
            float rem = max(0.f, VAULT_TIME - vaultTimer);
            Color tc = rem > 30 ? GREEN : rem > 15 ? ORANGE
                                                   : RED;
            char timebuf[32];
            sprintf(timebuf, "VAULT TIMER: %.0fs", rem);
            DC(timebuf, 56, 22, tc);
            DrawRectangle(200, 728, (int)(700 * rem / VAULT_TIME), 14, tc);
            DrawRectangleLines(200, 728, 700, 14, GRAY);

            if (!l5HashPhase)
            {
                // ── Phase 1: Fractional Knapsack ──
                DC("PHASE 1: FRACTIONAL KNAPSACK — Grab the Liquid Gold & Data Dust!", 82, 18, GOLD);

                // Left panel: sorted items list
                Panel(20, 108, 320, 530, {14, 18, 4, 220}, {180, 200, 40, 255});
                DrawText("Items (sorted by V/W ratio)", 30, 118, 14, {220, 240, 80, 255});
                DrawText("Name         Wt   Val  Ratio Taken", 28, 140, 12, LIGHTGRAY);
                for (int si = 0; si < FKN; si++)
                {
                    int idx = fkSorted[si];
                    char buf[80];
                    float taken = fkTaken[idx];
                    Color rc = taken > 0.99f ? GREEN : taken > 0.f ? YELLOW
                                                                   : LIGHTGRAY;
                    sprintf(buf, "%-12s %.1f  %.1f  %.1f  %.0f%%",
                            fkItems[idx].name, fkItems[idx].weight, fkItems[idx].value,
                            fkItems[idx].ratio, taken * 100.f);
                    DrawText(buf, 28, 162 + si * 36, 13, rc);
                    // bar showing fraction taken
                    DrawRectangle(28, 182 + si * 36, (int)(120 * taken), 10, rc);
                    DrawRectangleLines(28, 182 + si * 36, 120, 10, DARKGRAY);
                }

                // Right: Bag fill visualization
                Panel(360, 108, 440, 530, {10, 16, 4, 220}, {140, 200, 40, 255});
                DrawText("Vault Bag  (Capacity: 10 kg)", 374, 118, 14, {180, 240, 60, 255});
                float totalW = 0.f;
                for (int i = 0; i < FKN; i++)
                    totalW += fkTaken[i] * fkItems[i].weight;
                char vbuf[64];
                sprintf(vbuf, "Filled: %.1f / 10.0 kg   Value: %.1f", totalW, fkTotalVal);
                DrawText(vbuf, 374, 140, 13, GOLD);

                // Stacked bar showing bag contents
                int bx = 374, by = 170, bw = 380, bh = 300;
                DrawRectangle(bx, by, bw, bh, {20, 20, 10, 180});
                DrawRectangleLines(bx, by, bw, bh, {100, 120, 40, 255});
                Color itemColors[6] = {GOLD, {200, 255, 100, 255}, SKYBLUE, ORANGE, {200, 100, 255, 255}, {100, 200, 180, 255}};
                float fillX = 0.f;
                for (int si = 0; si < FKN; si++)
                {
                    int idx = fkSorted[si];
                    float w = fkTaken[idx] * fkItems[idx].weight;
                    if (w < 0.01f)
                        continue;
                    int segW = (int)(bw * w / FK_CAPACITY);
                    DrawRectangle(bx + (int)(fillX * bw / FK_CAPACITY), by, segW, bh, itemColors[si % 6]);
                    if (segW > 20)
                    {
                        DrawText(fkItems[idx].name, bx + (int)(fillX * bw / FK_CAPACITY) + 2, by + bh / 2 - 6, 10, BLACK);
                    }
                    fillX += w;
                }

                // Complexity note
                Panel(360, 648, 440, 68, {10, 20, 4, 200}, {100, 160, 40, 255});
                DrawText("Greedy Fractional KS: O(n log n)", 370, 658, 13, {180, 240, 80, 255});
                DrawText("Sort by ratio -> always optimal here!", 370, 678, 12, WHITE);

                // Info panel
                Panel(820, 108, 260, 530, {14, 14, 4, 220}, {160, 140, 30, 255});
                DrawText("Algorithm Info", 832, 118, 14, GOLD);
                DrawText("Step: sort by V/W,", 832, 140, 12, WHITE);
                DrawText("greedily fill bag.", 832, 156, 12, WHITE);
                DrawText("", 832, 172, 12, WHITE);
                DrawText("Unlike 0/1 KS,", 832, 188, 12, WHITE);
                DrawText("fractions allowed!", 832, 204, 12, WHITE);
                DrawText("Greedy = OPTIMAL", 832, 220, 12, {180, 255, 80, 255});
                char stbuf[32];
                sprintf(stbuf, "Step: %d/%d", fkAniStep, FKN);
                DrawText(stbuf, 832, 256, 13, SKYBLUE);
                DrawText("Watch items fill", 832, 280, 12, LIGHTGRAY);
                DrawText("the bag in order", 832, 296, 12, LIGHTGRAY);
                DrawText("of best ratio.", 832, 312, 12, LIGHTGRAY);
                DrawText("[SPACE] skip to", 832, 380, 12, YELLOW);
                DrawText("hashing phase", 832, 396, 12, YELLOW);
            }
            else
            {
                // ── Phase 2: Hash Collision ──
                DC("PHASE 2: HASH COLLISION RESOLUTION — Download the Master File!", 82, 18, {100, 200, 255, 255});

                // Left: Chaining table
                Panel(20, 108, 330, 560, {8, 10, 22, 220}, {80, 100, 220, 255});
                DrawText("CHAINING  (key mod 7)", 30, 118, 14, {120, 180, 255, 255});
                DrawText("Bucket  Keys (chain)          Probes", 28, 140, 11, LIGHTGRAY);
                for (int i = 0; i < HASH_TABLE_SIZE; i++)
                {
                    char buf[80] = "";
                    int off = 0;
                    off += sprintf(buf + off, "[%d]  ", i);
                    for (int k : hashChainTable[i])
                        off += sprintf(buf + off, "%d ", k);
                    DrawRectangle(28, 162 + i * 60, 290, 50,
                                  hashChainTable[i].empty() ? (Color){20, 20, 40, 180} : (Color){30, 60, 120, 200});
                    DrawRectangleLines(28, 162 + i * 60, 290, 50, {80, 100, 180, 255});
                    DrawText(buf, 32, 168 + i * 60, 13, hashChainTable[i].empty() ? DARKGRAY : WHITE);
                    if (!hashChainTable[i].empty())
                    {
                        char pb[16];
                        sprintf(pb, "probes:%d", hashProbesChain[i]);
                        DrawText(pb, 32, 184 + i * 60, 11, {160, 220, 255, 255});
                    }
                }
                char ctot[40];
                sprintf(ctot, "Total probes: %d", totalProbesChain);
                DrawText(ctot, 28, 584, 14, {120, 220, 255, 255});

                // Right: Open Addressing table
                Panel(370, 108, 330, 560, {8, 20, 10, 220}, {60, 180, 80, 255});
                DrawText("OPEN ADDRESSING (Linear Probe)", 380, 118, 13, {80, 220, 120, 255});
                DrawText("Slot   Key    Probes needed", 378, 140, 11, LIGHTGRAY);
                for (int i = 0; i < HASH_TABLE_SIZE; i++)
                {
                    bool occ = hashOATable[i] != -1;
                    DrawRectangle(378, 162 + i * 60, 290, 50,
                                  occ ? (Color){20, 80, 30, 200} : (Color){20, 30, 20, 180});
                    DrawRectangleLines(378, 162 + i * 60, 290, 50, {60, 160, 80, 255});
                    char buf[40];
                    if (occ)
                    {
                        sprintf(buf, "[%d]  key=%d   probes=%d", i, hashOATable[i], hashProbesOA[i]);
                        DrawText(buf, 382, 180 + i * 60, 13, WHITE);
                    }
                    else
                    {
                        sprintf(buf, "[%d]  EMPTY", i);
                        DrawText(buf, 382, 180 + i * 60, 13, DARKGRAY);
                    }
                }
                char otot[40];
                sprintf(otot, "Total probes: %d", totalProbesOA);
                DrawText(otot, 378, 584, 14, {80, 220, 120, 255});

                // Info panel
                Panel(720, 108, 360, 410, {10, 10, 22, 220}, {80, 80, 200, 255});
                DrawText("Hash Collision Info", 730, 118, 14, SKYBLUE);
                char kb[32];
                sprintf(kb, "Keys: step %d/%d", hashAniStep, HN);
                DrawText(kb, 730, 142, 13, WHITE);
                DrawText("Hash fn: key mod 7", 730, 162, 13, LIGHTGRAY);
                DrawText("", 730, 178, 13, WHITE);
                DrawText("CHAINING:", 730, 194, 13, {120, 180, 255, 255});
                DrawText("  Linked list per bucket", 730, 210, 12, WHITE);
                DrawText("  O(1+alpha) search avg", 730, 226, 12, WHITE);
                DrawText("OPEN ADDR:", 730, 248, 13, {80, 220, 120, 255});
                DrawText("  Linear probe next slot", 730, 264, 12, WHITE);
                DrawText("  Clustering degrades", 730, 280, 12, WHITE);
                DrawText("  O(1/(1-alpha)) avg", 730, 296, 12, WHITE);
                DrawText("", 730, 312, 12, WHITE);
                if (!hashDone)
                    DrawText("Inserting keys...", 730, 328, 13, YELLOW);
                else
                    DrawText("All keys inserted!", 730, 328, 13, GREEN);

                // Choice buttons (shown after hash phase active)
                Panel(720, 528, 360, 130, {10, 10, 22, 200}, {60, 60, 160, 255});
                DrawText("Choose collision method:", 730, 540, 14, WHITE);
                DrawText("(affects download speed)", 730, 558, 12, LIGHTGRAY);
                bool hCh = CheckCollisionPointRec(mouse, {730.f, 580, 160, 40});
                bool hOA = CheckCollisionPointRec(mouse, {900.f, 580, 160, 40});
                DrawRectangleRounded({730.f, 580, 160, 40}, 0.25f, 6, hCh ? (Color){100, 160, 255, 255} : (Color){40, 80, 160, 255});
                DrawText("CHAINING (+200)", 736, 592, 13, WHITE);
                DrawRectangleRounded({900.f, 580, 160, 40}, 0.25f, 6, hOA ? (Color){80, 200, 100, 255} : (Color){30, 100, 50, 255});
                DrawText("OPEN ADDR (+120)", 906, 592, 12, WHITE);

                // Override button rects to match update logic
                // (already matched: {100.f, 690, 240, 46} and {SW-340.f, 690, 240, 46} in update)
                // Redraw them properly positioned
                bool hCh2 = CheckCollisionPointRec(mouse, {100.f, 690, 240, 46});
                bool hOA2 = CheckCollisionPointRec(mouse, {SW - 340.f, 690, 240, 46});
                DrawRectangleRounded({100.f, 690, 240, 46}, 0.25f, 6, hCh2 ? (Color){100, 160, 255, 255} : (Color){40, 80, 160, 255});
                DrawText("CHAINING (+200)", 112, 704, 15, WHITE);
                DrawRectangleRounded({SW - 340.f, 690, 240, 46}, 0.25f, 6, hOA2 ? (Color){80, 200, 100, 255} : (Color){30, 100, 50, 255});
                DrawText("OPEN ADDRESSING (+120)", SW - 334, 704, 13, WHITE);
            }
        }
        else if (gState == S_L5_EXPLAIN)
        {
            DrawL5Explain();
        }

        // ── LEVEL 6 DRAW ──
        else if (gState == S_L6_INTRO)
        {
            DrawL6Intro();
            // KMP choice buttons
            bool hKMP = CheckCollisionPointRec(mouse, {SW - 420.f, SH - 90, 220, 52});
            bool hNv = CheckCollisionPointRec(mouse, {200.f, SH - 90, 220, 52});
            Color cKMP = chooseKMP_L6 ? (Color){80, 220, 100, 255} : hKMP ? (Color){50, 180, 70, 255}
                                                                          : (Color){30, 120, 50, 255};
            Color cNv = !chooseKMP_L6 ? (Color){255, 140, 60, 255} : hNv ? (Color){200, 90, 40, 255}
                                                                         : (Color){140, 60, 20, 255};
            DrawRectangleRounded({200.f, SH - 90, 220, 52}, 0.25f, 6, cNv);
            DrawText(!chooseKMP_L6 ? "NAIVE (selected)" : "NAIVE MATCH", 215, SH - 74, 17, WHITE);
            DrawRectangleRounded({SW - 420.f, SH - 90, 220, 52}, 0.25f, 6, cKMP);
            DrawText(chooseKMP_L6 ? "KMP (selected)" : "KMP MATCH", SW - 410, SH - 74, 17, WHITE);

            // Handle clicks for L6 intro choice
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, {200.f, SH - 90, 220, 52}))
                    chooseKMP_L6 = false;
                if (CheckCollisionPointRec(mouse, {SW - 420.f, SH - 90, 220, 52}))
                    chooseKMP_L6 = true;
            }
        }
        else if (gState == S_L6_PLAY)
        {
            DrawBG({10, 4, 20, 255}, {20, 8, 36, 255});
            DrawHUD();
            DC("LEVEL 6: THE GREAT ESCAPE", 56, 24, Color{255, 80, 80, 255});

            // ── Draw escape graph ──

            // Draw lane label backgrounds
            DrawRectangle(30, 88, 200, 22, Color{0, 0, 0, 120});
            DrawText("== ESCAPE ROUTE ==", 34, 92, 12, Color{255, 220, 80, 180});
            DrawRectangle(30, 560, 220, 22, Color{0, 0, 0, 120});
            DrawText("== GUARD ZONE ==", 34, 564, 12, Color{255, 80, 80, 180});

            // Step 1: Draw dim base edges
            for (int i = 0; i < EGN; i++)
                for (int j = i + 1; j < EGN; j++)
                    if (egAdj[i][j])
                    {
                        // Color edges: main path edges (0-1-2-3-4) are teal, guard zone edges are dark red
                        bool mainEdge = (i < 5 && j < 5);
                        Color ec = mainEdge ? Color{30, 120, 140, 140} : Color{100, 30, 30, 140};
                        DrawLineEx(egPos[i], egPos[j], 2.5f, ec);
                    }

            // Step 2: Guard path (red glow, drawn under yellow)
            for (int i = 0; i + 1 < (int)guardDijkPath.size(); i++)
            {
                int a = guardDijkPath[i], b = guardDijkPath[i + 1];
                DrawLineEx(egPos[a], egPos[b], 10.f, Color{255, 30, 30, 45});
                DrawLineEx(egPos[a], egPos[b], 4.f, Color{255, 70, 70, 220});
            }

            // Step 3: Player shortest path (yellow glow)
            for (int i = 0; i + 1 < (int)dijkPath.size(); i++)
            {
                int a = dijkPath[i], b = dijkPath[i + 1];
                DrawLineEx(egPos[a], egPos[b], 12.f, Color{255, 230, 0, 45});
                DrawLineEx(egPos[a], egPos[b], 5.f, Color{255, 220, 0, 230});
            }

            // Step 4: Edge weight labels
            for (int i = 0; i < EGN; i++)
                for (int j = i + 1; j < EGN; j++)
                    if (egAdj[i][j])
                    {
                        Vector2 mid = {(egPos[i].x + egPos[j].x) / 2, (egPos[i].y + egPos[j].y) / 2};
                        char wb[8];
                        sprintf(wb, "%d", egAdj[i][j]);
                        int tw = MeasureText(wb, 12);
                        DrawRectangleRounded({mid.x - tw / 2.f - 5, mid.y - 11, (float)(tw + 10), 16}, 0.4f, 4, Color{10, 10, 20, 200});
                        DrawText(wb, (int)mid.x - tw / 2, (int)mid.y - 10, 12, Color{255, 210, 70, 255});
                    }

            // Step 5: Draw nodes
            for (int i = 0; i < EGN; i++)
            {
                bool isPlayer = (i == l6PlayerNode);
                bool isGuard = (i == l6GuardNode);
                bool isExit = (i == 4);
                bool isGuardZone = (i == 5 || i == 6 || i == 7);

                // Outer glow
                if (isPlayer)
                    DrawCircle((int)egPos[i].x, (int)egPos[i].y, 40, Color{30, 160, 255, 40});
                else if (isGuard)
                    DrawCircle((int)egPos[i].x, (int)egPos[i].y, 40, Color{255, 40, 40, 40});
                else if (isExit)
                    DrawCircle((int)egPos[i].x, (int)egPos[i].y, 40, Color{0, 255, 100, 40});

                // Node fill color
                Color nc = isExit ? Color{0, 190, 65, 255} : isGuard   ? Color{200, 25, 25, 255}
                                                         : isPlayer    ? Color{25, 100, 220, 255}
                                                         : isGuardZone ? Color{80, 25, 25, 255}
                                                                       : Color{30, 60, 80, 255};
                DrawCircle((int)egPos[i].x, (int)egPos[i].y, 28, nc);

                // Border ring
                Color bc = isPlayer ? Color{80, 210, 255, 255} : isGuard   ? Color{255, 100, 100, 255}
                                                             : isExit      ? Color{60, 255, 110, 255}
                                                             : isGuardZone ? Color{180, 60, 60, 200}
                                                                           : Color{60, 160, 190, 200};
                DrawCircleLines((int)egPos[i].x, (int)egPos[i].y, 28, bc);
                DrawCircleLines((int)egPos[i].x, (int)egPos[i].y, 29, bc);

                // Icon symbol inside node
                if (isExit)
                {
                    DrawText("EXIT", (int)egPos[i].x - MeasureText("EXIT", 11) / 2, (int)egPos[i].y - 6, 11, WHITE);
                }
                else
                {
                    DrawText(egNames[i], (int)egPos[i].x - MeasureText(egNames[i], 10) / 2, (int)egPos[i].y - 6, 10, WHITE);
                }

                // Distance badge (pill below node)
                if (dijkDist[i] < INT_MAX)
                {
                    char db[16];
                    sprintf(db, "d=%d", dijkDist[i]);
                    int dw = MeasureText(db, 10);
                    DrawRectangleRounded({(float)((int)egPos[i].x - dw / 2 - 4), (float)((int)egPos[i].y + 32), (float)(dw + 8), 14}, 0.5f, 4, Color{15, 15, 25, 210});
                    DrawText(db, (int)egPos[i].x - dw / 2, (int)egPos[i].y + 33, 10, Color{220, 220, 80, 255});
                }
            }

            // Floating YOU / GUARD labels
            DrawText("YOU",
                     (int)egPos[l6PlayerNode].x - MeasureText("YOU", 14) / 2,
                     (int)egPos[l6PlayerNode].y - 50, 14, Color{80, 210, 255, 255});
            if (l6GuardNode != l6PlayerNode)
                DrawText("GUARD",
                         (int)egPos[l6GuardNode].x - MeasureText("GUARD", 13) / 2,
                         (int)egPos[l6GuardNode].y - 50, 13, Color{255, 90, 90, 255});

            // Right info panel
            Panel(820, 62, 260, 560, Color{10, 5, 25, 220}, Color{100, 60, 180, 255});
            DrawText("Escape HUD", 832, 72, 15, Color{180, 130, 255, 255});
            char cnb[32], gnb[32], tb[32];
            sprintf(cnb, "You: %s", egNames[l6PlayerNode]);
            sprintf(gnb, "Guard: %s", egNames[l6GuardNode]);
            sprintf(tb, "Time: %.1fs", l6EscapeTime);
            DrawText(cnb, 832, 96, 13, SKYBLUE);
            DrawText(gnb, 832, 114, 13, RED);
            DrawText(tb, 832, 132, 13, WHITE);
            DrawText("CONTROLS:", 832, 158, 13, GOLD);
            DrawText("[RIGHT/DOWN]", 832, 176, 12, WHITE);
            DrawText("  -> Move toward Exit", 832, 192, 12, WHITE);
            DrawText("[LEFT/UP]", 832, 210, 12, WHITE);
            DrawText("  -> Move back", 832, 226, 12, WHITE);
            DrawText("Yellow = your path", 832, 254, 12, YELLOW);
            DrawText("Red    = guard route", 832, 270, 12, RED);
            DrawText("GOAL:", 832, 298, 13, GOLD);
            DrawText("Reach EXIT node!", 832, 314, 12, GREEN);
            DrawText("Then press [SPACE]", 832, 330, 12, GREEN);
            DrawText("for KMP code match.", 832, 346, 12, GREEN);

            if (!l6GuardActive)
            {
                DrawRectangle(822, 378, 248, 60, Color{20, 10, 0, 160});
                DrawText("Guard is dormant!", 832, 384, 12, ORANGE);
                DrawText("Activates after", 832, 400, 12, ORANGE);
                DrawText("you solve KMP.", 832, 416, 12, ORANGE);
            }
            else
            {
                DrawRectangle(822, 378, 248, 60, Color{40, 0, 0, 160});
                DrawText("GUARD ACTIVE!", 832, 384, 13, RED);
                DrawText("Dijkstra chasing", 832, 400, 12, RED);
                DrawText("your position!", 832, 416, 12, RED);
            }

            // KMP panel at bottom
            Panel(20, 628, 790, 108, Color{8, 8, 20, 220}, Color{60, 80, 200, 255});
            DrawText("KMP PATTERN MATCH — Exit Door Code", 30, 638, 14, SKYBLUE);
            DrawText("Text stream:", 30, 660, 12, LIGHTGRAY);
            DrawText(kmpText.c_str(), 120, 660, 12, WHITE);
            DrawText("Pattern:    ", 30, 678, 12, LIGHTGRAY);
            DrawText(kmpPat.c_str(), 120, 678, 12, YELLOW);

            if (!kmpSearchDone)
            {
                DrawText(l6PlayerNode == 4 ? "[SPACE] Run KMP to open door!" : "Reach EXIT node first!", 30, 700, 13,
                         l6PlayerNode == 4 ? GREEN : ORANGE);
                char kmpchoice[64];
                sprintf(kmpchoice, "Method: %s", chooseKMP_L6 ? "KMP O(N+M)" : "Naive O(N*M)");
                DrawText(kmpchoice, 560, 700, 12, chooseKMP_L6 ? GREEN : ORANGE);
            }
            else
            {
                char res[80];
                sprintf(res, "Pattern found at index %d!  KMP ops: %d  Naive ops: %d",
                        kmpFoundIdx, kmpKmpOps, kmpNaiveOps);
                DrawText(res, 30, 700, 12, GREEN);
                DrawText("Door OPEN! Move to finish.", 30, 716, 12, Color{80, 255, 80, 255});
            }
        }
        else if (gState == S_L6_CAUGHT)
        {
            DrawBG({30, 0, 0, 255}, {60, 0, 0, 255});
            DrawHUD();
            DC("CAUGHT BY THE ELITE GUARD!", SH / 2 - 90, 40, RED);
            DC("The guard used Dijkstra to find the SHORTEST WEIGHTED path to you.", SH / 2 - 30, 18, WHITE);
            DC("Unlike BFS, Dijkstra respects edge weights — always optimal!", SH / 2 + 10, 17, ORANGE);
            DC("Tip: Move fast along the yellow Dijkstra path to Exit.", SH / 2 + 46, 16, LIGHTGRAY);
            DC("[R] Retry Level 6      [M] Main Menu", SH / 2 + 108, 22, YELLOW);
        }
        else if (gState == S_L6_EXPLAIN)
        {
            DrawL6Explain();
        }

        // ── WIN SCREEN ──
        else if (gState == S_WIN)
        {
            DrawBG({4, 18, 4, 255}, {6, 35, 6, 255});
            DrawHUD();
            float flash = (sinf(gTimer * 3.f) + 1.f) * 0.5f;
            DC("MISSION ACCOMPLISHED!", (int)(78), 42, {(uint8_t)(180 + 75 * flash), 220, 0, 255});
            DC("The vault is yours, Ghost. You cracked every algorithm.", 128, 20, WHITE);
            Panel(60, 162, SW - 120, 470, {8, 22, 8, 218}, GOLD);
            char fb[64];
            sprintf(fb, "FINAL SCORE: %d", score);
            DC(fb, 182, 30, GOLD);
            DrawLine(70, 224, SW - 70, 224, {60, 100, 60, 255});

            // Header
            DrawText("Level", 82, 234, 14, LIGHTGRAY);
            DrawText("Algorithm", 270, 234, 14, LIGHTGRAY);
            DrawText("Your Choice", 500, 234, 14, LIGHTGRAY);
            DrawText("Key Lesson", 720, 234, 14, LIGHTGRAY);
            DrawLine(70, 254, SW - 70, 254, {60, 100, 60, 255});

            struct WinRow
            {
                const char *lvl;
                const char *algo;
                const char *choice;
                const char *lesson;
                Color c;
            };
            WinRow rows[] = {
                {"1 Knapsack", "0/1 KS (DP)", chooseDP_L1 ? "DP (+200)" : "Greedy (+80)", "DP optimal; Greedy fails 0/1", SKYBLUE},
                {"2 BFS Maze", "BFS O(V+E)", "BFS hint", "Shortest path in unweighted graph", GREEN},
                {"3 Stealth", "QuickSort/Bubble", chooseQuick_L3 ? "QuickSort" : "BubbleSort", "O(NlogN) far quieter than O(N^2)", ORANGE},
                {"4 TSP Laser", "Held-Karp/BruteF", chooseDP_L4 ? "Held-Karp(+200)" : "BruteF(+80)", "2^N*N^2 beats N! for large N", {180, 100, 255, 255}},
                {"5 Vault", "Frac KS + Hashing", chooseChaining_L5 ? "Chaining" : "Open Addr", "Greedy optimal when fractions OK", GOLD},
                {"6 Escape", "Dijkstra + KMP", chooseKMP_L6 ? "KMP O(N+M)" : "Naive O(NM)", "KMP never re-scans; Dijkstra weighted", {255, 120, 120, 255}},
            };
            for (int i = 0; i < 6; i++)
            {
                DrawText(rows[i].lvl, 82, 268 + i * 54, 14, rows[i].c);
                DrawText(rows[i].algo, 270, 268 + i * 54, 13, WHITE);
                DrawText(rows[i].choice, 500, 268 + i * 54, 13, WHITE);
                DrawText(rows[i].lesson, 720, 268 + i * 54, 12, LIGHTGRAY);
            }

            DrawLine(70, 594, SW - 70, 594, {60, 100, 60, 255});
            DC("Algorithm complexity is not just theory — it's the difference between escape and capture.", 610, 14, {80, 220, 100, 255});
            Panel(380, 650, SW - 760, 56, {10, 10, 10, 180}, {80, 80, 80, 255});
            DC("[M] Return to Menu", 668, 19, {140, 140, 160, 255});
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
