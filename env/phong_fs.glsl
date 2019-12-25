#define DEFAULT_DIFF_COLOR vec3(0.7, 0.7, 0.7)
#define DEFAULT_TNG_SPC_NORM vec3(0.0, 0.0, 1.0)
#define DEFAULT_SPEC_STRENGTH 0.0

#define AMBIENT_INTENSITY 0.3
#define SPECULAR_POWER 32.0
#define DIR_LIGHT_INTENSITY 0.7

#define GAMMA 2.2

in vec3 tngSpcFragPos;
in vec3 tngSpcCamPos;
in vec3 tngSpcLightPos;
in vec2 uv_;
layout (location = 5) uniform sampler2D texdiff;
layout (location = 6) uniform sampler2D texnorm;
layout (location = 7) uniform sampler2D texspec;
layout (location = 8) uniform bool usetexdiff;
layout (location = 9) uniform bool usetexnorm;
layout (location = 10) uniform bool usetexspec;
out vec4 color;

void main() {
    vec3 diffColor;
    if (usetexdiff)
        diffColor = pow(texture(texdiff, uv_).rgb, vec3(GAMMA)); /* sRGB to linear RGB */
    else
        diffColor = DEFAULT_DIFF_COLOR;

    vec3 tngSpcNorm;
    if (usetexnorm)
        tngSpcNorm = normalize(2.0 * texture(texnorm, uv_).xyz - 1.0);
    else
        tngSpcNorm = DEFAULT_TNG_SPC_NORM;

    float specStrength;
    if (usetexspec)
        specStrength = texture(texspec, uv_).r;
    else
        specStrength = DEFAULT_SPEC_STRENGTH;

    vec3 tngSpcCamDir = normalize(tngSpcCamPos - tngSpcFragPos);
    vec3 tngSpcLightDir = normalize(tngSpcLightPos - vec3(0.0)); /* directional light instead of point light */

    float ambientFactor = AMBIENT_INTENSITY;
    float specFactor = specStrength * pow(max(0.0, dot(tngSpcCamDir, reflect(-tngSpcLightDir, tngSpcNorm))), SPECULAR_POWER);
    float diffuseFactor = DIR_LIGHT_INTENSITY * max(0.0, dot(tngSpcNorm, tngSpcLightDir));

    vec3 linearColor = (ambientFactor + specFactor + diffuseFactor) * diffColor;
    color = vec4(pow(linearColor, vec3(1.0 / GAMMA)), 1.0); /* linear RGB to sRBB */
}
