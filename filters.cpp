#include "filters.hh"
#include "voronoi_helper.hh"
#include "thinning_helper.hh"

int correctRGB(int channel){
    if(channel > 255){
        return 255;
    }
    if(channel < 0){
        return 0;
    }
    return channel;
}

bool isValidPixel(int row, int col, int width, int height){
  return row < height && row >= 0 && col < width && col >= 0;
}

// https://github.com/ratkins/RGBConverter/blob/master/RGBConverter.cpp#L92
std::vector<float> rgbToHsv(int r, int g, int b){
  float rf = r / 255.0;
  float gf = g / 255.0;
  float bf = b / 255.0;
  
  float max = std::max(rf, std::max(gf, bf));
  float min = std::min(rf, std::min(gf, bf));
  float delta = max - min;
  
  float h = max;
  float s = max;
  float v = max;
  
  s = (max == 0) ? 0 : (delta / max);
  
  if(max == min){
    h = 0;
  }else{
    if(max == rf){
      h = (gf - bf) / delta + (gf < bf ? 6 : 0);
    }else if(max == gf){
      h = (bf - rf) / delta + 2;
    }else{
      h = (rf - gf) / delta + 4;
    }
    h /= 6;
  }
  
  std::vector<float> result = {h, s, v};
  
  return result;
}

std::vector<int> getRgb(unsigned char* pixelData, int row, int col, int width, int height){
  int idx = (4 * width * row) + (4 * col);
  std::vector<int> rgb = {(int)pixelData[idx], (int)pixelData[idx+1], (int)pixelData[idx+2]};
  return rgb;
}

float getStdDev(std::vector<float> vValues){
  float sum = 0;
  float total = 0;
  int numVals = (int)vValues.size();
  for(float v : vValues){
    total += v;
  }
  float mean = total / numVals;
  for(float v : vValues){
    sum += std::pow(v - mean, 2);
  }
  return std::sqrt(sum / numVals);
}

void setFilterState(Filter filterToSet, std::map<Filter, bool>& filters){
    for(auto const& f : filters){
        if(f.first != filterToSet){
            filters[f.first] = false;
        }else{
            filters[f.first] = true;
        }
    }
}

void clearFilterState(std::map<Filter, bool>& filters){
    for(auto const& f : filters){
        filters[f.first] = false;
    }
}

void grayscale(unsigned char* pixelData, int pixelDataLen){
    for(int i = 0; i < pixelDataLen - 4; i+=4){
        unsigned char r = pixelData[i];
        unsigned char g = pixelData[i+1];
        unsigned char b = pixelData[i+2];
        unsigned char grey = (r+g+b)/3;
        pixelData[i] = grey;
        pixelData[i+1] = grey;
        pixelData[i+2] = grey;
    }
}

void saturate(unsigned char* pixelData, int pixelDataLen, FilterParameters& params){
    float lumG = params.lumG;
    float lumR = params.lumR;
    float lumB = params.lumB;
    float saturationVal = params.saturationVal;

    float r1 = ((1 - saturationVal) * lumR) + saturationVal;
    float g1 = ((1 - saturationVal) * lumG) + saturationVal;
    float b1 = ((1 - saturationVal) * lumB) + saturationVal;

    float r2 = (1 - saturationVal) * lumR;
    float g2 = (1 - saturationVal) * lumG;
    float b2 = (1 - saturationVal) * lumB;

    for(int i = 0; i <= pixelDataLen-4; i += 4){
        int r = (int)pixelData[i];
        int g = (int)pixelData[i+1];
        int b = (int)pixelData[i+2];
        
        int newR = (int)(r*r1 + g*g2 + b*b2);
        int newG = (int)(r*r2 + g*g1 + b*b2);
        int newB = (int)(r*r2 + g*g2 + b*b1);
        
        // ensure value is within range of 0 and 255
        newR = correctRGB(newR);
        newG = correctRGB(newG);
        newB = correctRGB(newB);
        
        pixelData[i] = (unsigned char)newR;
        pixelData[i+1] = (unsigned char)newG;
        pixelData[i+2] = (unsigned char)newB;
    }
}

// for each pixel, check the above pixel (if it exists)
// if the above pixel is 'significantly' different (i.e. more than +/- 5 of rgb),
// color the above pixel black and the current pixel white. otherwise, both become white. 
void outline(unsigned char* pixelData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params){
    int limit = params.outlineLimit;
    bool setSameColor = false;
            
    for(int i = 0; i < imageHeight; i++){
        for(int j = 0; j < imageWidth; j++){
            // the current pixel is i*width + j
            // the above pixel is (i-1)*width + j
            if(i > 0){
                int aboveIndexR = (i-1)*imageWidth*4 + j*4;
                int aboveIndexG = (i-1)*imageWidth*4 + j*4 + 1;
                int aboveIndexB = (i-1)*imageWidth*4 + j*4 + 2;
                
                int currIndexR = i*imageWidth*4 + j*4;
                int currIndexG = i*imageWidth*4 + j*4 + 1;
                int currIndexB = i*imageWidth*4 + j*4 + 2;
                
                uint8_t aboveR = sourceImageCopy[aboveIndexR];
                uint8_t aboveG = sourceImageCopy[aboveIndexG];
                uint8_t aboveB = sourceImageCopy[aboveIndexB];
            
                uint8_t currR = sourceImageCopy[currIndexR]; 
                uint8_t currG = sourceImageCopy[currIndexG];
                uint8_t currB = sourceImageCopy[currIndexB];
                
                if(aboveR - currR < limit && aboveR - currR > -limit){
                    if(aboveG - currG < limit && aboveG - currG > -limit){
                        if(aboveB - currB < limit && aboveB - currB > -limit){
                            setSameColor = true;
                        }else{
                            setSameColor = false;
                        }
                    }else{
                        setSameColor = false;
                    }
                }else{
                    setSameColor = false;
                }
                
                if(!setSameColor){
                    pixelData[aboveIndexR] = 0;
                    pixelData[aboveIndexG] = 0;
                    pixelData[aboveIndexB] = 0;
                    
                    pixelData[currIndexR] = 255; 
                    pixelData[currIndexG] = 255; 
                    pixelData[currIndexB] = 255; 
                }else{
                    pixelData[aboveIndexR] = 255;
                    pixelData[aboveIndexG] = 255;
                    pixelData[aboveIndexB] = 255;
                    
                    pixelData[currIndexR] = 255; 
                    pixelData[currIndexG] = 255; 
                    pixelData[currIndexB] = 255; 
                }
            }
        }
    }  
}

void invert(unsigned char* pixelData, int pixelDataLen){
    for(int i = 0; i < pixelDataLen - 4; i+=4){
        pixelData[i] = 255 - pixelData[i];
        pixelData[i+1] = 255 - pixelData[i+1];
        pixelData[i+2] = 255 - pixelData[i+2];
    }
}

void channelOffset(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params){
    for(int row = 0; row < imageHeight; row++){
        for(int col = 0; col < imageWidth; col++){
            int randNum = params.chanOffsetRandNum;
            int offset = params.chanOffset;
            if((offset + col) < imageWidth){
                uint8_t newR = sourceImageCopy[4*imageWidth*row + 4*(col+offset)];
                uint8_t newG = sourceImageCopy[4*imageWidth*row + 4*(col+offset) + 1];
                uint8_t newB = sourceImageCopy[4*imageWidth*row + 4*(col+offset) + 2];
                
                if(randNum == 0){
                    imageData[4*imageWidth*row + 4*col] = newR;
                }else if(randNum == 1){
                    imageData[4*imageWidth*row + 4*col + 1] = newG;
                }else{
                    imageData[4*imageWidth*row + 4*col + 2] = newB;
                }
            }
        }
    }
}

void crt(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params){
    // adapted from: https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-nes-mini.glsl
    for(int row = 0; row < imageHeight; row++){
        for(int col = 0; col < imageWidth; col++){
            int selectHigh = (params.scanLineThickness > 0 && row % params.scanLineThickness == 0) ? 1 : 0;
            int selectLow = 1 - selectHigh;
            
            for(int i = 0; i < 3; i++){
                float currChannel = sourceImageCopy[4*imageWidth*row + 4*col + i] / 255.0f;
                float channelHigh = ((1.0f + params.brightboost) - (0.2f * currChannel)) * currChannel;
                float channelLow = ((1.0f - params.intensity) + (0.1f * currChannel)) * currChannel;
                float newColorVal = (selectLow * channelLow) + (selectHigh * channelHigh);
                imageData[4*imageWidth*row + 4*col + i] = (unsigned char)(255.0f * newColorVal);
            }
        }
    }
}

void mosaic(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params){
    int chunkSize = params.chunkSize;
    
    // change sampling size here. lower for higher detail preservation, higher for less detail (because larger chunks)
    int chunkWidth = chunkSize;
    int chunkHeight = chunkSize;
    
    // take care of whole chunks in the mosaic that will be chunkWidth x chunkHeight
    for(int i = 0; i < imageWidth; i += chunkWidth){
        for(int j = 0; j < imageHeight; j += chunkHeight){
            // 4*i + 4*j*width = index of first pixel in chunk
            // get the color of the first pixel in this chunk
            // multiply by 4 because 4 channels per pixel
            // multiply by width because all the image data is in a single array and a row is dependent on width
            uint8_t r = sourceImageCopy[4*i + 4*j*imageWidth];
            uint8_t g = sourceImageCopy[4*i + 4*j*imageWidth + 1];
            uint8_t b = sourceImageCopy[4*i + 4*j*imageWidth + 2];
            
            // based on the chunk dimensions, there might be partial chunks
            // for the last chunk in a row, if there's a partial chunk chunkWidth-wise,
            // include it with this chunk too
            // do the same of any rows that are unable to have a full chunkHeight chunk
            int endWidth = (i+chunkWidth) > imageWidth ? (i+imageWidth%chunkWidth) : (i+chunkWidth);
            int endHeight = (j+chunkHeight) > imageHeight ? (j+imageHeight%chunkHeight) : (j+chunkHeight);
            
            // now for all the other pixels in this chunk, set them to this color
            for(int k = i; k < endWidth; k++){
                for(int l = j; l < endHeight; l++){
                    imageData[4*k + 4*l*imageWidth] = r;
                    imageData[4*k + 4*l*imageWidth + 1] = g;
                    imageData[4*k + 4*l*imageWidth + 2] = b;
                }
            }
        }
    }
}

void voronoi(unsigned char* imageData, int pixelDataLen, int width, int height, FilterParameters& params){
    int neighborConstant = params.voronoiNeighborCount;

    std::vector<CustomPoint> neighborList;

    // get neighbors
    for(int i = 0; i < pixelDataLen - 4; i+=4){        
        std::pair<int, int> pxCoords = getPixelCoords(i, width, height);
        
        if(pxCoords.first == -1) continue;
        
        if(pxCoords.first % (int)std::floor(width / neighborConstant) == 0 && 
           pxCoords.second % (int)std::floor(height / neighborConstant) == 0 && 
           pxCoords.first != 0){
            // add some offset to each neighbor for randomness (we don't really want evenly spaced neighbors)
            int offset = rand() % 10;
            int sign = (rand() % 5 + 1) > 5 ? 1 : -1;  // if random num is > 5, positive sign
               
            // larger neighborConstant == more neighbors == more Voronoi shapes
            int x = (sign * offset) + pxCoords.first;
            int y = (sign * offset) + pxCoords.second;
            CustomPoint p1{x, y, imageData[i], imageData[i+1], imageData[i+2]};
            neighborList.push_back(p1);
        }
    }
    
    // build 2d tree of nearest neighbors 
    Node* kdtree = build2dTree(neighborList, 0);
    
    for(int i = 0; i < pixelDataLen - 4; i+=4){
        std::pair<int, int> currCoords = getPixelCoords(i, width, height);
        
        if(currCoords.first == -1) continue;
        
        CustomPoint nearestNeighbor = findNearestNeighbor(kdtree, currCoords.first, currCoords.second);
        
        // found nearest neighbor. color the current pixel the color of the nearest neighbor. 
        imageData[i] = nearestNeighbor.r;
        imageData[i+1] = nearestNeighbor.g;
        imageData[i+2] = nearestNeighbor.b;
    }
    
    deleteTree(kdtree);
}

void thinning(unsigned char* imageData, int pixelDataLen, int width, int height, FilterParameters& params){
    int numIterations = params.thinningIterations;
    unsigned char* binarized = new unsigned char[pixelDataLen];
    unsigned char* binarizedCopy = new unsigned char[pixelDataLen];
    float threshold = 0.5;
    
    while(numIterations > 0){
        // get a grayscale copy and convert to black/white based on a threshold
        memcpy(binarized, imageData, pixelDataLen);
        
        binarize(binarized, width, height, threshold);
        
        memcpy(binarizedCopy, binarized, pixelDataLen);
        
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                if(
                    isBlackPixel(binarized, i, j, width) &&
                    checkBlackNeighbors(binarized, pixelDataLen, i, j, width) &&
                    testConnectivity(binarized, pixelDataLen, i, j, width) &&
                    verticalLineCheck(binarized, pixelDataLen, i, j, width) &&
                    horizontalLineCheck(binarized, pixelDataLen, i, j, width)
                ){
                    // this pixel should be erased
                    binarizedCopy[(4 * width * i) + (4 * j)] = 255;
                    binarizedCopy[(4 * width * i) + (4 * j) + 1] = 255;
                    binarizedCopy[(4 * width * i) + (4 * j) + 2] = 255;
                }
            }
        }
     
        for(int i = 0; i < pixelDataLen; i++){
            imageData[i] = binarizedCopy[i];
        }
        
        numIterations--;
    }
    
    delete[] binarized;
    delete[] binarizedCopy;
}

// trying something like https://github.com/syncopika/funSketch/blob/master/src/filters/dots.js
// https://discourse.libsdl.org/t/draw-sprites-off-screen/26747/4
// https://discourse.libsdl.org/t/i-have-rendered-using-sdl-renderdrawpoint-my-figure-keeps-redrawing-itself-i-want-it-to-stop-refreshing/33283/6
void dots(unsigned char* pixelData, int pixelDataLen, int imageWidth, int imageHeight, SDL_Renderer* renderer){
    SDL_Texture* target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, imageWidth, imageHeight);
    
    SDL_SetRenderTarget(renderer, target);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    int space = 3;
    
    for(int row = 0; row < imageHeight - space; row += space){
        for(int col = 0; col < imageWidth - space; col += space){
            int r = (int)pixelData[(4 * row * imageWidth) + (4 * col)];
            int g = (int)pixelData[(4 * row * imageWidth) + (4 * col) + 1];
            int b = (int)pixelData[(4 * row * imageWidth) + (4 * col) + 2];
            
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, col, row); // would be nice to be able to specify point width but not sure there's an easy way
            
            // try adding left, right, top and bottom as well for a larger "dot"
            int left = (4 * row * imageWidth) + (4 * (col - 1));
            int right = (4 * row * imageWidth) + (4 * (col + 1));
            int top = (4 * (row - 1) * imageWidth) + (4 * col);
            int bottom = (4 * (row + 1) * imageWidth) + (4 * col);
            
            if(left >= 0 && left < pixelDataLen){
                SDL_RenderDrawPoint(renderer, col-1, row);
            }
            if(right >= 0 && right < pixelDataLen){
                SDL_RenderDrawPoint(renderer, col+1, row);
            }
            if(top >= 0 && top < pixelDataLen){
                SDL_RenderDrawPoint(renderer, col, row-1);
            }
            if(bottom >= 0 && bottom < pixelDataLen){
                SDL_RenderDrawPoint(renderer, col, row+1);
            }
        }
    }
    
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = imageWidth;
    rect.h = imageHeight;
    
    SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_RGBA32, pixelData, 4 * imageWidth); // using SDL_PIXELFORMAT_RGBA8888 changes up the colors :)
    
    SDL_DestroyTexture(target);
    SDL_SetRenderTarget(renderer, nullptr);
}

/*** 
  Kuwahara filter 
  https://github.com/syncopika/funSketch/blob/master/src/filters/kuwahara_painting.js
***/
void kuwahara_helper(unsigned char* imageData, unsigned char* sourceImageCopy, int width, int height, int row, int col, FilterParameters& params){
  int pixelIdx = (4 * width * row) + (4 * col);
  // for this pixel:
  // get 4 quadrants
  // get std dev of each quad of the V value in HSV
  // get quad with smallest std dev
  // the curr pixel gets the avg of the quad
  
  int factor = 3; // TODO: make this a param
  
  // top left quad
  std::vector<float> topLeftV;
  std::vector<float> topLeftRgb = {0, 0, 0};
  for(int i = row - factor; i < row; i++){
    for(int j = col - factor; j < col; j++){
      if(isValidPixel(i, j, width, height)){
        std::vector<int> rgb = getRgb(sourceImageCopy, i, j, width, height);
        std::vector<float> hsv = rgbToHsv(rgb[0], rgb[1], rgb[2]);
        topLeftV.push_back(hsv[2]);
        topLeftRgb[0] += (float)rgb[0];
        topLeftRgb[1] += (float)rgb[1];
        topLeftRgb[2] += (float)rgb[2];
      }
    }
  }
  float topLeftStdDev = getStdDev(topLeftV);
  
  // top right quad
  std::vector<float> topRightV;
  std::vector<float> topRightRgb = {0, 0, 0};
  for(int i = row - factor; i < row; i++){
    for(int j = col; j < (col + factor); j++){
      if(isValidPixel(i, j, width, height)){
        std::vector<int> rgb = getRgb(sourceImageCopy, i, j, width, height);
        std::vector<float> hsv = rgbToHsv(rgb[0], rgb[1], rgb[2]);
        topRightV.push_back(hsv[2]);
        topRightRgb[0] += (float)rgb[0];
        topRightRgb[1] += (float)rgb[1];
        topRightRgb[2] += (float)rgb[2];
      }
    }
  }
  float topRightStdDev = getStdDev(topRightV);
  
  // bottom left quad
  std::vector<float> bottomLeftV;
  std::vector<float> bottomLeftRgb = {0, 0, 0};
  for(int i = row; i < row + factor; i++){
    for(int j = col - factor; j < col; j++){
      if(isValidPixel(i, j, width, height)){
        std::vector<int> rgb = getRgb(sourceImageCopy, i, j, width, height);
        std::vector<float> hsv = rgbToHsv(rgb[0], rgb[1], rgb[2]);
        bottomLeftV.push_back(hsv[2]);
        bottomLeftRgb[0] += (float)rgb[0];
        bottomLeftRgb[1] += (float)rgb[1];
        bottomLeftRgb[2] += (float)rgb[2];
      }
    }
  }
  float bottomLeftStdDev = getStdDev(bottomLeftV);
  
  // bottom right quad
  std::vector<float> bottomRightV;
  std::vector<float> bottomRightRgb = {0, 0, 0};
  for(int i = row; i > 0 && i < height && i < row + factor; i++){
    for(int j = col; j > 0 && j < (col + factor); j++){
      if(isValidPixel(i, j, width, height)){
        std::vector<int> rgb = getRgb(sourceImageCopy, i, j, width, height);
        std::vector<float> hsv = rgbToHsv(rgb[0], rgb[1], rgb[2]);
        bottomRightV.push_back(hsv[2]);
        bottomRightRgb[0] += (float)rgb[0];
        bottomRightRgb[1] += (float)rgb[1];
        bottomRightRgb[2] += (float)rgb[2];
      }
    }
  }
  float bottomRightStdDev = getStdDev(bottomRightV);
  
  // assign avg color of smallest std dev quadrant
  float minStdDev = std::min(topLeftStdDev, std::min(topRightStdDev, std::min(bottomRightStdDev, bottomLeftStdDev)));
  if(minStdDev == topLeftStdDev){
    imageData[pixelIdx] =     (unsigned char)(topLeftRgb[0] / (float)topLeftV.size());
    imageData[pixelIdx + 1] = (unsigned char)(topLeftRgb[1] / (float)topLeftV.size());
    imageData[pixelIdx + 2] = (unsigned char)(topLeftRgb[2] / (float)topLeftV.size());
  }else if(minStdDev == topRightStdDev){
    imageData[pixelIdx] =     (unsigned char)(topRightRgb[0] / (float)topRightV.size());
    imageData[pixelIdx + 1] = (unsigned char)(topRightRgb[1] / (float)topRightV.size());
    imageData[pixelIdx + 2] = (unsigned char)(topRightRgb[2] / (float)topRightV.size());
  }else if(minStdDev == bottomLeftStdDev){
    imageData[pixelIdx] =     (unsigned char)(bottomLeftRgb[0] / (float)bottomLeftV.size());
    imageData[pixelIdx + 1] = (unsigned char)(bottomLeftRgb[1] / (float)bottomLeftV.size());
    imageData[pixelIdx + 2] = (unsigned char)(bottomLeftRgb[2] / (float)bottomLeftV.size());
  }else{
    imageData[pixelIdx] =     (unsigned char)(bottomRightRgb[0] / (float)bottomRightV.size());
    imageData[pixelIdx + 1] = (unsigned char)(bottomRightRgb[1] / (float)bottomRightV.size());
    imageData[pixelIdx + 2] = (unsigned char)(bottomRightRgb[2] / (float)bottomRightV.size());
  }
}

void kuwahara(unsigned char* imageData, unsigned char* sourceImageCopy, int imageWidth, int imageHeight, FilterParameters& params){
  for(int i = 0; i < imageHeight; i++){
    for(int j = 0; j < imageWidth; j++){
      kuwahara_helper(imageData, sourceImageCopy, imageWidth, imageHeight, i, j, params);
    }
  }
}

/*** 
  Gaussian blur filter 
  https://github.com/syncopika/funSketch/blob/master/src/filters/blur.js
  
  TODO: this filter is still buggy and I get weird color artifacts
  could it be a numerical precision issue?
***/
std::vector<int> generateGaussBoxes(float stdDev, int numBoxes){
  float wIdeal = std::sqrt((12 * stdDev * stdDev / numBoxes) + 1); // ideal averaging filter width
  int wl = std::floor(wIdeal);
  
  if(wl % 2 == 0){
    wl--;
  }
      
  int wu = wl + 2;
      
  float mIdeal = (12 * stdDev * stdDev - numBoxes * wl * wl - 4 * numBoxes * wl - 3 * numBoxes) / (-4 * wl - 4);
  int m = std::round(mIdeal);
      
  std::vector<int> sizes;
      
  for(int i = 0; i < numBoxes; i++){
    sizes.push_back(i < m ? wl : wu);
  }
      
  return sizes;
}

void boxBlurHorz(std::vector<int>& src, std::vector<int>& trgt, int width, int height, float stdDev){
  float iarr = 1.0 / (stdDev + stdDev + 1.0);
  for(int i = 0; i < height; i++){
    int ti = i * width;
    int li = ti;
    int ri = ti + stdDev;
          
    int fv = src[ti];
    int lv = src[ti + width - 1];
    float val = (stdDev + 1) * fv;
    
    for(int j = 0; j < stdDev; j++){
      val += src[ti + j];
    }
          
    for(int j = 0; j <= stdDev; j++){
      val += src[ri++] - fv;
      trgt[ti++] = std::round(val * iarr);
    }
          
    for(int j = stdDev + 1; j < width - stdDev; j++){
      val += src[ri++] - src[li++];
      trgt[ti++] = std::round(val * iarr);
    }
          
    for(int j = width - stdDev; j < width; j++){
      val += lv - src[li++];
      trgt[ti++] = std::round(val * iarr);
    }
  }
}

void boxBlurTotal(std::vector<int>& src, std::vector<int>& trgt, int width, int height, float stdDev){
  float iarr = 1.0 / (stdDev + stdDev + 1.0);
  for(int i = 0; i < width; i++){
    int ti = i;
    int li = ti;
    int ri = ti + stdDev * width;
          
    int fv = src[ti];
    int lv = src[ti + width * (height - 1)];
    float val = (stdDev + 1) * fv;
          
    for(int j = 0; j < stdDev; j++){
      val += src[ti + j * width];
    }
          
    for(int j = 0; j <= stdDev; j++){
      val += src[ri] - fv;
      trgt[ti] = std::round(val * iarr);
      ri += width;
      ti += width;
    }
          
    for(int j = stdDev + 1; j < height - stdDev; j++){
      val += src[ri] - src[li];
      trgt[ti] = std::round(val * iarr);
      li += width;
      ri += width;
      ti += width;
    }
          
    for(int j = height - stdDev; j < height; j++){
      val += lv - src[li];
      trgt[ti] = std::round(val * iarr);
      li += width;
      ti += width;
    }
  }
}

void boxBlur(std::vector<int>& src, std::vector<int>& trgt, int width, int height, float stdDev){
  for(int i = 0; i < (int)src.size(); i++){
    trgt[i] = src[i];
  }
  boxBlurHorz(trgt, src, width, height, stdDev);
  boxBlurTotal(src, trgt, width, height, stdDev);
}

void gaussBlur(std::vector<int>& src, std::vector<int>& trgt, int width, int height, float stdDev){
  std::vector<int> boxes = generateGaussBoxes(stdDev, 3);
  boxBlur(src, trgt, width, height, (boxes[0] - 1.0) / 2.0);
  boxBlur(trgt, src, width, height, (boxes[1] - 1.0) / 2.0);
  boxBlur(src, trgt, width, height, (boxes[2] - 1.0) / 2.0);
}

void blur(unsigned char* imageData, int imageWidth, int imageHeight, FilterParameters& params){
  int dataLength = 4 * imageWidth * imageHeight;
  int numPixels = imageWidth * imageHeight;
  
  std::vector<int> redChannel(numPixels);
  std::vector<int> greenChannel(numPixels);
  std::vector<int> blueChannel(numPixels);
  
  for(int i = 0; i <= dataLength - 4; i += 4){
    redChannel[i/4] = (int)imageData[i];
    greenChannel[i/4] = (int)imageData[i + 1];
    blueChannel[i/4] = (int)imageData[i + 2];
  }
  
  float blurFactor = 3; // TODO: make this a param (also maybe don't make it a float)
  
  gaussBlur(redChannel, redChannel, imageWidth, imageHeight, blurFactor);
  gaussBlur(greenChannel, greenChannel, imageWidth, imageHeight, blurFactor);
  gaussBlur(blueChannel, blueChannel, imageWidth, imageHeight, blurFactor);
  
  for(int i = 0; i <= dataLength - 4; i += 4){
    imageData[i] = (unsigned char)redChannel[i/4];
    imageData[i + 1] = (unsigned char)greenChannel[i/4];
    imageData[i + 2] = (unsigned char)blueChannel[i/4];
  }
}

/*** 
  edge detection filter 
  https://github.com/syncopika/funSketch/blob/master/src/filters/edgedetection.js
***/
void edgeDetection(unsigned char* imageData, unsigned char* sourceImageCopy, int width, int height){
  int xKernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  int yKernel[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
      
  for(int i = 1; i < height - 1; i++){
    for(int j = 4; j < 4 * width - 4; j += 4){
      int left = (4 * i * width) + (j - 4);
      int right = (4 * i * width) + (j + 4);
      int top = (4 * (i - 1) * width) + j;
      int bottom = (4 * (i + 1) * width) + j;
      int topLeft = (4 * (i - 1) * width) + (j - 4);
      int topRight = (4 * (i - 1) * width) + (j + 4);
      int bottomLeft = (4 * (i + 1) * width) + (j - 4);
      int bottomRight = (4 * (i + 1) * width) + (j + 4);
      int center = (4 * width * i) + j;
              
      // use the xKernel to detect edges horizontally 
      int pX = (xKernel[0][0] * sourceImageCopy[topLeft]) + (xKernel[0][1] * sourceImageCopy[top]) + (xKernel[0][2] * sourceImageCopy[topRight]) +
                  (xKernel[1][0] * sourceImageCopy[left]) + (xKernel[1][1] * sourceImageCopy[center]) + (xKernel[1][2] * sourceImageCopy[right]) +
                  (xKernel[2][0] * sourceImageCopy[bottomLeft]) + (xKernel[2][1] * sourceImageCopy[bottom]) + (xKernel[2][2] * sourceImageCopy[bottomRight]);
                  
      // use the yKernel to detect edges vertically 
      int pY = (yKernel[0][0] * sourceImageCopy[topLeft]) + (yKernel[0][1] * sourceImageCopy[top]) + (yKernel[0][2] * sourceImageCopy[topRight]) +
                  (yKernel[1][0] * sourceImageCopy[left]) + (yKernel[1][1] * sourceImageCopy[center]) + (yKernel[1][2] * sourceImageCopy[right]) +
                  (yKernel[2][0] * sourceImageCopy[bottomLeft]) + (yKernel[2][1] * sourceImageCopy[bottom]) + (yKernel[2][2] * sourceImageCopy[bottomRight]);
              
      // finally set the current pixel to the new value based on the formula 
      int newVal = (std::ceil(std::sqrt((pX * pX) + (pY * pY))));
      imageData[center] = newVal;
      imageData[center + 1] = newVal;
      imageData[center + 2] = newVal;
    }
  }
}


