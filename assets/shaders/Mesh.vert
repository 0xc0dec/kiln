#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (set = 0, binding = 0) uniform ViewMatrices
{
	mat4 projection;
	mat4 view;
} viewMatrices;

layout (set = 1, binding = 0) uniform ModelMatrix 
{
	mat4 model;
} modelMatrix;

layout (location = 0) out vec2 outTexCood;

void main()
{
	outTexCood = inTexCoord;
	gl_Position = viewMatrices.projection * viewMatrices.view * modelMatrix.model * vec4(inPos.xyz, 1.0);
}
