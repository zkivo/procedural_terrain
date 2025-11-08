#version 330 core
in float vHeight;
out vec4 FragColor;

uniform float uMinY;
uniform float uMaxY;

void main()
{
    // Normalize height to [0,1]
    //float denom = max(uMaxY - uMinY, 1e-6);
    //float t = clamp((vHeight - uMinY) / denom, 0.0, 1.0);
    float t = ((vHeight + 250.0) / 500.0);

    // Smooth 2-color gradient: blue (low) → red (high)
    vec3 low  = vec3(0.0, 0.0, 1.0);  // blue
    vec3 high = vec3(1.0, 0.0, 0.0);  // red
    vec3 color = mix(low, high, t);

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(vec3(t), 1.0); // should show a smooth grayscale
}
