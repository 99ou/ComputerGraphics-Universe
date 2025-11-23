#include "Sun.h"

Sun::Sun()
    : mass(1.0f)   // 태양 질량 스케일
{
}

void Sun::addPlanet(const Planet& p)
{
    planets.push_back(p);
}
