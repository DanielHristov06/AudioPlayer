#include "TextureLoader.h"
#include "stb_image.h"
#include "Logger.h"

GLuint loadTexture(const std::string& filename) {
	int width, height, channels;
	stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

	if (!data) {
		Logger::get().log(Logger::Level::Error, "Failed to load image: {}\n because {}\n", filename, stbi_failure_reason());
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

GLuint loadTextureFromMemory(const unsigned char* data, unsigned int length) {
	int width, height, channels;
	stbi_uc* pixels = stbi_load_from_memory(data, length, &width, &height, &channels, 4);

	if (!pixels) {
		Logger::get().log(Logger::Level::Error, "Failed to decode embedded image: {}", stbi_failure_reason());
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	stbi_image_free(pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}

GLuint loadTextureFromResource(const std::string& internalPath) {
	const auto fs = cmrc::icons::get_filesystem();

	if (!fs.exists(internalPath)) {
		Logger::get().log(Logger::Level::Error, "Resource {} not found!", internalPath);
		return 0;
	}
	const auto file = fs.open(internalPath);

	int width, height, channels;
	stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.begin()), file.size(), &width, &height, &channels, 4);

	if (!pixels) {
		Logger::get().log(Logger::Level::Error, "Failed to decode resource: {} because {}", internalPath, stbi_failure_reason());
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	stbi_image_free(pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}