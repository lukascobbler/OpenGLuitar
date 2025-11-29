#pragma once

#include <GL/glew.h>
#include <vector>
#include <cmath>
#include <string>

struct GuitarString
{
    float x0, y0, x1, y1;
    float thickness;
    float r, g, b;
    std::string name;

    float vibrationTime = 0.0f;
    float currentAmplitude = 0.0f;
    bool isVibrating = false;
    bool hasBeenTriggered = false;
};

float pointLineDistance(float px, float py, float x0, float y0, float x1, float y1);