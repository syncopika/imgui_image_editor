#include "thinning_helper.hh"
#include <string>

void binarize(unsigned char* data, int width, int height, float threshold){
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            int r = (int)data[(i * width * 4) + (j * 4)];
            int g = (int)data[(i * width * 4) + (j * 4) + 1];
            int b = (int)data[(i * width * 4) + (j * 4) + 2];
            float avg = (r + g + b) / 3.0;
            if((avg / 255.0) >= threshold){
                // if resulting grayscale color of this pixel is >= threshold,
                // set it to white (255)
                data[(i * width * 4) + (j * 4)]     = 255;
                data[(i * width * 4) + (j * 4) + 1] = 255;
                data[(i * width * 4) + (j * 4) + 2] = 255;
            }else{
                // make pixel black (0)
                data[(i * width * 4) + (j * 4)]     = 0;
                data[(i * width * 4) + (j * 4) + 1] = 0;
                data[(i * width * 4) + (j * 4) + 2] = 0;
            }
        }
    }
}

bool checkBlackNeighbors(unsigned char* data, int dataLength, int i, int j, int width){
    int left = (4 * i * width) + ((j - 1) * 4);
    int right = (4 * i * width) + ((j + 1) * 4);
    int top = (4 * (i - 1) * width) + (j * 4);
    int bottom = (4 * (i + 1) * width) + (j * 4);
    int topLeft = (4 * (i - 1) * width) + ((j - 1) * 4);
    int topRight = (4 * (i - 1) * width) + ((j + 1) * 4);
    int bottomLeft = (4 * (i + 1) * width) + ((j - 1) * 4);
    int bottomRight = (4 * (i + 1) * width) + ((j + 1) * 4);
    
    // assume rgb channel values should be the same
    // so we don't need to look at g and b
    int numBlackNeighbors = 0;
    if(left >= 0){
        if((int)data[left] == 0){
            numBlackNeighbors++;
        }
    }
    if(right < dataLength){
        if((int)data[right] == 0){
            numBlackNeighbors++;
        }
    }
    if(top >= 0){
        if((int)data[top] == 0){
            numBlackNeighbors++;
        }
    }
    if(bottom < dataLength){
        if((int)data[bottom] == 0){
            numBlackNeighbors++;
        }
    }
    if(topLeft >= 0){
        if((int)data[topLeft] == 0){
            numBlackNeighbors++;
        }
    }
    if(topRight >= 0){
        if((int)data[topRight] == 0){
            numBlackNeighbors++;
        }
    }
    if(bottomLeft < dataLength){
        if((int)data[bottomLeft] == 0){
            numBlackNeighbors++;
        }
    }
    if(bottomRight < dataLength){
        if((int)data[bottomRight] == 0){
            numBlackNeighbors++;
        }
    }
    
    return numBlackNeighbors >= 2 && numBlackNeighbors <= 6;
}

bool testConnectivity(unsigned char* data, int dataLength, int i, int j, int width){
    
    if(i < 0 || j < 0){
        return false;
    }
    
    int left = (4 * i * width) + ((j - 1) * 4);
    int right = (4 * i * width) + ((j + 1) * 4);
    int top = (4 * (i - 1) * width) + (j * 4);
    int bottom = (4 * (i + 1) * width) + (j * 4);
    int topLeft = (4 * (i - 1) * width) + ((j - 1) * 4);
    int topRight = (4 * (i - 1) * width) + ((j + 1) * 4);
    int bottomLeft = (4 * (i + 1) * width) + ((j - 1) * 4);
    int bottomRight = (4 * (i + 1) * width) + ((j + 1) * 4);
    
    int numConnectedNeighbors = 0;
    std::string sequence = "";
    
    int currPixelIdx = (4 * i * width) + (j * 4);
    
    // don't operate on non-black pixels
    if((int)data[currPixelIdx] != 0){
        return false;
    }
    
    // note there is a specific sequence in which to evaluate the cells
    // e.g. top -> top-right -> right -> bottom-right -> ... -> top in clockwise-fashion
    if(top >= 0){
        if((int)data[top] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(topRight >= 0){
        if((int)data[topRight] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(right < dataLength){
        if((int)data[right] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(bottomRight < dataLength){
        if((int)data[bottomRight] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(bottom < dataLength){
        if((int)data[bottom] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(bottomLeft < dataLength){
        if((int)data[bottomLeft] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(left >= 0){
        if((int)data[left] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(topLeft >= 0){
        if((int)data[topLeft] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    if(top >= 0){
        if((int)data[top] == 255){
            sequence += "0";
        }else{
            sequence += "1";
        }
    }
    
    // evaluate sequence generated
    // we're looking for susbtrings with "01"
    for(int i = 0; i < sequence.length() - 1; i++){
        if(sequence[i] == '0' && sequence[i+1] == '1'){
            numConnectedNeighbors++;
        }
    }
    
    return numConnectedNeighbors == 1;
}

bool verticalLineCheck(unsigned char* data, int dataLength, int i, int j, int width){
    int left = (4 * i * width) + ((j - 1) * 4);
    int right = (4 * i * width) + ((j + 1) * 4);
    int top = (4 * (i - 1) * width) + (j * 4);
    
    int numConnectedNeighbors = 0;
    if(left >= 0){
        if((int)data[left] != 0){
            numConnectedNeighbors++;
        }
    }
    if(right < dataLength){
        if((int)data[right] != 0){
            numConnectedNeighbors++;
        }
    }
    if(top >= 0){
        if((int)data[top] != 0){
            numConnectedNeighbors++;
        }
    }
    
    return numConnectedNeighbors == 3 || !testConnectivity(data, dataLength, i-1, j, width);
}

bool horizontalLineCheck(unsigned char* data, int dataLength, int i, int j, int width){
    int bottom = (4 * (i + 1) * width) + (j * 4);
    int right = (4 * i * width) + ((j + 1) * 4);
    int top = (4 * (i - 1) * width) + (j * 4);
    
    int numConnectedNeighbors = 0;
    if(bottom < dataLength){
        if((int)data[bottom] != 0){
            numConnectedNeighbors++;
        }
    }
    if(right < dataLength){
        if((int)data[right] != 0){
            numConnectedNeighbors++;
        }
    }
    if(top >= 0){
        if((int)data[top] != 0){
            numConnectedNeighbors++;
        }
    }
    
    return numConnectedNeighbors == 3 || !testConnectivity(data, dataLength, i, j+1, width);
}

bool hasEightNeighbors(unsigned char* data, int dataLength, int i, int j, int width){
    int left = (4 * i * width) + ((j - 1) * 4);
    int right = (4 * i * width) + ((j + 1) * 4);
    int top = (4 * (i - 1) * width) + (j * 4);
    int bottom = (4 * (i + 1) * width) + (j * 4);
    int topLeft = (4 * (i - 1) * width) + ((j - 1) * 4);
    int topRight = (4 * (i - 1) * width) + ((j + 1) * 4);
    int bottomLeft = (4 * (i + 1) * width) + ((j - 1) * 4);
    int bottomRight = (4 * (i + 1) * width) + ((j + 1) * 4);
    
    return (
        (left >= 0 && left < dataLength) &&
        (right >= 0 && right < dataLength) &&
        (top >= 0 && top < dataLength) &&
        (bottom >= 0 && bottom < dataLength) &&
        (topLeft >= 0 && topLeft < dataLength) &&
        (topRight >= 0 && topRight < dataLength) &&
        (bottomLeft >= 0 && bottomLeft < dataLength) &&
        (bottomRight >= 0 && bottomRight < dataLength)
    );
}

bool isBlackPixel(unsigned char* data, int i, int j, int width){
    int idx = (4 * i * width) + (j * 4);
    return data[idx] == 0;
}
