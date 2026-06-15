#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <iostream>

#include "../NCLCoreClasses/Assets.h"
#include "../NCLCoreClasses/Mesh.h"
#include "../NCLCoreClasses/MeshAnimation.h"
#include "../NCLCoreClasses/Texture.h"
#include "../NCLCoreClasses/Matrix.h"

#include "../NCLCoreClasses/Vector.h"

namespace tinygltf {
	class Model;
}

namespace NCL::Assets {
	const std::string GLTFDIR(ASSETROOT + "GLTF/");
}

namespace NCL::Rendering {
	class MeshAnimation;
	class Texture;

	enum GLTFAlphaMode {
		Opaque,
		Mask,
		Cutoff
	};

	struct GLTFMaterial {
		SharedTexture albedo;
		SharedTexture bump;
		SharedTexture occlusion;
		SharedTexture emission;
		SharedTexture metallic;

		Vector4 albedoColour	= Vector4(1, 1, 1, 1);
		Vector3 emissionColour	= Vector3(0, 0, 0);
		float	metallicFactor	= 0.0f;
		float	roughnessFactor = 1.0f;
		float	alphaCutoff		= 0.5f;
		bool	doubleSided		= false;

		GLTFAlphaMode alphaMode = GLTFAlphaMode::Opaque;
		std::string name = "Unnamed Material Layer";
	};

	struct GLTFMeshMaterials {
		std::vector< int32_t > layers;
	};		

	struct GLTFNode {
		std::string		name;
		uint32_t		nodeID = 0;

		SharedMesh		mesh = nullptr;

		Matrix4			localMatrix;
		Matrix4			worldMatrix;

		int32_t			parent = -1;
		std::vector<int32_t> children;
	};	

	struct GLTFSkin {
		std::vector<std::string>	localJointNames;
		std::map<int, int>			sceneToLocalLookup;
		std::map<int, int>			localToSceneLookup;
		std::vector<Maths::Matrix4>	worldBindPose;
		std::vector<Maths::Matrix4> worldInverseBindPose;
		Maths::Matrix4				globalTransformInverse;
	};

	struct GLTFScene {
		std::vector<SharedMesh>			meshes;		
		std::vector<SharedMeshAnim>		animations;
		std::vector<SharedTexture>		textures;

		std::vector<GLTFMeshMaterials>	meshMaterials;
		std::vector<GLTFMaterial>		materials;

		std::map<int, GLTFSkin >		skinningData;

		std::vector<GLTFNode>			sceneNodes;

		std::vector<int32_t>			topLevelNodes;
	};

	class GLTFLoader	{
	public:
		typedef std::function<NCL::Rendering::SharedMesh (void)>				MeshConstructionFunction;
		typedef std::function<NCL::Rendering::SharedTexture (std::string&)>		TextureConstructionFunction;

		static bool Load(const std::string& filename, GLTFScene& intoScene);
		static void SetMeshConstructionFunction(MeshConstructionFunction func);
		static void SetTextureConstructionFunction(TextureConstructionFunction func);

	protected:		
		GLTFLoader()  = delete;
		~GLTFLoader() = delete;

		struct BaseState {
			uint32_t firstNode;
			uint32_t firstMesh;
			uint32_t firstTex;
			uint32_t firstMat;
			uint32_t firstAnim;
			uint32_t firstMatLayer;
		};

		static void LoadImages(tinygltf::Model& m, GLTFScene& scene, BaseState state, const std::string& rootFile, TextureConstructionFunction texFunc);
		static void LoadMaterials(tinygltf::Model& m, GLTFScene& scene, BaseState state);
		static void LoadSceneNodeData(tinygltf::Model& m, GLTFScene& scene, BaseState state);
		static void LoadVertexData(tinygltf::Model& m, GLTFScene& scene, BaseState state, GLTFLoader::MeshConstructionFunction meshConstructor);
		static void LoadSkinningData(tinygltf::Model& model, GLTFScene& scene, int32_t nodeID, int32_t skinID, BaseState state);
		static void LoadAnimationData(tinygltf::Model& m, GLTFScene& scene, BaseState state, Mesh& mesh, GLTFSkin& skin);

		static void LoadSceneAnimationData(tinygltf::Model& m, GLTFScene& scene, BaseState state);


		static void AssignNodeMeshes(tinygltf::Model& m, GLTFScene& scene, BaseState state);

		static MeshConstructionFunction		meshFunc;
		static TextureConstructionFunction	texFunc;
	};
}
