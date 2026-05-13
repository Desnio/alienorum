#include <math.h>
#include <algorithm>
#include <iostream>
#include "point.h"
#include "color.h"

using namespace std;
double global_brightness = default_brightness;
double global_inverse_gamma = 1.0 / default_gamma;
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
    /*if (bloom_radius < 1) bloom_radius = 1;
    double bloom = 1.0 / pow(bloom_radius, 3);*/

    // Treat c.red, c.green, c.blue as linear photon flux amounts.
    // Bloom radius zero will have output = value up to a maximum of 255.
    // Successive bloom radii will carry the overflow divided by the circumference
    // of a circle with radius = bloom radius.
    int red, green, blue;
    if (!bloom_radius)
    {
        red   = min(255, (int)(255.0 * pow(c.red,   global_inverse_gamma)));
        green = min(255, (int)(255.0 * pow(c.green, global_inverse_gamma)));
        blue  = min(255, (int)(255.0 * pow(c.blue,  global_inverse_gamma)));
    }
    else
    {
        int i;
        double lum = 0.29*c.red + 0.57*c.green + 0.14 * c.blue, rc = c.red, gc = c.green, bc = c.blue;
        for (i=0; i<bloom_radius; i++)
        {
            lum = fmax(0, lum - fmin(1.0, lum));
            if (!lum) return result;
        }

        gc = lum;
        rc = c.red/c.green*lum;
        bc = c.blue/c.green*lum;

        double circ = 2.0 * M_PI * bloom_radius;
        double invcirc = 1.0 / circ;

        red   = min(255, (int)(255.0 * pow(rc * invcirc, global_inverse_gamma)));
        green = min(255, (int)(255.0 * pow(gc * invcirc, global_inverse_gamma)));
        blue  = min(255, (int)(255.0 * pow(bc * invcirc, global_inverse_gamma)));
    }

    /*int red   = min(255, (int)(255.0 * pow(c.red   * bloom, global_inverse_gamma)));
    int green = min(255, (int)(255.0 * pow(c.green * bloom, global_inverse_gamma)));
    int blue  = min(255, (int)(255.0 * pow(c.blue  * bloom, global_inverse_gamma)));*/

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
