#ifndef SUN_H
#define SUN_H

#include <vector>
#include "Planet.h"

class Sun
{
public:
    Sun();

    void addPlanet(const Planet& p);

    std::vector<Planet>& getPlanets() { return planets; }
    const std::vector<Planet>& getPlanets() const { return planets; }

    float getMass() const { return mass; }

private:
    float mass;                  // 태양 질량 (기본 = 1.0)
    std::vector<Planet> planets; // 태양계의 행성들
};

#endif
