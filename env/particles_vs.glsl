#define pinfo(id) (texture(particleTex, uvec2((id), gl_InstanceID)).r)
#define id_x 0
#define id_y 1
#define id_z 2
#define id_scale 9

layout (location = 2) uniform mat4 model;
layout (location = 3) uniform sampler2DRect particleTex;
flat out float scale;

void main() {
    scale = pinfo(id_scale);
    gl_Position = model * vec4(pinfo(id_x), pinfo(id_y), pinfo(id_z), 1.0);
}
