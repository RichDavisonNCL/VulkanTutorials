/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include <map>
#include <functional>


namespace NCL {
	namespace Rendering {
		class Texture;
	}

	typedef std::function<bool(const std::string& filename, char*& outData, uint32_t& width, uint32_t& height, uint32_t& channels, uint32_t&flags)> TextureLoadFunction;

	typedef std::function<Rendering::Texture*(const std::string& filename)> APILoadFunction;

	class TextureLoader	{
	public:
		static bool LoadTexture(const std::string& filename, char*& outData, uint32_t& width, uint32_t& height, uint32_t& channels, uint32_t& flags);

		static void RegisterTextureLoadFunction(TextureLoadFunction f, const std::string&fileExtension);

		static void DeleteTextureData(char* data);
	protected:

		static std::string GetFileExtension(const std::string& fileExtension);

		static std::map<std::string, TextureLoadFunction> fileHandlers;
	};
}

