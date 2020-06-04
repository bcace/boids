#version 130

in vec3 pos;
in vec3 color;

out vec4 _color;

uniform mat4 projection;
uniform mat4 modelview;


void main(void) {
    vec4 modelview_pos = modelview * vec4(pos, 1.0);
    gl_Position = projection * modelview_pos;

    _color = vec4(color, 1.0);
}
