#version 330 core

in vec4 color; // smooth defaultnì
// flat in vec4 color; // bere first nebo last vertex barvu
//uniform vec4 color;
in vec3 normal;
in vec3 crntPos;

uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec4 FragColor; // Final output

void main() {
	float ambient = 0.2f;

	vec3 normal = normalize(normal);
	vec3 lightDirection = normalize(lightPos - crntPos);

	// Max aby nebylo negativni cislo
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	float specularLight = 0.5f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	// sila spekularni slozky (napr mocnina na 5)
	float specularAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 5);
	float specular = specularAmount * specularLight;

	FragColor = color * lightColor * (specular + diffuse + ambient);
}
