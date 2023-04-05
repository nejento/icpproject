#pragma once
#ifndef OBJloader_H
#define OBJloader_H

#include <vector>
#include <glm/fwd.hpp>
#include "vertex.h"

bool loadOBJ(
	const char * path,
	std::vector < glm::vec3 > & out_vertices,
	std::vector < glm::vec2 > & out_uvs,
	std::vector < glm::vec3 > & out_normals
);

bool loadOBJ(
	const char* path,
	std::vector <vertex>& out_vertices,
	std::vector <GLuint>& out_indices,
	glm::vec3 scale
);

#endif
