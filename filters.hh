#ifndef FILTERS
#define FILTERS

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
    
    void generateRandNum3(){
        chanOffsetRandNum = rand() % 3;
    }
};

int correctRGB(int channel);
void grayscale(unsigned char* pixelData, int pixelDataLen);
void saturate(unsigned char* pixelData, int pixelDataLen, FilterParameters& params);
void outline(unsigned char* pixelData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void invert(unsigned char* pixelData, int pixelDataLen);
void mosaic(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void channelOffset(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);
void crt(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params);

#endif