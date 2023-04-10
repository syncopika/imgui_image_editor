CXX = g++

# using mingw32
ECHO_MESSAGE = "MinGW"

# C:\SDL2\lib -lSDL2main -lSDL2
SDL_LIB = C:\libraries\SDL2-2.0.10\i686-w64-mingw32\lib -lSDL2main -lSDL2

# -C:\SDL2\include\SDL2
SDL_INCLUDE = C:\libraries\SDL2-2.0.10\i686-w64-mingw32\include\SDL2

#C:\glew-2.1.0\include
OPENGL_INCLUDE = C:\libraries\glew-2.1.0\include

# other dependencies like stb_img
OTHER_LIBS_DIR = external
GIFLIB_DIR = external/giflib
IMGUI_DIR = imgui

SOURCES = image_editor.cpp
SOURCES += utils.cpp filters.cpp voronoi_helper.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp

# specific to our backend (sdl + opengl)
SOURCES += $(IMGUI_DIR)/imgui_impl_sdl.cpp $(IMGUI_DIR)/imgui_impl_opengl3.cpp

# GIFLIB dependency
GIFLIB_SOURCE = $(GIFLIB_DIR)/dgif_lib.c \
$(GIFLIB_DIR)/gifalloc.c $(GIFLIB_DIR)/gif_font.c \
$(GIFLIB_DIR)/gif_hash.c $(GIFLIB_DIR)/openbsd_reallocarray.c

# note all the include flags for all the header file locations
# remove the -DWINDOWS_BUILD flag if you don't have windows.h
CXXFLAGS = -g -O2 -Wall -Wformat -std=c++14 -I$(SDL_INCLUDE) -I$(IMGUI_DIR) -I$(OTHER_LIBS_DIR) -I$(OPENGL_INCLUDE) -DWINDOWS_BUILD

GLEW_LIBS = -lglew32 -lglu32

# add '-mwindows' to LIBS to prevent an additional command line terminal from appearing (but it's useful for debugging) but also if using -DWINDOWS_BUILD
LIBS = -mwindows -lmingw32 -lgdi32 $(GLEW_LIBS) -lopengl32 -limm32 -static-libstdc++ -static-libgcc -L$(SDL_LIB)

# object files needed
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS += $(addsuffix .o, $(basename $(notdir $(GIFLIB_SOURCE))))

EXE = image_editor

# build main
%.o:%.cpp %.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# build imgui dependencies
%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
    
# GIFLIB dependency
%.o:$(GIFLIB_DIR)/%.c
	gcc -g -O2 -Wall -Wformat -std=c99 -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJS)
