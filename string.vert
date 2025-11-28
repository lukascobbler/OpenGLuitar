#version 330 core
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;

out vec4 chCol;

uniform float amplitudes[6];
uniform float time;
uniform float frequency;

void main()
{
    int lineIndex = gl_VertexID / 6;
    float amplitude = amplitudes[lineIndex];

    vec2 offset = vec2(0.0, amplitude * sin(time * frequency));

    gl_Position = vec4(inPos + offset, 0.0, 1.0);
    chCol = vec4(inCol, 1.0);
}