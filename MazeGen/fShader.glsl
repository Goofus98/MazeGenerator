#version 430
in vec2 tc; // interpolated incoming texture coordinate
out vec4 color;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
layout (binding=0) uniform sampler2D s;
void main(void)
{
	color = texture(s, tc);
	//color = vec4(1.0, 0.0, 0.0, 1.0);
}