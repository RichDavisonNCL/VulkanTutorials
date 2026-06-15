#include "MshLoader.h"
#include "Assets.h"

#include "Vector.h"
#include "Matrix.h"

#include "Maths.h"

#include "Mesh.h"

using namespace NCL;
using namespace Rendering;
using namespace Maths;

bool MshLoader::LoadMesh(const std::string& filename, Mesh& destinationMesh) {
	std::ifstream file(Assets::MESHDIR + filename);

	std::string filetype;
	int fileVersion;

	file >> filetype;

	if (filetype != "MeshGeometry") {
		std::cout << __FUNCTION__ << " File is not a Mesh file!\n";
		return false;
	}

	file >> fileVersion;

	if (fileVersion != 1) {
		std::cout << __FUNCTION__ << " Mesh file has incompatible version!\n";
		return false;
	}

	int numMeshes = 0; //read
	int numVertices = 0; //read
	int numIndices = 0; //read
	int numChunks = 0; //read

	file >> numMeshes;
	file >> numVertices;
	file >> numIndices;
	file >> numChunks;

	for (int i = 0; i < numChunks; ++i) {
		int chunkType = (int)GeometryChunkTypes::VPositions;

		file >> chunkType;

		switch ((GeometryChunkTypes)chunkType) {
		case GeometryChunkTypes::VPositions: {
			vector<Vector3> positions;
			ReadTextFloats(file, positions, numVertices);
			destinationMesh.SetVertexPositions(positions);
		}break;
		case GeometryChunkTypes::VColors: {
			vector<Vector4> colours;
			ReadTextFloats(file, colours, numVertices);
			destinationMesh.SetVertexColours(colours);
		}break;
		case GeometryChunkTypes::VNormals: {
			vector<Vector3> normals;
			ReadTextFloats(file, normals, numVertices);
			destinationMesh.SetVertexNormals(normals);
		}break;
		case GeometryChunkTypes::VTangents: {
			vector<Vector4> tangents;
			ReadTextFloats(file, tangents, numVertices);
			destinationMesh.SetVertexTangents(tangents);

		}break;
		case GeometryChunkTypes::VTex0: {
			vector<Vector2> texCoords;
			ReadTextFloats(file, texCoords, numVertices);
			destinationMesh.SetVertexTextureCoords(texCoords);

		}break;
		case GeometryChunkTypes::Indices: {
			vector<unsigned int> indices;
			ReadIntegers(file, indices, numIndices);
			destinationMesh.SetVertexIndices(indices);
		}break;

		case GeometryChunkTypes::VWeightValues: {
			vector<Vector4> skinWeights;
			ReadTextFloats(file, skinWeights, numVertices);
			destinationMesh.SetVertexSkinWeights(skinWeights);
		}break;
		case GeometryChunkTypes::VWeightIndices: {
			vector<Vector4i> skinIndices;
			ReadTextInts(file, skinIndices, numVertices);
			destinationMesh.SetVertexSkinIndices(skinIndices);
		}break;
		case GeometryChunkTypes::JointNames: {
			std::vector<std::string> jointNames;
			ReadJointNames(file, jointNames);
			destinationMesh.SetJointNames(jointNames);
		}break;
		case GeometryChunkTypes::JointParents: {
			vector<int> parents;
			ReadJointParents(file, parents);
			destinationMesh.SetJointParents(parents);
		}break;
		case GeometryChunkTypes::BindPose: {
			vector<Matrix4> bindPose;
			ReadRigPose(file, bindPose);
			destinationMesh.SetBindPose(bindPose);
		}break;
		case GeometryChunkTypes::BindPoseInv: {
			vector<Matrix4> inverseBindPose;
			ReadRigPose(file, inverseBindPose);
			destinationMesh.SetInverseBindPose(inverseBindPose);
		}break;
		case GeometryChunkTypes::SubMeshes: {
			vector<SubMesh> subMeshes;
			ReadSubMeshes(file, numMeshes, subMeshes);

			destinationMesh.SetSubMeshes(subMeshes);
		}break;
		case GeometryChunkTypes::SubMeshNames: {
			std::vector<std::string> subMeshNames;
			ReadSubMeshNames(file, numMeshes, subMeshNames);
			destinationMesh.SetSubMeshNames(subMeshNames);
		}break;
		}
	}

	if (!destinationMesh.GetBindPose().empty() && destinationMesh.GetInverseBindPose().empty()) {
		destinationMesh.CalculateInverseBindPose();
	}

	destinationMesh.SetPrimitiveType(GeometryPrimitive::Triangles);

	return true;
}

bool MshLoader::SaveMesh(const std::string& filename, Mesh& sourceMesh) {
	std::ofstream file(filename);


	static int FORMAT_VERSION = 1;

	file << "MeshGeometry\n";
	file << FORMAT_VERSION;

	/*
	int numMeshes = 0; //read
	int numVertices = 0; //read
	int numIndices = 0; //read
	int numChunks = 0; //read
	*/

	int numChunks = 0;

	if (!sourceMesh.GetPositionData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetColourData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetNormalData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetTangentData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetTextureCoordData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetIndexData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetSkinWeightData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetSkinIndexData().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetJointNames().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetJointParents().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetBindPose().empty()) {
		numChunks++;
	}
	if (!sourceMesh.GetInverseBindPose().empty()) {
		numChunks++;
	}
	if (sourceMesh.GetSubMeshCount() >= 1) {
		numChunks++;
	}
	if (!sourceMesh.GetSubMeshNames().empty()) {
		numChunks++;
	}
	
	file << sourceMesh.GetSubMeshCount();
	file << sourceMesh.GetVertexCount();
	file << sourceMesh.GetIndexCount();
	file << numChunks;

	//And now to save each chunk out!


	if (!sourceMesh.GetPositionData().empty()) {
		file << (int)GeometryChunkTypes::VPositions << "\n";
		WriteTextFloats(file, sourceMesh.GetPositionData());
	}
	if (!sourceMesh.GetColourData().empty()) {
		file << (int)GeometryChunkTypes::VColors << "\n";
		WriteTextFloats(file, sourceMesh.GetColourData());
	}
	if (!sourceMesh.GetNormalData().empty()) {
		file << (int)GeometryChunkTypes::VNormals << "\n";
		WriteTextFloats(file, sourceMesh.GetNormalData());
	}
	if (!sourceMesh.GetTangentData().empty()) {
		file << (int)GeometryChunkTypes::VTangents << "\n";
		WriteTextFloats(file, sourceMesh.GetTangentData());
	}
	if (!sourceMesh.GetTextureCoordData().empty()) {
		file << (int)GeometryChunkTypes::VTex0 << "\n";
		WriteTextFloats(file, sourceMesh.GetTextureCoordData());
	}
	if (!sourceMesh.GetIndexData().empty()) {
		file << (int)GeometryChunkTypes::Indices << "\n";
		WriteIntegers(file, sourceMesh.GetIndexData());
	}
	if (!sourceMesh.GetSkinWeightData().empty()) {
		file << (int)GeometryChunkTypes::VWeightValues << "\n";
		WriteTextFloats(file, sourceMesh.GetSkinWeightData());
	}
	if (!sourceMesh.GetSkinIndexData().empty()) {
		file << (int)GeometryChunkTypes::VWeightIndices << "\n";
		WriteIntegers(file, sourceMesh.GetSkinIndexData());
	}
	if (!sourceMesh.GetJointNames().empty()) {
		file << (int)GeometryChunkTypes::JointNames << "\n";

	}
	if (!sourceMesh.GetJointParents().empty()) {
		file << (int)GeometryChunkTypes::JointParents << "\n";

	}
	if (!sourceMesh.GetBindPose().empty()) {
		file << (int)GeometryChunkTypes::BindPose << "\n";
		WriteMatrices(file, sourceMesh.GetBindPose());
	}
	if (!sourceMesh.GetInverseBindPose().empty()) {
		file << (int)GeometryChunkTypes::BindPoseInv << "\n";
		WriteMatrices(file, sourceMesh.GetInverseBindPose());
	}
	if (sourceMesh.GetSubMeshCount() >= 1) {
		file << (int)GeometryChunkTypes::SubMeshes << "\n";

	}
	if (!sourceMesh.GetSubMeshNames().empty()) {
		file << (int)GeometryChunkTypes::SubMeshNames << "\n";

	}



	return true;
}

void MshLoader::ReadRigPose(std::ifstream& file, vector<Matrix4>& into) {
	int matCount = 0;
	file >> matCount;

	for (int m = 0; m < matCount; ++m) {
		Matrix4 mat;

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				file >> mat.array[i][j];
			}
		}

		into.emplace_back(mat);
	}
}

void MshLoader::ReadJointParents(std::ifstream& file, std::vector<int>& parentIDs) {
	int jointCount = 0;
	file >> jointCount;

	for (int i = 0; i < jointCount; ++i) {
		int id = -1;
		file >> id;
		parentIDs.emplace_back(id);
	}
}

void MshLoader::ReadJointNames(std::ifstream& file, std::vector<std::string>& jointNames) {
	int jointCount = 0;
	file >> jointCount;
	std::string jointName;
	std::getline(file, jointName);

	for (int i = 0; i < jointCount; ++i) {
		std::string jointName;
		std::getline(file, jointName);
		jointNames.emplace_back(jointName);
	}
}

void MshLoader::ReadSubMeshes(std::ifstream& file, int count, std::vector<SubMesh>& subMeshes) {
	for (int i = 0; i < count; ++i) {
		SubMesh m;
		file >> m.start;
		file >> m.count;
		subMeshes.emplace_back(m);
	}
}

void MshLoader::ReadSubMeshNames(std::ifstream& file, int count, std::vector<std::string>& subMeshNames) {
	std::string scrap;
	std::getline(file, scrap);

	for (int i = 0; i < count; ++i) {
		std::string meshName;
		std::getline(file, meshName);
		subMeshNames.emplace_back(meshName);
	}
}

void* MshLoader::ReadVertexData(GeometryChunkData dataType, GeometryChunkTypes chunkType, int numVertices) {
	int numElements = 3;

	if (chunkType == GeometryChunkTypes::VTex0 ||
		chunkType == GeometryChunkTypes::VTex1) {
		numElements = 2;
	}
	else if (chunkType == GeometryChunkTypes::VColors) {
		numElements = 4;
	}

	int bytesPerElement = 4;

	if (dataType == GeometryChunkData::dShort) {
		bytesPerElement = 2;
	}
	else if (dataType == GeometryChunkData::dByte) {
		bytesPerElement = 1;
	}

	char* data = new char[numElements * bytesPerElement];

	return data;
}

void MshLoader::ReadTextInts(std::ifstream& file, vector<Vector2i>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector2i temp;
		file >> temp[0];
		file >> temp[1];
		element.emplace_back(temp);
	}
}

void MshLoader::ReadTeReadTextIntsxtFloats(std::ifstream& file, vector<Vector3i>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector3i temp;
		file >> temp[0];
		file >> temp[1];
		file >> temp[2];
		element.emplace_back(temp);
	}
}

void MshLoader::ReadTextInts(std::ifstream& file, vector<Vector4i>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector4i temp;
		file >> temp[0];
		file >> temp[1];
		file >> temp[2];
		file >> temp[3];
		element.emplace_back(temp);
	}
}

void MshLoader::ReadTextFloats(std::ifstream& file, vector<Vector2>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector2 temp;
		file >> temp.x;
		file >> temp.y;
		element.emplace_back(temp);
	}
}

void MshLoader::ReadTextFloats(std::ifstream& file, vector<Vector3>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector3 temp;
		file >> temp.x;
		file >> temp.y;
		file >> temp.z;
		element.emplace_back(temp);
	}
}

void MshLoader::ReadTextFloats(std::ifstream& file, vector<Vector4>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector4 temp;
		file >> temp.x;
		file >> temp.y;
		file >> temp.z;
		file >> temp.w;
		element.emplace_back(temp);
	}
}

void MshLoader::ReadIntegers(std::ifstream& file, vector<unsigned int>& elements, int intCount) {
	for (int i = 0; i < intCount; ++i) {
		unsigned int temp;
		file >> temp;
		elements.emplace_back(temp);
	}
}

void MshLoader::WriteTextFloats(std::ofstream& file, const vector<Maths::Vector2>& elements) {
	for(const auto& v : elements) {
		file << v.x << " " << v.y << "\n";
	}
}

void MshLoader::WriteTextFloats(std::ofstream& file, const vector<Maths::Vector3>& elements) {
	for (const auto& v : elements) {
		file << v.x << " " << v.y << " " << v.z << "\n";
	}
}

void MshLoader::WriteTextFloats(std::ofstream& file, const vector<Maths::Vector4>& elements) {
	for (const auto& v : elements) {
		file << v.x << " " << v.y << " " << v.z << " " << v.w << "\n";
	}
}

void MshLoader::WriteIntegers(std::ofstream& file, const vector<unsigned int>& elements) {
	for (const auto& i : elements) {
		file << i << "\n";
	}
}

void MshLoader::WriteIntegers(std::ofstream& file, const vector<Maths::Vector4i>& elements) {
	for (const auto& v : elements) {
		file << v.x << " " << v.y << " " << v.z << " " << v.w << "\n";
	}
}

void MshLoader::WriteMatrices(std::ofstream& file, const vector<Matrix4>& elements) {
	for (const auto& m : elements) {
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				file << m.array[i][j] << " ";
			}
		}
		file << "\n";
	}
}

void MshLoader::WriteSubMeshes(std::ofstream& file, const vector<struct SubMesh>& elements) {
	for (const auto& m : elements) {
		file << m.start << " " << m.count << "\n";
	}
}