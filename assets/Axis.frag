#version 450

layout (binding = 2) uniform Variables 
{
	vec3 color;
} variables;

layout (location = 0) out vec3 outFragColor;

void main()
{
	outFragColor = variables.color;
}