/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include <cstdint>
#include "Vector.h"
#include "Matrix.h"

namespace NCL::Rendering {
	class RendererBase;
	
	using namespace NCL::Maths;

	namespace GeometryPrimitive {
		enum Type : uint32_t {
			Points,
			Lines,
			Triangles,
			TriangleFan,
			TriangleStrip,
			Patches,
			MAX_PRIM
		};
	};

	namespace VertexAttribute {
		enum Type : uint32_t {
			Positions,
			Colours,
			TextureCoords,
			Normals,
			Tangents,
			JointWeights,
			JointIndices,
			MAX_ATTRIBUTES
		};	
		
		const std::string Names[VertexAttribute::MAX_ATTRIBUTES] = {
			std::string("Positions"),
			std::string("Colours"),
			std::string("Tex Coords"),
			std::string("Normals"),
			std::string("Tangents"),
			std::string("Joint Weights"),
			std::string("Joint Indices")
		};
	};

	struct SubMesh {
		int start = 0;
		int count = 0;
		int base  = 0;
	};

	class Mesh	{
	public:		
		virtual ~Mesh();

		GeometryPrimitive::Type GetPrimitiveType() const {
			return primType;
		}

		void SetPrimitiveType(GeometryPrimitive::Type type) {
			primType = type;
		}

		size_t GetPrimitiveCount(size_t subMesh) const {
			if (subMesh >= subMeshes.size()) {
				return 0;
			}

			size_t entryCount = subMeshes[subMesh].count;

			switch (GetPrimitiveType()) {
				case GeometryPrimitive::Points:		return entryCount;
				case GeometryPrimitive::Lines:			return entryCount / 2;
				case GeometryPrimitive::Triangles:		return entryCount / 3;
				case GeometryPrimitive::TriangleFan:	return 0;
				case GeometryPrimitive::TriangleStrip:	return 0;
				case GeometryPrimitive::Patches:		return 0;
				default: return 0;
			}
			return 0;
		}

		size_t GetPrimitiveCount() const {
			size_t vertCount	= GetVertexCount();
			size_t indexCount	= GetIndexCount();

			size_t entryCount = indexCount ? indexCount : vertCount;

			switch(GetPrimitiveType()) {
				case GeometryPrimitive::Points:			return entryCount;
				case GeometryPrimitive::Lines:			return entryCount / 2;
				case GeometryPrimitive::Triangles:		return entryCount / 3;
				case GeometryPrimitive::TriangleFan:	return 0;
				case GeometryPrimitive::TriangleStrip:	return 0;
				case GeometryPrimitive::Patches:		return 0;
				default: return 0;
			}
			return 0;
		}

		size_t GetVertexCount() const {
			return positions.size();
		}

		size_t GetIndexCount()  const {
			return indices.size();
		}

		size_t GetJointCount() const {
			return jointNames.size();
		}

		size_t GetSubMeshCount() const {
			return subMeshes.size();
		}

		const SubMesh* GetSubMesh(unsigned int i) const {
			if (i > subMeshes.size()) {
				return nullptr;
			}
			return &subMeshes[i];
		}

		void AddSubMesh(SubMesh sub, const std::string& newName = "") {
			subMeshes.push_back(sub);
			subMeshNames.push_back(newName);
		}

		void AddSubMesh(int startIndex, int indexCount, int baseVertex, const std::string& newName = "") {
			SubMesh m;
			m.base = baseVertex;
			m.count = indexCount;
			m.start = startIndex;

			subMeshes.push_back(m);
			subMeshNames.push_back(newName);
		}

		int GetIndexForJoint(const std::string &name) const;

		const std::vector<Matrix4>& GetBindPose() const {
			return bindPose;
		}
		const std::vector<Matrix4>& GetInverseBindPose() const {
			return inverseBindPose;
		}
		void SetSubMeshes(const std::vector < SubMesh>& meshes);
		void SetSubMeshNames(const std::vector < std::string>& newnames);

		void SetJointNames(const std::vector < std::string > & newnames);
		void SetJointParents(const std::vector<int>& newParents);
		void SetBindPose(const std::vector<Matrix4>& newMats);
		void SetInverseBindPose(const std::vector<Matrix4>& newMats);
		void CalculateInverseBindPose();

		bool	GetVertexIndicesForTri(unsigned int i, unsigned int& a, unsigned int& b, unsigned int& c) const;
		bool	GetTriangle(unsigned int i, Vector3& a, Vector3& b, Vector3& c) const;
		bool	GetNormalForTri(unsigned int i, Vector3& n) const;
		bool	HasTriangle(unsigned int i) const;

		const std::vector<Vector3>&		GetPositionData()		const { return positions;	}
		const std::vector<Vector2>&		GetTextureCoordData()	const { return texCoords;	}
		const std::vector<Vector4>&		GetColourData()			const { return colours;		}
		const std::vector<Vector3>&		GetNormalData()			const { return normals;		}
		const std::vector<Vector4>&		GetTangentData()		const { return tangents;	}
		const std::vector<Vector4>&		GetSkinWeightData()		const { return skinWeights; }
		const std::vector<Vector4i>&	GetSkinIndexData()		const { return skinIndices; }

		const std::vector<int>& GetJointParents()	const {
			return jointParents;
		}

		const std::vector<std::string>& GetJointNames()	const {
			return jointNames;
		}

		const std::vector<std::string>& GetSubMeshNames()	const {
			return subMeshNames;
		}

		const std::vector<unsigned int>& GetIndexData()			const { return indices;		}

		void SetVertexPositions(const std::vector<Vector3>& newVerts);
		void SetVertexTextureCoords(const std::vector<Vector2>& newTex);

		void SetVertexColours(const std::vector<Vector4>& newColours);
		void SetVertexNormals(const std::vector<Vector3>& newNorms);
		void SetVertexTangents(const std::vector<Vector4>& newTans);
		void SetVertexIndices(const std::vector<unsigned int>& newIndices);

		void SetVertexSkinWeights(const std::vector<Vector4>& newSkinWeights);
		void SetVertexSkinIndices(const std::vector<Vector4i>& newSkinIndices);

		void SetDebugName(const std::string& debugName);

		virtual void UploadToGPU(Rendering::RendererBase* renderer = nullptr) = 0;

		uint32_t GetAssetID() const {
			return assetID;
		}

		void SetAssetID(uint32_t newID) {
			assetID = newID;
		}

	protected:
		Mesh();

		virtual bool ValidateMeshData();

		GeometryPrimitive::Type		primType;
		std::string					debugName;
		uint32_t					assetID;

		std::vector<Vector3>		positions;
		std::vector<Vector2>		texCoords;
		std::vector<Vector4>		colours;
		std::vector<Vector3>		normals;
		std::vector<Vector4>		tangents;
		std::vector<unsigned int>	indices;
		std::vector<SubMesh>		subMeshes;
		std::vector<std::string>	subMeshNames;

		std::vector<Vector4>		skinWeights;	//Allows us to have 4 weight skinning 
		std::vector<Vector4i>		skinIndices;
		std::vector<std::string>	jointNames;
		std::vector<int>			jointParents;
		std::vector<Matrix4>		bindPose;
		std::vector<Matrix4>		inverseBindPose;
	};

	using UniqueMesh = std::unique_ptr<Mesh>;
	using SharedMesh = std::shared_ptr<Mesh>;

};