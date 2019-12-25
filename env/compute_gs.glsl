layout (points) in;
layout (triangle_strip, max_vertices = 4) out;
noperspective out float instance_f;
noperspective out float id_f;

void main() {
    instance_f = 0.0;
    id_f = 0.0;
    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    instance_f = 0.0;
    id_f = 1.0;
    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    instance_f = 1.0;
    id_f = 0.0;
    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    EmitVertex();

    instance_f = 1.0;
    id_f = 1.0;
    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
}
