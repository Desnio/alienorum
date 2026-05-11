#include <math.h>
#include <algorithm>
#include "point.h"
#include "color.h"

using namespace std;
double global_brightness = 10;
double global_inverse_gamma = 1.0 / 1.7;

Color Color::color_from_magnitude_indices(double Vmag, double BV)
{
    Color c;

    // Literal B-V indices look too red on the screen.
    BV -= 0.7;

    c.green = global_brightness * pow(magnbase, -Vmag);
    c.blue = global_brightness * pow(magnbase, -Vmag-BV);
    c.red = global_brightness * pow(magnbase, -Vmag+BV);

    // Literal B-V indices look too saturated on the screen.
    c.red = (c.red + c.green + c.green) / 3;
    // c.blue = (c.blue + c.green) / 2;

    return c;
}

RGB Color::rgb_from_color(Color c, double bloom_radius)
{
    RGB result;
    if (bloom_radius < 1) bloom_radius = 1;
    double bloom = 1.0 / pow(bloom_radius, 3);

    int red   = 255.0 * pow(c.red   * bloom, global_inverse_gamma);
    int green = 255.0 * pow(c.green * bloom, global_inverse_gamma);
    int blue  = 255.0 * pow(c.blue  * bloom, global_inverse_gamma);

    result.r = max(0, min(255, red));
    result.g = max(0, min(255, green));
    result.b = max(0, min(255, blue));
    return result;
}

void set_gamma(double new_gamma)
{
    global_inverse_gamma = 1.0 / new_gamma;
}
