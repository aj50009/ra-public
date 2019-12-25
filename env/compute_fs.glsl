#define pinfo(id) (texture(particleTex, uvec2((id), instance)).r)
#define id_x 0
#define id_y 1
#define id_z 2
#define id_vx 3
#define id_vy 4
#define id_vz 5
#define id_ax 6
#define id_ay 7
#define id_az 8
#define id_scale 9
#define id_initialx 10
#define id_initialy 11
#define id_initialz 12
#define id_initialvx 13
#define id_initialvy 14
#define id_initialvz 15

noperspective in float instance_f;
noperspective in float id_f;
layout (location = 3) uniform sampler2DRect particleTex;
layout (location = 5) uniform float deltaTime;
layout (location = 6) uniform float minY;
out float value;

void main() {
    uint instance = uint(instance_f * nparticles), id = uint(id_f * szparticle);
    switch (id) {
    case id_x: case id_y: case id_z: case id_vx: case id_vy: case id_vz:
        if (pinfo(id_y) < minY)
            value = pinfo(id + 10);
        else
            value = pinfo(id) + pinfo(id + 3) * deltaTime;
        break;
    default:
        value = pinfo(id);
    }
}
