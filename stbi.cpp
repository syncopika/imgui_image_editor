// https://stackoverflow.com/questions/64058036/preventing-multiple-define-when-including-headers

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"