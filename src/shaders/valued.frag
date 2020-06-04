#version 130

in vec4 _color;
in vec3 _light;
in vec3 _pos;

out vec4 color;


void main(void) {
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));
    float d = dot(n, _light);
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.1);
/*    float zebra = sin(_pos.y * 100.0) * 10.0;
    if (zebra < 0.0)
        zebra = 0.0;
    else if (zebra > 1.0)
        zebra = 1.0;
    color.r = 0.8;
    color.g = zebra * 0.8;
    color.b = zebra * 0.8;*/
}
