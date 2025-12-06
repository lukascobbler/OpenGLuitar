#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h> 
#include <vector>
#include "GuitarString.h"

unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
void preprocessTexture(unsigned& texture, const char* filepath);
GLFWcursor* loadImageToCursor(const char* filePath);
GLFWwindow* createFullScreenWindow(const char* windowName);
float pointLineDistance(float px, float py, float x0, float y0, float x1, float y1);
void findClosestStringAndFret(float mouseXNDC, float mouseYNDC, const std::vector<GuitarString>& strings,
    int& outStringIndex, int& outFretIndex, float& outDistUp);