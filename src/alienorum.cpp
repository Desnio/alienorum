
#include <iostream>
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <algorithm> 
#include <thread>
#include <chrono>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif
#include "classes/color.h"
#include "classes/galaxy.h"
#include "classes/star.h"
#include "classes/planet.h"
#include "classes/cat.h"

// Learn more about ImGui here: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md

using namespace std;
#define MAX_CELOBJS 262144

int main (int argc, char** argv)
{
    CelestialObject **cels = new CelestialObject*[MAX_CELOBJS];
    memset(cels, 0, MAX_CELOBJS*sizeof(CelestialObject*));

    int ncelobjs = 0;
    CelestialLocation here;
    Point velocity;
    double azimuth = 0, altitude = 0;
    double spin = 0;
    int i, j;
    double gamma = 1.8;
    double zoom = 1;
    int cursor_size = 10, circle_size = 3;
    ImU32 cursor_color = IM_COL32(255, 32, 0, 255);
    ImU32 grid_color = IM_COL32(255, 0, 0, 96);
    bool is_an_obj_under_cursor;
    double obj_magn_under_cursor;
    std::string objname, objinfo;
    bool is_mouse_over_window;
    int tsatwnd_hei = 0;
    int timeout_ms = 5;
    bool interacted;

    std::filesystem::path p = "catalogs";
    bool catalogs_found = false;
    try
    {
        while (!std::filesystem::exists(p))
        {
            std::filesystem::path up = "..";
            std::filesystem::current_path(up);
            if (strlen(std::filesystem::current_path().c_str()) < 5) break;
        }
        if (std::filesystem::exists(p)) catalogs_found = true;
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << endl;
        return -1;
    }
    if (!catalogs_found)
    {
        std::cerr << "No star catalogs found. Ensure the catalogs folder exists, contains data, and that the files are readable." << endl;
        return -1;
    }

    // TODO: Read data from a star catalog.
    // Catalogs are available from the following links:
    // Bright Star Catalog: https://cdsarc.cds.unistra.fr/viz-bin/cat/V/50#/browse
    // Hipparcos catalog: https://cdsarc.cds.unistra.fr/viz-bin/cat/I/239#/browse
    // 2MASS: https://cdsarc.cds.unistra.fr/viz-bin/cat/II/246#/browse
    // Gliese: https://cdsarc.cds.unistra.fr/viz-bin/cat/V/70A#/browse
    // Full list: https://vizier.cds.unistra.fr/vizier/cats/U.htx
    CatalogReader cr;
    cr.download_catalogs();
    std::vector<std::string> cats = cr.find_catalogs("catalogs");

    for (i=0; i<cats.size(); i++)
    {
        cout << "Found " << cats[i] << endl;
        if (!strcmp(cats[i].c_str(), "catalogs/BSC"))
            cr.read_BrightStars_catalog(cels, MAX_CELOBJS);
    }
    // return 0;

    /*
    for (i=0; i<5381; i++)
    {
        cels[i] = new Star();
        cels[i]->type = star;
        cels[i]->location.local_position = Point(frand(-MAX_CELOBJS, MAX_CELOBJS), frand(-MAX_CELOBJS, MAX_CELOBJS), frand(-MAX_CELOBJS, MAX_CELOBJS));
    }
    ncelobjs = i;
    */

    //////////////////////////////////////////////////
    // Begin ImGui-specific setup code              //
    // This section is subject to the same license  //
    // as the contents of the imgui folder.         //
    //////////////////////////////////////////////////

    // Setup SDL
#ifdef _WIN32
    ::SetProcessDPIAware();
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Alienorum", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If fonts are not explicitly loaded, ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you have to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 background = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    set_gamma(gamma);

    // Main loop
    bool done = false;
    bool tsatwnd = true;
    bool frist = true;
    while (!done)
    {
        if (!is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //////////////////////////////////////////////////
        // End ImGui-specific setup code                //
        //////////////////////////////////////////////////

        if (!is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);
        int dispcx = (int)io.DisplaySize.x/2, dispcy = (int)io.DisplaySize.y / 2;

        Cartesian2D prev, zdes;
        bool prev_valid = false;
        // RA and Dec lines.
        for (i=0; i<24; i++)
        {
            prev_valid = false;
            for (j=-80; j<=80; j+=10)
            {
                Point ihavetomove = Point::from_ra_dec(fiftyseventh * i * 15, fiftyseventh * j, 5);
                try
                {
                    zdes = Cartesian2D(ihavetomove, azimuth, altitude, zoom);
                }
                catch (...)
                {
                    prev_valid = false;
                    continue;
                }

                if (j > -80)
                {
                    int dx1 = dispcx + zdes.x * dispcx,
                        dy1 = dispcy + zdes.y * dispcx,
                        dx2 = dispcx + prev.x * dispcx,
                        dy2 = dispcy + prev.y * dispcx;

                        if (prev_valid)
                        ImGui::GetBackgroundDrawList()->AddLine(
                            ImVec2(dx1, dy1), ImVec2(dx2, dy2),
                            grid_color, 1);
                }

                prev = zdes;
                prev_valid = true;
            }
        }
        for (j=-80; j <= 80; j+=10)
        {
            prev_valid = false;
            for (i=0; i<=24; i++)
            {
                Point ihavetomove = Point::from_ra_dec(fiftyseventh * i * 15, fiftyseventh * j, 5);
                try
                {
                    zdes = Cartesian2D(ihavetomove, azimuth, altitude, zoom);
                }
                catch (...)
                {
                    prev_valid = false;
                    continue;
                }

                if (j > -80)
                {
                    int dx1 = dispcx + zdes.x * dispcx,
                        dy1 = dispcy + zdes.y * dispcx,
                        dx2 = dispcx + prev.x * dispcx,
                        dy2 = dispcy + prev.y * dispcx;

                        if (prev_valid)
                        ImGui::GetBackgroundDrawList()->AddLine(
                            ImVec2(dx1, dy1), ImVec2(dx2, dy2),
                            grid_color, 1);
                }

                prev = zdes;
                prev_valid = true;
            }
        }

        // Compute object draw coordinates.
        for (i=0; cels[i] && i<MAX_CELOBJS; i++)
        {
            Star* s = (Star*)cels[i];
            Point rel = cels[i]->location;
            rel -= here;

            try
            {
                Cartesian2D cart(rel, azimuth, altitude, zoom);
                int dx = (int)(dispcx + cart.x * dispcx), dy = (int)(dispcy + cart.y * dispcx);
                cels[i]->drawnx = dx;
                cels[i]->drawny = dy;
                if (dx < 0 or dx >= io.DisplaySize.x) continue;
                if (dy < 0 or dy >= io.DisplaySize.y) continue;
            }
            catch (...)
            {
                // Object is behind the camera.
                s->drawnx = s->drawny = -1e9;;
            }
        }

        // Draw objects.
        for (i=0; cels[i] && i<MAX_CELOBJS; i++)
        {
            Star* s = (Star*)cels[i];
            Point rel = cels[i]->location;
            rel -= here;
            {
                if (s->drawnx < 0 or s->drawnx >= io.DisplaySize.x) continue;
                if (s->drawny < 0 or s->drawny >= io.DisplaySize.y) continue;

                // Any brighter object within reach, skip this one.
                bool skip = false;
                for (j=0; cels[j] && j<MAX_CELOBJS; j++)
                {
                    if (j==i) continue;
                    if (fabs(s->drawnx - cels[j]->drawnx) < 3
                        &&
                        fabs(s->drawny - cels[j]->drawny) < 3
                        && cels[j]->viewer_magnitude(here) < s->viewer_magnitude(here)
                        )
                    {
                        skip = true;
                        break;
                    }
                }
                if (skip) continue;

                ImVec2 xycoord = ImVec2(s->drawnx, s->drawny);
                float appmag = (cels[i]->type == star) ? s->viewer_magnitude(here) : cels[i]->absolute_magnitude;
                float magrad = (6.0 - appmag)*1.5;
                if (magrad < 1) magrad = 1;
                for (j=magrad; j>0; j-=0.5)
                {
                    Color col = Color::color_from_magnitude_indices(appmag, s->BV_magnitude);
                    RGB rgb = Color::rgb_from_color(col, j);
                    ImGui::GetBackgroundDrawList()->AddCircleFilled(xycoord, 0.7+0.8*j, IM_COL32(rgb.r, rgb.g, rgb.b, 255), 0);
                }
            }
        }


        // Custom mouse cursor.
        if (!is_mouse_over_window)
        {
            cursor_size = (int)io.DisplaySize.x/93;
            circle_size = cursor_size / 3;

            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2(io.MousePos.x, io.MousePos.y - cursor_size),
                ImVec2(io.MousePos.x, io.MousePos.y - circle_size - 1),
                cursor_color, 1);
            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2(io.MousePos.x, io.MousePos.y + cursor_size + 1),
                ImVec2(io.MousePos.x, io.MousePos.y + circle_size + 2),
                cursor_color, 1);
            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2(io.MousePos.x - cursor_size, io.MousePos.y),
                ImVec2(io.MousePos.x - circle_size - 1, io.MousePos.y),
                cursor_color, 1);
            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2(io.MousePos.x + cursor_size + 1, io.MousePos.y),
                ImVec2(io.MousePos.x + circle_size + 2, io.MousePos.y),
                cursor_color, 1);
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(io.MousePos.x, io.MousePos.y),
                circle_size, cursor_color, 8, 1);

            // Object under cursor
            is_an_obj_under_cursor = false;
            obj_magn_under_cursor = 1e9;
            for (i=0; cels[i] && i<MAX_CELOBJS; i++)
            {
                if (abs(cels[i]->drawnx - io.MousePos.x) < circle_size
                    &&
                    abs(cels[i]->drawny - io.MousePos.y) < circle_size
                    )
                {
                    is_an_obj_under_cursor = true;

                    // Prioritize by brightness.
                    double lmag = cels[i]->viewer_magnitude(here);
                    if (lmag < obj_magn_under_cursor)
                    {
                        obj_magn_under_cursor = lmag;
                        objname = cels[i]->name;
                        objinfo = (std::string)"RA: " + cels[i]->RA_as_hms() + (std::string)"\n"
                                + (std::string)"Decl: " + cels[i]->Decl_as_degms() + (std::string)"\n"
                                + (std::string)"Mag: " + std::to_string(lmag) + (std::string)"\n"
                                ;
                        if (cels[i]->distance_known)
                            objinfo += (std::string)"Dist: " + cels[i]->scaled_distance(here) + (std::string)"\n";
                        if (cels[i]->type == star)
                        {
                            Star* s = (Star*)cels[i];
                            objinfo += (std::string)"SpTyp: " + s->spectral_type + (std::string)"\n";
                        }
                    }
                }
            }

            if (!is_an_obj_under_cursor) objname = objinfo = "";
        }

        // Object under cursor info
        is_mouse_over_window = false;
        if (tsatwnd)
        {
            ImGui::Begin("Object", &tsatwnd);
            ImGui::Text(objname.c_str());
            ImGui::Text(objinfo.c_str());
            /* if (ImGui::Button("Close Me"))
                tsatwnd = false; */
            int txtlines = std::count(objinfo.begin(), objinfo.end(), '\n') + 2;
            float txtyscale = ImGui::GetTextLineHeightWithSpacing();
            int tsattop = 18, tsatleft = (int)io.DisplaySize.x - 225, tsatwidth = 211, tsatheight = (int)txtlines*txtyscale;
            if (tsatheight > tsatwnd_hei) tsatwnd_hei = tsatheight;
            else tsatheight = tsatwnd_hei;
            ImGui::SetWindowSize(ImVec2(tsatwidth, tsatheight));
            ImGui::SetWindowPos(ImVec2(tsatleft, tsattop));
            ImGui::End();

            if (io.MousePos.x >= tsatleft && io.MousePos.y >= tsattop
                && io.MousePos.x < tsatleft+tsatwidth && io.MousePos.y < tsattop+tsatheight)
                is_mouse_over_window = true;
        }

        // Positioning updates
        here.local_position += velocity;
        azimuth += spin;

        // Pan with crosshairs
        if (io.MouseDown && !is_mouse_over_window)
        {
            if (ImGui::IsMouseDown(2))
            {
                azimuth -= 0.01 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.01 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, IM_COL32(0, 0, 255, 96), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, IM_COL32(0, 0, 255, 96), 1);
            }
            else if (ImGui::IsMouseDown(1))
            {
                azimuth -= 0.03 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.03 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, IM_COL32(0, 255, 0, 40), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, IM_COL32(0, 255, 0, 40), 1);
            }
            else if (ImGui::IsMouseDown(0))
            {
                azimuth -= 0.1 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.1 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, IM_COL32(255, 0, 0, 64), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, IM_COL32(255, 0, 0, 64), 1);
            }
        }

        // Scroll wheel to zoom
        if (io.MouseWheel > 0)
        {
            zoom *= 1.1;
            global_brightness *= 1.1;
        }
        else if (io.MouseWheel < 0 && zoom > 1)
        {
            zoom *= 0.9;
            global_brightness *= 0.9;
        }

        // Keyboard commands
        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            ImWchar c = io.InputQueueCharacters[i];
            switch (c)
            {
                case 'w':
                velocity.x =  sin(azimuth) * cos(altitude) * light_year / 10;
                velocity.z =  cos(azimuth) * cos(altitude) * light_year / 10;
                velocity.y =  sin(altitude) * light_year / 10;
                spin = 0;
                break;

                case '+':
                velocity.scale(velocity.magnitude() * 1.5);
                break;

                case '-':
                velocity.scale(velocity.magnitude() * 0.666);
                break;

                case 'r':
                velocity = Point(0,0,0);
                spin = 0;
                here.local_position = here.system_center = Point(0,0,0);
                break;

                case 'g':
                gamma += 0.2;
                set_gamma(gamma);
                break;

                case 'G':
                gamma -= 0.2;
                set_gamma(gamma);
                break;


                default:
                ;
            }
        }

        // More code copied from the ImGui example:
        // Rendering
        ImGui::Render();
        if (!is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(background.x * background.w, background.y * background.w, background.z * background.w, background.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        if (io.MousePos.x != io.MousePosPrev.x || io.MousePos.y != io.MousePosPrev.y || velocity.magnitude())
        {
            timeout_ms *= 0.5;
            if (timeout_ms < 5) timeout_ms = 5;
        }
        else
        {
            timeout_ms *= 1.1;
            if (timeout_ms > 250) timeout_ms = 250;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

        frist = false;
    }

    for (i=0; cels[i]; i++) delete cels[i];
    delete[] cels;
    return 0;
}