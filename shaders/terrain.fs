#version 330 core
in vec3 vWorldPos;
in vec3 vWorldNormal;
in float vHeight;
in vec4 vLightSpacePos; // unused here

out vec4 FragColor;

// Lighting
uniform vec3 uCamPos;
uniform vec3 uLightDir = normalize(vec3(-0.5, -1.0, -0.3));
uniform vec3 uLightColor = vec3(1.0);
uniform vec3 uAmbientColor = vec3(0.25);

// Artistic controls
uniform float uMinHeight;
uniform float uMaxHeight;

// Simple albedo: gradient by height (green->rock->snow)
vec3 heightColor(float h) {
    float t = clamp(h / 500.0, 0.0, 1.0);
    vec3 grass = vec3(0.18, 0.32, 0.12);
    vec3 rock  = vec3(0.35, 0.34, 0.33);
    vec3 snow  = vec3(0.92, 0.94, 0.97);
    // three-way blend using smoothsteps around mid heights
    float snowMask = smoothstep(0.65, 0.85, t);
    float rockMask = smoothstep(0.15, 0.65, t) * (1.0 - snowMask);
    float grassMask = 1.0 - rockMask - snowMask;
    return grassMask * grass + rockMask * rock + snowMask * snow;
}

void main() {
    vec3 N = normalize(vWorldNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float diffuse = NdotL;
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.15;

    vec3 albedo = heightColor(vHeight);

    vec3 lit = uAmbientColor * albedo + uLightColor * (albedo * diffuse + spec);
    FragColor = vec4(lit, 1.0);
}
