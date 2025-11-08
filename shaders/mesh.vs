#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out float vHeight;  // pass height to fragment shader

void main() {
    vHeight = (uModel * vec4(aPos, 1.0)).y;   // store height
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
