
#include <iostream>
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
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
    double spin = -0.01*fiftyseventh;
    int i, j;

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

    // Main loop
    bool done = false;
    bool alienwnd = true;
    while (!done)
    {
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

        if (alienwnd)
        {
            ImGui::Begin("Greetings", &alienwnd);
            ImGui::Text("Hello from another world!");
            if (ImGui::Button("Close Me"))
                alienwnd = false;
            ImGui::End();
        }

        // TODO:
        int dispcx = (int)io.DisplaySize.x/2, dispcy = (int)io.DisplaySize.y / 2;
        for (i=0; cels[i] && i<MAX_CELOBJS; i++)
        {
            if (cels[i]->type != star) continue;
            Star* s = (Star*)cels[i];
            Point rel = s->location;
            rel -= here;
            try
            {
                Cartesian2D cart(rel, azimuth, altitude, 1);
                ImVec2 xycoord = ImVec2((int)(dispcx + cart.x * dispcx), (int)(dispcy + cart.y * dispcx));
                float magrad = (6.0 - s->viewer_magnitude(here)) / 2;
                if (magrad < 0.1) continue;
                for (j=0; j<magrad; j++)
                    ImGui::GetBackgroundDrawList()->AddCircleFilled(xycoord, 0.7+0.8*j, IM_COL32(255, 255, 255, 200), 0);
            }
            catch (...)
            {
                // Object is behind the camera.
                ;
            }
        }

        here.local_position += velocity;
        azimuth += spin;

        if (io.MouseDown && ImGui::IsMouseDown(0))
        {
            azimuth -= 0.1 * fiftyseventh * io.MouseDelta.x;
            altitude += 0.1 * fiftyseventh * io.MouseDelta.y;
            spin = 0;
        }

        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            ImWchar c = io.InputQueueCharacters[i];
            switch (c)
            {
                case 'w':
                velocity.x =  sin(azimuth) * cos(altitude) * light_year;
                velocity.z =  cos(azimuth) * cos(altitude) * light_year;
                velocity.y =  sin(altitude) * light_year;
                spin = 0;
                break;

                case '+':
                velocity.scale(velocity.magnitude() * 1.5);
                break;

                case '-':
                velocity.scale(velocity.magnitude() * 0.666);
                break;

                case 'g':
                velocity = Point(0,0,0);
                spin = 0;
                here.local_position = here.system_center = Point(0,0,0);
                break;

                default:
                ;
            }
        }

        if (spin && frand(0,1) < 0.001)
        {
            velocity = Point(0,0,light_year);
            spin = 0;
        }

        // More code copied from the ImGui example:
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(background.x * background.w, background.y * background.w, background.z * background.w, background.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

    }

    for (i=0; cels[i]; i++) delete cels[i];
    delete[] cels;
    return 0;
}