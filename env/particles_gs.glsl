layout (points) in;
layout (triangle_strip, max_vertices = 4) out;
flat in float scale[];
layout (location = 0) uniform mat4 proj;
layout (location = 1) uniform mat4 view;
out vec2 uv;

void main() {
    vec4 viewpos = view * gl_in[0].gl_Position;

    uv = vec2(0.0, 0.0);
    gl_Position = proj * (viewpos + scale[0] * vec4(-0.5, -0.5, 0.0, 0.0));
    EmitVertex();

    uv = vec2(1.0, 0.0);
    gl_Position = proj * (viewpos + scale[0] * vec4(0.5, -0.5, 0.0, 0.0));
    EmitVertex();

    uv = vec2(0.0, 1.0);
    gl_Position = proj * (viewpos + scale[0] * vec4(-0.5, 0.5, 0.0, 0.0));
    EmitVertex();

    uv = vec2(1.0, 1.0);
    gl_Position = proj * (viewpos + scale[0] * vec4(0.5, 0.5, 0.0, 0.0));
    EmitVertex();
}
