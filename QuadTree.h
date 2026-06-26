#ifndef QUADTREE_H
#define QUADTREE_H

#include <vector>
#include <windows.h>
#include "Particle.h"

class QuadTree {
private:
    int capacity;
    Rectangle2D boundary;
    std::vector<Particle> particles;
    bool divided;

    QuadTree* northwest;
    QuadTree* northeast;
    QuadTree* southwest;
    QuadTree* southeast;

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
        : boundary(bounds), capacity(cap), divided(false),
          northwest(nullptr), northeast(nullptr), southwest(nullptr), southeast(nullptr) {}

    ~QuadTree() {
        if (divided) {
            delete northwest; delete northeast;
            delete southwest; delete southeast;
        }
    }

    bool insert(const Particle& p) {
        if (!boundary.contains(p)) return false;

        if (particles.size() < (size_t)capacity && !divided) {
            particles.push_back(p);
            return true;
        }

        if (!divided) subdivide();

        if (northwest->insert(p)) return true;
        if (northeast->insert(p)) return true;
        if (southwest->insert(p)) return true;
        if (southeast->insert(p)) return true;

        return false;
    }

    void query(Rectangle2D range, std::vector<Particle>& found, int& nodesVisited) {
        nodesVisited++;
        if (!boundary.intersects(range)) return;

        for (const auto& p : particles) {
            if (range.contains(p)) found.push_back(p);
        }

        if (divided) {
            northwest->query(range, found, nodesVisited);
            northeast->query(range, found, nodesVisited);
            southwest->query(range, found, nodesVisited);
            southeast->query(range, found, nodesVisited);
        }
    }

    // Dibuja las líneas utilizando la ventana de Windows nativa
    void drawLines(HDC hdc) {
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        MoveToEx(hdc, boundary.x, boundary.y, NULL);
        LineTo(hdc, boundary.x + boundary.width, boundary.y);
        LineTo(hdc, boundary.x + boundary.width, boundary.y + boundary.height);
        LineTo(hdc, boundary.x, boundary.y + boundary.height);
        LineTo(hdc, boundary.x, boundary.y);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        if (divided) {
            northwest->drawLines(hdc);
            northeast->drawLines(hdc);
            southwest->drawLines(hdc);
            southeast->drawLines(hdc);
        }
    }
};

#endif
