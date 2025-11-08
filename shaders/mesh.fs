#version 330 core
in float vHeight;
out vec4 FragColor;

uniform float uMinY;
uniform float uMaxY;

void main()
{
    float t = (vHeight - uMinY) / (uMaxY - uMinY);
    t = clamp(t, 0.0, 1.0);

    // Map: low = blue, high = red
    vec3 color = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), t);

    FragColor = vec4(color, 1.0);
}
