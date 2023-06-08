
#ifndef VERTEX_H
#define VERTEX_H
#include <glm/glm.hpp>
struct vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;   // Normals are used for light reflectivity
    glm::vec2 texCoor;  // Texture coordinates

};

#endif