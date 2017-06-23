#version 450

layout (binding = 2) uniform sampler2D colorSampler;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec3 outFragColor;

void main()
{
	vec4 color = texture(colorSampler, inTexCoord, 1);
	outFragColor = color.rgb;
}