/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Vector.h"

namespace NCL {
	namespace Rendering {
		class Texture;

		class SimpleFont	{
		public:
			SimpleFont(const std::string&fontName, Texture& tex);
			~SimpleFont();

			struct InterleavedTextVertex {
				NCL::Maths::Vector2 pos;
				NCL::Maths::Vector2 texCoord;
				NCL::Maths::Vector4 colour;
			};

			int  GetVertexCountForString(const std::string& text);
			void BuildVerticesForString(const std::string& text, const Maths::Vector2& startPos, const Maths::Vector4& colour, float size, std::vector<Maths::Vector3>& positions, std::vector<Maths::Vector2>& texCoords, std::vector<Maths::Vector4>& colours);
			void BuildInterleavedVerticesForString(const std::string& text, const Maths::Vector2& startPos, const Maths::Vector4& colour, float size, std::vector<InterleavedTextVertex>& vertices);
			
			const Texture* GetTexture() const {
				return &texture;
			}

		protected:
			//matches stbtt_bakedchar
			struct FontChar {
				unsigned short x0;
				unsigned short y0;
				unsigned short x1;
				unsigned short y1;
				float xOff;
				float yOff;
				float xAdvance;
			};

			FontChar*	allCharData;
			Texture&	texture;

			int startChar;
			int numChars;

			float texWidth;
			float texHeight;
			float texWidthRecip;
			float texHeightRecip;
		};
	}
}

