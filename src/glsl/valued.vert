#version 130

in vec3 pos;
in float val;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;
uniform mat4 modelview;


void main(void) {
    vec4 modelview_pos = modelview * vec4(pos, 1.0);
    gl_Position = projection * modelview_pos;

    vec3 _modelview_pos;
    _modelview_pos.x = modelview_pos.x;
    _modelview_pos.y = modelview_pos.y;
    _modelview_pos.z = modelview_pos.z;

    float r;
    if (val < 0.25) {
        r = val / 0.25;
        _color.r = 0.0;
        _color.g = r;
        _color.b = 1.0;
    }
    else if (val < 0.5) {
        r = (val - 0.25) / 0.25;
        _color.r = 0.0;
        _color.g = 1.0;
        _color.b = 1.0 - r;
    }
    else if (val < 0.75) {
        r = (val - 0.5) / 0.25;
        _color.r = r;
        _color.g = 1.0;
        _color.b = 0.0;
    }
    else if (val < 2.0) {
        r = (val - 0.75) / 0.25;
        _color.r = 1.0;
        _color.g = 1.0 - r;
        _color.b = 0.0;
    }
    else {
        _color.r = 0.8;
        _color.g = 0.8;
        _color.b = 0.8;
    }
    _color.a = 1.0f;
    _light = normalize(_modelview_pos - vec3(1000, -1000, 1000));
    _pos = _modelview_pos;
}
