#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNormal;

out vec2 texcoord;
out vec3 normal;
out vec3 crntPos;

uniform mat4 uP_m;
uniform mat4 uV_m;
uniform mat4 uM_m;

void main()
{
    crntPos = aPos;
    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * uV_m * uM_m * vec4(crntPos, 1.0f);
    texcoord = aTex;
	normal = aNormal;
}