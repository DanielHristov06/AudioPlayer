#pragma once
#include <string>
#include <glad/glad.h>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(icons);

GLuint loadTexture(const std::string& filename);
GLuint loadTextureFromMemory(const unsigned char* data, unsigned int length);
GLuint loadTextureFromResource(const std::string& internalPath);