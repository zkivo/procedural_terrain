#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

// For shadow mapping (optional)
uniform mat4 uLightSpaceMatrix;

out vec3 vWorldPos;
out vec3 vWorldNormal;
out float vHeight;
out vec4 vLightSpacePos; // used only if you bind uLightSpaceMatrix

void main() {
    vec4 wpos = uModel * vec4(aPos, 1.0);
    vWorldPos = wpos.xyz;
    vWorldNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vHeight = wpos.y;
    vLightSpacePos = uLightSpaceMatrix * wpos;
    gl_Position = uProj * uView * wpos;
}
