#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec3 outEyeDir;

void main()
{
	mat4 modelViewMatrix = ubo.viewMatrix * ubo.modelMatrix;
	mat4 invProjMatrix = inverse(ubo.projectionMatrix);
	mat3 invModelViewMatrix = inverse(mat3(modelViewMatrix));
	vec3 unprojected = (invProjMatrix * vec4(inPos, 1)).xyz;
	outEyeDir = invModelViewMatrix * unprojected;
	gl_Position = vec4(inPos, 1);
}
