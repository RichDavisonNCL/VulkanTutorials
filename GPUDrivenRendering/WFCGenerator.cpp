/** @file WFCGenerator.cpp
 * Implements adjacency-constrained WFC with weight-based density control.
 * Uses superposition propagation for constraint satisfaction across the grid.
 */
#include "WFCGenerator.h"
#include <algorithm>

using namespace NCL::Rendering::Vulkan;

static const WFCTile s_tiles[] = {
	{ 0, "EMPTY",    false,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
	{ 1, "LOW",      true,   1.5f,  3.0f, 1.0f, 0.204f, 0.596f, 0.859f },
	{ 2, "MID",      true,   4.0f,  8.0f, 1.0f, 0.180f, 0.800f, 0.443f },
	{ 3, "HIGH",     true,  12.0f, 20.0f, 1.0f, 0.906f, 0.298f, 0.235f },
	{ 4, "SPHERE_S", false,  1.0f,  2.0f, 0.5f, 0.953f, 0.612f, 0.071f },
	{ 5, "SPHERE_L", false,  3.0f,  5.0f, 0.8f, 0.608f, 0.349f, 0.714f },
};

static const bool s_adjacency[6][6] = {
	{ 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0 },
	{ 1, 0, 1, 0, 0, 0 },
	{ 1, 0, 0, 0, 1, 1 },
	{ 1, 0, 0, 0, 1, 0 },
};

WFCGenerator::WFCGenerator() : m_rng(42) {}

const WFCTile& WFCGenerator::GetTile(uint32_t id) { return s_tiles[id]; }

bool WFCGenerator::IsValidAdjacency(uint32_t a, uint32_t b) const {
	return s_adjacency[a][b];
}

std::vector<uint32_t> WFCGenerator::Generate(const WFCConfig& config) {
	m_rng.seed(config.seed);
	const uint32_t N = config.gridSize;
	const uint32_t total = N * N;

	std::vector<uint32_t> grid(total, 0);
	std::vector<std::vector<uint32_t>> possibilities(total);
	std::vector<float> weights(kTileCount);
	weights[0] = config.emptyWeight;
	for (uint32_t i = 1; i < kTileCount; ++i) weights[i] = config.otherWeight;

	std::vector<uint32_t> allTiles(kTileCount);
	for (uint32_t i = 0; i < kTileCount; ++i) allTiles[i] = i;
	for (uint32_t i = 0; i < total; ++i) possibilities[i] = allTiles;

	for (uint32_t iter = 0; iter < total; ++iter) {
		uint32_t bestIdx = 0;
		size_t bestCount = kTileCount + 1;
		for (uint32_t i = 0; i < total; ++i) {
			size_t count = possibilities[i].size();
			if (count > 1 && count < bestCount) {
				bestCount = count;
				bestIdx = i;
			}
		}
		if (bestCount > kTileCount) break;

		float totalWeight = 0.0f;
		for (uint32_t t : possibilities[bestIdx]) totalWeight += weights[t];
		std::uniform_real_distribution<float> dist(0.0f, totalWeight);
		float pick = dist(m_rng);
		uint32_t chosen = possibilities[bestIdx][0];
		float accum = 0.0f;
		for (uint32_t t : possibilities[bestIdx]) {
			accum += weights[t];
			if (pick <= accum) { chosen = t; break; }
		}

		possibilities[bestIdx] = { chosen };
		grid[bestIdx] = chosen;
		Propagate(possibilities, N, bestIdx % N, bestIdx / N);

		if (config.onProgress && (iter % 500 == 0)) {
			config.onProgress(iter + 1, total);
		}
	}

	if (config.onProgress) config.onProgress(total, total);

	return grid;
}

void WFCGenerator::Propagate(std::vector<std::vector<uint32_t>>& possibilities,
	uint32_t gridSize, uint32_t startX, uint32_t startY) {

	std::vector<std::pair<uint32_t, uint32_t>> stack;
	stack.emplace_back(startX, startY);

	const int dx[] = { 1, -1, 0, 0 };
	const int dy[] = { 0, 0, 1, -1 };

	while (!stack.empty()) {
		auto [cx, cy] = stack.back();
		stack.pop_back();
		uint32_t ci = cy * gridSize + cx;

		for (int d = 0; d < 4; ++d) {
			int nx = (int)cx + dx[d];
			int ny = (int)cy + dy[d];
			if (nx < 0 || ny < 0 || nx >= (int)gridSize || ny >= (int)gridSize) continue;
			uint32_t ni = ny * gridSize + nx;

			auto& neighborPoss = possibilities[ni];
			size_t before = neighborPoss.size();
			neighborPoss.erase(
				std::remove_if(neighborPoss.begin(), neighborPoss.end(),
					[&](uint32_t nt) {
						bool anyValid = false;
						for (uint32_t ct : possibilities[ci]) {
							if (IsValidAdjacency(nt, ct)) { anyValid = true; break; }
						}
						return !anyValid;
					}),
				neighborPoss.end());

			if (neighborPoss.size() < before && neighborPoss.size() == 1) {
				stack.emplace_back(nx, ny);
			}
		}
	}
}

std::vector<WFCInstance> WFCGenerator::TileGridToInstances(
	const std::vector<uint32_t>& tileGrid, uint32_t gridSize, float cellSize) const {

	std::vector<WFCInstance> instances;
	instances.reserve(tileGrid.size());

	std::uniform_real_distribution<float> scaleDist(0.0f, 1.0f);

	for (uint32_t y = 0; y < gridSize; ++y) {
		for (uint32_t x = 0; x < gridSize; ++x) {
			uint32_t tileId = tileGrid[y * gridSize + x];
			if (tileId == 0) continue;

			const WFCTile& tile = s_tiles[tileId];
			float t = scaleDist(m_rng);
			float scaleY = tile.scaleYMin + t * (tile.scaleYMax - tile.scaleYMin);

			instances.push_back({
				(x + 0.5f) * cellSize,
				scaleY * 0.5f,
				(y + 0.5f) * cellSize,
				tile.scaleXZ,
				scaleY,
				tile.scaleXZ,
				tile.r, tile.g, tile.b,
				tile.isCube
			});
		}
	}
	return instances;
}
