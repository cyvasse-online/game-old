/* Copyright 2014 Jonas Platte
*
* This file is NO part of Cyvasse Online.
*
* Use it however you want to.
*/

#ifndef _TEXTUREMAKER_HPP_
#define _TEXTUREMAKER_HPP_

#include <string>
#include <utility>
#include <fea/render2d.hpp>
#include "lodepng.h"

inline std::pair<fea::Texture, glm::uvec2> makeTexture(std::string path)
{
    unsigned width, height;

    std::vector<unsigned char> image; //the raw pixels

    // decode
    unsigned error = lodepng::decode(image, width, height, path);

    // if there's an error, display it
    if(error)
        throw std::runtime_error("lodepng error " + std::to_string(error) + ": " + lodepng_error_text(error));

    fea::Texture texture;
    texture.create(width, height, &image[0]);

    return std::pair<fea::Texture, glm::uvec2>(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(texture)),
        std::forward_as_tuple(width, height)
    );
}

#endif // _TEXTUREMAKER_HPP_
