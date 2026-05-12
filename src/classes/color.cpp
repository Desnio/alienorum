#include <math.h>
#include <algorithm>
#include "point.h"
#include "color.h"

using namespace std;
double global_brightness = 10;
double global_inverse_gamma = 1.0 / 1.7;
bool redlight_mode = false;

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

    int red   = min(255, (int)(255.0 * pow(c.red   * bloom, global_inverse_gamma)));
    int green = min(255, (int)(255.0 * pow(c.green * bloom, global_inverse_gamma)));
    int blue  = min(255, (int)(255.0 * pow(c.blue  * bloom, global_inverse_gamma)));

    if (redlight_mode)
    {
        red += 0.5 * green + 0.3 * blue;
        green /= 3;
        blue /= 3;
    }

    result.r = max(0, red);
    result.g = max(0, green);
    result.b = max(0, blue);
    return result;
}

void set_gamma(double new_gamma)
{
    global_inverse_gamma = 1.0 / new_gamma;
}

__uint32_t rgba_apply_redlight(__uint32_t i)
{
    if (!redlight_mode) return i;
    __uint32_t r = (i & 0xFF), g = (i & 0xFF00) >> 8, b = (i & 0xFF0000) >> 16;
    r += 0.5 * g + 0.3 * b;
    if (r > 0xFF) r = 0xFF;
    g /= 3;
    b /= 3;
    return __uint32_t((i & 0xFF000000) + r + (g << 8) + (b << 16));
}
