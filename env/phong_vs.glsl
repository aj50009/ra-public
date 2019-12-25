layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 norm;
layout (location = 3) in vec3 tng;
layout (location = 4) in vec3 bitng;
layout (location = 0) uniform mat4 projViewModel;
layout (location = 1) uniform mat3 modelNormal;
layout (location = 2) uniform mat4 model;
layout (location = 3) uniform vec3 campos;
layout (location = 4) uniform vec3 lightpos;
out vec3 tngSpcFragPos;
out vec3 tngSpcCamPos;
out vec3 tngSpcLightPos;
out vec2 uv_;

void main() {
    vec3 worldNorm = normalize(modelNormal * norm);
    vec3 worldTng = normalize(modelNormal * tng);
    vec3 worldBitng = normalize(modelNormal * bitng);
    mat3 tngSpcMat = transpose(mat3(worldTng, worldBitng, worldNorm));

    tngSpcFragPos = tngSpcMat * (model * vec4(pos, 1.0)).xyz;
    tngSpcCamPos = tngSpcMat * campos;
    tngSpcLightPos = tngSpcMat * lightpos;

    uv_ = uv;

    gl_Position = projViewModel * vec4(pos, 1.0);
}
