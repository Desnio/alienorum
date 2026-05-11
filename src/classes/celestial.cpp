
#include <math.h>
#include "celestial.h"

double CelestialObject::viewer_magnitude(CelestialLocation seen_from)
{
    double r = seen_from.distance_to(location) / parsec / 10;
    double intrinsic = pow(magnbase, -absolute_magnitude);
    double apparent = intrinsic / (r*r);
    return -log(apparent) / log(magnbase);
}