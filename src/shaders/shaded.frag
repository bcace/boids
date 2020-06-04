#version 130

in vec4 _color;
in vec3 _light;
in vec3 _pos;

out vec4 color;


void main(void) {
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));
    float d = dot(n, _light);
    float f = 0.0;
    if (_pos.y < 0.0)
        f = 0.1;
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.1 + f);
}
