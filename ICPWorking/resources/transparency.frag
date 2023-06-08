#version 330 core

// In
in vec4 color;
in vec3 normal;
in vec3 crntPos;

uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform float transparency; // Transparency uniform

// Out
out vec4 FragColor; 

void main() {
	vec3 normal = normalize(normal);
	vec3 lightDirection = normalize(lightPos - crntPos);

	// Max, aby nebylo negative
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// Sm�r pohledu
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);

	// Specular
	float specularLight = 0.5f;
	float specularAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 5);
	float specular = specularAmount * specularLight;

	// Ambient
	float ambient = 0.2f;

	//			      Barva   Barva sv�tla (Slo�ky sv�tla               )
	vec4 finalColor = color * lightColor * (specular + diffuse + ambient);
	
	// Nastaven� alphy (pr�hlednosti)
	finalColor.a = transparency; 

	FragColor = finalColor;
}
