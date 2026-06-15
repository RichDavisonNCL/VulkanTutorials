/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#include "TextureLoader.h"
#define STB_IMAGE_IMPLEMENTATION

#include "./stb/stb_image.h"

#include "Assets.h"

using namespace NCL;
using namespace Rendering;

std::map<std::string, TextureLoadFunction> TextureLoader::fileHandlers;

bool TextureLoader::LoadTexture(const std::string& filename, char*& outData, uint32_t& width, uint32_t& height, uint32_t& channels, uint32_t& flags) {
	if (filename.empty()) {
		return false;
	}

	std::filesystem::path path(filename);
	
	std::string extension = path.extension().string();

	bool isAbsolute = path.is_absolute();

	auto it = fileHandlers.find(extension);

	std::string realPath = isAbsolute ? filename : Assets::TEXTUREDIR + filename;

	if (it != fileHandlers.end()) {
		//There's a custom handler function for this, just use that
		return it->second(realPath, outData, width, height, channels, flags);
	}
	//By default, attempt to use stb image to get this texture
	int stbiWidth = 0;
	int stbiHeight = 0;
	int stbiChannels = 0;

	stbi_uc *texData = stbi_load(realPath.c_str(), &stbiWidth, &stbiHeight, &stbiChannels, 4); //4 forces this to always be rgba!
	width		= stbiWidth;
	height		= stbiHeight;
	channels	= 4; //it gets forced, we don't care about the 'real' channel size

	if (texData) {
		outData = (char*)texData;
		return true;
	}

	return false;
}

void TextureLoader::RegisterTextureLoadFunction(TextureLoadFunction f, const std::string&fileExtension) {
	fileHandlers.insert(std::make_pair(fileExtension, f));
}

std::string TextureLoader::GetFileExtension(const std::string& fileExtension) {
	std::filesystem::path p = std::filesystem::path(fileExtension);

	std::filesystem::path ext = p.extension();

	return ext.string();
}

void TextureLoader::DeleteTextureData(char* data) {
	free(data);
}