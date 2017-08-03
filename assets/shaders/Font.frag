#version 450

layout (set = 1, binding = 1) uniform sampler2D colorSampler;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec4 color = texture(colorSampler, inTexCoord, 1);
	outFragColor = vec4(color.r, color.r, color.r, color.r);
}