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
CPPFLAGS = -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -g -Wall -Wformat

# Debug mode
# CPPFLAGS += -g

IMGUI_DIR = src/imgui
CLASSES_DIR = src/classes
IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
		    $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
CLASSES_SRC = $(CLASSES_DIR)/point.cpp $(CLASSES_DIR)/cat.cpp
BIN = bin
OBJ = obj
UNAME_S := $(shell uname -s)
LIBS =
LINUX_GL_LIBS = -lGL
OBJS = $(addsuffix .o, $(addprefix $(OBJ)/, $(basename $(notdir $(IMGUI_SRC)))))
OBJS += $(addsuffix .o, $(addprefix $(OBJ)/, $(basename $(notdir $(CLASSES_SRC)))))

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

all: $(BIN) $(OBJ) objs apps

$(BIN):
	if [ ! -f $(BIN) ]; then mkdir -p $(BIN); fi

$(OBJ):
	if [ ! -f $(OBJ) ]; then mkdir -p $(OBJ); fi

apps: alienorum

objs: $(OBJS)

alienorum: $(BIN)/alienorum

%.o:$(IMGUI_DIR)/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $(OBJ)/$@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $(OBJ)/$@ $<

$(OBJ)/cat.o:$(CLASSES_DIR)/cat.cpp
	$(CPP) $(CLASSES_DIR)/cat.cpp $(CPPFLAGS) -c -o $(OBJ)/cat.o

$(OBJ)/point.o:$(CLASSES_DIR)/point.cpp
	$(CPP) $(CLASSES_DIR)/point.cpp $(CPPFLAGS) -c -o $(OBJ)/point.o

$(BIN)/alienorum: src/alienorum.cpp $(OBJS)
	$(CPP) src/alienorum.cpp $(OBJ)/*.o -o $(BIN)/alienorum $(CPPFLAGS) $(LIBS)
