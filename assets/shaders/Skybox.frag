#version 450

layout (set = 1, binding = 1) uniform samplerCube colorSampler;

layout (location = 0) in vec3 inEyeDir;

layout (location = 0) out vec3 outFragColor;

void main()
{
	vec4 color = texture(colorSampler, inEyeDir);
	outFragColor = color.rgb;
}