// see https://github.com/syncopika/funSketch/blob/master/src/filters/thinning.js
#ifndef THINNING_HELPER_H
#define THINNING_HELPER_H

#include <iostream>

void binarize(unsigned char* data, int width, int height, float threshold);
bool checkBlackNeighbors(unsigned char* data, int dataLength, int i, int j, int width);
bool testConnectivity(unsigned char* data, int dataLength, int i, int j, int width);
bool verticalLineCheck(unsigned char* data, int dataLength, int i, int j, int width);
bool horizontalLineCheck(unsigned char* data, int dataLength, int i, int j, int width);
bool hasEightNeighbors(unsigned char* data, int dataLength, int i, int j, int width);
bool isBlackPixel(unsigned char* data, int i, int j, int width);

#endif