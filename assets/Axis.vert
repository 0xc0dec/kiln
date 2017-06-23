#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform ViewMatrices
{
	mat4 projection;
	mat4 view;
} viewMatrices;

layout (binding = 1) uniform ModelMatrix 
{
	mat4 model;
} modelMatrix;

void main()
{
	gl_Position = viewMatrices.projection * viewMatrices.view * modelMatrix.model * vec4(inPos.xyz, 1.0);
}
