#version 330 core

out vec4 FragColor;
uniform vec4 ucolor;

void main()
{
    FragColor = ucolor;
}