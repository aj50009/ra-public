#define BUMP_INTENSITY 1.5

in vec2 uv;
layout (location = 4) uniform sampler2D flakeTex;
out vec4 color;

void main() {
    vec4 flakeCol = texture(flakeTex, uv);
    if (flakeCol.a < 0.4)
        discard;
    color = vec4(BUMP_INTENSITY * flakeCol.rgb, flakeCol.a);
}
