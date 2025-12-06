#include "GuitarString.h"

void GuitarString::computeFretMiddles()
{
    this->fretMiddles.clear();
    this->fretMiddles.reserve(21);

    float nutX = this->x0 - 0.122f;
    float bridgeX = this->x1;
    float y = this->y0;

    this->fretMiddles.push_back({ -1.0f, bridgeX - 0.004f, y }); // Fret 0

    float prevX = bridgeX;
    for (int n = 0; n <= 20; n++)
    {
        float f = 1.0f - 1.0f / std::pow(2.0f, n / 12.6f);
        float x = bridgeX + f * (nutX - bridgeX);
        if (n > 0) this->fretMiddles.push_back({ (float)(n - 1), (prevX + x) * 0.5f, y });
        prevX = x;
    }
}
