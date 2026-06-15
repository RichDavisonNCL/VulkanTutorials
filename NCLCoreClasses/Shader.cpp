#include "Shader.h"
using namespace NCL;
using namespace Rendering;

using std::string;

Shader::Shader(const string& vertex, const string& fragment, const string& geometry, const string& domain, const string& hull)
{
	m_shaderFiles[ShaderStages::Vertex]	= vertex;
	m_shaderFiles[ShaderStages::Fragment]	= fragment;
	m_shaderFiles[ShaderStages::Geometry]	= geometry;
	m_shaderFiles[ShaderStages::Domain]	= domain;
	m_shaderFiles[ShaderStages::Hull]		= hull;
}

Shader::~Shader()
{
}