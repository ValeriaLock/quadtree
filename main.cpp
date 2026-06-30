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

// --- Parámetros configurables (modo Visualización) ---
int currentVisCount = 1000;
double visRadius = 5.0;
double visSpeedFactor = 3.0;
int qtCapacity = 4;
int spaceW = 1000;
int spaceH = 800;
bool runBruteForce = true;

// --- Métricas instantáneas ---
long long timeQtUs = 0;
long long timeBuildUs = 0;
long long timeQueryUs = 0;
long long timeBruteUs = 0;
int checksQt = 0;
int checksBrute = 0;
int totalCandidatesQt = 0;

// --- Métricas promedio ---
int frameCount = 0;
long long accumTimeQt = 0, accumTimeBuild = 0, accumTimeQuery = 0,
          accumTimeBrute = 0;
long long accumChecksQt = 0, accumChecksBrute = 0;
long long accumCandidatesQt = 0;

// --- Métricas modo juego ---
long long gameTimeTakenUs = 0;
int gameTotalChecks = 0;

void ResetAverages() {
  frameCount = 0;
  accumTimeQt = 0;
  accumTimeBuild = 0;
  accumTimeQuery = 0;
  accumTimeBrute = 0;
  accumChecksQt = 0;
  accumChecksBrute = 0;
  accumCandidatesQt = 0;
}

void SetupVisualizationMode(int count, Distribution dist) {
  visParticles.clear();
  nextId = 1;
  ResetAverages();

  float cx1 = spaceW * 0.2f, cy1 = spaceH * 0.25f;
  float cx2 = spaceW * 0.8f, cy2 = spaceH * 0.25f;
  float cx3 = spaceW * 0.5f, cy3 = spaceH * 0.75f;
  vector<Vector2> clusters = {{cx1, cy1}, {cx2, cy2}, {cx3, cy3}};

  int margin = (int)visRadius + 2;

  for (int i = 0; i < count; i++) {
    double vx = ((GetRandomValue(0, 100) / 50.0) - 1.0) * visSpeedFactor;
    double vy = ((GetRandomValue(0, 100) / 50.0) - 1.0) * visSpeedFactor;
    if (vx == 0 && vy == 0) {
      vx = 0.5 * visSpeedFactor;
      vy = 0.5 * visSpeedFactor;
    }

    double px = 0, py = 0;

    if (dist == UNIFORM) {
      px = GetRandomValue(margin, spaceW - margin);
      py = GetRandomValue(margin, spaceH - margin);
    } else if (dist == CLUSTER) {
      int c = GetRandomValue(0, 2);
      px = clusters[c].x + GetRandomValue(-100, 100);
      py = clusters[c].y + GetRandomValue(-100, 100);
    } else { // HIGH_DENSITY
      if (GetRandomValue(1, 100) <= 80) {
        px = spaceW / 2 + GetRandomValue(-150, 150);
        py = spaceH / 2 + GetRandomValue(-150, 150);
      } else {
        px = GetRandomValue(margin, spaceW - margin);
        py = GetRandomValue(margin, spaceH - margin);
      }
    }

    if (px < margin)
      px = margin;
    if (px > spaceW - margin)
      px = spaceW - margin;
    if (py < margin)
      py = margin;
    if (py > spaceH - margin)
      py = spaceH - margin;

    visParticles.push_back(
        {nextId++, px, py, vx, vy, visRadius, false, false, false, false});
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

    if (currentScreen == VISUALIZATION) {
      bool needsReset = false;

      // --- Controles de parámetros configurables ---
      if (IsKeyPressed(KEY_D)) {
        currentDist = static_cast<Distribution>((currentDist + 1) % 3);
        needsReset = true;
      }
      if (IsKeyPressed(KEY_UP)) {
        currentVisCount += 1000;
        needsReset = true;
      }
      if (IsKeyPressed(KEY_DOWN) && currentVisCount > 1000) {
        currentVisCount -= 1000;
        needsReset = true;
      }
      // Espacio 2D: W/S
      if (IsKeyPressed(KEY_W) && spaceW < 1600) {
        spaceW += 100;
        spaceH = (int)(spaceW * 0.8);
        needsReset = true;
      }
      if (IsKeyPressed(KEY_S) && spaceW > 400) {
        spaceW -= 100;
        spaceH = (int)(spaceW * 0.8);
        needsReset = true;
      }
      // Capacidad del QuadTree: Q/E
      if (IsKeyPressed(KEY_Q) && qtCapacity > 1) {
        qtCapacity--;
        needsReset = true;
      }
      if (IsKeyPressed(KEY_E) && qtCapacity < 16) {
        qtCapacity++;
        needsReset = true;
      }
      // Radio de partículas: R/F
      if (IsKeyPressed(KEY_R) && visRadius < 20.0) {
        visRadius += 1.0;
        needsReset = true;
      }
      if (IsKeyPressed(KEY_F) && visRadius > 2.0) {
        visRadius -= 1.0;
        needsReset = true;
      }
      // Velocidad: V/B (se puede cambiar sin reiniciar)
      if (IsKeyPressed(KEY_V) && visSpeedFactor < 10.0) {
        visSpeedFactor += 0.5;
        needsReset = true;
      }
      if (IsKeyPressed(KEY_B) && visSpeedFactor > 0.5) {
        visSpeedFactor -= 0.5;
        needsReset = true;
      }
      // Toggle fuerza bruta: C
      if (IsKeyPressed(KEY_C)) {
        runBruteForce = !runBruteForce;
        ResetAverages();
      }

      if (needsReset)
        SetupVisualizationMode(currentVisCount, currentDist);

      // --- Actualizar posiciones ---
      Rectangle2D spaceBounds = {0, 0, (double)spaceW, (double)spaceH};

      for (auto &p : visParticles) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.x - p.radius < 0 || p.x + p.radius > spaceW)
          p.vx *= -1;
        if (p.y - p.radius < 0 || p.y + p.radius > spaceH)
          p.vy *= -1;
        p.x = fmax(p.radius, fmin(p.x, spaceW - p.radius));
        p.y = fmax(p.radius, fmin(p.y, spaceH - p.radius));

        p.highlighted = false;
        p.isFood = false;
      }

      // =============================================
      //  QUADTREE: Construcción
      // =============================================
      auto startBuild = chrono::high_resolution_clock::now();
      QuadTree qtree(spaceBounds, qtCapacity);
      for (const auto &p : visParticles)
        qtree.insert(p);
      auto endBuild = chrono::high_resolution_clock::now();
      timeBuildUs =
          chrono::duration_cast<chrono::microseconds>(endBuild - startBuild)
              .count();

      // =============================================
      //  QUADTREE: Consultas
      // =============================================
      auto startQuery = chrono::high_resolution_clock::now();
      checksQt = 0;
      totalCandidatesQt = 0;

      for (size_t i = 0; i < visParticles.size(); i++) {
        auto &p = visParticles[i];
        vector<Particle> vecindario;
        qtree.queryRadius(p.x, p.y, p.radius * 2.0, vecindario, checksQt);

        int candidatos = 0;
        for (const auto &vecino : vecindario) {
          if (vecino.id == p.id)
            continue;
          candidatos++;
          checksQt++;

          // Marcar candidatos de la partícula 0 para visualización
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
        totalCandidatesQt += candidatos;
      }
      auto endQuery = chrono::high_resolution_clock::now();
      timeQueryUs =
          chrono::duration_cast<chrono::microseconds>(endQuery - startQuery)
              .count();

      timeQtUs = timeBuildUs + timeQueryUs; // Total para referencia

      // =============================================
      //  FUERZA BRUTA: medición solamente
      // =============================================
      if (runBruteForce) {
        auto startBrute = chrono::high_resolution_clock::now();
        checksBrute = 0;

        for (size_t i = 0; i < visParticles.size(); i++) {
          for (size_t j = i + 1; j < visParticles.size(); j++) {
            checksBrute++;
            hypot(visParticles[i].x - visParticles[j].x,
                  visParticles[i].y - visParticles[j].y);
          }
        }

        auto endBrute = chrono::high_resolution_clock::now();
        timeBruteUs =
            chrono::duration_cast<chrono::microseconds>(endBrute - startBrute)
                .count();
      }

      // --- Acumular promedios ---
      frameCount++;
      accumTimeBuild += timeBuildUs;
      accumTimeQuery += timeQueryUs;
      accumTimeQt += timeQtUs;
      accumChecksQt += checksQt;
      accumCandidatesQt += totalCandidatesQt;
      if (runBruteForce) {
        accumTimeBrute += timeBruteUs;
        accumChecksBrute += checksBrute;
      }

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

      Rectangle2D screenBounds = {0, 0, (double)WIDTH, (double)HEIGHT};

      auto gameStart = chrono::high_resolution_clock::now();
      gameTotalChecks = 0;

      QuadTree qtree(screenBounds, 4);
      for (const auto &e : entities)
        qtree.insert(e);
      qtree.insert(player);

      vector<int> idsToRemove;

      vector<Particle> playerNearby;
      qtree.queryRadius(player.x, player.y, player.radius * 2.0, playerNearby,
                        gameTotalChecks);

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
        vector<Particle> enemyNearby;
        qtree.queryRadius(e.x, e.y, e.radius * 2.0, enemyNearby,
                          gameTotalChecks);

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

      auto gameEnd = chrono::high_resolution_clock::now();
      gameTimeTakenUs =
          chrono::duration_cast<chrono::microseconds>(gameEnd - gameStart)
              .count();
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
        Rectangle2D spaceBounds = {0, 0, (double)spaceW, (double)spaceH};

        // Dibujar borde del espacio de simulación si es menor que la ventana
        if (spaceW < WIDTH || spaceH < HEIGHT) {
          DrawRectangleLines(0, 0, spaceW, spaceH, DARKGRAY);
        }

        // Dibujar subdivisiones del QuadTree
        QuadTree drawTree(spaceBounds, qtCapacity);
        for (const auto &p : visParticles)
          drawTree.insert(p);
        drawTree.drawLines();

        // Visualización de la consulta sobre la partícula 0
        if (!visParticles.empty()) {
          Particle p0 = visParticles[0];
          DrawCircleLines((int)p0.x, (int)p0.y, (int)(p0.radius * 2.0), YELLOW);
          DrawRectangleLines((int)(p0.x - p0.radius * 2),
                             (int)(p0.y - p0.radius * 2), (int)(p0.radius * 4),
                             (int)(p0.radius * 4), Fade(YELLOW, 0.4f));
        }

        // Dibujar partículas
        for (size_t i = 0; i < visParticles.size(); i++) {
          const auto &p = visParticles[i];
          Color c = colorVisNormal;
          if (p.isFood && i != 0)
            c = colorCandidate;
          if (p.highlighted)
            c = colorVisHighlight;
          if (i == 0)
            c = BLUE;
          DrawCircle(p.x, p.y, p.radius, c);
        }

        // ==================
        //  PANEL DE MÉTRICAS
        // ==================
        int panelW = 590;
        int panelH = 370; // Aumentado para acomodar las nuevas métricas
        DrawRectangle(10, 10, panelW, panelH, Fade(BLACK, 0.85f));

        int lx = 25; // left x
        int y = 20;

        DrawTextEx(customFont, "MODO 1: Analisis de Rendimiento",
                   {(float)lx, (float)y}, 24, 1, WHITE);
        y += 35;

        // Línea de parámetros 1
        string distStr = (currentDist == UNIFORM)   ? "Uniforme"
                         : (currentDist == CLUSTER) ? "Clusters"
                                                    : "Alta Densidad";
        DrawTextEx(
            customFont,
            TextFormat("N: %d | Dist: %s", currentVisCount, distStr.c_str()),
            {(float)lx, (float)y}, 17, 1, YELLOW);
        y += 22;

        // Línea de parámetros 2
        DrawTextEx(
            customFont,
            TextFormat("Espacio: %dx%d | Cap: %d | Radio: %.1f | Vel: %.1f",
                       spaceW, spaceH, qtCapacity, visRadius, visSpeedFactor),
            {(float)lx, (float)y}, 17, 1, LIGHTGRAY);
        y += 28;

        // --- QUADTREE ---
        DrawTextEx(customFont, "QUADTREE", {(float)lx, (float)y}, 18, 1, GREEN);
        y += 22;

        long long avgTimeBuild =
            frameCount > 0 ? accumTimeBuild / frameCount : 0;
        long long avgTimeQuery =
            frameCount > 0 ? accumTimeQuery / frameCount : 0;
        long long avgTimeQt = frameCount > 0 ? accumTimeQt / frameCount : 0;

        long long avgChecksQt = frameCount > 0 ? accumChecksQt / frameCount : 0;
        double avgCandPerObjQt =
            (frameCount > 0 && currentVisCount > 0)
                ? (double)accumCandidatesQt /
                      ((long long)frameCount * currentVisCount)
                : 0.0;
        double candPerObjQt = currentVisCount > 0
                                  ? (double)totalCandidatesQt / currentVisCount
                                  : 0.0;

        DrawTextEx(customFont,
                   TextFormat("  Construccion: %lld us (prom: %lld us)",
                              timeBuildUs, avgTimeBuild),
                   {(float)lx, (float)y}, 16, 1, WHITE);
        y += 20;
        DrawTextEx(customFont,
                   TextFormat("  Consultas:    %lld us (prom: %lld us)",
                              timeQueryUs, avgTimeQuery),
                   {(float)lx, (float)y}, 16, 1, WHITE);
        y += 20;
        DrawTextEx(customFont,
                   TextFormat("  Total:        %lld us (prom: %lld us)",
                              timeQtUs, avgTimeQt),
                   {(float)lx, (float)y}, 16, 1, WHITE);
        y += 20;
        DrawTextEx(
            customFont,
            TextFormat("  Checks: %d (prom: %lld)", checksQt, avgChecksQt),
            {(float)lx, (float)y}, 16, 1, WHITE);
        y += 20;
        DrawTextEx(customFont,
                   TextFormat("  Candidatos/obj: %.1f (prom: %.1f)",
                              candPerObjQt, avgCandPerObjQt),
                   {(float)lx, (float)y}, 16, 1, WHITE);
        y += 28;

        // --- FUERZA BRUTA ---
        DrawTextEx(customFont,
                   TextFormat("FUERZA BRUTA %s",
                              runBruteForce ? "" : "(OFF - pulsa C)"),
                   {(float)lx, (float)y}, 18, 1, RED);
        y += 22;

        if (runBruteForce) {
          long long avgTimeBrute =
              frameCount > 0 ? accumTimeBrute / frameCount : 0;
          long long avgChecksBrute =
              frameCount > 0 ? accumChecksBrute / frameCount : 0;

          DrawTextEx(customFont,
                     TextFormat("  Tiempo: %lld us (prom: %lld us)",
                                timeBruteUs, avgTimeBrute),
                     {(float)lx, (float)y}, 16, 1, WHITE);
          y += 20;
          DrawTextEx(customFont,
                     TextFormat("  Checks: %d (prom: %lld)", checksBrute,
                                avgChecksBrute),
                     {(float)lx, (float)y}, 16, 1, WHITE);
          y += 20;
          DrawTextEx(customFont,
                     TextFormat("  Candidatos/obj: %d", currentVisCount - 1),
                     {(float)lx, (float)y}, 16, 1, WHITE);
        } else {
          DrawTextEx(customFont, "  (Desactivado para mejor FPS)",
                     {(float)lx, (float)y}, 16, 1, GRAY);
          y += 20;
        }
        y += 28;

        // --- Controles ---
        DrawTextEx(customFont,
                   "[D] Dist | [UP/DOWN] N | [W/S] Espacio | [Q/E] Capacidad",
                   {(float)lx, (float)y}, 14, 1, GRAY);
        y += 18;
        DrawTextEx(
            customFont,
            "[R/F] Radio | [V/B] Velocidad | [C] Bruta ON/OFF | [M] Menu",
            {(float)lx, (float)y}, 14, 1, GRAY);

      } else { // MODO JUEGO
        Rectangle2D screenBounds = {0, 0, (double)WIDTH, (double)HEIGHT};

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

        // Panel de métricas del modo juego
        DrawRectangle(10, 10, 430, 130, Fade(BLACK, 0.7f));

        string status = "MODO 2: Tu Radio: " + to_string((int)player.radius) +
                        " / " + to_string((int)MAX_PLAYER_RADIUS);
        if (player.radius >= MAX_PLAYER_RADIUS)
          status += " (MAX!)";

        int totalEntities = (int)entities.size() + 1;
        int bruteForceChecks = totalEntities * (totalEntities - 1) / 2;

        DrawTextEx(customFont, status.c_str(), {20, 20}, 22, 1, WHITE);
        DrawTextEx(customFont,
                   TextFormat("QuadTree  - Tiempo: %lld us | Checks: %d",
                              gameTimeTakenUs, gameTotalChecks),
                   {20, 55}, 17, 1, GREEN);
        DrawTextEx(customFont,
                   TextFormat("Fuerza Bruta (estimado) - Checks: %d",
                              bruteForceChecks),
                   {20, 82}, 17, 1, RED);
        DrawTextEx(customFont, "[M] Regresar al Menu", {20, 110}, 17, 1,
                   LIGHTGRAY);
      }
    }
    EndDrawing();
  }
  UnloadFont(customFont);
  CloseWindow();
  return 0;
}
