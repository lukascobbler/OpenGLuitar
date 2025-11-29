#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
void preprocessTexture(unsigned& texture, const char* filepath);
GLFWcursor* loadImageToCursor(const char* filePath);
GLFWwindow* createFullScreenWindow(const char* windowName);
float pointLineDistance(float px, float py, float x0, float y0, float x1, float y1);