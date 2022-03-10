#pragma once
#include <string>
using std::string;
namespace NCL::Rendering {
	enum class ShaderStages {
		Vertex,
		Fragment,
		Geometry,
		Domain,
		Hull,
		Mesh,
		Task,
		MAXSIZE,
		//Aliases
		TessControl = Domain,
		TessEval	= Hull,
	};

	class ShaderBase	{
	public:
		ShaderBase() {
		}
		ShaderBase(const string& vertex, const string& fragment, const string& geometry = "", const string& domain = "", const string& hull = "");
		virtual ~ShaderBase();

		virtual void ReloadShader() = 0;
	protected:

		string shaderFiles[(int)ShaderStages::MAXSIZE];
	};
}