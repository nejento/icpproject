#version 330 core

// In
in vec2 texcoord;
in vec3 normal;
in vec3 crntPos;

uniform sampler2D tex0; // texture unit from C++
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

// Out
out vec4 FragColor; 

void main()
{
	vec3 normal = normalize(normal);
	vec3 lightDirection = normalize(lightPos - crntPos);

	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// Odraz svìtla
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);

	// Specular
	float specularLight = 0.5f;
	float specularAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 5);
	float specular = specularAmount * specularLight;

	// Ambient
	float ambient = 0.2f;

	//			Textura	                  Barva svìtla (Složky svìtla               )
	FragColor = texture(tex0, texcoord) * lightColor * (specular + diffuse + ambient);
}