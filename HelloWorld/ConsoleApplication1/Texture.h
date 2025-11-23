#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>

// JPG / PNG 텍스처 로더
// 반환값: OpenGL texture ID
unsigned int loadTexture(const std::string& path);

#endif

