/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once

namespace NCL::Rendering {
	class TextureBase	{
	public:
		virtual ~TextureBase();
	protected:
		TextureBase();
	};
}