#version 450

layout (binding = 0) uniform sampler2D colorSampler;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec3 outFragColor;

void main()
{
	vec4 color = texture(colorSampler, inTexCoord, 1);
	float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
	outFragColor = vec3(gray, gray, gray);
}