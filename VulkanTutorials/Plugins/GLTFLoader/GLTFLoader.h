#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include "../../Common/Assets.h"
#include "../../Common/Matrix4.h"

namespace tinygltf {
	class Model;
}

namespace NCL {
	class MeshGeometry;
	class MeshAnimation;

	namespace Rendering {
		class TextureBase;
	}

	namespace Assets {
		const std::string GLTFDIR(ASSETROOT + "GLTF/");
	}

	class GLTFLoader	{
	public:
		typedef std::function<NCL::MeshGeometry* (void)> MeshConstructionFunction;

		struct GLTFMaterialLayer {
			Rendering::TextureBase* diffuse;
			Rendering::TextureBase* bump;
			Rendering::TextureBase* occlusion;
			Rendering::TextureBase* emission;
			Rendering::TextureBase* metallic;

			GLTFMaterialLayer() {
				diffuse		= nullptr;
				bump		= nullptr;
				occlusion	= nullptr;
				emission	= nullptr;
				metallic	= nullptr;
			}
		};

		struct GLTFMaterial {
			std::vector< GLTFMaterialLayer > allLayers;
		};

		GLTFLoader() {};
		GLTFLoader(const std::string& filename, MeshConstructionFunction meshConstructor);

		~GLTFLoader();

		std::vector<MeshGeometry*> outMeshes;
		std::vector<Rendering::TextureBase*> outTextures;
		std::vector<GLTFMaterial> outMats;
		std::vector<MeshAnimation*> outAnims;	
		
	protected:
		struct GLTFSkin {
			std::vector<int>			nodesUsed;
			std::vector<Maths::Matrix4>	worldBindPose;
			std::vector<Maths::Matrix4> worldInverseBindPose;
			Maths::Matrix4 globalTransformInverse;
		};

		void LoadImages(tinygltf::Model& m, const std::string& rootFile);
		void LoadMaterials(tinygltf::Model& m);
		void LoadSceneNodeData(tinygltf::Model& m);

		void LoadVertexData(tinygltf::Model& m, GLTFLoader::MeshConstructionFunction meshConstructor);
		void LoadSkinningData(tinygltf::Model& m, MeshGeometry* geometry);
		void LoadAnimationData(tinygltf::Model& m, MeshGeometry* mesh, GLTFSkin& skin);

		std::map<int, int> parentChildNodeLookup;		
		std::vector<Maths::Matrix4> localNodeMatrices;
		std::vector<Maths::Matrix4> worldNodeMatrices;
		
		std::vector<GLTFMaterialLayer> fileMats;
	};
}
