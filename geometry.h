#ifndef GEO_GEOMETRY_H
#define GEO_GEOMETRY_H

#include "flux/types.h"

namespace geo
{
	struct Vertex
	{
		fx::vec4 pos;
		fx::vec3 col;
	};

	class Block
	{
	public:
		fx::vec3 _color;
		std::vector<Vertex> _vertices;
		std::vector<GLuint> _indices;
		std::vector<GLuint> _normals;

	public:
		Block(const fx::vec3 color)
			: _color{ color }
		{
			_vertices = {};
			_indices = {};
			_normals = {};
		}
	};

}

#endif
