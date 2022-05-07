#ifndef IMAGE_EDITOR_UTILS
#define IMAGE_EDITOR_UTILS

#include "imgui.h"
#include "filters.hh"

#include <SDL.h>
#include <string>
#include <GL/glew.h>

std::string trimString(std::string& str);
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height, int* channels);
void updateTempImageState(int imageWidth, int imageHeight);
void resetImageState(int imageWidth, int imageHeight);
void resizeSDLWindow(SDL_Window* window, int width, int height);
void showImageEditor(SDL_Window* window);
void rotateImage(int imageWidth, int imageHeight);

void setFilter(Filter filter, std::map<Filter, bool>& filtersWithParams,  int imageWidth, int imageHeight);
void doFilter(int imageWidth, int imageHeight, Filter filter, FilterParameters& filterParams);

#endif