#
# Makefile for Linux, Windows, Mac OS. Make sure to install SDL2 (http://www.libsdl.org)
# Linux:
# 	apt-get install -y libsdl2-dev
# Mac OS:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL2
#

CPP = g++
CPPFLAGS = -std=c++14 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -g -Wall -Wformat
IMGUI_DIR = src/imgui
IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
		  + $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
BIN = bin
UNAME_S := $(shell uname -s)
LIBS =
LINUX_GL_LIBS = -lGL

# Platform-specific stuff for ImGui:
ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Building for Linux..."
	LIBS += $(LINUX_GL_LIBS) -ldl `sdl2-config --libs`

	CPPFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CPPFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Building for Mac OS..."
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CPPFLAGS += `sdl2-config --cflags`
	CPPFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CPPFLAGS)
endif

ifeq ($(OS), Windows_NT)
    ECHO_MESSAGE = "Building for Windows in MinGW..."
    LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

    CPPFLAGS += `pkg-config --cflags sdl2`
    CFLAGS = $(CPPFLAGS)
endif

all: $(BIN) alienorum

$(BIN):
	if [ ! -f $(BIN) ]; then mkdir -p $(BIN); fi

alienorum: $(BIN)/alienorum

$(BIN)/alienorum: src/alienorum.cpp
	$(CPP) src/alienorum.cpp -o $(BIN)/alienorum $(CPPFLAGS) $(LIBS)
