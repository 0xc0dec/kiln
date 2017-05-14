#version 450

layout (location = 0) in vec2 inPos;

layout (binding = 0) uniform UBO 
{
	vec3 color;
} ubo;

layout (location = 0) smooth out vec3 outColor;

void main()
{
	gl_Position = vec4(inPos.xy, 0.0, 1.0);
	outColor = ubo.color;
}
