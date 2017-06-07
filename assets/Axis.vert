#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform Matrices
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} matrices;

void main()
{
	gl_Position = matrices.projectionMatrix * matrices.viewMatrix * matrices.modelMatrix * vec4(inPos.xyz, 1.0);
}
