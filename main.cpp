#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include "QuadTree.h"

const int WIDTH = 1000;
const int HEIGHT = 800;

// Constantes de población para mantener el mapa vivo
const int MAX_FOOD = 60;
const int MAX_ENEMIES = 15;

enum Screen { MENU, VISUALIZATION, GAME_MODE };
Screen currentScreen = MENU;

Particle player;
std::vector<Particle> entities;
int nextId = 1; // Generador de IDs únicos para el control de entidades

void SpawnFood() {
    entities.push_back({ nextId++, (double)(rand() % (WIDTH - 40) + 20), (double)(rand() % (HEIGHT - 40) + 20), 0, 0, 5.0, true, false, false, false });
}

void SpawnEnemy() {
    double vx = ((rand() % 100) / 50.0) - 1.0;
    double vy = ((rand() % 100) / 50.0) - 1.0;
    if (vx == 0 && vy == 0) { vx = 1; vy = 1; }
    entities.push_back({ nextId++, (double)(rand() % (WIDTH - 100) + 50), (double)(rand() % (HEIGHT - 100) + 50), 
                          vx * 2, vy * 2, (double)(rand() % 20 + 15), false, true, false, false });
}

void ResetSimulation() {
    entities.clear();
    nextId = 1;
    player = { nextId++, (double)WIDTH/2, (double)HEIGHT/2, 0, 0, 20.0, false, false, true, false };
    
    // Generar población inicial
    for (int i = 0; i < MAX_FOOD; i++) SpawnFood();
    for (int i = 0; i < MAX_ENEMIES; i++) SpawnEnemy();
}

void DrawCircleHelper(HDC hdc, int x, int y, int radius, COLORREF color, bool outlineOnly = false) {
    HBRUSH hBrush = outlineOnly ? (HBRUSH)GetStockObject(NULL_BRUSH) : CreateSolidBrush(color);
    HPEN hPen = CreatePen(PS_SOLID, outlineOnly ? 2 : 1, color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    if (!outlineOnly) DeleteObject(hBrush);
    DeleteObject(hPen);
}

void UpdateAndRender(HWND hwnd, HDC hdc) {
    // --- REAPARECER ENTIDADES CONSTANTEMENTE (Sustitución en tiempo real) ---
    if (currentScreen != MENU) {
        int currentFoodCount = 0;
        int currentEnemyCount = 0;
        for (const auto& e : entities) {
            if (e.isFood) currentFoodCount++;
            if (e.isEnemy) currentEnemyCount++;
        }
        while (currentFoodCount < MAX_FOOD) { SpawnFood(); currentFoodCount++; }
        while (currentEnemyCount < MAX_ENEMIES) { SpawnEnemy(); currentEnemyCount++; }
    }

    // --- LÓGICA DE ACTUALIZACIÓN ---
    if (currentScreen != MENU) {
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hwnd, &mousePos);
        player.x = mousePos.x;
        player.y = mousePos.y;

        // Actualizar movimiento de enemigos
        for (auto& e : entities) {
            if (e.isEnemy) {
                e.x += e.vx; e.y += e.vy;
                if (e.x < 0 || e.x > WIDTH) e.vx *= -1;
                if (e.y < 0 || e.y > HEIGHT) e.vy *= -1;
            }
            e.highlighted = false;
        }

        // Construir QuadTree dinámico del Frame
        Rectangle2D screenBounds = { 0, 0, (double)WIDTH, (double)HEIGHT };
        QuadTree qtree(screenBounds, 4);
        for (const auto& e : entities) qtree.insert(e);
        qtree.insert(player);

        std::vector<int> idsToRemove;

        if (currentScreen == VISUALIZATION) {
            double rangeSize = 100.0;
            Rectangle2D searchBox = { player.x - rangeSize/2, player.y - rangeSize/2, rangeSize, rangeSize };
            std::vector<Particle> candidates;
            int nodesVisited = 0;
            qtree.query(searchBox, candidates, nodesVisited);

            for (auto& c : candidates) {
                for (auto& e : entities) {
                    if (e.id == c.id) e.highlighted = true;
                }
            }
        } 
        else if (currentScreen == GAME_MODE) {
            // 1. COLISIONES DEL JUGADOR USANDO QUADTREE
            Rectangle2D playerBox = { player.x - player.radius, player.y - player.radius, player.radius * 2, player.radius * 2 };
            std::vector<Particle> playerNearby;
            int dummy = 0;
            qtree.query(playerBox, playerNearby, dummy);

            for (const auto& n : playerNearby) {
                if (n.isPlayer) continue;
                double dist = hypot(player.x - n.x, player.y - n.y);
                if (dist < player.radius + n.radius) {
                    if (n.isFood) {
                        player.radius += 0.5; // Crece con comida
                        idsToRemove.push_back(n.id);
                    } 
                    else if (n.isEnemy) {
                        if (player.radius > n.radius) {
                            player.radius += n.radius * 0.3; // ¡El jugador se come al enemigo menor!
                            idsToRemove.push_back(n.id);
                        } else {
                            ResetSimulation(); // El enemigo es más grande: El jugador muere
                            return; 
                        }
                    }
                }
            }

            // 2. COLISIONES DE ENEMIGOS USANDO QUADTREE (Enemigos comen comida y otros enemigos)
            for (auto& e : entities) {
                if (!e.isEnemy) continue;

                Rectangle2D enemyBox = { e.x - e.radius, e.y - e.radius, e.radius * 2, e.radius * 2 };
                std::vector<Particle> enemyNearby;
                qtree.query(enemyBox, enemyNearby, dummy);

                for (const auto& n : enemyNearby) {
                    if (n.id == e.id || n.isPlayer) continue; // Ignorarse a sí mismo y al jugador (ya procesado)

                    double dist = hypot(e.x - n.x, e.y - n.y);
                    if (dist < e.radius + n.radius) {
                        if (n.isFood) {
                            e.radius += 0.3; // El enemigo crece comiendo comida estática
                            idsToRemove.push_back(n.id);
                        } 
                        else if (n.isEnemy && e.radius > n.radius) {
                            e.radius += n.radius * 0.2; // ¡El enemigo grande canibaliza al enemigo chico!
                            idsToRemove.push_back(n.id);
                        }
                    }
                }
            }

            // Limpieza eficiente de entidades comidas de forma segura
            if (!idsToRemove.empty()) {
                std::vector<Particle> remainingEntities;
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
    }

    // --- RENDERIZADO (Doble Buffer nativo) ---
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);

    HBRUSH hBg = CreateSolidBrush(RGB(255, 255, 255));
    RECT rectText = { 0, 0, WIDTH, HEIGHT };
    FillRect(hdcMem, &rectText, hBg);
    DeleteObject(hBg);

    if (currentScreen == MENU) {
        TextOutA(hdcMem, WIDTH/2 - 120, 200, "PROYECTO QUADTREE (Agar.io)", 27);
        TextOutA(hdcMem, WIDTH/2 - 150, 300, "Presiona [1] para Modo Colisiones", 33);
        TextOutA(hdcMem, WIDTH/2 - 150, 350, "Presiona [2] para Modo Juego Activo", 35);
    } 
    else {
        Rectangle2D screenBounds = { 0, 0, (double)WIDTH, (double)HEIGHT };
        QuadTree drawTree(screenBounds, 4);
        for (const auto& e : entities) drawTree.insert(e);
        drawTree.drawLines(hdcMem);

        for (const auto& e : entities) {
            COLORREF c = e.isFood ? RGB(0, 200, 0) : RGB(230, 0, 0);
            if (e.highlighted && currentScreen == VISUALIZATION) c = RGB(255, 200, 0);
            DrawCircleHelper(hdcMem, e.x, e.y, e.radius, c);
        }

        DrawCircleHelper(hdcMem, player.x, player.y, player.radius, RGB(0, 100, 255));

        if (currentScreen == VISUALIZATION) {
            TextOutA(hdcMem, 10, 10, "Modo: Visualizacion de Vecinos. Menu: [M]", 41);
            DrawCircleHelper(hdcMem, player.x, player.y, 50, RGB(150, 0, 255), true);
        } else {
            std::string scoreStr = "Modo: Juego Activo. Menu: [M] | Tu Radio: " + std::to_string((int)player.radius);
            TextOutA(hdcMem, 10, 10, scoreStr.c_str(), scoreStr.length());
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
            if (wParam == '1') { currentScreen = VISUALIZATION; ResetSimulation(); }
            if (wParam == '2') { currentScreen = GAME_MODE; ResetSimulation(); }
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
    ResetSimulation();

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "QuadTreeWinClass";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, "QuadTreeWinClass", "Proyecto QuadTree - Agar.io Evolucionado", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
                                WIDTH + 16, HEIGHT + 39, NULL, NULL, hInstance, NULL);

    MSG msg = { 0 };
    HDC hdc = GetDC(hwnd);

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);DispatchMessage(&msg);
        } else {
            UpdateAndRender(hwnd, hdc);
            Sleep(16);
        }
    }
    ReleaseDC(hwnd, hdc);
    return 0;
}