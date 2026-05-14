
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
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
#include <SDL_image.h> 
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif
#include "classes/misc.h"
#include "classes/color.h"
#include "classes/galaxy.h"
#include "classes/star.h"
#include "classes/planet.h"
#include "classes/cat.h"

// Learn more about ImGui here: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md

using namespace std;
#define MAX_CELOBJS 262144
#define drawn_cache_split 25

int main (int argc, char** argv)
{
    CelestialObject **cels = new CelestialObject*[MAX_CELOBJS];
    double *vmag_cache = new double[MAX_CELOBJS];
    memset(cels, 0, MAX_CELOBJS*sizeof(CelestialObject*));
    std::vector<int> drawnblocks[drawn_cache_split][drawn_cache_split];
    float drawblxscalex, drawblxscaley;
    int *bx_cache = new int[MAX_CELOBJS], *by_cache = new int[MAX_CELOBJS];

    std::vector<std::string> consline_a, consline_b;
    std::vector<int> considx, lnpercons;
    std::vector<Cartesian2D> conscen;
    int nconsln = 0;

    int ncelobjs = 0;
    int selected = -1;
    CelestialLocation here;
    Point velocity;
    double azimuth = 0, altitude = 0;
    double spin = 0;
    int i, j, l, n;
    double gamma = 1.8;
    double zoom = 1, vm;
    bool show_grid = true, show_consln = true, show_xonsm = false, show_labels = true;
    int cursor_size = 10, circle_size = 3, xaorngsim = 0;
    ImU32 cursor_color = IM_COL32(255, 32, 0, 255);
    ImU32 grid_color = IM_COL32(255, 0, 0, 96);
    ImU32 grid_color_brighter = IM_COL32(255, 0, 0, 140);
    ImU32 consline_color = IM_COL32(0, 128, 255, 128);
    ImU32 conslbl_color = IM_COL32(255, 192, 0, 128);
    ImU32 selected_color = IM_COL32(0, 255, 96, 192);
    ImU32 objlbl_color = IM_COL32(64, 255, 0, 176);
    bool is_an_obj_under_cursor;
    double obj_magn_under_cursor;
    std::string objname, objinfo;
    bool is_mouse_over_window;
    int objinfwnd_hei = 0;
    int timeout_ms = 5;
    bool dragging, dragged, viewchanged;
    int lmx, lmy, whereami=0;
    double velocmag;
    time_t simnow = std::time(nullptr);
    double JDnow = ((double)simnow - J2000_TIME_T)/86400 + J2000;
    char lookfor[256];

    memset(lookfor, 0, 256);

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

    for (l=1; l<argc; l++)
    {
        n = strlen(argv[l]);
        if (n == ((xonsm[4] & 017) ^ 015))
        {
            char* ucpdhahzs = "\x2b\x85\xe9\x80\x57\xe4\x70\x00";
            i = 0;
            for (j=0; ucpdhahzs[j]; j++)
                if (argv[l][j] == ucpdhahzs[j] ^ (xonsm[j] & 0377)) i++;

            if (i==n)
            {
                show_xonsm = true;
                xaorngsim = l;
            }
        }
    }

    FILE* fp = fopen("consline.dat", "rb");
    if (fp)
    {
        char buffer[256];
        l = -1;
        while (fgets(buffer, 253, fp))
        {
            char* newline = strchr(buffer, '\n');
            if (newline) *newline = 0;
            newline = strchr(buffer, '\r');
            if (newline) *newline = 0;
            if (*buffer == '~')
            {
                char* name2 = strchr(buffer, ',');
                if (!name2) continue;
                *name2 = 0;
                name2++;
                while (*name2 == ' ')
                {
                    *name2 = 0;
                    name2++;
                }
                char* name3 = strchr(name2, ',');
                if (name3)
                {
                    *name3 = 0;
                    name3++;
                    while (*name3 == ' ')
                    {
                        *name3 = 0;
                        name3++;
                    }
                }
                if (strlen(name2))
                {
                    consname.push_back(name2);
                    consabbrev.push_back(&buffer[1]);
                    lnpercons.push_back(0);
                    l++;
                }
                if (name3 && strlen(name3)) consgen.push_back(name3);
                else consgen.push_back("");
            }
            else if (l>=0)
            {
                char* name2 = strchr(buffer, ',');
                if (!name2) continue;
                *name2 = 0;
                name2++;
                while (*name2 == ' ')
                {
                    *name2 = 0;
                    name2++;
                }
                if (strlen(name2))
                {
                    consline_a.push_back(buffer);
                    consline_b.push_back(trim(name2));
                    considx.push_back(l);
                    lnpercons[l]++;
                    nconsln++;
                }
            }
        }
        fclose(fp);
    }

    // TODO: Read data from more star catalogs.
    CatalogReader cr;
    cr.download_catalogs();
    std::vector<std::string> cats = cr.find_catalogs("catalogs");

    bool have_Gliese = false, have_BSC = false, have_HIP = false;
    n = cats.size();
    for (i=0; i<n; i++)
    {
        cout << "Found " << cats[i] << endl;
        if (!strcmp(cats[i].c_str(), "catalogs/Gliese")) have_Gliese = true;
        if (!strcmp(cats[i].c_str(), "catalogs/BSC")) have_BSC = true;
        if (!strcmp(cats[i].c_str(), "catalogs/Hipparcos")) have_HIP = true;
    }

    if (have_Gliese)
    {
        cout << "Reading Gliese catalog..." << endl << flush;
        int nGliese = cr.read_Gliese_catalog(cels, MAX_CELOBJS);
        cout << "Read " << nGliese << " objects." << endl << flush;
        ncelobjs += nGliese;
    }
    if (have_BSC)
    {
        cout << "Reading Bright Star Catalog..." << endl << flush;
        int nBSC = cr.read_BrightStars_catalog(cels, MAX_CELOBJS);
        cout << "Read " << nBSC << " objects." << endl << flush;
        ncelobjs += nBSC;
    }
    #ifndef DEBUG
    if (have_HIP)
    {
        cout << "Reading Hipparcos catalog..." << endl << flush;
        int nHIP = cr.read_Hipparcos_catalog(cels, MAX_CELOBJS);
        cout << "Updated " << nHIP << " objects." << endl << flush;
    }
    #endif

    // Cache star indices of consline termini
    int consaidx[nconsln+16], consbidx[nconsln+16];
    for (i=0; i<nconsln; i++)
    {
        int founda = -1, foundb = -1;
        for (j=0; cels[j]; j++)
        {
            if (cels[j]->type != star) continue;
            Star* s = (Star*)cels[j];
            if (founda < 0
                && 
                (
                    !strcmp(s->Bayer.c_str(), consline_a[i].c_str()) 
                    ||
                    !strcmp(s->Flamsteed.c_str(), consline_a[i].c_str())
                    ||
                    (
                        consline_a[i].c_str()[0] == 'H' && consline_a[i].c_str()[1] == 'D'
                        && s->HD && (unsigned)atoi(&consline_a[i].c_str()[2]) == s->HD
                    )
                ))
            {
                founda = j;
            }
            else if (foundb < 0
                &&
                (
                    !strcmp(s->Bayer.c_str(), consline_b[i].c_str())
                    ||
                    !strcmp(s->Flamsteed.c_str(), consline_b[i].c_str())
                    ||
                    (
                        consline_b[i].c_str()[0] == 'H' && consline_b[i].c_str()[1] == 'D'
                        && s->HD && (unsigned)atoi(&consline_b[i].c_str()[2]) == s->HD
                    )
                ))
            {
                foundb = j;
            }
        }

        consaidx[i] = founda;
        consbidx[i] = foundb;
    }

    if (show_xonsm)
    {
        for (i=0; i<11; i++)
        {
            int founda = -1, foundb = -1;
            __uint32_t ztym = xonsm[i] & 65535, srap = xonsm[i] / 65536;
            for (j=0; cels[j]; j++)
            {
                if (cels[j]->type != star) continue;
                Star* s = (Star*)cels[j];
                if (founda < 0 && ((!j && !ztym) || s->HD == ztym)) founda = j;
                else if (foundb < 0 && ((!j && !srap) || s->HD == srap)) foundb = j;
            }

            consname.push_back(argv[xaorngsim]);
            if (founda >= 0 && foundb >= 0)
            {
                consaidx[i+nconsln] = founda;
                consbidx[i+nconsln] = foundb;
            }
        }
    }

    for (i=0; cels[i]; i++)
    {
        if (cels[i]->type == star)
        {
            Star* s = (Star*)cels[i];
            s->rename_from_Bayer_Flamsteed();           // has no effect if not a Bayer-Flamsteed star.
        }
    }

    cr.read_starname_dat(cels);

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

    SDL_Surface* icon = IMG_Load("assets/icon48.png");
    if (icon)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
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
    bool objinfwnd = true;
    bool statuswnd = true;
    bool hide_mouse = true;
    bool searched = false;
    double frame_dur = 0, best_frame_dur = 1e9;
    viewchanged = true;
    ImVec2 PrevDispSize;
    while (!done)
    {
        auto frame_began = std::chrono::high_resolution_clock::now();
        if (hide_mouse && !is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);

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

        if (hide_mouse && !is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);
        int dispcx = (int)io.DisplaySize.x/2, dispcy = (int)io.DisplaySize.y / 2;
        drawblxscalex = 2.0 / (dispcx/drawn_cache_split);
        drawblxscaley = 2.0 / (dispcy/drawn_cache_split);
        for (i=0; i<drawn_cache_split; i++) for (j=0; j<drawn_cache_split; j++) drawnblocks[i][j].clear();
        if (whereami >= 0) here = cels[whereami]->location;

        if (show_grid)
        {
            Cartesian2D prev, zdes;
            ImU32 gc = rgba_apply_redlight(grid_color);
            ImU32 gcb = rgba_apply_redlight(grid_color_brighter);
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
                                gc, 1);
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

                    if (i)
                    {
                        int dx1 = dispcx + zdes.x * dispcx,
                            dy1 = dispcy + zdes.y * dispcx,
                            dx2 = dispcx + prev.x * dispcx,
                            dy2 = dispcy + prev.y * dispcx;

                            if (prev_valid)
                            ImGui::GetBackgroundDrawList()->AddLine(
                                ImVec2(dx1, dy1), ImVec2(dx2, dy2),
                                j?gc:gcb, 1);
                    }

                    prev = zdes;
                    prev_valid = true;
                }
            }
        }

        // Compute object draw coordinates.
        if (viewchanged) for (i=0; cels[i] && i<MAX_CELOBJS; i++)
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
                if (viewchanged) vmag_cache[i] = s->viewer_magnitude(here);
                if (dx < 0 or dx >= io.DisplaySize.x) continue;
                if (dy < 0 or dy >= io.DisplaySize.y) continue;
                int bx = dx*drawblxscalex, by = dy*drawblxscaley;
                if (bx<0 || bx>=drawn_cache_split || by<0 || by>=drawn_cache_split) continue;
                drawnblocks[bx][by].push_back(i);
                bx_cache[i] = bx;
                by_cache[i] = by;
            }
            catch (...)
            {
                // Object is behind the camera.
                s->drawnx = s->drawny = -1e9;;
            }
        }

        // Constellation lines
        if (show_consln)
        {
            conscen.clear();
            n = consname.size();
            for (l=0; l<n; l++)
            {
                conscen.push_back(Cartesian2D(0,0));
                lnpercons[l] = 0;
            }
            n = show_xonsm ? (nconsln+11) : nconsln;
            for (i=0; i<n; i++)
            {
                int dx1, dx2, dy1, dy2;

                if (consaidx[i] < 0 || consbidx[i] < 0) continue;
                if (i >= nconsln) considx[i] = consname.size()-1;
                l = considx[i];

                dx1 = cels[consaidx[i]]->drawnx;
                dy1 = cels[consaidx[i]]->drawny;
                if (dx1 < -1e3) continue;
                if (dy1 < -1e3) continue;

                dx2 = cels[consbidx[i]]->drawnx;
                dy2 = cels[consbidx[i]]->drawny;
                if (dx2 < -1e3) continue;
                if (dy2 < -1e3) continue;

                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(dx1, dy1), ImVec2(dx2, dy2),
                    rgba_apply_redlight((i<nconsln) ? consline_color : IM_COL32(255, 64, 0, 128)), 1);

                assert (l < conscen.size());
                conscen[l] += Cartesian2D((dx1+dx2)/2, (dy1+dy2)/2);
                lnpercons[l]++;
            }

            // Constellation labels
            n=l;
            if (show_labels) for (l=0; l<n; l++)
            {
                if (!lnpercons[l]) continue;
                conscen[l] /= lnpercons[l];
                if (conscen[l].x < 0 || conscen[l].y < 0) continue;
                int dx = conscen[l].x, dy = conscen[l].y;
                ImVec2 sz = ImGui::CalcTextSize(consname[l].c_str());
                dx -= sz.x/2;
                dy -= sz.y/2;
                if (dx >= 0 && dx < io.DisplaySize.x && dy >= 0 && dy < io.DisplaySize.y)
                {
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(dx, dy),
                        rgba_apply_redlight((l<nconsln) ? conslbl_color : IM_COL32(255, 64, 0, 128)),
                        consname[l].c_str());
                }
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
                // TODO: Find a better way than the commented out block below.
                // It does the job well, however it is a huge performance killer.
                if (bx_cache[i] >= 0 && bx_cache[i] < drawn_cache_split && by_cache[i] >= 0 && by_cache[i] < drawn_cache_split)
                {
                    n = drawnblocks[bx_cache[i]][by_cache[i]].size();
                    for (l=0; l<n; l++)
                    {
                        j = drawnblocks[bx_cache[i]][by_cache[i]][l];
                        if (j==i) continue;
                        if (fabs(s->drawnx - cels[j]->drawnx) < 3
                            &&
                            fabs(s->drawny - cels[j]->drawny) < 3
                            && vmag_cache[j] < vmag_cache[i]
                            )
                        {
                            skip = true;
                            break;
                        }
                    }
                }
                if (skip) continue;

                ImVec2 xycoord = ImVec2(cels[i]->drawnx, cels[i]->drawny);
                float appmag = (cels[i]->type == star) ? vmag_cache[i] : cels[i]->absolute_magnitude;
                float magrad = (5.0 - appmag)*1.5;
                if (magrad < 1) magrad = 1;
                Color col = Color::color_from_magnitude_indices(appmag, s->BV_magnitude);
                if (s->HD == 106591)
                {
                    j = 0;
                }
                for (j=magrad; j>0.6; j-=0.5)
                {
                    RGB rgb = Color::rgb_from_color(col, j-1);
                    if (rgb.r < 16 && rgb.b < 16) continue;
                    ImGui::GetBackgroundDrawList()->AddCircle(xycoord, 0.7+0.8*j, IM_COL32(rgb.r, rgb.g, rgb.b, 255), 0, 1.5);
                }
                if (selected == i)
                {
                    ImGui::GetBackgroundDrawList()->AddCircle(xycoord, magrad+2, rgba_apply_redlight(selected_color), 0, 2);
                }
                if (show_labels && (appmag <= 1.5 || i == selected))
                {
                    ImVec2 sz = ImGui::CalcTextSize(cels[i]->name.c_str());
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(cels[i]->drawnx - sz.x/2, cels[i]->drawny+magrad+1),
                        rgba_apply_redlight(objlbl_color),
                        cels[i]->name.c_str());
                }
            }
        }

        // Custom mouse cursor.
        bool is_click = io.MouseReleased[0];
        if (!is_mouse_over_window)
        {
            if (!ImGui::IsMouseDown(0) && !ImGui::IsMouseDown(1) && !ImGui::IsMouseDown(2))
            {
                cursor_size = (int)io.DisplaySize.x/81;
                circle_size = cursor_size / 2.5;

                ImU32 c = rgba_apply_redlight(cursor_color);
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(io.MousePos.x, io.MousePos.y - cursor_size),
                    ImVec2(io.MousePos.x, io.MousePos.y - circle_size - 1),
                    c, 1);
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(io.MousePos.x, io.MousePos.y + cursor_size + 1),
                    ImVec2(io.MousePos.x, io.MousePos.y + circle_size + 2),
                    c, 1);
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(io.MousePos.x - cursor_size, io.MousePos.y),
                    ImVec2(io.MousePos.x - circle_size - 1, io.MousePos.y),
                    c, 1);
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(io.MousePos.x + cursor_size + 1, io.MousePos.y),
                    ImVec2(io.MousePos.x + circle_size + 2, io.MousePos.y),
                    c, 1);
                ImGui::GetBackgroundDrawList()->AddCircle(
                    ImVec2(io.MousePos.x, io.MousePos.y),
                    circle_size, c, 8, 1);
            }

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
                    double lmag = vmag_cache[i];
                    if (lmag < obj_magn_under_cursor)
                    {
                        obj_magn_under_cursor = lmag;

                        objname = cels[i]->name;
                        objinfo = "";
                        if (cels[i]->type == star)
                        {
                            if (((Star*)cels[i])->Bayer.size() && ((Star*)cels[i])->Flamsteed.size())
                            {
                                int Fl = atoi(((Star*)cels[i])->Flamsteed.c_str());
                                objinfo += std::to_string(Fl) + ((Star*)cels[i])->Bayer + (std::string)"\n";
                            }
                            else if (((Star*)cels[i])->Flamsteed.size()) objinfo += ((Star*)cels[i])->Flamsteed + (std::string)"\n";
                            else if (((Star*)cels[i])->Bayer.size()) objinfo += ((Star*)cels[i])->Bayer + (std::string)"\n";

                            if (((Star*)cels[i])->Gliese.size()) objinfo += ((Star*)cels[i])->Gliese + (std::string)"\n";
                            if (((Star*)cels[i])->HD) objinfo += (std::string)"HD" + std::to_string(((Star*)cels[i])->HD) + (std::string)"\n";
                            if (((Star*)cels[i])->HR) objinfo += (std::string)"HR" + std::to_string(((Star*)cels[i])->HR) + (std::string)"\n";
                            if (((Star*)cels[i])->HIP) objinfo += (std::string)"HIP" + std::to_string(((Star*)cels[i])->HIP) + (std::string)"\n";
                        }
                        objinfo += (std::string)"RA:    " + cels[i]->RA_as_hms(here) + (std::string)"\n"
                                + (std::string)"Decl:  " + cels[i]->Decl_as_degms(here) + (std::string)"\n"
                                + (std::string)"Mag:   " + std::to_string(lmag) + (std::string)"\n"
                                + (std::string)"Epoch: " + std::to_string((cels[i]->epoch-J2000)/365.2425+2000) + (std::string)"\n"
                                ;
                        if (cels[i]->distance_known)
                            objinfo += (std::string)"Dist:  " + cels[i]->scaled_distance(here) + (std::string)"\n";
                        if (cels[i]->type == star)
                        {
                            Star* s = (Star*)cels[i];
                            objinfo += (std::string)"SpTyp: " + s->spectral_type + (std::string)"\n";
                        }

                        if (i == selected) break;
                        if (is_click && !dragged) selected = i;
                    }
                }
            }

            if (!is_an_obj_under_cursor)
            {
                objname = std::to_string(ncelobjs) + (std::string)" objects.";
                objinfo = "Press N to toggle\nthis window.\n\n";
                if (is_click && !dragged) selected = -1;
            }
        }

        is_mouse_over_window = false;
        float txtyscale = ImGui::GetTextLineHeightWithSpacing(), txtycompact = ImGui::GetTextLineHeight();

        // Status window
        if (statuswnd)
        {
            // TODO: If redlight_mode, set all window and text colors accordingly.
            int stattop = 18, statleft = 15, statwidth = 211, statheight = txtyscale*2;
            ImGui::Begin("Status", &statuswnd);

            /////////////////////////////////////////////////////

            ImGui::InputText("##find", lookfor, 255);
            ImGui::SameLine();
            if (ImGui::Button("Find"))
            {
                selected = -1;
                for (i=0; cels[i]; i++)
                {
                    if (!strcmp(cels[i]->name.c_str(), lookfor))
                    {
                        azimuth = -cels[i]->RA_as_radians(here);
                        altitude = cels[i]->Decl_as_radians(here);
                        selected = i;
                        searched = true;
                        break;
                    }
                }

                if (selected < 0)
                {
                    int best_Levenshtein = 1e6;
                    std::string lookstr = lookfor;
                    for (i=0; cels[i]; i++)
                    {
                        int lev = Damerau_Levenshtein(cels[i]->name.c_str(), lookstr);
                        if (lev < best_Levenshtein)
                        {
                            best_Levenshtein = lev;
                            azimuth = -cels[i]->RA_as_radians(here);
                            altitude = cels[i]->Decl_as_radians(here);
                            selected = i;
                            searched = true;
                        }
                    }
                }
            }
            statheight += txtyscale*1.3;

            std::string flagstr;

            flagstr = (std::string)"Brghtns (B): " + std::to_string(global_brightness);
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Gamma (`): " + std::to_string(get_gamma());
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Cons ln (C): "
                + std::string(show_consln ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"RA/Decl (G): "
                + std::string(show_grid ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Labels (L): "
                + std::string(show_labels ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Redlgt (Sh+R): "
                + std::string(redlight_mode ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Obj info (N): "
                + std::string(objinfwnd ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            flagstr = (std::string)"Status (S): "
                + std::string(statuswnd ? "ON" : "OFF");
            ImGui::Text(flagstr.c_str());
            statheight += txtyscale;

            ImGui::Text("-----");
            statheight += txtyscale;

            std::string vfstr;
            if (whereami >= 0)
                vfstr = std::string("View from ") + cels[whereami]->name;
            else vfstr = std::string("View from space");
            ImGui::Text(vfstr.c_str());
            statheight += txtyscale;

            double vm = velocity.magnitude() * target_frame_rate;
            if (isnan(vm)) vm = 0;
            velocmag = vm;
            std::string velocstr;
            if (velocmag < 0.01 * speed_of_light) velocstr = std::string("Velocity: ") + std::to_string(velocmag / 1000 * 3600) + std::string(" km/h");
            else if (velocmag < speed_of_light) velocstr = std::string("Velocity: ") + std::to_string(velocmag / speed_of_light) + std::string(" c");
            else
            {
                std::ostringstream oss;
                oss << std::scientific << std::setprecision(2) << (velocmag / speed_of_light);
                velocstr = std::string("Velocity: ") + oss.str() + std::string(" warp");
            }
            ImGui::Text(velocstr.c_str());
            statheight += txtyscale;

            struct tm *utc_time = std::gmtime(&simnow);
            int mon = utc_time->tm_mon + 1, mday = utc_time->tm_mday;
            std::string datedisp = std::to_string(utc_time->tm_year + 1900)
                + std::string("-") + std::string((mon<10)?"0":"") + std::to_string(mon)
                + std::string("-") + std::string((mday<10)?"0":"") + std::to_string(mday);
            ImGui::Text(datedisp.c_str());
            statheight += txtyscale;

            int hr = utc_time->tm_hour, mn = utc_time->tm_min, sec = utc_time->tm_sec;
            std::string timedisp = std::string((hr<10)?"0":"") + std::to_string(hr)
                + std::string(":") + std::string((mn<10)?"0":"") + std::to_string(mn)
                + std::string(":") + std::string((sec<10)?"0":"") + std::to_string(sec)
                + std::string(" UTC");
            ImGui::Text(timedisp.c_str());
            statheight += txtyscale;

            std::string JDdisp = std::string("JD") + std::to_string(JDnow);
            ImGui::Text(JDdisp.c_str());
            statheight += txtyscale;

            std::string frame_rate = std::to_string(1.0 / frame_dur) + std::string(" frames/s");
            ImGui::Text(frame_rate.c_str());
            statheight += txtyscale;

            /////////////////////////////////////////////////////

            ImGui::SetWindowPos(ImVec2(statleft, stattop));
            ImGui::SetWindowSize(ImVec2(statwidth, statheight));
            ImGui::End();

            if (io.MousePos.x >= statleft && io.MousePos.y >= stattop
                && io.MousePos.x < statleft+statwidth && io.MousePos.y < stattop+statheight)
                is_mouse_over_window = true;
        }

        // Object under cursor info
        if (objinfwnd)
        {
            // TODO: If redlight_mode, set all window and text colors accordingly.
            ImGui::Begin("Object", &objinfwnd);
            int objinftop = 18, objinfleft = (int)io.DisplaySize.x - 225, objinfwidth = 211, objinfheight = txtyscale*2;

            ImGui::Text(objname.c_str());
            objinfheight += txtyscale;

            int txtlines = std::count(objinfo.begin(), objinfo.end(), '\n');
            ImGui::Text(objinfo.c_str());
            objinfheight += txtlines*txtycompact;

            ImGui::SetWindowPos(ImVec2(objinfleft, objinftop));
            ImGui::SetWindowSize(ImVec2(objinfwidth, objinfheight));
            ImGui::End();

            if (io.MousePos.x >= objinfleft && io.MousePos.y >= objinftop
                && io.MousePos.x < objinfleft+objinfwidth && io.MousePos.y < objinftop+objinfheight)
                is_mouse_over_window = true;
        }

        // Positioning updates
        here.local_position += velocity;
        azimuth += spin;
        viewchanged = searched || spin || velocity.magnitude() || (PrevDispSize.x != io.DisplaySize.x) || (PrevDispSize.y != io.DisplaySize.y);

        // Pan with crosshairs
        bool is_mouse_down = ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1) || ImGui::IsMouseDown(2);
        if (is_mouse_down && !is_mouse_over_window && dragging)
        {
            if (ImGui::IsMouseDown(2))
            {
                azimuth -= 0.01 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.01 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;
                viewchanged = true;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, rgba_apply_redlight(IM_COL32(0, 0, 255, 128)), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, rgba_apply_redlight(IM_COL32(0, 0, 255, 128)), 1);
            }
            else if (ImGui::IsMouseDown(1))
            {
                azimuth -= 0.03 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.03 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;
                viewchanged = true;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, rgba_apply_redlight(IM_COL32(0, 255, 0, 64)), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, rgba_apply_redlight(IM_COL32(0, 255, 0, 64)), 1);
            }
            else if (ImGui::IsMouseDown(0))
            {
                azimuth -= 0.1 * fiftyseventh * io.MouseDelta.x / zoom;
                altitude += 0.1 * fiftyseventh * io.MouseDelta.y / zoom;
                if (altitude >  M_PI/2) altitude =  M_PI/2;
                if (altitude < -M_PI/2) altitude = -M_PI/2;
                spin = 0;
                viewchanged = true;

                ImVec2 topcen(dispcx, 0), botcen(dispcx, (int)io.DisplaySize.y-1),
                    leftcen(0, dispcy), rightcen((int)io.DisplaySize.x-1, dispcy);
                ImGui::GetBackgroundDrawList()->AddLine(topcen, botcen, rgba_apply_redlight(IM_COL32(255, 96, 0, 96)), 1);
                ImGui::GetBackgroundDrawList()->AddLine(leftcen, rightcen, rgba_apply_redlight(IM_COL32(255, 96, 0, 96)), 1);
            }
        }
        if (is_mouse_down && (io.MousePos.x != lmx || io.MousePos.y != lmy)) dragging = true;
        else if (is_click) dragging = false;

        // Scroll wheel to zoom
        if (io.MouseWheel > 0)
        {
            zoom *= 1.1;
            global_brightness *= 1.1;
            viewchanged = true;
        }
        else if (io.MouseWheel < 0 && zoom > 1)
        {
            zoom *= 0.9;
            global_brightness *= 0.9;
            viewchanged = true;
        }

        // Keyboard commands
        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            ImWchar c = io.InputQueueCharacters[i];
            switch (c)
            {
                case 'b': global_brightness *= 1.5; break;
                case 'B': global_brightness *= 0.666; break;
                case 'c': show_consln = !show_consln; break;
                case 'g': show_grid = !show_grid; break;
                case 'l': show_labels = !show_labels; break;
                case 'n': objinfwnd = !objinfwnd; break;

                case 'o':
                if (selected >= 0)
                {
                    here = cels[selected]->location;
                    whereami = selected;
                }
                velocity = Point(0,0,0);
                viewchanged = true;
                break;

                case 'r':
                velocity = Point(0,0,0);
                spin = 0;
                whereami = 0;
                here.local_position = here.system_center = Point(0,0,0);
                viewchanged = true;
                break;

                case 'R': redlight_mode = !redlight_mode; break;
                case 's': statuswnd = !statuswnd; break;

                case 'w':
                velocity.x =  sin(azimuth) * cos(altitude) * speed_of_light / target_frame_rate;
                velocity.z =  cos(azimuth) * cos(altitude) * speed_of_light / target_frame_rate;
                velocity.y =  sin(altitude) * speed_of_light / target_frame_rate;
                spin = 0;
                viewchanged = true;
                whereami = -1;
                break;

                case 'x':
                velocity = Point(0,0,0);
                viewchanged = true;
                break;

                case '+':
                vm = velocity.magnitude();
                if (vm) velocity.scale(vm * 1.5);
                else
                {
                    velocity.x =  sin(azimuth) * cos(altitude) * 1000;
                    velocity.z =  cos(azimuth) * cos(altitude) * 1000;
                    velocity.y =  sin(altitude) * 1000;
                    whereami = -1;
                }
                viewchanged = true;
                break;

                case '-':
                vm = velocity.magnitude();
                velocity.scale(vm * 0.666);
                viewchanged = true;
                break;

                case '`':
                gamma += 0.2;
                set_gamma(gamma);
                break;

                case '~':
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
        if (hide_mouse && !is_mouse_over_window) SDL_ShowCursor(SDL_DISABLE);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(background.x * background.w, background.y * background.w, background.z * background.w, background.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        if ((io.MousePos.x != lmx || io.MousePos.y != lmy || velocity.magnitude()) && frame_dur > (1.0/target_frame_rate))
        {
            timeout_ms *= 0.333;
            if (timeout_ms < 5) timeout_ms = 5;
        }
        else
        {
            timeout_ms *= 1.5;
            if (timeout_ms > 250) timeout_ms = 250;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

        #ifdef DEBUG
        hide_mouse = false;
        #else
        hide_mouse = abs(lmx - io.MousePos.x) <= 4 || abs(lmy - io.MousePos.y) <= 4;
        #endif

        lmx = io.MousePos.x;
        lmy = io.MousePos.y;
        dragged = dragging;
        searched = false;
        PrevDispSize = io.DisplaySize;

        auto frame_finished = std::chrono::high_resolution_clock::now();
        auto frame_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_finished - frame_began);
        frame_dur = frame_elapsed.count() * 1e-6;
        if (frame_dur < best_frame_dur) best_frame_dur = frame_dur;

        JDnow += frame_dur/86400;
        simnow = (JDnow - J2000)*86400 + J2000_TIME_T;
    }

    for (i=0; cels[i]; i++) delete cels[i];
    delete[] cels;
    delete[] vmag_cache;
    delete[] bx_cache;
    delete[] by_cache;
    return 0;
}