#version 330 core

// In
in vec4 color;
in vec3 normal;
in vec3 crntPos;

uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

// Out
out vec4 FragColor; 

void main() {
	vec3 normal = normalize(normal);
	vec3 lightDirection = normalize(lightPos - crntPos);

	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// Smìr pohledu
	vec3 viewDirection = normalize(camPos - crntPos);
	
	// Odraz svìtla
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	
	// Specular
	float specularLight = 0.5f;
	float specularAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 5);
	float specular = specularAmount * specularLight;
	
	// Ambient
	float ambient = 0.2f;

	//			Barva	Barva svìtla (Složky svìtla               )
	FragColor = color * lightColor * (specular + diffuse + ambient);

}
