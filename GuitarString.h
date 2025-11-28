#pragma once

#include <GL/glew.h>
#include <vector>
#include <cmath>

struct Line2D
{
    float x0, y0, x1, y1;
    float thickness;
    float r, g, b;

    float vibrationTime = 0.0f;
    float currentAmplitude = 0.0f;
    bool isVibrating = false;
};

inline float pointLineDistance(float px, float py, float x0, float y0, float x1, float y1)
{
    // calculating the distance from a line using the normalized projection factor formula
    float dx = x1 - x0; float dy = y1 - y0;
    float lenSq = dx * dx + dy * dy;
    float t = ((px - x0) * dx + (py - y0) * dy) / lenSq;
    t = std::fmax(0.0f, std::fmin(1.0f, t));
    float projX = x0 + t * dx;
    float projY = y0 + t * dy;
    dx = px - projX;
    dy = py - projY;
    return std::sqrt(dx * dx + dy * dy);
}

inline void formLinesVAO(const std::vector<Line2D>& lines, unsigned int& VAO, unsigned int& vertexCount)
{
    std::vector<float> vertices;

    for (const auto& line : lines)
    {
        float dx = line.x1 - line.x0; float dy = line.y1 - line.y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0) continue;
        float px = -dy / len; float py = dx / len;
        float hx = px * line.thickness * 0.5f; float hy = py * line.thickness * 0.5f;

        float rectVertices[30] = {
            line.x0 - hx, line.y0 - hy, line.r, line.g, line.b,
            line.x1 - hx, line.y1 - hy, line.r, line.g, line.b,
            line.x1 + hx, line.y1 + hy, line.r, line.g, line.b,
            line.x0 - hx, line.y0 - hy, line.r, line.g, line.b,
            line.x1 + hx, line.y1 + hy, line.r, line.g, line.b,
            line.x0 + hx, line.y0 + hy, line.r, line.g, line.b
        };
        vertices.insert(vertices.end(), rectVertices, rectVertices + 30);
    }

    vertexCount = vertices.size() / 5;

    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}
