layout (location = 0) in vec2 pos;
layout (location = 1) in vec3 color;
noperspective out vec3 _color;

void main() {
    _color = color;
    gl_Position = vec4(pos, 1.0, 1.0);
}
