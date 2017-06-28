#version 450

layout (location = 0) in vec3 inPos;

layout (set = 0, binding = 0) uniform ViewMatrices 
{
	mat4 projection;
	mat4 view;
} viewMatrices;

layout (set = 1, binding = 0) uniform ModelMatrix 
{
	mat4 model;
} modelMatrix;

layout (location = 0) out vec3 outEyeDir;

void main()
{
	mat4 modelViewMatrix = viewMatrices.view * modelMatrix.model;
	mat4 invProjMatrix = inverse(viewMatrices.projection);
	mat3 invModelViewMatrix = inverse(mat3(modelViewMatrix));
	vec3 unprojected = (invProjMatrix * vec4(inPos, 1)).xyz;
	outEyeDir = invModelViewMatrix * unprojected;
	gl_Position = vec4(inPos, 1);
}
