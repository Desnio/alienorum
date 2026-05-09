
#include <math.h>
#include "point.h"

double compute_time_dilation(double velocity)
{
    return sqrt(1.0 - (velocity*velocity)/(speed_of_light*speed_of_light));
}