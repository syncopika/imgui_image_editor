# imgui_image_editor    
### a basic image editor using Dear ImGui and C++    
    
a work-in-progress. not much to see here right now, but it has a few filters and you can rotate the image and export your edits :)    
    
![screenshot of project](screenshot.png)    
    
    
### installation    
I'm currently using MinGW and MSYS to compile this project. Note that I have some windows.h specific stuff (just for the file dialog to more easily import an image) - if windows.h is not available, remove the `-DWINDOWS_BUILD` flag in the Makefile before running `make`.    