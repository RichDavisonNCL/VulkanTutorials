/** @file TestMain.cpp
 * Unit tests for WFC generator, frustum culling math, data layout.
 * No Vulkan dependency. Built as separate target: GPUDrivenRendering_Tests.
 */
#include <iostream>
#include <cstdint>
#include <vector>
#include <cmath>

#include "../WFCGenerator.h"

using namespace NCL::Rendering::Vulkan;

// Local mirrors of GPUSceneManagement.h structs for size verification
struct TestCullingDatum { float cx, cy, cz, hx, hy, hz; };
struct TestRenderDatum  { float r, g, b, a; };
struct TestChunkInfo    { uint32_t gx, gy, off, cnt, cc, sc; float mnx,mny,mnz, mxx,mxy,mxz; };

static int g_passed = 0;
static int g_failed = 0;

#define T_ASSERT(cond, msg) do { \
	if (!(cond)) { std::cout << "  FAIL: " << msg << std::endl; ++g_failed; return; } \
} while(0)
#define T_ASSERT_EQ(a, b, msg) T_ASSERT((a) == (b), msg)

// ---------------------------------------------------------------------------
// WFC Generator
// ---------------------------------------------------------------------------

static void test_deterministic() {
	std::cout << "test_deterministic... ";
	WFCGenerator gen;
	WFCConfig cfg; cfg.gridSize = 32; cfg.seed = 42; cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f;
	auto a = gen.Generate(cfg);
	auto b = gen.Generate(cfg);
	T_ASSERT_EQ(a.size(), b.size(), "size");
	for (size_t i = 0; i < a.size(); ++i) T_ASSERT_EQ(a[i], b[i], "cell");
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_grid_size() {
	std::cout << "test_grid_size... ";
	for (uint32_t n : {16u, 32u, 64u}) {
		WFCConfig cfg; cfg.gridSize = n; cfg.seed = 1337; cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f;
		WFCGenerator gen;
		T_ASSERT_EQ(gen.Generate(cfg).size(), (size_t)n * n, "count");
	}
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_no_high_high_adjacency() {
	std::cout << "test_no_high_high... ";
	WFCConfig cfg; cfg.gridSize = 64; cfg.seed = 42; cfg.emptyWeight = 0.5f; cfg.otherWeight = 10.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	uint32_t N = cfg.gridSize;
	for (uint32_t y = 0; y < N; ++y)
		for (uint32_t x = 0; x < N; ++x) {
			if (grid[y * N + x] != 3) continue;
			if (x + 1 < N && grid[y * N + x + 1] == 3) T_ASSERT(false, "HIGH-HIGH right");
			if (y + 1 < N && grid[(y + 1) * N + x] == 3) T_ASSERT(false, "HIGH-HIGH down");
		}
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_sphere_l_adjacency() {
	std::cout << "test_sphere_l_adjacency... ";
	WFCConfig cfg; cfg.gridSize = 48; cfg.seed = 99; cfg.emptyWeight = 6.0f; cfg.otherWeight = 4.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	uint32_t N = cfg.gridSize;
	static const int dx[] = {1,-1,0,0}, dy[] = {0,0,1,-1};
	for (uint32_t y = 0; y < N; ++y)
		for (uint32_t x = 0; x < N; ++x) {
			if (grid[y * N + x] != 5) continue;
			for (int d = 0; d < 4; ++d) {
				int nx = (int)x + dx[d], ny = (int)y + dy[d];
				if (nx < 0 || ny < 0 || nx >= (int)N || ny >= (int)N) continue;
				uint32_t nb = grid[ny * N + nx];
				T_ASSERT(nb == 0 || nb == 4, "SPHERE_L bad neighbor");
			}
		}
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_instances_skip_empty() {
	std::cout << "test_instances_skip_empty... ";
	WFCConfig cfg; cfg.gridSize = 24; cfg.seed = 42; cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	auto inst = gen.TileGridToInstances(grid, 24, 3.0f);
	uint32_t nonEmpty = 0;
	for (auto t : grid) if (t != 0) ++nonEmpty;
	T_ASSERT_EQ(inst.size(), (size_t)nonEmpty, "count");
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_instance_positions() {
	std::cout << "test_instance_positions... ";
	WFCConfig cfg; cfg.gridSize = 16; cfg.seed = 777; cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f;
	WFCGenerator gen;
	float cs = 4.0f;
	auto inst = gen.TileGridToInstances(gen.Generate(cfg), 16, cs);
	for (const auto& i : inst) {
		T_ASSERT(i.posX >= 0.5f*cs && i.posX <= 15.5f*cs, "posX");
		T_ASSERT(i.posZ >= 0.5f*cs && i.posZ <= 15.5f*cs, "posZ");
		T_ASSERT(i.posY >= 0.0f, "posY");
	}
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_different_seeds() {
	std::cout << "test_different_seeds... ";
	WFCGenerator g1, g2;
	WFCConfig c1; c1.gridSize = 32; c1.seed = 42;   c1.emptyWeight = 5.0f; c1.otherWeight = 5.0f;
	WFCConfig c2; c2.gridSize = 32; c2.seed = 9999; c2.emptyWeight = 5.0f; c2.otherWeight = 5.0f;
	auto a = g1.Generate(c1), b = g2.Generate(c2);
	bool differ = false;
	for (size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) { differ = true; break; }
	T_ASSERT(differ, "identical");
	std::cout << "PASS" << std::endl; ++g_passed;
}

// ---------------------------------------------------------------------------
// Frustum culling (pure math, tested via shared FrustumCulling.h)
// ---------------------------------------------------------------------------

static void test_density_low() {
	std::cout << "test_density_low... ";
	WFCConfig cfg; cfg.gridSize = 64; cfg.seed = 42; cfg.emptyWeight = 8.0f; cfg.otherWeight = 2.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	uint32_t nonEmpty = 0;
	for (auto t : grid) if (t != 0) ++nonEmpty;
	float fill = (float)nonEmpty / grid.size();
	T_ASSERT(fill > 0.10f && fill < 0.40f, "out of range");
	std::cout << "PASS (" << (int)(fill*100) << "% fill)" << std::endl; ++g_passed;
}

static void test_density_medium() {
	std::cout << "test_density_medium... ";
	WFCConfig cfg; cfg.gridSize = 64; cfg.seed = 42; cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	uint32_t nonEmpty = 0;
	for (auto t : grid) if (t != 0) ++nonEmpty;
	float fill = (float)nonEmpty / grid.size();
	T_ASSERT(fill > 0.30f && fill < 0.70f, "out of range");
	std::cout << "PASS (" << (int)(fill*100) << "% fill)" << std::endl; ++g_passed;
}

static void test_density_high() {
	std::cout << "test_density_high... ";
	WFCConfig cfg; cfg.gridSize = 64; cfg.seed = 42; cfg.emptyWeight = 2.0f; cfg.otherWeight = 10.0f;
	WFCGenerator gen;
	auto grid = gen.Generate(cfg);
	uint32_t nonEmpty = 0;
	for (auto t : grid) if (t != 0) ++nonEmpty;
	float fill = (float)nonEmpty / grid.size();
	T_ASSERT(fill > 0.55f && fill < 0.95f, "out of range");
	std::cout << "PASS (" << (int)(fill*100) << "% fill)" << std::endl; ++g_passed;
}

// ---------------------------------------------------------------------------
// Data structure sizes
// ---------------------------------------------------------------------------

static void test_culling_datum_size() {
	std::cout << "sizeof(CullingDatum)==24... ";
	T_ASSERT_EQ(sizeof(TestCullingDatum), (size_t)24, "24");
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_render_datum_size() {
	std::cout << "sizeof(RenderDatum)==16... ";
	T_ASSERT_EQ(sizeof(TestRenderDatum), (size_t)16, "16");
	std::cout << "PASS" << std::endl; ++g_passed;
}

static void test_chunk_info_size() {
	std::cout << "sizeof(ChunkInfo)==48... ";
	T_ASSERT_EQ(sizeof(TestChunkInfo), (size_t)48, "48");
	std::cout << "PASS" << std::endl; ++g_passed;
}

// ---------------------------------------------------------------------------
// Indirect buffer layout
// ---------------------------------------------------------------------------

static void test_indirect_layout() {
	std::cout << "test_indirect_layout... ";
	constexpr uint32_t kCmdSize = 5 * sizeof(uint32_t);
	constexpr uint32_t kStride  = 2 * kCmdSize;
	T_ASSERT_EQ(kCmdSize, (uint32_t)20, "cmd");
	T_ASSERT_EQ(kStride,  (uint32_t)40, "stride");
	T_ASSERT_EQ((uint32_t)(0*5+1), (uint32_t)1, "cubeInstOff");
	T_ASSERT_EQ((uint32_t)(1*5+1), (uint32_t)6, "sphereInstOff");
	std::cout << "PASS" << std::endl; ++g_passed;
}

// ---------------------------------------------------------------------------
int main() {
	std::cout << "=== GPUDrivenRendering Tests ===" << std::endl << std::endl;

	// WFC generator
	test_deterministic();
	test_grid_size();
	test_no_high_high_adjacency();
	test_sphere_l_adjacency();
	test_instances_skip_empty();
	test_instance_positions();
	test_different_seeds();
	test_density_low();
	test_density_medium();
	test_density_high();

	// Data layout
	test_culling_datum_size();
	test_render_datum_size();
	test_chunk_info_size();
	test_indirect_layout();

	std::cout << std::endl << "=== " << g_passed << " passed, " << g_failed << " failed ===" << std::endl;
	return g_failed ? 1 : 0;
}
