#ifndef QUADTREE_H
#define QUADTREE_H

#include "Particle.h"
#include <cmath>
#include <raylib.h>
#include <vector>
using namespace std;

class QuadTree {
private:
  int capacity;
  Rectangle2D boundary;
  vector<Particle> particles;
  bool divided;

  QuadTree *northwest;
  QuadTree *northeast;
  QuadTree *southwest;
  QuadTree *southeast;

  void subdivide() {
    double x = boundary.x;
    double y = boundary.y;
    double w = boundary.width / 2.0;
    double h = boundary.height / 2.0;

    northwest = new QuadTree({x, y, w, h}, capacity);
    northeast = new QuadTree({x + w, y, w, h}, capacity);
    southwest = new QuadTree({x, y + h, w, h}, capacity);
    southeast = new QuadTree({x + w, y + h, w, h}, capacity);
    divided = true;
  }

public:
  QuadTree(Rectangle2D bounds, int cap)
      : boundary(bounds), capacity(cap), divided(false), northwest(nullptr),
        northeast(nullptr), southwest(nullptr), southeast(nullptr) {}

  ~QuadTree() {
    if (divided) {
      delete northwest;
      delete northeast;
      delete southwest;
      delete southeast;
    }
  }

  bool insert(const Particle &p) {
    if (!boundary.contains(p))
      return false;

    if (particles.size() < (size_t)capacity && !divided) {
      particles.push_back(p);
      return true;
    }

    if (!divided)
      subdivide();

    if (northwest->insert(p))
      return true;
    if (northeast->insert(p))
      return true;
    if (southwest->insert(p))
      return true;
    if (southeast->insert(p))
      return true;

    return false;
  }

  void query(Rectangle2D range, vector<Particle> &found, int &nodesVisited) {
    nodesVisited++;
    if (!boundary.intersects(range))
      return;

    for (const auto &p : particles) {
      if (range.contains(p))
        found.push_back(p);
    }

    if (divided) {
      northwest->query(range, found, nodesVisited);
      northeast->query(range, found, nodesVisited);
      southwest->query(range, found, nodesVisited);
      southeast->query(range, found, nodesVisited);
    }
  }

  void queryRadius(double cx, double cy, double radius, vector<Particle> &found,
                   int &nodesVisited) {
    nodesVisited++;
    Rectangle2D radiusBounds = {cx - radius, cy - radius, radius * 2.0,
                                radius * 2.0};
    if (!boundary.intersects(radiusBounds))
      return;

    for (const auto &p : particles) {
      double dist = hypot(p.x - cx, p.y - cy);
      if (dist <= radius)
        found.push_back(p);
    }

    if (divided) {
      northwest->queryRadius(cx, cy, radius, found, nodesVisited);
      northeast->queryRadius(cx, cy, radius, found, nodesVisited);
      southwest->queryRadius(cx, cy, radius, found, nodesVisited);
      southeast->queryRadius(cx, cy, radius, found, nodesVisited);
    }
  }

  void drawLines() {
    DrawRectangleLines(boundary.x, boundary.y, boundary.width, boundary.height,
                       LIGHTGRAY);

    if (divided) {
      northwest->drawLines();
      northeast->drawLines();
      southwest->drawLines();
      southeast->drawLines();
    }
  }
};

#endif
