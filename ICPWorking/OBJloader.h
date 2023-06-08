#pragma once
#ifndef OBJloader_H
#define OBJloader_H

#include <vector>
#include <glm/fwd.hpp>
#include "vertex.h"

bool loadOBJ(
	const char* path, 
	std::vector <vertex>& out_vertices, 
	std::vector <GLuint>& indices, 
	glm::vec3 color, 
	glm::vec3 scale, 
	glm::vec3 coords
);

#endif
