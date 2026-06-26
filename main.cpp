#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include "QuadTree.h"

const int WIDTH = 1000;
const int HEIGHT = 800;

enum Screen { MENU, VISUALIZATION, GAME_MODE };
Screen currentScreen = MENU;

Particle player;
std::vector<Particle> entities;

void ResetSimulation() {
    entities.clear();
    player = { 0, (double)WIDTH/2, (double)HEIGHT/2, 0, 0, 20.0, false, false, true, false };
    
    // Generar comida (Verde)
    for (int i = 1; i <= 60; i++) {
        entities.push_back({ i, (double)(rand() % (WIDTH - 40) + 20), (double)(rand() % (HEIGHT - 40) + 20), 0, 0, 5.0, true, false, false, false });
    }
    // Generar enemigos (Rojo)
    for (int i = 61; i <= 75; i++) {
        double vx = ((rand() % 100) / 50.0) - 1.0;
        double vy = ((rand() % 100) / 50.0) - 1.0;
        entities.push_back({ i, (double)(rand() % (WIDTH - 100) + 50), (double)(rand() % (HEIGHT - 100) + 50), 
                              vx * 2, vy * 2, (double)(rand() % 20 + 15), false, true, false, false });
    }
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
    // --- LÓGICA DE ACTUALIZACIÓN ---
    if (currentScreen != MENU) {
        // Obtener posición del mouse relativo a la ventana
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hwnd, &mousePos);
        player.x = mousePos.x;
        player.y = mousePos.y;

        // Actualizar enemigos
        for (auto& e : entities) {
            if (e.isEnemy) {
                e.x += e.vx; e.y += e.vy;
                if (e.x < 0 || e.x > WIDTH) e.vx *= -1;
                if (e.y < 0 || e.y > HEIGHT) e.vy *= -1;
            }
            e.highlighted = false;
        }

        // Construir QuadTree
        Rectangle2D screenBounds = { 0, 0, (double)WIDTH, (double)HEIGHT };
        QuadTree qtree(screenBounds, 4);
        for (const auto& e : entities) qtree.insert(e);
        qtree.insert(player);

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
            Rectangle2D playerBox = { player.x - player.radius, player.y - player.radius, player.radius * 2, player.radius * 2 };
            std::vector<Particle> nearby;
            int dummy = 0;
            qtree.query(playerBox, nearby, dummy);

            for (const auto& n : nearby) {
                if (n.isPlayer) continue;
                double dist = hypot(player.x - n.x, player.y - n.y);
                if (dist < player.radius + n.radius) {
                    if (n.isFood) {
                        player.radius += 0.5;
                        for (auto it = entities.begin(); it != entities.end(); ++it) {
                            if (it->id == n.id) { entities.erase(it); break; }
                        }
                    } else if (n.isEnemy && n.radius > player.radius) {
                        ResetSimulation(); // Muere y reinicia
                    }
                }
            }
        }
    }

    // --- RENDERIZADO (Doble Buffer para evitar parpadeo) ---
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Fondo Blanco
    HBRUSH hBg = CreateSolidBrush(RGB(255, 255, 255));
    RECT rectText = { 0, 0, WIDTH, HEIGHT };
    FillRect(hdcMem, &rectText, hBg);
    DeleteObject(hBg);

    if (currentScreen == MENU) {
        TextOutA(hdcMem, WIDTH/2 - 120, 200, "PROYECTO QUADTREE (Agar.io)", 27);
        TextOutA(hdcMem, WIDTH/2 - 150, 300, "Presiona [1] para Ver Modo Colisiones", 37);
        TextOutA(hdcMem, WIDTH/2 - 150, 350, "Presiona [2] para Ver Modo Juego Activo", 39);
    } 
    else {
        // Dibujar líneas del QuadTree
        Rectangle2D screenBounds = { 0, 0, (double)WIDTH, (double)HEIGHT };
        QuadTree drawTree(screenBounds, 4);
        for (const auto& e : entities) drawTree.insert(e);
        drawTree.drawLines(hdcMem);

        // Dibujar entidades
        for (const auto& e : entities) {
            COLORREF c = e.isFood ? RGB(0, 200, 0) : RGB(230, 0, 0);
            if (e.highlighted && currentScreen == VISUALIZATION) c = RGB(255, 200, 0); // Amarillo
            DrawCircleHelper(hdcMem, e.x, e.y, e.radius, c);
        }

        // Dibujar jugador (Azul)
        DrawCircleHelper(hdcMem, player.x, player.y, player.radius, RGB(0, 100, 255));

        if (currentScreen == VISUALIZATION) {
            TextOutA(hdcMem, 10, 10, "Modo: Visualizacion de Vecinos. Regresar al Menu: [M]", 52);
            DrawCircleHelper(hdcMem, player.x, player.y, 50, RGB(150, 0, 255), true); // Radio de búsqueda visual
        } else {
            TextOutA(hdcMem, 10, 10, "Modo: Juego Activo. Regresar al Menu: [M]", 41);
        }
    }

    // Copiar buffer a la pantalla
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

    HWND hwnd = CreateWindowExA(0, "QuadTreeWinClass", "Proyecto QuadTree - Agar.io Nativo", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
                                WIDTH + 16, HEIGHT + 39, NULL, NULL, hInstance, NULL);

    MSG msg = { 0 };
    HDC hdc = GetDC(hwnd);

    // Bucle principal de simulación
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            UpdateAndRender(hwnd, hdc);
            Sleep(16); // Controlar FPS aproximado (~60 FPS)
        }
    }

    ReleaseDC(hwnd, hdc);
    return 0;
}
