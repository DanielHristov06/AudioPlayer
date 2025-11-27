#include "TextureLoader.h"
#include "stb_image.h"
#include <print>

GLuint loadTexture(const std::string& filename) {
	int width, height, channels;
	stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

	if (!data) {
		std::println("Failed to load image: {}\n", filename);
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}