#ifndef IMAGE_EDITOR_UTILS
#define IMAGE_EDITOR_UTILS

#include "imgui.h"

#include <string>
#include <GL/glew.h>

std::string trimString(std::string& str);
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height, int* channels);
void updateTempImageState(int imageWidth, int imageHeight);
void resetImageState(int imageWidth, int imageHeight);
void showImageEditor();

#endif