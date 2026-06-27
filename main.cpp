#include "QuadTree.h"
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
const int VIS_PARTICLE_COUNT = 100;

enum Screen { MENU, VISUALIZATION, GAME_MODE };
Screen currentScreen = MENU;

Particle player;
vector<Particle> entities;
vector<Particle> visParticles;
int nextId = 1;

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

void SetupVisualizationMode() {
  visParticles.clear();
  nextId = 1;
  for (int i = 0; i < VIS_PARTICLE_COUNT; i++) {
    double vx = (GetRandomValue(0, 100) / 50.0) - 1.0;
    double vy = (GetRandomValue(0, 100) / 50.0) - 1.0;
    if (vx == 0 && vy == 0) {
      vx = 0.5;
      vy = 0.5;
    }

    visParticles.push_back({nextId++, (double)(GetRandomValue(20, WIDTH - 20)),
                            (double)(GetRandomValue(20, HEIGHT - 20)), vx * 3.0,
                            vy * 3.0, 8.0, false, false, false, false});
  }
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
  // Inicialización de la ventana de Raylib
  InitWindow(WIDTH, HEIGHT, "Proyecto QuadTree - C++ & Raylib");
  SetTargetFPS(60); // Limitar a 60 FPS

  SetupVisualizationMode();
  ResetGameMode();

  // Colores personalizados
  Color bgColor = {245, 245, 250, 255};
  Color colorFood = {34, 177, 76, 255};
  Color colorEnemy = {220, 20, 60, 255};
  Color colorPlayer = {0, 162, 232, 255};
  Color colorVisNormal = {44, 62, 80, 255};
  Color colorVisHighlight = {255, 191, 0, 255};

  while (!WindowShouldClose()) {
    // --- 1. LÓGICA (UPDATE) ---
    if (IsKeyPressed(KEY_ONE)) {
      currentScreen = VISUALIZATION;
      SetupVisualizationMode();
    }
    if (IsKeyPressed(KEY_TWO)) {
      currentScreen = GAME_MODE;
      ResetGameMode();
    }
    if (IsKeyPressed(KEY_M))
      currentScreen = MENU;

    Rectangle2D screenBounds = {0, 0, (double)WIDTH, (double)HEIGHT};
    int dummy = 0;

    if (currentScreen == VISUALIZATION) {
      for (auto &p : visParticles) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.x - p.radius < 0 || p.x + p.radius > WIDTH)
          p.vx *= -1;
        if (p.y - p.radius < 0 || p.y + p.radius > HEIGHT)
          p.vy *= -1;
        p.highlighted = false;
      }

      QuadTree qtree(screenBounds, 4);
      for (const auto &p : visParticles)
        qtree.insert(p);

      for (auto &p : visParticles) {
        Rectangle2D areaDeChoque = {p.x - p.radius * 2, p.y - p.radius * 2,
                                    p.radius * 4, p.radius * 4};
        vector<Particle> vecindario;
        qtree.query(areaDeChoque, vecindario, dummy);

        for (const auto &vecino : vecindario) {
          if (vecino.id == p.id)
            continue;
          double dist = hypot(p.x - vecino.x, p.y - vecino.y);
          if (dist < p.radius + vecino.radius) {
            p.highlighted = true;
          }
        }
      }
    } else if (currentScreen == GAME_MODE) {
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

    // --- 2. RENDERIZADO (DRAW) ---
    BeginDrawing();
    ClearBackground(bgColor);

    if (currentScreen == MENU) {
      DrawText("PROYECTO 2", WIDTH / 2 - 120, 200, 40, DARKGRAY);
      DrawText("[1] QUADTREE VISUALIZATION", WIDTH / 2 - 150, 300, 20,
               DARKGRAY);
      DrawText("[2] JUEGO", WIDTH / 2 - 150, 350, 20, DARKGRAY);
    } else {
      QuadTree drawTree(screenBounds, 4);
      if (currentScreen == VISUALIZATION) {
        for (const auto &p : visParticles)
          drawTree.insert(p);
      } else {
        for (const auto &e : entities)
          drawTree.insert(e);
        drawTree.insert(player);
      }

      drawTree.drawLines();

      if (currentScreen == VISUALIZATION) {
        for (const auto &p : visParticles) {
          Color c = p.highlighted ? colorVisHighlight : colorVisNormal;
          DrawCircle(p.x, p.y, p.radius, c);
        }
        DrawText("MODO 1: Colisiones Autonomas", 10, 10, 20, DARKGRAY);
        DrawText("[M] Regresar al Menu", 10, 35, 15, GRAY);
      } else {
        for (const auto &e : entities) {
          Color c = e.isFood ? colorFood : colorEnemy;
          DrawCircle(e.x, e.y, e.radius, c);
        }
        DrawCircle(player.x, player.y, player.radius, colorPlayer);

        string status =
            "MODO 2: Juego | Tu Radio: " + to_string((int)player.radius) +
            " / " + to_string((int)MAX_PLAYER_RADIUS);
        if (player.radius >= MAX_PLAYER_RADIUS)
          status += " (MAX!)";

        DrawText(status.c_str(), 10, 10, 20, DARKGRAY);
        DrawText("[M] Regresar al Menu", 10, 35, 15, GRAY);
      }
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
