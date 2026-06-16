/** @file WFCGenerator.h
 * Wave Function Collapse scene generator for 2D modular grid scenes.
 * Produces tile grids with adjacency-constrained placement and
 * weight-based density control.
 */
#pragma once
#include <vector>
#include <cstdint>
#include <random>
#include <functional>

namespace NCL::Rendering::Vulkan {

struct WFCTile {
	uint32_t id;
	const char* name;
	bool isCube;
	float scaleYMin, scaleYMax;
	float scaleXZ;
	float r, g, b;
};

struct WFCInstance {
	float posX, posY, posZ;
	float scaleX, scaleY, scaleZ;
	float r, g, b;
	bool isCube;
};

using WFCProgressCallback = std::function<void(uint32_t collapsed, uint32_t total)>;

struct WFCConfig {
	uint32_t gridSize        = 128;
	uint32_t seed            = 42;
	float emptyWeight        = 5.0f;
	float otherWeight        = 5.0f;
	WFCProgressCallback onProgress;
};

class WFCGenerator {
public:
	WFCGenerator();

	std::vector<uint32_t> Generate(const WFCConfig& config);

	std::vector<WFCInstance> TileGridToInstances(
		const std::vector<uint32_t>& tileGrid,
		uint32_t gridSize,
		float cellSize = 2.0f) const;

	static const WFCTile& GetTile(uint32_t id);
	static constexpr uint32_t kTileCount = 6;
	static constexpr uint32_t kEmptyTile = 0;

private:
	void Propagate(std::vector<std::vector<uint32_t>>& possibilities,
		uint32_t gridSize, uint32_t changedX, uint32_t changedY);
	bool IsValidAdjacency(uint32_t tileA, uint32_t tileB) const;

	mutable std::mt19937 m_rng;
};
}
