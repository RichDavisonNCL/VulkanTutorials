/** @file WFCCache.cpp
 * Binary cache format: [uint32_t gridSize][uint32_t tileGrid[gridSize*gridSize]]
 */
#include "WFCCache.h"
#include <filesystem>
#include <fstream>
#include <iostream>

void WFCCache::EnsureCacheDir() {
    std::filesystem::create_directories("cache");
}

std::string WFCCache::Path(uint32_t gridSize, uint32_t seed, uint32_t density) {
    return "cache/wfc_" + std::to_string(gridSize) + "_"
           + std::to_string(seed) + "_" + std::to_string(density) + ".bin";
}

void WFCCache::Save(const std::vector<uint32_t>& tileGrid, uint32_t gridSize,
                    uint32_t seed, uint32_t density) {
    EnsureCacheDir();
    std::string path = Path(gridSize, seed, density);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[Cache] Failed to write " << path << "\n";
        return;
    }
    file.write(reinterpret_cast<const char*>(&gridSize), sizeof(gridSize));
    file.write(reinterpret_cast<const char*>(tileGrid.data()),
               tileGrid.size() * sizeof(uint32_t));
    std::cout << "[Cache] Saved " << tileGrid.size() << " cells to " << path << "\n";
}

bool WFCCache::Load(std::vector<uint32_t>& outGrid, uint32_t& outGridSize,
                    uint32_t gridSize, uint32_t seed, uint32_t density) {
    std::string path = Path(gridSize, seed, density);
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    uint32_t savedGridSize;
    file.read(reinterpret_cast<char*>(&savedGridSize), sizeof(savedGridSize));
    if (savedGridSize != gridSize) {
        std::cerr << "[Cache] Grid size mismatch: cached=" << savedGridSize
                  << " requested=" << gridSize << "\n";
        return false;
    }

    outGrid.resize(savedGridSize * savedGridSize);
    file.read(reinterpret_cast<char*>(outGrid.data()),
              outGrid.size() * sizeof(uint32_t));
    outGridSize = savedGridSize;

    if (file.gcount() != (std::streamsize)(outGrid.size() * sizeof(uint32_t))) {
        std::cerr << "[Cache] Incomplete read from " << path << "\n";
        return false;
    }

    std::cout << "[Cache] Loaded " << outGrid.size() << " cells from " << path << "\n";
    return true;
}
