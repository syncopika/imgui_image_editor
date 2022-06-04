#include "utils.hh"

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#if WINDOWS_BUILD
#include <windows.h>
#include <commctrl.h>
#endif

#include "stb_image.h"
#include "stb_image_write.h"

#define FILEPATH_MAX_LENGTH 260

#define TEMP_IMAGE GL_TEXTURE1
#define IMAGE_DISPLAY GL_TEXTURE2
#define ORIGINAL_IMAGE GL_TEXTURE3

std::string trimString(std::string& str){
    std::string trimmed("");
    std::string::iterator it;
    for(it = str.begin(); it < str.end(); it++){
        if(*it != ' '){
            trimmed += *it;
        }
    }
    return trimmed;
}

std::string colorText(int r, int g, int b){
    std::ostringstream output;
    output << "rgb(" << r << "," << g << "," << b << ")";
    return output.str();
}


// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height, int* channels){
    int imageWidth = 0;
    int imageHeight = 0;
    int imageChannels = 4;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* imageData = stbi_load(filename, &imageWidth, &imageHeight, &imageChannels, 4);
    if(imageData == NULL){
        return false;
    }
    
    // the current image texture
    glActiveTexture(IMAGE_DISPLAY);
    GLuint imageTexture;
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    
    // TODO: understand this stuff
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    #if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    #endif
    
    // create the texture with the image data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    // store the image in another texture that we won't touch (but just read from)
    glActiveTexture(ORIGINAL_IMAGE);
    GLuint imageTexture2;
    glGenTextures(1, &imageTexture2);
    glBindTexture(GL_TEXTURE_2D, imageTexture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    // have a texture to act as an intermediary so that successive changes (e.g. when changing parameter values for a filter)
    // won't completely clobber the image
    // we'll do changes on this texture for any filters that have parameters
    glActiveTexture(TEMP_IMAGE);
    GLuint imageTexture3;
    glGenTextures(1, &imageTexture3);
    glBindTexture(GL_TEXTURE_2D, imageTexture3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    stbi_image_free(imageData);
    
    *tex = imageTexture;
    *originalImage = imageTexture2;
    *width = imageWidth;
    *height = imageHeight;
    *channels = imageChannels;
    
    return true;
}

void resizeSDLWindow(SDL_Window* window, int width, int height){
    int widthbuffer = 200;
    int heightbuffer = 200;
    SDL_SetWindowSize(window, width + widthbuffer, height + heightbuffer);
}

void rotateImage(int& imageWidth, int& imageHeight){
    int pixelDataLen = imageWidth*imageHeight*4;
    unsigned char* pixelData = new unsigned char[pixelDataLen];
    unsigned char* pixelDataCopy = new unsigned char[pixelDataLen];
    
    glActiveTexture(IMAGE_DISPLAY);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelDataCopy);
    
    // rotate the pixel data and write it back
    // https://stackoverflow.com/questions/16684856/rotating-a-2d-pixel-array-by-90-degrees
    int cols = imageWidth*4;
    int rows = imageHeight;
    
    int counter = 0;
    for(int i = 0; i < cols; i += 4){
        for(int j = rows-1; j >= 0; j--){
            // make sure to 'move' the whole pixel, which means 4 channels
            pixelData[counter++] = pixelDataCopy[(j*4*imageWidth)+i]; //r
            pixelData[counter++] = pixelDataCopy[(j*4*imageWidth)+i+1]; //g
            pixelData[counter++] = pixelDataCopy[(j*4*imageWidth)+i+2]; //b
            pixelData[counter++] = pixelDataCopy[(j*4*imageWidth)+i+3]; //a
        }
    }
    // swap image height and width
    int temp = imageWidth;
    imageWidth = imageHeight;
    imageHeight = temp;
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glActiveTexture(TEMP_IMAGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    delete[] pixelData;
    delete[] pixelDataCopy;
}

void updateTempImageState(int imageWidth, int imageHeight){
    // take current image data in IMAGE_DISPLAY and update TEMP_IMAGE with that data
    int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
    unsigned char* pixelData = new unsigned char[pixelDataLen];
        
    glActiveTexture(IMAGE_DISPLAY);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
    glActiveTexture(TEMP_IMAGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    delete[] pixelData;
}

void resetImageState(int& imageWidth, int& imageHeight, int originalWidth, int originalHeight){
    imageWidth = originalWidth;
    imageHeight = originalHeight;
    int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
    unsigned char* pixelData = new unsigned char[pixelDataLen];
        
    glActiveTexture(ORIGINAL_IMAGE);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    // update temp image and display image
    glActiveTexture(IMAGE_DISPLAY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);    
    glActiveTexture(TEMP_IMAGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    delete[] pixelData;
}

void setFilter(Filter filter, std::map<Filter, bool>& filtersWithParams, int imageWidth, int imageHeight){
    setFilterState(filter, filtersWithParams);
    updateTempImageState(imageWidth, imageHeight);
}

void doFilter(int imageWidth, int imageHeight, Filter filter, FilterParameters& filterParams){
    // allocate memory for the image 
    int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
    unsigned char* pixelData = new unsigned char[pixelDataLen];
            
    // do the thing
    switch(filter){
        case Filter::Grayscale:
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            grayscale(pixelData, pixelDataLen);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            break;
        case Filter::Invert:
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            invert(pixelData, pixelDataLen);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            break;
        case Filter::Saturation:
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            saturate(pixelData, pixelDataLen, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            break;
        case Filter::Outline: {
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            outline(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            delete[] sourceImageCopy;            
            break;
        }
        case Filter::Mosaic: {
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            mosaic(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            delete[] sourceImageCopy;
            break;
        }
        case Filter::ChannelOffset: {
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            channelOffset(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            delete[] sourceImageCopy;
            break;
        }
        case Filter::Crt: {
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            crt(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            delete[] sourceImageCopy;            
            break;
        }
        case Filter::Voronoi: {
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            voronoi(pixelData, pixelDataLen, imageWidth, imageHeight, filterParams);
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            break;
        }
        default:
            break;
    }
    
    // reclaim memory
    delete[] pixelData;
}

std::vector<int> extractPixelColor(int xCoord, int yCoord, int imageWidth, int imageHeight){
    int pixelDataLen = imageWidth*imageHeight*4;
    unsigned char* imageData = new unsigned char[pixelDataLen];
    
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    int r = (int)imageData[xCoord*4 + yCoord*imageWidth*4];
    int g = (int)imageData[xCoord*4 + yCoord*imageWidth*4 + 1];
    int b = (int)imageData[xCoord*4 + yCoord*imageWidth*4 + 2];
    
    delete[] imageData;
    
    std::vector<int> color{r, g, b, 255};
    return color;
}

void swapColors(ImVec4& colorToChange, ImVec4& colorToChangeTo, int imageWidth, int imageHeight){
    int pixelDataLen = imageWidth*imageHeight*4;
    unsigned char* imageData = new unsigned char[pixelDataLen];
    
    glActiveTexture(IMAGE_DISPLAY);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    for(int row = 0; row < imageHeight; row++){
        for(int col = 0; col < imageWidth; col++){
            float r = imageData[col*4 + row*imageWidth*4] / 255.0;
            float g = imageData[col*4 + row*imageWidth*4 + 1] / 255.0;
            float b = imageData[col*4 + row*imageWidth*4 + 2] / 255.0;
            
            if(r == colorToChange.x && g == colorToChange.y && b == colorToChange.z){
                imageData[col*4 + row*imageWidth*4] = (unsigned char)(colorToChangeTo.x * 255);
                imageData[col*4 + row*imageWidth*4 + 1] = (unsigned char)(colorToChangeTo.y * 255);
                imageData[col*4 + row*imageWidth*4 + 2] = (unsigned char)(colorToChangeTo.z * 255);
            }
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    
    delete[] imageData;
}


void showImageEditor(SDL_Window* window){
    static FilterParameters filterParams;
    static GLuint texture;
    static GLuint originalImage;
    static bool showImage = false;
    static int imageHeight = 0;
    static int imageWidth = 0;
    static int originalImageHeight = 0;
    static int originalImageWidth = 0;
    static int imageChannels = 4; //rgba
    static char importImageFilepath[FILEPATH_MAX_LENGTH] = "test_image.png";
    static char exportImageName[FILEPATH_MAX_LENGTH] = "";
    static std::vector<int> selectedPixelColor{0, 0, 0, 255};
    
    // for filters that have customizable parameters,
    // have a bool flag so we can toggle the params for a specific filter
    static std::map<Filter, bool> filtersWithParams{
        {Filter::Saturation, false},
        {Filter::Outline, false},
        {Filter::Mosaic, false},
        {Filter::ChannelOffset, false},
        {Filter::Crt, false},
        {Filter::Voronoi, false}
    };
    
    bool importImageClicked = ImGui::Button("import image");
    ImGui::SameLine();
    ImGui::InputText("filepath", importImageFilepath, FILEPATH_MAX_LENGTH);
    
    if(importImageClicked){
        // open file dialog to allow user to find and select an image if windows.h is available
        #if WINDOWS_BUILD
        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = importImageFilepath;
        ofn.nMaxFile = sizeof(importImageFilepath);
        ofn.lpstrFilter = "Image Files\0*.bmp;*.png;*.jpg;*.jpeg\0\0";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        GetOpenFileName(&ofn);
        #endif
        
        // set the texture (the imported image) to be used for the filters
        // until a new image is imported, the current one will be used
        std::string filepath(importImageFilepath);
        
        if(trimString(filepath) != ""){
            bool loaded = importImage(
                filepath.c_str(), 
                &texture, 
                &originalImage, 
                &imageWidth, 
                &imageHeight, 
                &imageChannels
            );
            
            if(loaded){
                showImage = true;
                resizeSDLWindow(window, imageWidth, imageHeight);
                originalImageWidth = imageWidth;
                originalImageHeight = imageHeight;
            }else{
                ImGui::Text("import image failed");
                showImage = false;
            }
        }
        
        filterParams.generateRandNum3();
    }
    
    if(showImage){
        if(ImGui::Button("rotate image")){
            rotateImage(imageWidth, imageHeight);
        }
        
        ImGui::Text("size = %d x %d", imageWidth, imageHeight);
        
        // https://github.com/ocornut/imgui/issues/3404 - mouse interaction
        const ImVec2 origin = ImGui::GetCursorScreenPos(); // Lock scrolled origin
        
        // show the image
        ImGui::Image((void *)(intptr_t)texture, ImVec2(imageWidth, imageHeight));
        
        // handle clicking on the image
        ImGuiIO& io = ImGui::GetIO();
        
        // Hovered - this is so we ensure that we take into account only mouse interactions that occur
        // on this particular canvas. otherwise it could pick up mouse clicks that occur on other windows as well.
        const bool isHovered = ImGui::IsItemHovered();        
        const ImVec2 mousePosInImage(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

        if(isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            //ImGui::Text("%d, %d", (int)mousePosInImage.x, (int)mousePosInImage.y);
            selectedPixelColor = extractPixelColor((int)mousePosInImage.x, (int)mousePosInImage.y, imageWidth, imageHeight);
        }
        
        int r = selectedPixelColor[0];
        int g = selectedPixelColor[1];
        int b = selectedPixelColor[2];
        
        // show selected pixel color
        // something to try? https://github.com/ocornut/imgui/issues/1566
        // change text color: https://stackoverflow.com/questions/61853584/how-can-i-change-text-color-of-my-inputtext-in-imgui
        ImVec2 canvasPos0 = ImVec2(0, origin.y+imageHeight+5); 
        ImVec2 canvasPos1 = ImVec2(250, origin.y+imageHeight+20);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(canvasPos0, canvasPos1, IM_COL32(r, g, b, 255));
        
        if(r > 190 || g > 190 || b > 190){
            // for light colors, set text color to black
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        }
        
        ImGui::Text(colorText(r, g, b).c_str());
        
        if(r > 190 || g > 190 || b > 190){
            ImGui::PopStyleColor();
        }
        
        // spacer
        ImGui::Dummy(ImVec2(0.0f, 3.0f));
        
        // be able to swap colors
        // colorpicker help - https://github.com/ocornut/imgui/issues/3583
        static ImVec4 colorToChange;
        static ImVec4 colorToChangeTo;
        
        ImGui::ColorEdit4(":color to change", (float*)&colorToChange, ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        ImGui::ColorEdit4(":color to change to", (float*)&colorToChangeTo, ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        if(ImGui::Button("swap colors")){
            swapColors(colorToChange, colorToChangeTo, imageWidth, imageHeight);
        }
        
        // spacer
        ImGui::Dummy(ImVec2(0.0f, 3.0f));
        
        // show filter options
        // GRAYSCALE
        if(ImGui::Button("grayscale")){
            doFilter(imageWidth, imageHeight, Filter::Grayscale, filterParams);
        }
        ImGui::SameLine();
        
        // INVERT
        if(ImGui::Button("invert")){
            doFilter(imageWidth, imageHeight, Filter::Invert, filterParams);
        }
        ImGui::SameLine();
        
        // SATURATION
        if(ImGui::Button("saturate")){
            setFilter(Filter::Saturation, filtersWithParams, imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // OUTLINE
        if(ImGui::Button("outline")){
            setFilter(Filter::Outline, filtersWithParams, imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // CHANNEL OFFSET
        if(ImGui::Button("channel offset")){
            setFilter(Filter::ChannelOffset, filtersWithParams, imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // MOSAIC
        if(ImGui::Button("mosaic")){
            setFilter(Filter::Mosaic, filtersWithParams, imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // CRT
        if(ImGui::Button("crt")){
            setFilter(Filter::Crt, filtersWithParams, imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // Voronoi
        if(ImGui::Button("voronoi")){
            setFilter(Filter::Voronoi, filtersWithParams, imageWidth, imageHeight);
        }
        
        // spacer
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        
        // RESET IMAGE
        if(ImGui::Button("reset image")){
            resetImageState(imageWidth, imageHeight, originalImageWidth, originalImageHeight);
            filterParams.generateRandNum3();
        }
        
        if(filtersWithParams[Filter::Saturation]){
            ImGui::Text("saturation filter parameters");
            
            // d = delta
            bool d1 = ImGui::SliderFloat("saturation val", &filterParams.saturationVal, 0.0f, 5.0f);
            bool d2 = ImGui::SliderFloat("lumR", &filterParams.lumR, 0.0f, 5.0f);
            bool d3 = ImGui::SliderFloat("lumG", &filterParams.lumG, 0.0f, 5.0f);
            bool d4 = ImGui::SliderFloat("lumB", &filterParams.lumB, 0.0f, 5.0f);
            
            // if any of the saturation parameters change, re-run the filter
            if(d1 || d2 || d3 || d4){
                doFilter(imageWidth, imageHeight, Filter::Saturation, filterParams);
            }
        }
        
        if(filtersWithParams[Filter::Outline]){
            ImGui::Text("outline filter parameters");
            if(ImGui::SliderInt("color difference limit", &filterParams.outlineLimit, 1, 20)){
                doFilter(imageWidth, imageHeight, Filter::Outline, filterParams);
            }
        }
        
        if(filtersWithParams[Filter::Mosaic]){
            ImGui::Text("mosaic filter parameters");
            if(ImGui::SliderInt("mosaic chunk size", &filterParams.chunkSize, 1, 20)){
                doFilter(imageWidth, imageHeight, Filter::Mosaic, filterParams);
            }
        }
        
        if(filtersWithParams[Filter::ChannelOffset]){
            ImGui::Text("channel offset parameters");
            if(ImGui::SliderInt("chan offset", &filterParams.chanOffset, 1, 15)){ // TODO: find out why using "channel offset" for the label produces an assertion error :0
                doFilter(imageWidth, imageHeight, Filter::ChannelOffset, filterParams);
            }
        }
        
        if(filtersWithParams[Filter::Crt]){
            ImGui::Text("CRT (cathode-ray tube) filter parameters");
            
            bool d1 = ImGui::SliderInt("scanline thickness", &filterParams.scanLineThickness, 0, 10);
            bool d2 = ImGui::SliderFloat("brightboost", &filterParams.brightboost, 0.0f, 1.0f);
            bool d3 = ImGui::SliderFloat("intensity", &filterParams.intensity, 0.0f, 1.0f);

            if(d1 || d2 || d3){
                doFilter(imageWidth, imageHeight, Filter::Crt, filterParams);
            }
        }
        
        if(filtersWithParams[Filter::Voronoi]){
            ImGui::Text("voronoi filter parameters");
            if(ImGui::SliderInt("neighbor count", &filterParams.voronoiNeighborCount, 10, 60)){
                doFilter(imageWidth, imageHeight, Filter::Voronoi, filterParams);
            }
        }
        
        // spacer
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        
        // EXPORT IMAGE
        bool exportImageClicked = ImGui::Button("export image (.bmp)");
        ImGui::SameLine();
        ImGui::PushItemWidth(150);
        ImGui::InputText("image name", exportImageName, 64); // TODO: is this long enough?
        
        std::string exportName(exportImageName);
        if(exportImageClicked){
            glActiveTexture(IMAGE_DISPLAY);
            
            int pixelDataLen = imageWidth*imageHeight*4;
            unsigned char* pixelData = new unsigned char[pixelDataLen];
        
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            if(exportName == ""){
                exportName = std::string(importImageFilepath) + "-edit"; // this will include the original image's extension?
            }
            exportName += ".bmp";
            
            stbi_write_bmp(exportName.c_str(), imageWidth, imageHeight, 4, (void *)pixelData);
            
            delete[] pixelData;
            
            ImGui::OpenPopup("message"); // show popup
        }
        
        // signal that the image export happened in popup
        if(ImGui::BeginPopupModal("message")){
            ImGui::Text((std::string("exported image: ") + exportName).c_str());
            if(ImGui::Button("close")){
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    //ImGui::EndChild();
}