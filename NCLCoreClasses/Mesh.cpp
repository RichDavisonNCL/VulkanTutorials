#include "Mesh.h"
#include "Assets.h"
#include "Vector.h"
#include "Matrix.h"

#include "Maths.h"

using namespace NCL;
using namespace Rendering;
using namespace Maths;

Mesh::Mesh()	{
	primType = GeometryPrimitive::Triangles;
	assetID  = 0;
}

Mesh::~Mesh()	{
}

bool Mesh::HasTriangle(unsigned int i) const {
	int triCount = 0;
	if (GetIndexCount() > 0) {
		triCount = GetIndexCount() / 3;
	}
	else {
		triCount = GetVertexCount() / 3;
	}
	return i < (unsigned int)triCount;
}

bool	Mesh::GetVertexIndicesForTri(unsigned int i, unsigned int& a, unsigned int& b, unsigned int& c) const {
	if (!HasTriangle(i)) {
		return false;
	}
	if (GetIndexCount() > 0) {
		a = indices[(i * 3)];
		b = indices[(i * 3) + 1];
		c = indices[(i * 3) + 2];
	}
	else {
		a = (i * 3);
		b = (i * 3) + 1;
		c = (i * 3) + 2;
	}
	return true;
}

bool Mesh::GetTriangle(unsigned int i, Vector3& va, Vector3& vb, Vector3& vc) const {
	bool hasTri = false;
	unsigned int a, b, c;
	hasTri = GetVertexIndicesForTri(i, a, b, c);
	if (!hasTri) {
		return false;
	}
	va = positions[a];
	vb = positions[b];
	vc = positions[c];
	return true;
}

bool Mesh::GetNormalForTri(unsigned int i, Vector3& n) const {
	Vector3 a, b, c;

	bool hasTri = GetTriangle(i, a, b, c);
	if (!hasTri) {
		return false;
	}

	Vector3 ba = b - a;
	Vector3 ca = c - a;
	n = Vector::Cross(ba, ca);
	n = Vector::Normalise(n);
	return true;
}

int Mesh::GetIndexForJoint(const std::string& name) const {
	for (int i = 0; i < jointNames.size(); ++i) {
		if (jointNames[i] == name) {
			return i;
		}
	}
	return -1;
}

void Mesh::SetVertexPositions(const std::vector<Vector3>& newVerts) {
	positions = newVerts;
}

void Mesh::SetVertexTextureCoords(const std::vector<Vector2>& newTex) {
	texCoords = newTex;
}

void Mesh::SetVertexColours(const std::vector<Vector4>& newColours) {
	colours = newColours;
}

void Mesh::SetVertexNormals(const std::vector<Vector3>& newNorms) {
	normals = newNorms;
}

void Mesh::SetVertexTangents(const std::vector<Vector4>& newTans) {
	tangents = newTans;
}

void Mesh::SetVertexIndices(const std::vector<unsigned int>& newIndices) {
	indices = newIndices;
}

void Mesh::SetVertexSkinWeights(const std::vector<Vector4>& newSkinWeights) {
	skinWeights = newSkinWeights;
}

void Mesh::SetVertexSkinIndices(const std::vector<Vector4i>& newSkinIndices) {
	skinIndices = newSkinIndices;
}

void Mesh::SetDebugName(const std::string& newName) {
	debugName = newName;
}

void Mesh::SetJointNames(const std::vector < std::string >& newNames) {
	jointNames = newNames;
}

void Mesh::SetJointParents(const std::vector<int>& newParents) {
	jointParents = newParents;
}

void Mesh::SetBindPose(const std::vector<Matrix4>& newMats) {
	bindPose = newMats;
}

void Mesh::SetInverseBindPose(const std::vector<Matrix4>& newMats) {
	inverseBindPose = newMats;
}

void Mesh::SetSubMeshes(const std::vector < SubMesh>& meshes) {
	subMeshes = meshes;
}

void Mesh::SetSubMeshNames(const std::vector < std::string>& newNames) {
	subMeshNames = newNames;
}

void Mesh::CalculateInverseBindPose() {
	inverseBindPose.resize(bindPose.size());

	for (int i = 0; i < bindPose.size(); ++i) {
		inverseBindPose[i] = Matrix::Inverse(bindPose[i]);
	}
}

bool Mesh::ValidateMeshData() {
	if (GetPositionData().empty()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " does not have any vertex positions!\n";
		return false;
	}
	if (!GetTextureCoordData().empty() && GetTextureCoordData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect texture coordinate attribute count!\n";
		return false;
	}
	if (!GetColourData().empty() && GetColourData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect colour attribute count!\n";
		return false;
	}
	if (!GetNormalData().empty() && GetNormalData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect normal attribute count!\n";
		return false;
	}
	if (!GetTangentData().empty() && GetTangentData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect tangent attribute count!\n";
		return false;
	}

	if (!GetSkinWeightData().empty() && GetSkinWeightData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect skin weight attribute count!\n";
		return false;
	}

	if (!GetSkinIndexData().empty() && GetSkinIndexData().size() != GetVertexCount()) {
		std::cout << __FUNCTION__ << " mesh " << debugName << " has an incorrect skin index attribute count!\n";
		return false;
	}
	return true;
}