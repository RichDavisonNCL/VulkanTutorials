#include "ShaderBase.h"
using namespace NCL;
using namespace Rendering;

ShaderBase::ShaderBase(const string& vertex, const string& fragment, const string& geometry, const string& domain, const string& hull)
{
	shaderFiles[(int)ShaderStages::Vertex]	= vertex;
	shaderFiles[(int)ShaderStages::Fragment]	= fragment;
	shaderFiles[(int)ShaderStages::Geometry]	= geometry;
	shaderFiles[(int)ShaderStages::Domain]	= domain;
	shaderFiles[(int)ShaderStages::Hull]		= hull;
}

ShaderBase::~ShaderBase()
{
}