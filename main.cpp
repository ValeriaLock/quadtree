#include "QuadTree.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <raylib.h>
#include <string>
#include <vector>

using namespace std;

const int WIDTH = 1000;
const int HEIGHT = 800;
const int MAX_FOOD = 35;
const int MAX_ENEMIES = 15;
const double MAX_PLAYER_RADIUS = 65.0;
const double MAX_ENEMY_RADIUS = 80.0;

enum Screen { MENU, VISUALIZATION, GAME_MODE };
Screen currentScreen = MENU;

enum Distribution { UNIFORM, CLUSTER, HIGH_DENSITY };
Distribution currentDist = UNIFORM;

Particle player;
vector<Particle> entities;
vector<Particle> visParticles;
int nextId = 1;

// Variables para la Comparación Experimental
bool useQuadTree = true;
long long timeTakenUs = 0;
int totalChecks = 0;
int currentVisCount = 1000;

void SetupVisualizationMode(int count, Distribution dist) {
  visParticles.clear();
  nextId = 1;

  vector<Vector2> clusters = {{200, 200}, {800, 200}, {500, 600}};

  for (int i = 0; i < count; i++) {
    double vx = (GetRandomValue(0, 100) / 50.0) - 1.0;
    double vy = (GetRandomValue(0, 100) / 50.0) - 1.0;
    if (vx == 0 && vy == 0) {
      vx = 0.5;
      vy = 0.5;
    }

    double px, py;

    if (dist == UNIFORM) {
      px = GetRandomValue(20, WIDTH - 20);
      py = GetRandomValue(20, HEIGHT - 20);
    } else if (dist == CLUSTER) {
      int c = GetRandomValue(0, 2);
      px = clusters[c].x + GetRandomValue(-100, 100);
      py = clusters[c].y + GetRandomValue(-100, 100);
    } else if (dist == HIGH_DENSITY) {
      if (GetRandomValue(1, 100) <= 80) {
        px = WIDTH / 2 + GetRandomValue(-150, 150);
        py = HEIGHT / 2 + GetRandomValue(-150, 150);
      } else {
        px = GetRandomValue(20, WIDTH - 20);
        py = GetRandomValue(20, HEIGHT - 20);
      }
    }

    if (px < 20)
      px = 20;
    if (px > WIDTH - 20)
      px = WIDTH - 20;
    if (py < 20)
      py = 20;
    if (py > HEIGHT - 20)
      py = HEIGHT - 20;

    visParticles.push_back({nextId++, px, py, vx * 3.0, vy * 3.0, 5.0, false,
                            false, false, false});
  }
}

void SpawnFood() {
  entities.push_back({nextId++, (double)(GetRandomValue(20, WIDTH - 20)),
                      (double)(GetRandomValue(20, HEIGHT - 20)), 0, 0, 4.0,
                      true, false, false, false});
}

void SpawnEnemy() {
  double vx = (GetRandomValue(0, 100) / 50.0) - 1.0;
  double vy = (GetRandomValue(0, 100) / 50.0) - 1.0;
  if (vx == 0 && vy == 0) {
    vx = 1;
    vy = 1;
  }

  entities.push_back({nextId++, (double)(GetRandomValue(50, WIDTH - 50)),
                      (double)(GetRandomValue(50, HEIGHT - 50)), vx * 2.5,
                      vy * 2.5, (double)GetRandomValue(15, 30), false, true,
                      false, false});
}

void ResetGameMode() {
  entities.clear();
  nextId = 1;
  player = {nextId++,
            (double)WIDTH / 2,
            (double)HEIGHT / 2,
            0,
            0,
            18.0,
            false,
            false,
            true,
            false};
  for (int i = 0; i < MAX_FOOD; i++)
    SpawnFood();
  for (int i = 0; i < MAX_ENEMIES; i++)
    SpawnEnemy();
}

int main() {
  InitWindow(WIDTH, HEIGHT, "Proyecto 2: Simulador QuadTree");
  SetTargetFPS(60);

  Font customFont = LoadFontEx("font.ttf", 40, 0, 250);

  SetupVisualizationMode(currentVisCount, currentDist);
  ResetGameMode();

  Color bgColor = {245, 245, 250, 255};
  Color menuBgColor = {20, 30, 48, 255};
  Color colorFood = {34, 177, 76, 255};
  Color colorEnemy = {220, 20, 60, 255};
  Color colorPlayer = {0, 162, 232, 255};
  Color colorVisNormal = {44, 62, 80, 255};
  Color colorVisHighlight = {255, 60, 60, 255};
  Color colorCandidate = {255, 165, 0, 255};

  while (!WindowShouldClose()) {

    // --- LÓGICA (UPDATE) ---
    if (IsKeyPressed(KEY_ONE)) {
      currentScreen = VISUALIZATION;
      SetupVisualizationMode(currentVisCount, currentDist);
    }
    if (IsKeyPressed(KEY_TWO)) {
      currentScreen = GAME_MODE;
      ResetGameMode();
    }
    if (IsKeyPressed(KEY_M))
      currentScreen = MENU;

    Rectangle2D screenBounds = {0, 0, (double)WIDTH, (double)HEIGHT};

    if (currentScreen == VISUALIZATION) {
      if (IsKeyPressed(KEY_C))
        useQuadTree = !useQuadTree;

      if (IsKeyPressed(KEY_D)) {
        int nextDist = (currentDist + 1) % 3;
        currentDist = static_cast<Distribution>(nextDist);
        SetupVisualizationMode(currentVisCount, currentDist);
      }

      if (IsKeyPressed(KEY_UP)) {
        currentVisCount += 1000;
        SetupVisualizationMode(currentVisCount, currentDist);
      }
      if (IsKeyPressed(KEY_DOWN) && currentVisCount > 1000) {
        currentVisCount -= 1000;
        SetupVisualizationMode(currentVisCount, currentDist);
      }

      for (auto &p : visParticles) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.x - p.radius < 0 || p.x + p.radius > WIDTH)
          p.vx *= -1;
        if (p.y - p.radius < 0 || p.y + p.radius > HEIGHT)
          p.vy *= -1;

        p.highlighted = false;
        p.isFood = false;
      }

      auto start = chrono::high_resolution_clock::now();
      totalChecks = 0;

      if (useQuadTree) {
        QuadTree qtree(screenBounds, 4);
        for (const auto &p : visParticles)
          qtree.insert(p);

        for (size_t i = 0; i < visParticles.size(); i++) {
          auto &p = visParticles[i];
          Rectangle2D areaDeChoque = {p.x - p.radius * 2, p.y - p.radius * 2,
                                      p.radius * 4, p.radius * 4};
          vector<Particle> vecindario;
          qtree.query(areaDeChoque, vecindario, totalChecks);

          for (const auto &vecino : vecindario) {
            if (vecino.id == p.id)
              continue;
            totalChecks++;

            if (i == 0) {
              for (auto &vp : visParticles) {
                if (vp.id == vecino.id)
                  vp.isFood = true;
              }
            }

            double dist = hypot(p.x - vecino.x, p.y - vecino.y);
            if (dist < p.radius + vecino.radius) {
              p.highlighted = true;
            }
          }
        }
      } else {
        for (size_t i = 0; i < visParticles.size(); i++) {
          for (size_t j = i + 1; j < visParticles.size(); j++) {
            totalChecks++;
            double dist = hypot(visParticles[i].x - visParticles[j].x,
                                visParticles[i].y - visParticles[j].y);
            if (dist < visParticles[i].radius + visParticles[j].radius) {
              visParticles[i].highlighted = true;
              visParticles[j].highlighted = true;
            }
          }
        }
      }
      auto end = chrono::high_resolution_clock::now();
      timeTakenUs =
          chrono::duration_cast<chrono::microseconds>(end - start).count();
    } else if (currentScreen == GAME_MODE) {
      int dummy = 0;
      int currentFoodCount = 0, currentEnemyCount = 0;
      for (const auto &e : entities) {
        if (e.isFood)
          currentFoodCount++;
        if (e.isEnemy)
          currentEnemyCount++;
      }
      while (currentFoodCount < MAX_FOOD) {
        SpawnFood();
        currentFoodCount++;
      }
      while (currentEnemyCount < MAX_ENEMIES) {
        SpawnEnemy();
        currentEnemyCount++;
      }

      player.x = GetMouseX();
      player.y = GetMouseY();

      for (auto &e : entities) {
        if (e.isEnemy) {
          double targetX = e.x + e.vx, targetY = e.y + e.vy;
          if (e.radius > player.radius) {
            double dx = player.x - e.x, dy = player.y - e.y;
            double dist = hypot(dx, dy);
            if (dist > 0 && dist < 350) {
              targetX = e.x + (dx / dist) * 2.8;
              targetY = e.y + (dy / dist) * 2.8;
            }
          }
          e.x = targetX;
          e.y = targetY;
          if (e.x < 0 || e.x > WIDTH)
            e.vx *= -1;
          if (e.y < 0 || e.y > HEIGHT)
            e.vy *= -1;
        }
      }

      QuadTree qtree(screenBounds, 4);
      for (const auto &e : entities)
        qtree.insert(e);
      qtree.insert(player);

      vector<int> idsToRemove;
      Rectangle2D playerBox = {player.x - player.radius,
                               player.y - player.radius, player.radius * 2,
                               player.radius * 2};
      vector<Particle> playerNearby;
      qtree.query(playerBox, playerNearby, dummy);

      for (const auto &n : playerNearby) {
        if (n.isPlayer)
          continue;
        double dist = hypot(player.x - n.x, player.y - n.y);
        if (dist < player.radius + n.radius) {
          if (n.isFood) {
            if (player.radius < MAX_PLAYER_RADIUS)
              player.radius += 0.25;
            idsToRemove.push_back(n.id);
          } else if (n.isEnemy) {
            if (player.radius > n.radius * 1.15) {
              if (player.radius < MAX_PLAYER_RADIUS)
                player.radius += n.radius * 0.15;
              idsToRemove.push_back(n.id);
            } else {
              ResetGameMode();
              continue;
            }
          }
        }
      }

      for (auto &e : entities) {
        if (!e.isEnemy)
          continue;
        Rectangle2D enemyBox = {e.x - e.radius, e.y - e.radius, e.radius * 2,
                                e.radius * 2};
        vector<Particle> enemyNearby;
        qtree.query(enemyBox, enemyNearby, dummy);

        for (const auto &n : enemyNearby) {
          if (n.id == e.id || n.isPlayer)
            continue;
          double dist = hypot(e.x - n.x, e.y - n.y);
          if (dist < e.radius + n.radius) {
            if (n.isFood) {
              if (e.radius < MAX_ENEMY_RADIUS)
                e.radius += 0.2;
              idsToRemove.push_back(n.id);
            } else if (n.isEnemy && e.radius > n.radius * 1.1) {
              if (e.radius < MAX_ENEMY_RADIUS)
                e.radius += n.radius * 0.1;
              idsToRemove.push_back(n.id);
            }
          }
        }
      }

      if (!idsToRemove.empty()) {
        vector<Particle> remainingEntities;
        for (const auto &e : entities) {
          bool remove = false;
          for (int id : idsToRemove) {
            if (e.id == id) {
              remove = true;
              break;
            }
          }
          if (!remove)
            remainingEntities.push_back(e);
        }
        entities = remainingEntities;
      }
    }

    // --- RENDERIZADO (DRAW) ---
    BeginDrawing();

    if (currentScreen == MENU) {
      ClearBackground(menuBgColor);

      const char *title = "PROYECTO 2: AED";
      const char *opt1 = "[1] ANALISIS ALGORITMICO (QuadTree vs Fuerza Bruta)";
      const char *opt2 = "[2] DEMOSTRACION APLICADA (Juego Interactivo)";

      Vector2 titleSize = MeasureTextEx(customFont, title, 40, 2);
      Vector2 opt1Size = MeasureTextEx(customFont, opt1, 24, 2);
      Vector2 opt2Size = MeasureTextEx(customFont, opt2, 24, 2);

      DrawTextEx(customFont, title, {(WIDTH - titleSize.x) / 2, 250}, 40, 2,
                 WHITE);
      DrawTextEx(customFont, opt1, {(WIDTH - opt1Size.x) / 2, 380}, 24, 2,
                 LIGHTGRAY);
      DrawTextEx(customFont, opt2, {(WIDTH - opt2Size.x) / 2, 440}, 24, 2,
                 LIGHTGRAY);
    } else {
      ClearBackground(bgColor);

      if (currentScreen == VISUALIZATION) {
        if (useQuadTree) {
          QuadTree drawTree(screenBounds, 4);
          for (const auto &p : visParticles)
            drawTree.insert(p);
          drawTree.drawLines();

          if (!visParticles.empty()) {
            Particle p0 = visParticles[0];
            DrawRectangleLines(p0.x - p0.radius * 2, p0.y - p0.radius * 2,
                               p0.radius * 4, p0.radius * 4, YELLOW);
            DrawCircleLines(p0.x, p0.y, p0.radius + 15, YELLOW);
          }
        }

        for (size_t i = 0; i < visParticles.size(); i++) {
          const auto &p = visParticles[i];
          Color c = colorVisNormal;
          if (useQuadTree && p.isFood && i != 0)
            c = colorCandidate;
          if (p.highlighted)
            c = colorVisHighlight;
          if (i == 0)
            c = BLUE;

          DrawCircle(p.x, p.y, p.radius, c);
        }

        DrawRectangle(10, 10, 520, 220, Fade(BLACK, 0.8f));

        DrawTextEx(customFont, "MODO 1: Analisis de Rendimiento", {25, 20}, 24,
                   1, WHITE);
        DrawTextEx(customFont,
                   TextFormat("Algoritmo: %s", useQuadTree
                                                   ? "QUADTREE (O(N log N))"
                                                   : "FUERZA BRUTA (O(N^2))"),
                   {25, 60}, 20, 1, useQuadTree ? GREEN : RED);

        string distStr = (currentDist == UNIFORM)   ? "Uniforme"
                         : (currentDist == CLUSTER) ? "Clusters"
                                                    : "Alta Densidad";
        DrawTextEx(customFont,
                   TextFormat("Distribucion [D]: %s", distStr.c_str()),
                   {25, 90}, 18, 1, YELLOW);
        DrawTextEx(
            customFont,
            TextFormat("Tamano de entrada (N): %d particulas", currentVisCount),
            {25, 120}, 18, 1, LIGHTGRAY);
        DrawTextEx(customFont,
                   TextFormat("Tiempo de ejecucion: %lld us", timeTakenUs),
                   {25, 150}, 18, 1, LIGHTGRAY);
        DrawTextEx(customFont,
                   TextFormat("Comprobaciones / Nodos: %d", totalChecks),
                   {25, 180}, 18, 1, LIGHTGRAY);

        DrawTextEx(
            customFont,
            "[C] Algoritmo | [D] Distribucion | [UP]/[DOWN] Tamano | [M] Menu",
            {15, 240}, 16, 1, GRAY);

      } else { // MODO JUEGO
        QuadTree drawTree(screenBounds, 4);
        for (const auto &e : entities)
          drawTree.insert(e);
        drawTree.insert(player);
        drawTree.drawLines();

        for (const auto &e : entities) {
          Color c = e.isFood ? colorFood : colorEnemy;
          DrawCircle(e.x, e.y, e.radius, c);
        }
        DrawCircle(player.x, player.y, player.radius, colorPlayer);

        DrawRectangle(10, 10, 400, 80, Fade(BLACK, 0.7f));
        string status = "MODO 2: Tu Radio: " + to_string((int)player.radius) +
                        " / " + to_string((int)MAX_PLAYER_RADIUS);
        if (player.radius >= MAX_PLAYER_RADIUS)
          status += " (MAX!)";

        DrawTextEx(customFont, status.c_str(), {20, 20}, 22, 1, WHITE);
        DrawTextEx(customFont, "[M] Regresar al Menu", {20, 50}, 18, 1,
                   LIGHTGRAY);
      }
    }
    EndDrawing();
  }
  UnloadFont(customFont);
  CloseWindow();
  return 0;
}
