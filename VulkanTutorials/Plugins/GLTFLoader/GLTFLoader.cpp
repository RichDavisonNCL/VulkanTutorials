/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "GLTFLoader.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE 
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "../../Common/TextureLoader.h"

#include "../../Common/Matrix4.h"
#include "../../Common/Quaternion.h"

#include <filesystem>
#include <cmath>
#include <stack>

using namespace tinygltf;
using namespace NCL;
using namespace Maths;
using namespace Rendering;

const std::string GLTFAttributeTags[] = {
	"POSITION",
	"COLOR",
	"TEXCOORD_0",
	"NORMAL",
	"TANGENT",
	"WEIGHTS_0",
	"JOINTS_0",
};

template <class toType, class fromType>
void ReadDataInternal(toType* destination, const Accessor& accessor, const Model& model, int firstElement, int elementCount) {
	const BufferView& v	= model.bufferViews[accessor.bufferView];
	const Buffer& b		= model.buffers[v.buffer];
	const unsigned char* data = &(b.data[v.byteOffset + accessor.byteOffset]);
	int stride		= accessor.ByteStride(v);

	int inAxisCount = 1;

	if (accessor.type == TINYGLTF_TYPE_VEC2) {
		inAxisCount = 2;
	}
	else if (accessor.type == TINYGLTF_TYPE_VEC3) {
		inAxisCount = 3;
	}
	else if (accessor.type == TINYGLTF_TYPE_VEC4) {
		inAxisCount = 4;
	}

	size_t realCount = std::min((size_t)elementCount, accessor.count);

	for (int i = firstElement; i < (firstElement + realCount); ++i) {
		fromType* aData = (fromType*)(data + (stride * i));
		for (int j = 0; j < inAxisCount; ++j) {
			*destination = (toType)*aData;
			destination++;
			aData++;
		}
	}
}

template <class toType>
void	ReadData(const Accessor& accessor, const Model& model, void* d, int firstElement = 0, int elementCount = -1) {
	toType* destination = (toType*)d;
	if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		ReadDataInternal<toType, unsigned int>(destination, accessor, model, firstElement, elementCount);
	}
	else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
		ReadDataInternal<toType, unsigned char>(destination, accessor, model, firstElement, elementCount);
	}
	else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
		ReadDataInternal<toType, unsigned short>(destination, accessor, model, firstElement, elementCount);
	}
	else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		ReadDataInternal<toType, float>(destination, accessor, model, firstElement, elementCount);
	}
	else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
		ReadDataInternal<toType, double>(destination, accessor, model, firstElement, elementCount);
	}
}

template <class vecType, class elemType>
void CopyVectorData(vector<vecType>& vec, size_t destStart, const Accessor& accessor, const Model& model) {
	ReadData<elemType>(accessor, model, &vec[destStart]);
}

void GetInterpolationData(float forTime, int& indexA, int& indexB, float& t, const Accessor& accessor, const Model& model) {
	const auto& inBufferView = model.bufferViews[accessor.bufferView];
	const auto& inBuffer	 = model.buffers[inBufferView.buffer];

	const unsigned char* inData = inBuffer.data.data() + inBufferView.byteOffset + accessor.byteOffset;
	int stride = accessor.ByteStride(inBufferView);

	float a = 0.0f;
	float b = 0.0f;

	if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
		double* ddata = (double*)inData;
		for (int i = 0; i < accessor.count; ++i) {
			if (ddata[i] > forTime) {
				indexA = i > 1 ? (i - 1) : 0;
				indexB = i;
				b = (float)*((double*)(inData + (stride * indexB)));
				a = (float)*((double*)(inData + (stride * indexA)));
				break;
			}
		}
	}
	else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		float* fdata = (float*)inData;
		for (int i = 0; i < accessor.count; ++i) {
			if (fdata[i] > forTime || i == accessor.count - 1) {
				indexA = i > 0 ? (i - 1) : 0;
				indexB = i;
				b = *(float*)(inData + (stride * indexB));
				a = *(float*)(inData + (stride * indexA));
				break;
			}
		}
	}
	if (indexA == indexB) {
		t = 0.0f;
	}
	else {
		t = std::clamp((forTime - a) / (b - a), 0.0f, 1.0f);
	}
}

Quaternion GetSlerpQuaterion(float t, int indexA, int indexB, const Accessor& accessor, Model& model) {
	Quaternion qa;
	Quaternion qb;

	ReadData<float>(accessor, model, (float*)&qa, indexA, 1);
	ReadData<float>(accessor, model, (float*)&qb, indexB, 1);

	if (indexA == indexB) {
		return qa;
	}
	return Quaternion::Slerp(qa, qb, t);
}

template <class vecType>
vecType GetInterpolatedVector(float t, int indexA, int indexB, const Accessor& accessor, Model& model) {
	vecType a;
	vecType b;
	ReadData<float>(accessor, model, (float*)&a, indexA, 1);
	ReadData<float>(accessor, model, (float*)&b, indexB, 1);

	if (indexA == indexB) {
		return a;
	}

	return (a * (1.0f - t)) + (b * t);
}

GLTFLoader::GLTFLoader(const std::string& filename, GLTFLoader::MeshConstructionFunction meshConstructor) {
	TinyGLTF loader;
	Model	 model;

	loader.LoadASCIIFromFile(&model, nullptr, nullptr, NCL::Assets::GLTFDIR + filename);

	LoadImages(model, filename);
	LoadMaterials(model);
	LoadSceneNodeData(model);

	LoadVertexData(model, meshConstructor);
}

GLTFLoader::~GLTFLoader() {
	for (MeshGeometry* m : outMeshes) {
		delete m;
	}
	for (NCL::Rendering::TextureBase* t : outTextures) {
		delete t;
	}
}


void GLTFLoader::LoadImages(tinygltf::Model& m, const std::string& rootFile) {
	std::map<std::string, NCL::Rendering::TextureBase*> loadedTexturesMap;

	std::filesystem::path p			= rootFile;
	std::filesystem::path subPath	= p.parent_path();

	for (const auto& i : m.images) {
		std::filesystem::path imagePath = std::filesystem::path(NCL::Assets::GLTFDIR);
		imagePath += std::filesystem::path(p.parent_path());
		imagePath.append(i.uri);
		TextureBase* tex = TextureLoader::LoadAPITexture(imagePath.string());
		outTextures.push_back(tex);
		loadedTexturesMap.insert({ i.uri,tex });
	}
}

void GLTFLoader::LoadMaterials(tinygltf::Model& m) {
	for (const auto& m : m.materials) {
		GLTFMaterialLayer layer;
		layer.diffuse	= m.pbrMetallicRoughness.baseColorTexture.index			>= 0 ? outTextures[m.pbrMetallicRoughness.baseColorTexture.index]		  : nullptr;	
		layer.metallic	= m.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 ? outTextures[m.pbrMetallicRoughness.metallicRoughnessTexture.index] : nullptr;
			
		layer.bump		= m.normalTexture.index		>= 0 ? outTextures[m.normalTexture.index]	 : nullptr;
		layer.occlusion = m.occlusionTexture.index	>= 0 ? outTextures[m.occlusionTexture.index] : nullptr;
		layer.emission	= m.emissiveTexture.index	>= 0 ? outTextures[m.emissiveTexture.index]  : nullptr;
		
		fileMats.push_back(layer);
	}
}

void GLTFLoader::LoadVertexData(tinygltf::Model& model, GLTFLoader::MeshConstructionFunction meshConstructor) {
	for (const auto& m : model.meshes) {
		MeshGeometry* mesh = meshConstructor();
		GLTFMaterial material;

		if (m.primitives.empty()) {
			continue;
		}
		size_t totalVertexCount = 0;

		bool hasAttribute[MAX_ATTRIBUTES] = { false };

		for (const auto& p : m.primitives) {
			for (int i = 0; i < MAX_ATTRIBUTES; ++i) {
				hasAttribute[i] |= p.attributes.find(GLTFAttributeTags[i]) != p.attributes.end();
			}

			auto hasVerts = p.attributes.find(GLTFAttributeTags[Positions]);

			if (hasVerts != p.attributes.end()) {
				totalVertexCount += model.accessors[hasVerts->second].count;
			}
		}
		vector<Vector3> vPositions(totalVertexCount);
		vector<Vector3> vNormals(hasAttribute[Normals] ? totalVertexCount : 0);
		vector<Vector4> vTangents(hasAttribute[Tangents] ? totalVertexCount : 0);
		vector<Vector2> vTexCoords(hasAttribute[TextureCoords] ? totalVertexCount : 0);

		vector<Vector4>  vJointWeights(hasAttribute[JointWeights] ? totalVertexCount : 0);
		vector<Vector4i> vJointIndices(hasAttribute[JointIndices] ? totalVertexCount : 0);

		vector<unsigned int>		vIndices;

		const Primitive& p = m.primitives[0];

		size_t vArrayPos = 0;

		//now load up the actual vertex data
		for (const auto& p : m.primitives) {
			std::map<std::string, int>::const_iterator vPrims[MAX_ATTRIBUTES];

			for (int i = 0; i < MAX_ATTRIBUTES; ++i) {
				vPrims[i] = p.attributes.find(GLTFAttributeTags[i]);
			}

			size_t baseVertex = vArrayPos;
			size_t firstIndex = vIndices.size();

			if (vPrims[JointWeights] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[JointWeights]->second];
				CopyVectorData<Vector4, float>(vJointWeights, vArrayPos, a, model);
			}
			if (vPrims[JointIndices] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[JointIndices]->second];
				CopyVectorData<Vector4i, unsigned int>(vJointIndices, vArrayPos, a, model);
			}
			if (vPrims[Normals] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[Normals]->second];
				CopyVectorData<Vector3, float>(vNormals, vArrayPos, a, model);
			}
			if (vPrims[Tangents] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[Tangents]->second];
				CopyVectorData<Vector4, float>(vTangents, vArrayPos, a, model);
			}
			if (vPrims[TextureCoords] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[TextureCoords]->second];
				CopyVectorData<Vector2, float>(vTexCoords, vArrayPos, a, model);
			}
			if (vPrims[Positions] != p.attributes.end()) {
				Accessor& a = model.accessors[vPrims[Positions]->second];
				size_t oldPos = vPositions.size();
				CopyVectorData<Vector3, float>(vPositions, vArrayPos, a, model);
				vArrayPos += a.count;
			}

			if (p.indices >= 0) {
				Accessor& a = model.accessors[p.indices];

				size_t start = vIndices.size();
				vIndices.resize(start + a.count);

				CopyVectorData<unsigned int, unsigned int>(vIndices, start, a, model);
			}

			GLTFMaterialLayer matLayer;

			if (p.material >= 0) { //fcan ever be false?
				matLayer = fileMats[p.material];
			}
			material.allLayers.push_back(matLayer);

			mesh->AddSubMesh((int)firstIndex, (int)(vIndices.size() - firstIndex), (int)baseVertex);
		}
		mesh->SetVertexPositions(vPositions);
		mesh->SetVertexIndices(vIndices);
		mesh->SetVertexNormals(vNormals);
		mesh->SetVertexTangents(vTangents);
		mesh->SetVertexTextureCoords(vTexCoords);

		mesh->SetVertexSkinIndices(vJointIndices);
		mesh->SetVertexSkinWeights(vJointWeights);

		outMeshes.push_back(mesh);
		outMats.push_back(material);

		LoadSkinningData(model, mesh);
	}
}

void GLTFLoader::LoadSceneNodeData(tinygltf::Model& m) {
	for (int i = 0; i < m.nodes.size(); ++i) {
		auto& node = m.nodes[i];

		Matrix4 mat;
		if (!node.matrix.empty()) { //node can be defined with a matrix - never targeted by animations!
			float* data = (float*)(&(mat.array));
			for (int j = 0; j < node.matrix.size(); ++j) {
				*data = (float)node.matrix[j];
				++data;
			}
		}
		else {
			Matrix4 rotation;
			Matrix4 translation;
			Matrix4 scale;

			if (!node.scale.empty()) {
				Vector3 s = { (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
				scale = Matrix4::Scale(s);
			}
			if (!node.translation.empty()) {
				Vector3 t = { (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
				translation = Matrix4::Translation(t);
			}
			if (!node.rotation.empty()) {
				rotation = Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]).Normalised();
			}
			mat = translation * rotation * scale;
		}
		localNodeMatrices.push_back(mat);

		for (auto& j : node.children) {
			parentChildNodeLookup.insert({ j,i });
		}
	}
	//There's seemingly no guarantee that a child node comes after its parent...(RiggedSimple demonstrates this)
	//So instead we need to traverse any top-level nodes and build up the world matrices from there
	worldNodeMatrices = localNodeMatrices;

	std::stack<int> nodesToVisit;

	for (int i = 0; i < m.nodes.size(); ++i) {
		const auto& p = parentChildNodeLookup.find(i);
		if (p == parentChildNodeLookup.end()) { //a top level node!
			nodesToVisit.push(i);
		}
	}
	while (!nodesToVisit.empty()) {
		int node = nodesToVisit.top();
		nodesToVisit.pop();
		for (int i = 0; i < m.nodes[node].children.size(); ++i) {
			int cNode = m.nodes[node].children[i];
			worldNodeMatrices[cNode] = worldNodeMatrices[node] * worldNodeMatrices[cNode];
			nodesToVisit.push(cNode);
		}
	}
}

void GLTFLoader::LoadSkinningData(tinygltf::Model& model, MeshGeometry* mesh) {
	if (model.skins.empty()) {
		return;
	}
	std::map<int, int> nodeToJointLookup;

	vector<std::string> jointNames;
	vector<int>			jointParents;

	GLTFSkin skinData;

	int meshSkinID = 0; //TODO

	auto& skin = model.skins[meshSkinID];

	int rootIndex = 0;
	for (int i = 0; i < model.nodes.size(); ++i) {
		if (model.nodes[i].skin == meshSkinID) {
			rootIndex = i;
			break;
		}
	}
	skinData.globalTransformInverse = worldNodeMatrices[rootIndex].Inverse();

	size_t jointCount = skin.joints.size();

	skinData.worldInverseBindPose.resize(jointCount);

	int index = model.skins[0].inverseBindMatrices;
	if (index >= 0) {
		Accessor& a = model.accessors[index];

		const auto& inBufferView = model.bufferViews[a.bufferView];
		const auto& inBuffer = model.buffers[inBufferView.buffer];

		const unsigned char* inData = inBuffer.data.data() + inBufferView.byteOffset + a.byteOffset;
		Matrix4* test = (Matrix4*)inData;

		size_t count = inBufferView.byteLength / sizeof(Matrix4);

		for (int i = 0; i < count; ++i) {
			skinData.worldInverseBindPose[i] = *test;
			test++;
		}
	}
	else {
		//???
	}

	for (int i = 0; i < skin.joints.size(); ++i) {
		nodeToJointLookup.insert({ skin.joints[i], i });
	}

	for (int i = 0; i < skin.joints.size(); ++i) {
		auto& node = model.nodes[skin.joints[i]];
		jointNames.push_back(node.name);
		skinData.worldBindPose.push_back(worldNodeMatrices[skin.joints[i]]);
	}

	for (int i = 0; i < skin.joints.size(); ++i) {
		skinData.nodesUsed.push_back(skin.joints[i]);
		const auto& parent = parentChildNodeLookup.find(skin.joints[i]);
		if (parent == parentChildNodeLookup.end()) {
			jointParents.push_back(-1);
		}
		else {
			//now we need to go back the other way!
			int skinParent = nodeToJointLookup[parent->second];
			jointParents.push_back(skinParent);
		}
	}
	mesh->SetJointNames(jointNames);
	mesh->SetJointParents(jointParents);
	mesh->SetBindPose(skinData.worldBindPose);
	mesh->SetInverseBindPose(skinData.worldInverseBindPose);
	LoadAnimationData(model, mesh, skinData);
}

void GLTFLoader::LoadAnimationData(tinygltf::Model& model, MeshGeometry* mesh, GLTFSkin& skinData) {
	size_t jointCount = mesh->GetBindPose().size();
	vector<int> jointParents = mesh->GetJointParents();
	
	for (const auto& anim : model.animations) {
		float animLength = 0.0f;
		for (int i = 0; i < anim.samplers.size(); ++i) {
			int timeSrc = anim.samplers[i].input;
			animLength = std::max(animLength, (float)(model.accessors[timeSrc].maxValues[0]));
		}
		float time = 0.0f;
		float frameRate = 1.0f / 30.0f;
		unsigned int frameCount = 0;

		std::vector<Matrix4> allMatrices;

		size_t matrixStart = 0;
		while (time <= animLength) {
			allMatrices.resize(allMatrices.size() + jointCount);

			std::map<int, Vector3> frameJointTranslations;
			std::map<int, Vector3> frameJointScales;
			std::map<int, Quaternion> frameJointRotations;

			std::vector<int> inAnim(model.nodes.size(), 0);

			static int TRANSLATION_BIT	= 1;
			static int ROTATION_BIT		= 2;
			static int SCALE_BIT		= 4;

			for (const auto& channel : anim.channels) {
				const auto& sampler = anim.samplers[channel.sampler];
				const auto& input	= model.accessors[sampler.input];
				const auto& output	= model.accessors[sampler.output];

				int indexA	= 0;
				int indexB	= 0;
				float t		= 0.0f;

				int nodeID = channel.target_node; //seems correct!
				GetInterpolationData(time, indexA, indexB, t, input, model);

				if (channel.target_path == "translation") {
					frameJointTranslations.insert({ nodeID, GetInterpolatedVector<Vector3>(t, indexA, indexB, output, model) });
					inAnim[nodeID] |= TRANSLATION_BIT;
				}
				else if (channel.target_path == "rotation") {
					Quaternion q = GetSlerpQuaterion(t, indexA, indexB, output, model);
					frameJointRotations.insert({ nodeID, q });
					inAnim[nodeID] |= ROTATION_BIT;
				}
				else if (channel.target_path == "scale") {
					frameJointScales.insert({ nodeID, GetInterpolatedVector<Vector3>(t, indexA, indexB, output, model) });
					inAnim[nodeID] |= SCALE_BIT;
				}
			}

			for (int i = 0; i < skinData.nodesUsed.size(); ++i) {
				int nodeID = skinData.nodesUsed[i];

				int in = inAnim[nodeID];
				Matrix4 transform;
				if (in > 0) {
					Vector3 translation;
					Vector3 scale(1, 1, 1);
					Quaternion rotation;

					if (in & TRANSLATION_BIT) {
						translation = frameJointTranslations[nodeID];
					}
					if (in & ROTATION_BIT) {
						rotation = frameJointRotations[nodeID];
					}
					if (in & SCALE_BIT) {
						scale = frameJointScales[nodeID];
					}
					transform = Matrix4::Translation(translation) * Matrix4(rotation) * Matrix4::Scale(scale);

					int p = jointParents[i];
					if (p >= 0) {//It's a local transform!
						transform = allMatrices[matrixStart + p] * transform;
					}
				}
				else {
					transform = worldNodeMatrices[nodeID];
				}
				allMatrices[matrixStart + i] = transform;
			}
			time += frameRate;
			frameCount++;
			matrixStart += jointCount;
		}

		for (int i = 0; i < allMatrices.size(); ++i) {
			allMatrices[i] = skinData.globalTransformInverse * allMatrices[i];
		}

		outAnims.push_back(MeshAnimation((unsigned int)jointCount, frameCount, frameRate, allMatrices));
	}
}
