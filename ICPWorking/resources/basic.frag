#version 330 core

uniform vec4 material;
out vec4 FragColor;

void main()
{
    FragColor = material;
}