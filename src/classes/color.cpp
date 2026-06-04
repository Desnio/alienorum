#include <math.h>
#include <algorithm>
#include <iostream>
#include "point.h"
#include "color.h"

using namespace std;
double global_brightness = default_brightness;
double global_inverse_gamma = 1.0 / default_gamma;
bool redlight_mode = false;

double drawblxscalex, drawblxscaley;
int *bx_cache = new int[MAX_CELOBJS], *by_cache = new int[MAX_CELOBJS];

double Color::luminance()
{
    return 0.29 * red + 0.57 * green + 0.14 * blue;
}

Color Color::color_from_magnitude_indices(double Vmag, double BV)
{
    return color_from_magnitude_indices(Vmag, BV, BV);
}

Color Color::color_from_magnitude_indices(double Vmag, double BV, double VR)
{
    Color c;
    double BV_literal = BV+bv_correction, VR_literal = VR+bv_correction,            // TODO:
        brightness = global_brightness * pow(magnbase, -Vmag) * 128;

    c.green = 1;
    c.blue = pow(magnbase, -BV_literal);
    c.red = pow(magnbase,   VR_literal);

    double lum = c.luminance(), invlum = 1.0 / lum;
    c.red   *= brightness * invlum;
    c.green *= brightness * invlum;
    c.blue  *= brightness * invlum;

    return c;
}

RGB Color::rgb_from_color(Color c, double bloom_radius)
{
    RGB result;
    int red, green, blue;
    double circ, invcirc;

    if (bloom_radius < 0)
    {
        // Normalize
        invcirc = 1.0 / fmax(c.red, fmax(c.green, c.blue));
    }
    else
    {
        circ = 2.0 * M_PI * bloom_radius;
        invcirc = 1.0 / circ;
    }

    red   = 255 * fmin(1.0, pow(c.red   * invcirc, global_inverse_gamma));
    green = 255 * fmin(1.0, pow(c.green * invcirc, global_inverse_gamma));
    blue  = 255 * fmin(1.0, pow(c.blue  * invcirc, global_inverse_gamma));

    if (redlight_mode)
    {
        red = min(255, (int)(red + 0.5 * green + 0.3 * blue));
        green *= 0.333;
        blue *= 0.333;
    }

    result.r = red;
    result.g = green;
    result.b = blue;
    return result;
}

ImU32 Color::black_to_transparent(ImU32 input)
{
    int a = input >> 24;
    int r = input&0xff, g = (input&0xff00)>>8, b = (input&0xff0000)>>16;
    double highest = fmax(fmax(r,g),b);
    if (highest < 255)
    {
        double normalize = 255.0 / highest;
        a *= (highest/255);
        r *= normalize;
        g *= normalize;
        b *= normalize;
    }
    return (a<<24) + (b<<16) + (g<<8) + r;
}

json Color::to_json()
{
    return
    {
        {"red", red},
        {"green", green},
        {"blue", blue}
    };
}

bool Color::from_json(json j)
{
    try { j.at("red").get_to(red); } catch (...) { ; }
    try { j.at("green").get_to(green); } catch (...) { ; }
    try { j.at("blue").get_to(blue); } catch (...) { ; }
    return true;
}

void set_gamma(double new_gamma)
{
    global_inverse_gamma = 1.0 / new_gamma;
}

double get_gamma()
{
    return 1.0 / global_inverse_gamma;
}

void rgb_apply_redlight(float *r, float *g, float *b)
{
    *r += 0.5 * (*g) + 0.3 * (*b);
    if (*r > 0xFF) *r = 0xFF;
    *g /= 3;
    *b /= 3;
}

__uint32_t rgba_apply_redlight(__uint32_t i)
{
    if (!redlight_mode) return i;
    float r = (i & 0xFF), g = (i & 0xFF00) >> 8, b = (i & 0xFF0000) >> 16;
    rgb_apply_redlight(&r, &g, &b);
    return __uint32_t((i & 0xFF000000) + (__uint32_t)r + ((__uint32_t)g << 8) + ((__uint32_t)b << 16));
}

ImVec4 rgba_apply_redlight(ImVec4 i)
{
    if (!redlight_mode) return i;
    float r = i.x, g = i.y, b = i.z, a = i.w;
    rgb_apply_redlight(&r, &g, &b);
    return ImVec4(r, g, b, a);
}

void apply_default_style()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    // Lines without rgba_apply_redlight() added haven't been changed yet from ImGui defaults.
    colors[ImGuiCol_Text]                   = redlight_mode ? ImVec4(0.90f, 0.05f, 0.00f, 1.00f) : ImVec4(0.50f, 0.90f, 0.30f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = redlight_mode ? ImVec4(0.60f, 0.05f, 0.00f, 1.00f) : ImVec4(0.90f, 0.60f, 0.10f, 1.00f);
    colors[ImGuiCol_WindowBg]               = rgba_apply_redlight(ImVec4(0.00f, 0.03f, 0.06f, 0.97f));
    colors[ImGuiCol_ChildBg]                = rgba_apply_redlight(ImVec4(0.00f, 0.05f, 0.10f, 0.00f));
    colors[ImGuiCol_PopupBg]                = rgba_apply_redlight(ImVec4(0.00f, 0.06f, 0.13f, 0.92f));
    colors[ImGuiCol_Border]                 = rgba_apply_redlight(ImVec4(0.00f, 0.00f, 0.70f, 0.50f));
    colors[ImGuiCol_BorderShadow]           = rgba_apply_redlight(ImVec4(0.00f, 0.00f, 0.05f, 0.00f));
    colors[ImGuiCol_FrameBg]                = rgba_apply_redlight(ImVec4(0.00f, 0.21f, 0.44f, 0.39f));
    colors[ImGuiCol_FrameBgHovered]         = rgba_apply_redlight(ImVec4(0.47f, 0.47f, 0.69f, 0.40f));
    colors[ImGuiCol_FrameBgActive]          = rgba_apply_redlight(ImVec4(0.42f, 0.41f, 0.64f, 0.69f));
    colors[ImGuiCol_TitleBg]                = rgba_apply_redlight(ImVec4(0.13f, 0.37f, 0.53f, 0.93f));
    colors[ImGuiCol_TitleBgActive]          = rgba_apply_redlight(ImVec4(0.21f, 0.44f, 0.67f, 0.94f));
    colors[ImGuiCol_TitleBgCollapsed]       = rgba_apply_redlight(ImVec4(0.00f, 0.10f, 0.80f, 0.81f));
    colors[ImGuiCol_MenuBarBg]              = rgba_apply_redlight(ImVec4(0.20f, 0.20f, 0.60f, 0.80f));
    colors[ImGuiCol_ScrollbarBg]            = rgba_apply_redlight(ImVec4(0.20f, 0.35f, 0.40f, 0.60f));
    colors[ImGuiCol_ScrollbarGrab]          = rgba_apply_redlight(ImVec4(0.40f, 0.65f, 0.80f, 0.30f));
    colors[ImGuiCol_ScrollbarGrabHovered]   = rgba_apply_redlight(ImVec4(0.40f, 0.65f, 0.80f, 0.40f));
    colors[ImGuiCol_ScrollbarGrabActive]    = rgba_apply_redlight(ImVec4(0.41f, 0.68f, 0.80f, 0.60f));
    colors[ImGuiCol_CheckMark]              = rgba_apply_redlight(ImVec4(0.60f, 0.90f, 0.40f, 0.60f));
    colors[ImGuiCol_SliderGrab]             = rgba_apply_redlight(ImVec4(1.00f, 0.80f, 0.30f, 0.30f));
    colors[ImGuiCol_SliderGrabActive]       = rgba_apply_redlight(ImVec4(1.00f, 0.80f, 0.20f, 0.60f));
    colors[ImGuiCol_Button]                 = rgba_apply_redlight(ImVec4(0.50f, 0.30f, 0.10f, 0.62f));
    colors[ImGuiCol_ButtonHovered]          = rgba_apply_redlight(ImVec4(0.60f, 0.48f, 0.11f, 0.79f));
    colors[ImGuiCol_ButtonActive]           = rgba_apply_redlight(ImVec4(0.68f, 0.54f, 0.13f, 1.00f));
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = rgba_apply_redlight(ImVec4(0.80f, 0.10f, 0.10f, 0.60f));
    colors[ImGuiCol_SeparatorHovered]       = rgba_apply_redlight(ImVec4(0.90f, 0.10f, 0.10f, 0.60f));
    colors[ImGuiCol_SeparatorActive]        = rgba_apply_redlight(ImVec4(0.95, 0.10f, 0.10f, 0.60f));
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_InputTextCursor]        = rgba_apply_redlight(ImVec4(0.90f, 0.05f, 0.08f, 1.00f));
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    // TODO: Bring these back if ever we add tabbed interfaces. May require #include imgui_internal.h.
    // colors[ImGuiCol_Tab]                    = ImGui::ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    // colors[ImGuiCol_TabSelected]            = ImGui::ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    // colors[ImGuiCol_TabDimmed]              = ImGui::ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    // colors[ImGuiCol_TabDimmedSelected]      = ImGui::ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.53f, 0.53f, 0.87f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_DragDropTargetBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_NavCursor]              = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

