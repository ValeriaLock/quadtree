#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include "QuadTree.h"

using namespace std;

const int WIDTH = 1000;
const int HEIGHT = 800;

const int MAX_FOOD = 35;         
const int MAX_ENEMIES = 15;      
const double MAX_PLAYER_RADIUS = 65.0; 
const double MAX_ENEMY_RADIUS = 80.0;  

const int VIS_PARTICLE_COUNT = 100; 

enum Screen { MENU, VISUALIZATION, GAME_MODE };
Screen currentScreen = MENU;

Particle player;
vector<Particle> entities; 
vector<Particle> visParticles; 
int nextId = 1;

void SpawnFood() {
    entities.push_back({ nextId++, (double)(rand() % (WIDTH - 40) + 20), (double)(rand() % (HEIGHT - 40) + 20), 0, 0, 4.0, true, false, false, false });
}

void SpawnEnemy() {
    double vx = ((rand() % 100) / 50.0) - 1.0;
    double vy = ((rand() % 100) / 50.0) - 1.0;
    if (vx == 0 && vy == 0) { vx = 1; vy = 1; }
    entities.push_back({ nextId++, (double)(rand() % (WIDTH - 100) + 50), (double)(rand() % (HEIGHT - 100) + 50), 
                          vx * 2.5, vy * 2.5, (double)(rand() % 15 + 15), false, true, false, false });
}

void SetupVisualizationMode() {
    visParticles.clear();
    nextId = 1;
    for (int i = 0; i < VIS_PARTICLE_COUNT; i++) {
        double vx = ((rand() % 100) / 50.0) - 1.0;
        double vy = ((rand() % 100) / 50.0) - 1.0;
        if (vx == 0 && vy == 0) { vx = 0.5; vy = 0.5; }
        
        visParticles.push_back({
            nextId++, 
            (double)(rand() % (WIDTH - 40) + 20), 
            (double)(rand() % (HEIGHT - 40) + 20), 
            vx * 3.0, 
            vy * 3.0, 
            8.0, 
            false, false, false, false
        });
    }
}

void ResetGameMode() {
    entities.clear();
    nextId = 1;
    player = { nextId++, (double)WIDTH/2, (double)HEIGHT/2, 0, 0, 18.0, false, false, true, false };
    for (int i = 0; i < MAX_FOOD; i++) SpawnFood();
    for (int i = 0; i < MAX_ENEMIES; i++) SpawnEnemy();
}

void DrawCircleHelper(HDC hdc, int x, int y, int radius, COLORREF color) {
    HBRUSH hBrush = CreateSolidBrush(color);
    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void UpdateAndRender(HWND hwnd, HDC hdc) {
    Rectangle2D screenBounds = { 0, 0, (double)WIDTH, (double)HEIGHT };
    int dummy = 0;

    if (currentScreen == VISUALIZATION) {
        for (auto& p : visParticles) {
            p.x += p.vx;
            p.y += p.vy;
            if (p.x - p.radius < 0 || p.x + p.radius > WIDTH) p.vx *= -1;
            if (p.y - p.radius < 0 || p.y + p.radius > HEIGHT) p.vy *= -1;
            p.highlighted = false; 
        }

        QuadTree qtree(screenBounds, 4); 
        for (const auto& p : visParticles) qtree.insert(p);

        for (auto& p : visParticles) {
            Rectangle2D areaDeChoque = { p.x - p.radius*2, p.y - p.radius*2, p.radius * 4, p.radius * 4 };
            vector<Particle> vecindario;
            qtree.query(areaDeChoque, vecindario, dummy);

            for (const auto& vecino : vecindario) {
                if (vecino.id == p.id) continue;
                double dist = hypot(p.x - vecino.x, p.y - vecino.y);
                if (dist < p.radius + vecino.radius) {
                    p.highlighted = true; 
                }
            }
        }
    }
    else if (currentScreen == GAME_MODE) {
        int currentFoodCount = 0;
        int currentEnemyCount = 0;
        for (const auto& e : entities) {
            if (e.isFood) currentFoodCount++;
            if (e.isEnemy) currentEnemyCount++;
        }
        while (currentFoodCount < MAX_FOOD) { SpawnFood(); currentFoodCount++; }
        while (currentEnemyCount < MAX_ENEMIES) { SpawnEnemy(); currentEnemyCount++; }

        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hwnd, &mousePos);
        player.x = mousePos.x;
        player.y = mousePos.y;

        for (auto& e : entities) {
            if (e.isEnemy) {
                double targetX = e.x + e.vx;
                double targetY = e.y + e.vy;

                if (e.radius > player.radius) {
                    double dx = player.x - e.x;
                    double dy = player.y - e.y;
                    double dist = hypot(dx, dy);
                    if (dist > 0 && dist < 350) {
                        targetX = e.x + (dx / dist) * 2.8; 
                        targetY = e.y + (dy / dist) * 2.8;
                    }
                }
                e.x = targetX; e.y = targetY;
                if (e.x < 0 || e.x > WIDTH) e.vx *= -1;
                if (e.y < 0 || e.y > HEIGHT) e.vy *= -1;
            }
        }

        QuadTree qtree(screenBounds, 4);
        for (const auto& e : entities) qtree.insert(e);
        qtree.insert(player);

        vector<int> idsToRemove;

        Rectangle2D playerBox = { player.x - player.radius, player.y - player.radius, player.radius * 2, player.radius * 2 };
        vector<Particle> playerNearby;
        qtree.query(playerBox, playerNearby, dummy);

        for (const auto& n : playerNearby) {
            if (n.isPlayer) continue;
            double dist = hypot(player.x - n.x, player.y - n.y);
            if (dist < player.radius + n.radius) {
                if (n.isFood) {
                    if (player.radius < MAX_PLAYER_RADIUS) player.radius += 0.25; 
                    idsToRemove.push_back(n.id);
                } 
                else if (n.isEnemy) {
                    if (player.radius > n.radius * 1.15) {
                        if (player.radius < MAX_PLAYER_RADIUS) player.radius += n.radius * 0.15;
                        idsToRemove.push_back(n.id);
                    } else {
                        ResetGameMode();
                        return; 
                    }
                }
            }
        }

        for (auto& e : entities) {
            if (!e.isEnemy) continue;
            Rectangle2D enemyBox = { e.x - e.radius, e.y - e.radius, e.radius * 2, e.radius * 2 };
            vector<Particle> enemyNearby;
            qtree.query(enemyBox, enemyNearby, dummy);

            for (const auto& n : enemyNearby) {
                if (n.id == e.id || n.isPlayer) continue;
                double dist = hypot(e.x - n.x, e.y - n.y);
                if (dist < e.radius + n.radius) {
                    if (n.isFood) {
                        if (e.radius < MAX_ENEMY_RADIUS) e.radius += 0.2; 
                        idsToRemove.push_back(n.id);
                    } 
                    else if (n.isEnemy && e.radius > n.radius * 1.1) {
                        if (e.radius < MAX_ENEMY_RADIUS) e.radius += n.radius * 0.1;
                        idsToRemove.push_back(n.id);
                    }
                }
            }
        }

        if (!idsToRemove.empty()) {
            vector<Particle> remainingEntities;
            for (const auto& e : entities) {
                bool remove = false;
                for (int id : idsToRemove) {
                    if (e.id == id) { remove = true; break; }
                }
                if (!remove) remainingEntities.push_back(e);
            }
            entities = remainingEntities;
        }
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);

    HBRUSH hBg = CreateSolidBrush(RGB(245, 245, 250)); 
    RECT rectText = { 0, 0, WIDTH, HEIGHT };
    FillRect(hdcMem, &rectText, hBg);
    DeleteObject(hBg);

    if (currentScreen == MENU) {
        TextOutA(hdcMem, WIDTH/2 - 120, 200, "PROYECTO 2", 10);
        TextOutA(hdcMem, WIDTH/2 - 150, 300, "[1] QUADTREE", 12);
        TextOutA(hdcMem, WIDTH/2 - 150, 350, "[2] JUEGO   ", 12);
    } 
    else {
        QuadTree drawTree(screenBounds, 4);
        if (currentScreen == VISUALIZATION) {
            for (const auto& p : visParticles) drawTree.insert(p);
        } else {
            for (const auto& e : entities) drawTree.insert(e);
            drawTree.insert(player);
        }
        drawTree.drawLines(hdcMem);

        if (currentScreen == VISUALIZATION) {
            for (const auto& p : visParticles) {
                COLORREF c = p.highlighted ? RGB(255, 191, 0) : RGB(44, 62, 80);
                DrawCircleHelper(hdcMem, p.x, p.y, p.radius, c);
            }
            TextOutA(hdcMem, 10, 10, "MODO 1: Colisiones Autonomas", 28);
            TextOutA(hdcMem, 10, 30, "[M] Regresar al Menu", 20);
        } 
        else {
            for (const auto& e : entities) {
                COLORREF c = e.isFood ? RGB(34, 177, 76) : RGB(220, 20, 60);
                DrawCircleHelper(hdcMem, e.x, e.y, e.radius, c);
            }
            DrawCircleHelper(hdcMem, player.x, player.y, player.radius, RGB(0, 162, 232));

            string status = "MODO 2: Juego | Tu Radio: " + to_string((int)player.radius) + " / " + to_string((int)MAX_PLAYER_RADIUS);
            if (player.radius >= MAX_PLAYER_RADIUS) status += " (MAX!)";
            TextOutA(hdcMem, 10, 10, status.c_str(), status.length());
            TextOutA(hdcMem, 10, 30, "[M] Regresar al Menu", 20);
        }
    }

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldBm);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            if (wParam == '1') { currentScreen = VISUALIZATION; SetupVisualizationMode(); }
            if (wParam == '2') { currentScreen = GAME_MODE; ResetGameMode(); }
            if (wParam == 'M' || wParam == 'm') currentScreen = MENU;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main() {
    srand(time(0));
    SetupVisualizationMode();
    ResetGameMode();
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "QuadTreeWinClass";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(0, "QuadTreeWinClass", "Proyecto QuadTree", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH + 16, HEIGHT + 39, NULL, NULL, hInstance, NULL);
    MSG msg = { 0 };
    HDC hdc = GetDC(hwnd);
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            UpdateAndRender(hwnd, hdc);
            Sleep(16);
        }
    }
    ReleaseDC(hwnd, hdc);
    return 0;
}