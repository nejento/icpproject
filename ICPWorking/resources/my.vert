#version 330 core

layout (location = 0) in vec3 aPos;    // Coords
layout (location = 1) in vec3 aColor;  // Color RGB
layout (location = 2) in vec3 aNormal; // Normal

uniform mat4 uP_m;
uniform mat4 uV_m;
uniform mat4 uM_m;

// Outputs to fragment shader
out vec4 color;
out vec3 normal;
out vec3 crntPos;

void main() {
	crntPos = aPos;

	// Outputs coordinates of all vertices
	gl_Position = uP_m * uV_m * uM_m * vec4(crntPos, 1.0f);
	
	color = vec4(aColor, 1.0f);
	normal = aNormal;
}
