#ifndef PARTICLE_H
#define PARTICLE_H

struct Particle {
    int id;
    double x, y;
    double vx, vy;
    double radius;
    bool isFood;
    bool isEnemy;
    bool isPlayer;
    bool highlighted; 
};

struct Rectangle2D {
    double x, y, width, height;
    bool contains(const Particle& p) const {
        return (p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height);
    }
    bool intersects(const Rectangle2D& other) const {
        return !(other.x > x + width || other.x + other.width < x ||
                 other.y > y + height || other.y + other.height < y);
    }
};

#endif
