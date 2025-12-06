#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;

out vec4 chCol;

uniform float amplitudes[6];
uniform float fretXNut[6];
uniform float fretXBridge[6];
uniform float time;
uniform float frequency;
uniform int vertsPerString;

void main()
{
    int lineIndex = (gl_VertexID / vertsPerString) % 6;

    float amp = amplitudes[lineIndex];
    float cutoffNut = fretXNut[lineIndex];
    float cutoffBridge = fretXBridge[lineIndex];

    float vibrateNut = (inPos.x < cutoffNut) ? 1.0 : 0.0;
    float vibrateBridge = (inPos.x > cutoffBridge) ? 1.0 : 0.0;
    float yOff = vibrateNut * vibrateBridge * amp * sin(time * frequency);

    gl_Position = vec4(inPos.x, inPos.y + yOff, 0.0, 1.0);

    chCol = vec4(inCol, 1.0);
}
