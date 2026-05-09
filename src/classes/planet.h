
#ifndef _Planet
#define _Planet

#include "celestial.h"

// Includes planets, moons, asteroids, comets, KBOs, etc.
// If it's natural and orbits a star, and isn't a star itself,
// odds are it goes in this class.
class Planet : public CelestialObject
{
    public:
    double albedo;
    Color color; 
};

#endif