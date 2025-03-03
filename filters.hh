#ifndef FILTERS
#define FILTERS

#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <SDL.h>
#include <stdint.h> // for uint8_t
#include <stdlib.h> // for rand()

struct FilterParameters {
    // for saturation
    float lumG = 0.5f;
    float lumR = 0.5f;
    float lumB = 0.5f;
    float saturationVal = 0.5f;
    
    // for outline
    int outlineLimit = 10;
    
    // for channel offset
    int chanOffset = 7;
    int chanOffsetRandNum;
    
    // for mosaic
    int chunkSize = 5;
    
    // for CRT
    int scanLineThickness = 4;
    float brightboost = 0.35f;
    float intensity = 0.25f;
    
    // for Voronoi
    int voronoiNeighborCount = 30;
    
    // for Hilditch thinning
    int thinningIterations = 1;
    
    void generateRandNum3(){
        chanOffsetRandNum = rand() % 3;
    }
};

enum Filter {
    Grayscale,
    Invert,
    Dots,
    Saturation,
    Outline,
    Mosaic,
    ChannelOffset,
    Crt,
    Voronoi,
    Thinning,
    Kuwahara,
};

int correctRGB(int channel);
bool isValidPixel(int row, int col, int width, int height);
std::vector<float> rgbToHsv(int r, int g, int b);
std::vector<int> getRgb(unsigned char* pixelData, int row, int col, int width, int height);

void setFilterState(Filter filterToSet, std::map<Filter, bool>& filters);
void clearFilterState(std::map<Filter, bool>& filters);

void grayscale(unsigned char* pixelData, int pixelDataLen);
void saturate(unsigned char* pixelData, int pixelDataLen, FilterParameters& params);
void outline(unsigned char* pixelData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void invert(unsigned char* pixelData, int pixelDataLen);
void mosaic(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void channelOffset(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void crt(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void voronoi(unsigned char* imageData, int pixelDataLen, int width, int height, FilterParameters& params);
void thinning(unsigned char* imageData, int pixelDataLen, int width, int height, FilterParameters& params);
void dots(unsigned char* pixelData, int pixelDataLen, int imageWidth, int imageHeight, SDL_Renderer* renderer);
void kuwahara_helper(unsigned char* imageData, unsigned char* sourceImageCopy, int width, int height, int row, int col, FilterParameters& params);
void kuwahara(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);

#endif