#include "filters.hh"
#include "utils.hh"

#include "stb_image.h"
#include "stb_image_write.h"

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

void updateTempImageState(int imageWidth, int imageHeight){
    // take current image data in IMAGE_DISPLAY and
    // update TEMP_IMAGE with that data
    int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
    unsigned char* pixelData = new unsigned char[pixelDataLen];
        
    glActiveTexture(IMAGE_DISPLAY);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
    glActiveTexture(TEMP_IMAGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    delete[] pixelData;
}

void resetImageState(int imageWidth, int imageHeight){
    int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
    unsigned char* pixelData = new unsigned char[pixelDataLen];
        
    glActiveTexture(ORIGINAL_IMAGE);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    // update temp image and display image
    glActiveTexture(TEMP_IMAGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glActiveTexture(IMAGE_DISPLAY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    delete[] pixelData;
}

void showImageEditor(){
    //ImGui::BeginChild("image editor", ImVec2(0, 800), true);
    static FilterParameters filterParams;
    static bool showImage = false;
    static GLuint texture;
    static GLuint originalImage;
    static int imageHeight = 0;
    static int imageWidth = 0;
    static int imageChannels = 4; //rgba
    static char importImageFilepath[64] = "test_image.png";
    
    // for filters that have customizable parameters,
    // have a bool flag so we can toggle the params
    static bool showSaturateParams = false;
    static bool showOutlineParams = false;
    static bool showMosaicParams = false;
    static bool showChannelOffsetParams = false;
    static bool showCrtParams = false;
    
    bool importImageClicked = ImGui::Button("import image");
    ImGui::SameLine();
    ImGui::InputText("filepath", importImageFilepath, 64);
    
    if(importImageClicked){
        // set the texture (the imported image) to be used for the filters
        // until a new image is imported, the current one will be used
        std::string filepath(importImageFilepath);
        
        if(trimString(filepath) != ""){
            bool loaded = importImage(filepath.c_str(), &texture, &originalImage, &imageWidth, &imageHeight, &imageChannels);
            if(loaded){
                showImage = true;
            }else{
                ImGui::Text("import image failed");
            }
        }
        
        filterParams.generateRandNum3();
    }
    
    if(showImage){
        // TODO: get an open file dialog working?
        ImGui::Text("size = %d x %d", imageWidth, imageHeight);
        ImGui::Image((void *)(intptr_t)texture, ImVec2(imageWidth, imageHeight));
        //ImGui::Text("image imported");
        
        int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
        unsigned char* pixelData = new unsigned char[pixelDataLen];
        
        // GRAYSCALE
        if(ImGui::Button("grayscale")){
            // get current image
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
            
            grayscale(pixelData, pixelDataLen);
            
            // write it back
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        }
        ImGui::SameLine();
        
        // INVERT
        if(ImGui::Button("invert")){
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
            invert(pixelData, pixelDataLen);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        }
        ImGui::SameLine();
        
        // SATURATION
        if(ImGui::Button("saturate")){
            showSaturateParams = !showSaturateParams;
            showOutlineParams = false; // TODO: figure out better way to do this
            showMosaicParams = false;
            showChannelOffsetParams = false;
            showCrtParams = false;
            updateTempImageState(imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // OUTLINE
        if(ImGui::Button("outline")){
            showOutlineParams = !showOutlineParams;
            showSaturateParams = false;
            showMosaicParams = false;
            showChannelOffsetParams = false;
            showCrtParams = false;
            updateTempImageState(imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // CHANNEL OFFSET
        if(ImGui::Button("channel offset")){
            showChannelOffsetParams = !showChannelOffsetParams;
            showSaturateParams = false;
            showOutlineParams = false;
            showMosaicParams = false;
            showCrtParams = false;
            updateTempImageState(imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // MOSAIC
        if(ImGui::Button("mosaic")){
            showMosaicParams = !showMosaicParams;
            showSaturateParams = false;
            showOutlineParams = false;
            showChannelOffsetParams = false;
            showCrtParams = false;
            updateTempImageState(imageWidth, imageHeight);
        }
        ImGui::SameLine();
        
        // CRT
        if(ImGui::Button("crt")){
            showCrtParams = !showCrtParams;
            showMosaicParams = false;
            showSaturateParams = false;
            showOutlineParams = false;
            showChannelOffsetParams = false;
            updateTempImageState(imageWidth, imageHeight);
        }
        
        // RESET IMAGE
        if(ImGui::Button("reset image")){
            resetImageState(imageWidth, imageHeight);
            filterParams.generateRandNum3();
        }
        
        if(showSaturateParams){
            glActiveTexture(TEMP_IMAGE); // get current state - we'll edit this image data
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            saturate(pixelData, pixelDataLen, filterParams);
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderFloat("saturation val", &filterParams.saturationVal, 0.0f, 5.0f);
            ImGui::SliderFloat("lumR", &filterParams.lumR, 0.0f, 5.0f);
            ImGui::SliderFloat("lumG", &filterParams.lumG, 0.0f, 5.0f);
            ImGui::SliderFloat("lumB", &filterParams.lumB, 0.0f, 5.0f);
        }
        
        if(showOutlineParams){
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            
            // use the current texture to get pixel data from (so we can stack filter effects)
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            
            outline(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            
            delete[] sourceImageCopy;
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderInt("color difference limit", &filterParams.outlineLimit, 1, 20);
        }
        
        if(showMosaicParams){
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            
            // use the current texture to get pixel data from (so we can stack filter effects)
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            mosaic(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            
            delete[] sourceImageCopy;
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderInt("mosaic chunk size", &filterParams.chunkSize, 1, 20);
        }
        
        if(showChannelOffsetParams){
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            
            // use the current texture to get pixel data from (so we can stack filter effects)
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            
            channelOffset(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            
            delete[] sourceImageCopy;
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderInt("chan offset", &filterParams.chanOffset, 1, 15); // TODO: find out why using "channel offset" for the label produces an assertion error :0
        }
        
        if(showCrtParams){
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            
            // use the current texture to get pixel data from (so we can stack filter effects)
            glActiveTexture(TEMP_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            crt(pixelData, sourceImageCopy, imageWidth, imageHeight, filterParams);
            
            delete[] sourceImageCopy;
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderInt("scanline thickness", &filterParams.scanLineThickness, 0, 10);
            ImGui::SliderFloat("brightboost", &filterParams.brightboost, 0.0f, 1.0f);
            ImGui::SliderFloat("intensity", &filterParams.intensity, 0.0f, 1.0f);
        }
        
        // spacer
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        
        // EXPORT IMAGE
        if(ImGui::Button("export image (.bmp)")){
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            // TODO: allow image naming
            // TODO: hardcoding 4 channels b/c not sure we were getting the right channel value back for the imageChannels variable?
            stbi_write_bmp("image_editor_export.bmp", imageWidth, imageHeight, 4, (void *)pixelData);
        }
        
        
        delete[] pixelData;
    }
    
    //ImGui::EndChild();
}