#include "MeshAnimation.h"
#include "Matrix.h"
#include "Assets.h"

using namespace NCL;
using namespace Rendering;
using namespace Maths;

MeshAnimation::MeshAnimation() {
	jointCount	= 0;
	frameCount	= 0;
	frameRate	= 0.0f;
}

MeshAnimation::MeshAnimation(size_t jointCount, size_t frameCount, float frameRate, std::vector<Matrix4>& frames, const std::vector<std::string>& jointNames) {
	this->jointCount = jointCount;
	this->frameCount = frameCount;
	this->frameRate  = frameRate;
	this->allJoints  = frames;
	this->jointNames = jointNames;
}

MeshAnimation::MeshAnimation(const std::string& filename) : MeshAnimation() {
	std::ifstream file(Assets::MESHDIR + filename);

	std::string filetype;
	int fileVersion;

	file >> filetype;

	if (filetype != "MeshAnim") {
		std::cout << __FUNCTION__ << " File is not a MeshAnim file!\n";
		return;
	}
	file >> fileVersion;
	file >> frameCount;
	file >> jointCount;
	file >> frameRate;

	allJoints.reserve((size_t)frameCount * jointCount);

	for (unsigned int frame = 0; frame < frameCount; ++frame) {
		for (unsigned int joint = 0; joint < jointCount; ++joint) {
			Matrix4 mat;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					file >> mat.array[i][j];
				}
			}
			allJoints.emplace_back(mat);
		}
	}
}

MeshAnimation::~MeshAnimation() {

}

const Matrix4* MeshAnimation::GetJointData(size_t frame) const {
	if (frame >= frameCount) {
		return nullptr;
	}
	int matStart = frame * jointCount;

	Matrix4* dataStart = (Matrix4*)allJoints.data();

	return dataStart + matStart;
}