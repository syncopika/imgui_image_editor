#ifndef IMAGE_EDITOR_UTILS
#define IMAGE_EDITOR_UTILS

#include "imgui.h"
#include "filters.hh"

#include <SDL.h>
#include <GL/glew.h>
#include "external/giflib/gif_lib.h"

#include <string>
#include <vector>

#if WINDOWS_BUILD
#include <windows.h>
#include <commctrl.h>
#endif

// struct for holding reconstructed gif frames (making sure all the frames are properly colored
// since each frame given back by giflib only shows pixels that changed between frames)
// TODO: memory management
struct ReconstructedGifFrames {
    std::vector<unsigned char*> frames;
    int currFrameIndex = 0;
};

std::string trimString(std::string& str);
std::string colorText(int r, int g, int b);

bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height, int* channels);

void swapColors(ImVec4& colorToChange, ImVec4& colorToChangeTo, int imageWidth, int imageHeight);
void updateTempImageState(int imageWidth, int imageHeight);
void resetImageState(int& imageWidth, int& imageHeight, int originalWidth, int originalHeight);
void resizeSDLWindow(SDL_Window* window, int width, int height);
void showImageEditor(SDL_Window* window);
void rotateImage(int& imageWidth, int& imageHeight);
std::vector<int> extractPixelColor(int xCoord, int yCoord, int imageWidth, int imageHeight);
void displayGifFrame(GifFileType* gifImage, ReconstructedGifFrames& gifFrames);

void reconstructGifFrames(ReconstructedGifFrames& gifFrames, GifFileType* gifImage); // TODO: maybe make a method of the ReconstructedGifFrames struct?

void setFilter(Filter filter, std::map<Filter, bool>& filtersWithParams,  int imageWidth, int imageHeight);
void doFilter(int imageWidth, int imageHeight, Filter filter, FilterParameters& filterParams, bool isGif, ReconstructedGifFrames& gifFrames);

#endif