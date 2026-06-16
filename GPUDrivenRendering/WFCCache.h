/** @file WFCCache.h
 * Binary cache for WFC tile grids. Save/load raw uint32_t grids to disk.
 */
#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace NCL::Rendering::Vulkan {

class WFCCache {
public:
    static void Save(const std::vector<uint32_t>& tileGrid, uint32_t gridSize,
                     uint32_t seed, uint32_t density);
    static bool Load(std::vector<uint32_t>& outGrid, uint32_t& outGridSize,
                     uint32_t gridSize, uint32_t seed, uint32_t density);
    static std::string Path(uint32_t gridSize, uint32_t seed, uint32_t density);

private:
    static void EnsureCacheDir();
};

} // namespace NCL::Rendering::Vulkan
