
#ifndef VERTEX_H
#define VERTEX_H
#include <glm/glm.hpp>
struct vertex
{
    glm::vec3 position; // Vertex pos
    glm::vec3 color; // Color
    glm::vec2 texCoor; // Texture coordinates
    glm::vec3 normal; // Normal used for light reflectivity
};

#endif