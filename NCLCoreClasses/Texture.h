/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Vector.h"

namespace NCL::Rendering {
	using namespace Maths;

	using UniqueTexture = std::unique_ptr<class Texture>;
	using SharedTexture = std::shared_ptr<class Texture>;

	class Texture	{
	public:
		virtual ~Texture();

		Vector2ui GetDimensions() const {
			return dimensions;
		}

		uint32_t GetAssetID() const {
			return assetID;
		}

		void SetAssetID(uint32_t newID) {
			assetID = newID;
		}

	protected:
		Texture();

		Vector2ui		dimensions;
		uint32_t		assetID;
	};
}