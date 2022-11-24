#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uProj_m = mat4(1.0f);
uniform mat4 uMV_m = mat4(1.0f);

out vec4 color;

void main()
{
    // Outputs the positions/coordinates of all vertices
    gl_Position = uProj_m * uMV_m * vec4(aPos, 1.0f);
    color = vec4(aColor,1.0f); //barvy a alpha
}