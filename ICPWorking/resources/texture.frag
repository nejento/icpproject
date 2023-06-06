#version 330 core

in vec2 texcoord;
in vec3 normal;
in vec3 crntPos;

uniform sampler2D tex0; // texture unit from C++
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec4 FragColor; // final output

void main()
{
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

	FragColor = texture(tex0, texcoord) * lightColor * (specular + diffuse + ambient);
}