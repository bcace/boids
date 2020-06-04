#version 130

in vec3 position;
in vec3 norm1;
in vec3 norm2;

out vec4 _color;

uniform mat4 projection;
uniform mat4 modelview;
uniform vec4 color;
uniform vec3 camera_pos;


void main(void) {
    vec4 _pos = modelview * vec4(position, 1.0);
    gl_Position = projection * _pos;

    _color = color;

    vec3 c = camera_pos - vec3(_pos);
    if (sign(dot(norm1, c)) == sign(dot(norm2, c)))
        _color.a = 0.0;
}
