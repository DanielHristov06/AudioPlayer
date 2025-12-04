#pragma once
#include <string>
#include <glad/glad.h>

GLuint loadTexture(const std::string& filename);
GLuint loadTextureFromMemory(const unsigned char* data, unsigned int length);