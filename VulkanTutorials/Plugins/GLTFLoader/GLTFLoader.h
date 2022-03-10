/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include "../../Common/MeshGeometry.h"
#include "../../Common/TextureBase.h"

#include "../../Common/MeshAnimation.h"

#include "../../Common/Matrix4.h"

namespace NCL {
	class MeshGeometry;
	class Rendering::TextureBase;
}

namespace tinygltf {
	class Model;
}

namespace NCL::Assets {
	const std::string GLTFDIR("../../Assets/GLTF/");
}

namespace NCL {
	class GLTFLoader	{
	public:
		typedef std::function<NCL::MeshGeometry* (void)> MeshConstructionFunction;

		struct GLTFMaterialLayer {
			NCL::Rendering::TextureBase* diffuse;
			NCL::Rendering::TextureBase* bump;
			NCL::Rendering::TextureBase* occlusion;
			NCL::Rendering::TextureBase* emission;
			NCL::Rendering::TextureBase* metallic;

			GLTFMaterialLayer() {
				diffuse = nullptr;
				bump = nullptr;
				occlusion = nullptr;
				emission = nullptr;
				metallic = nullptr;
			}
		};

		struct GLTFSkin {
			vector<int>			nodesUsed;
			vector<Matrix4>		worldBindPose;		
			std::vector<Matrix4> worldInverseBindPose;		
			NCL::Maths::Matrix4 globalTransformInverse;
		};

		struct GLTFMaterial {
			std::vector< GLTFMaterialLayer > allLayers;
		};
		GLTFLoader() {};
		GLTFLoader(const std::string& filename, MeshConstructionFunction meshConstructor);

		~GLTFLoader();

		vector<MeshGeometry*> outMeshes;
		vector<NCL::Rendering::TextureBase*> outTextures;
		vector<GLTFMaterial> outMats;
		vector<MeshAnimation> outAnims;	
		
		std::vector<GLTFMaterialLayer> fileMats;

		std::vector<Matrix4> localNodeMatrices;
		std::vector<Matrix4> worldNodeMatrices;		
		
	protected:
		void LoadImages(tinygltf::Model& m, const std::string& rootFile);
		void LoadMaterials(tinygltf::Model& m);
		void LoadSceneNodeData(tinygltf::Model& m);

		void LoadVertexData(tinygltf::Model& m, GLTFLoader::MeshConstructionFunction meshConstructor);
		void LoadSkinningData(tinygltf::Model& m, MeshGeometry* geometry);
		void LoadAnimationData(tinygltf::Model& m, MeshGeometry* mesh, GLTFSkin& skin);

		std::map<int, int> parentChildNodeLookup;
	};
}
