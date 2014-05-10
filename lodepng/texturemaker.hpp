// This file was copied from therocode, it has no copyright and was only altered a tiny bit

#ifndef _TEXTUREMAKER_HPP_
#define _TEXTUREMAKER_HPP_

#include <iostream>
#include <featherkit/render2d.hpp>
#include "lodepng.h"

fea::Texture makeTexture(std::string path, uint32_t width, uint32_t height)
{
    std::vector<unsigned char> image; //the raw pixels

    //decode
    unsigned error = lodepng::decode(image, width, height, path);

    //if there's an error, display it
    if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fea::Texture texture;
    texture.create(width, height, &image[0]);
    return texture;
}

#endif // _TEXTUREMAKER_HPP_
