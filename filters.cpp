#include "filters.hh"

int correctRGB(int channel){
    if(channel > 255){
        return 255;
    }
    if(channel < 0){
        return 0;
    }
    return channel;
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
            int selectHigh = row % params.scanLineThickness == 0 ? 1 : 0;
            int selectLow = 1 - selectHigh;
            
            for(int i = 0; i < 3; i++){
                float currChannel = sourceImageCopy[4*imageWidth*row + 4*col + i] / 255.0f;
                float channelHigh = ((1.0 + params.brightboost) - (0.2 * currChannel)) * currChannel;
                float channelLow = ((1.0 - params.intensity) + (0.1 * currChannel)) * currChannel;
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
            uint8_t r = sourceImageCopy[4*i+4*j*imageWidth];
            uint8_t g = sourceImageCopy[4*i+4*j*imageWidth+1];
            uint8_t b = sourceImageCopy[4*i+4*j*imageWidth+2];
            
            // based on the chunk dimensions, there might be partial chunks
            // for the last chunk in a row, if there's a partial chunk chunkWidth-wise,
            // include it with this chunk too
            // do the same of any rows that are unable to have a full chunkHeight chunk
            int endWidth = (i+chunkWidth) > imageWidth ? (i+imageWidth%chunkWidth) : (i+chunkWidth);
            int endHeight = (j+chunkHeight) > imageHeight ? (j+imageHeight%chunkHeight) : (j+chunkHeight);
            
            // now for all the other pixels in this chunk, set them to this color
            for(int k = i; k < endWidth; k++){
                for(int l = j; l < endHeight; l++){
                    imageData[4*k+4*l*imageWidth] = r;
                    imageData[4*k+4*l*imageWidth+1] = g;
                    imageData[4*k+4*l*imageWidth+2] = b;
                }
            }
        }
    }
}


