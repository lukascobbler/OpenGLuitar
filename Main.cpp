#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

#include "Util.h"
#include "GuitarString.h"
#include "Audio.h"

#define NOMINMAX
#include <windows.h>

// window
#define FPS 75
int width, height;
float aspectRatio;

// textures
unsigned guitarTexture;
unsigned signatureTexture;

// strings related stuff
std::vector<GuitarString> strings;
std::string lastHitStringName = "";
int lastHitFret = 0;
#define STRING_SEGMENTS 256
#define MAXIMUM_AMPLITUDE 0.008f
#define DECAY_RATE 2.3f
#define MARKER_RADIUS 0.013

// mouse related stuff
boolean isPressedLeft;
boolean isPressedRight;
GLFWcursor* cursorReleased;
GLFWcursor* cursorPressed;
double mouseXNDC = 0.0, mouseYNDC = 0.0;

// keyboard related stuff
bool keyStates[1024] = { false };

// frame limiting
double lastTimeForRefresh;

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

void limitFPS()
{
    double now = glfwGetTime();
    double targetFrameTime = 1.0 / FPS;
    double remaining = (lastTimeForRefresh + targetFrameTime) - now;

    if (remaining > 0.0)
    {
        glfwWaitEventsTimeout(remaining);
    }
    else
    {
        glfwPollEvents();
    }

    lastTimeForRefresh = glfwGetTime();
}

void formRectVAO(float* verticesRect, size_t rectSize, unsigned int& VAOrect) {
    unsigned int VBOrect;
    glGenVertexArrays(1, &VAOrect);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOrect);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, rectSize, verticesRect, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void formStringsVAO(const std::vector<GuitarString>& strings, unsigned int& VAO, unsigned int& vertexCount)
{
    std::vector<float> vertices;

    for (const auto& string : strings)
    {
        float x0 = string.x0, y0 = string.y0;
        float x1 = string.x1, y1 = string.y1;

        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0) continue;

        float px = -dy / len;
        float py = dx / len;
        float hx = px * string.thickness * 0.5f;
        float hy = py * string.thickness * 0.5f;

        for (int i = 0; i < STRING_SEGMENTS; i++)
        {
            float t0 = (float)i / STRING_SEGMENTS;
            float t1 = (float)(i + 1) / STRING_SEGMENTS;

            float sx0 = x0 + dx * t0;
            float sy0 = y0 + dy * t0;

            float sx1 = x0 + dx * t1;
            float sy1 = y0 + dy * t1;

            float quad[30] = {
                sx0 - hx, sy0 - hy, string.r, string.g, string.b,
                sx1 - hx, sy1 - hy, string.r, string.g, string.b,
                sx1 + hx, sy1 + hy, string.r, string.g, string.b,

                sx0 - hx, sy0 - hy, string.r, string.g, string.b,
                sx1 + hx, sy1 + hy, string.r, string.g, string.b,
                sx0 + hx, sy0 + hy, string.r, string.g, string.b
            };

            vertices.insert(vertices.end(), quad, quad + 30);
        }
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

void formUnitCircleVAO(unsigned int& VAO, unsigned int& vertexCount, int segments = 256) {
    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 2);

    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / segments;
        vertices.push_back(cosf(theta));
        vertices.push_back(sinf(theta));
    }

    unsigned int VBOcircle;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBOcircle);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBOcircle);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
        vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    vertexCount = vertices.size() / 2;
}

void drawRect(unsigned int rectShader, unsigned int VAOrect, unsigned int texture) {
    glUseProgram(rectShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(VAOrect);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawFretCircle(unsigned int circleShader, unsigned int unitCircleVAO, 
    unsigned int unitCircleVertexCount, float cx, float cy, float r)
{
    glUseProgram(circleShader);

    glUniform1f(glGetUniformLocation(circleShader, "centerX"), cx);
    glUniform1f(glGetUniformLocation(circleShader, "centerY"), cy);
    glUniform1f(glGetUniformLocation(circleShader, "radius"), r);
    glUniform1f(glGetUniformLocation(circleShader, "aspectRatio"), aspectRatio);

    glBindVertexArray(unitCircleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, unitCircleVertexCount);
    glBindVertexArray(0);
}

void drawStrings(unsigned int stringShader, unsigned int stringsVAO, unsigned int stringsVertexCount)
{
    float time = (float)glfwGetTime();

    std::vector<float> amplitudes;
    std::vector<float> fretXNut;
    std::vector<float> fretXBridge;

    for (auto& string : strings)
    {
        bool trigger = false;
        float t = 1.0f;

        if (isPressedLeft) {
            float distUp = pointLineDistance(mouseXNDC, mouseYNDC, string.x0, string.y0, string.x1, string.y1);
            if (distUp < string.thickness * 2.5f && string.fretPressed != -1) {
                trigger = !string.hasBeenTriggered;
                string.hasBeenTriggered = true;
            } else {
                string.hasBeenTriggered = false;
            }
        }

        if (isPressedRight) {
            int closestString, closestFret;
            float distUp;
            findClosestStringAndFret(mouseXNDC, mouseYNDC, strings, closestString, closestFret, distUp);
            if (distUp < string.thickness * 2.5f && strings[closestString].name == string.name 
                && (closestFret != lastHitFret || string.name != lastHitStringName)) {
                string.fretPressed = closestFret;
                lastHitFret = closestFret;
                lastHitStringName = string.name;
                trigger = true;
                string.hasBeenTriggered = true;
            }
        }

        if (trigger) {
            AudioEngine::playNote(string.name, string.fretPressed, 1);
            string.isVibrating = true;
            string.vibrationTime = 0.0f;
        }

        if (string.isVibrating) {
            string.vibrationTime += 0.016f;
            string.currentAmplitude = MAXIMUM_AMPLITUDE * (1.0f / (1.0f + DECAY_RATE * string.vibrationTime));
            if (string.currentAmplitude < 0.001f) {
                string.isVibrating = false;
                string.currentAmplitude = 0.0f;
            }
        } else {
            string.currentAmplitude = 0.0f;
        }

        float fretCut = string.x1;
        if (string.fretPressed >= 0 && string.fretPressed < string.fretMiddles.size()) {
            fretCut = string.fretMiddles[string.fretPressed][1];
        }

        fretXNut.push_back(fretCut);
        fretXBridge.push_back(string.x0 + 0.0099f);
        amplitudes.push_back(string.currentAmplitude);
    }

    glUseProgram(stringShader);

    glUniform1fv(glGetUniformLocation(stringShader, "amplitudes"), amplitudes.size(), amplitudes.data());
    glUniform1f(glGetUniformLocation(stringShader, "time"), time);
    glUniform1f(glGetUniformLocation(stringShader, "frequency"), 200.0f);
    glUniform1fv(glGetUniformLocation(stringShader, "fretXNut"), fretXNut.size(), fretXNut.data());
    glUniform1fv(glGetUniformLocation(stringShader, "fretXBridge"), fretXBridge.size(), fretXBridge.data());
    glUniform1i(glGetUniformLocation(stringShader, "vertsPerString"), STRING_SEGMENTS * 6);

    glBindVertexArray(stringsVAO);
    glDrawArrays(GL_TRIANGLES, 0, stringsVertexCount);

    glBindVertexArray(0);
}

void drawFretCircles(unsigned int circleShader, unsigned int unitCircleVAO, unsigned int unitCircleVertexCount) {
    for (auto& string : strings) {
        int fretPressed = string.fretPressed;

        if (fretPressed != -1) {
            auto fretCenter = string.fretMiddles[fretPressed];
            float x = fretCenter[1];
            float y = fretCenter[2];

            drawFretCircle(circleShader, unitCircleVAO, unitCircleVertexCount, x, y, MARKER_RADIUS);
        }
    }
}

void resetChord() {
    strings[0].fretPressed = 0; strings[1].fretPressed = 0; strings[2].fretPressed = 0;
    strings[3].fretPressed = 0; strings[4].fretPressed = 0; strings[5].fretPressed = 0;
}

void detectChords() {
    // major chords
    if (!keyStates[GLFW_KEY_LEFT_SHIFT]) {
        if (keyStates[GLFW_KEY_A]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 0; strings[2].fretPressed = 2;
            strings[3].fretPressed = 2; strings[4].fretPressed = 2; strings[5].fretPressed = 0;
        } else if (keyStates[GLFW_KEY_E]) {
            strings[0].fretPressed = 0; strings[1].fretPressed = 2; strings[2].fretPressed = 2;
            strings[3].fretPressed = 1; strings[4].fretPressed = 0; strings[5].fretPressed = 0;
        } else if (keyStates[GLFW_KEY_D]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = -1; strings[2].fretPressed = 0;
            strings[3].fretPressed = 2; strings[4].fretPressed = 3; strings[5].fretPressed = 2;
        } else if (keyStates[GLFW_KEY_G]) {
            strings[0].fretPressed = 3; strings[1].fretPressed = 2; strings[2].fretPressed = 0;
            strings[3].fretPressed = 0; strings[4].fretPressed = 0; strings[5].fretPressed = 3;
        } else if (keyStates[GLFW_KEY_B]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 2; strings[2].fretPressed = 4;
            strings[3].fretPressed = 4; strings[4].fretPressed = 4; strings[5].fretPressed = 2;
        } else if (keyStates[GLFW_KEY_F]) {
            strings[0].fretPressed = 1; strings[1].fretPressed = 3; strings[2].fretPressed = 3;
            strings[3].fretPressed = 2; strings[4].fretPressed = 1; strings[5].fretPressed = 1;
        } else if (keyStates[GLFW_KEY_C]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 3; strings[2].fretPressed = 2;
            strings[3].fretPressed = 0; strings[4].fretPressed = 1; strings[5].fretPressed = 0;
        }
    }

    // minor chords
    else if (keyStates[GLFW_KEY_LEFT_SHIFT]) {
        if (keyStates[GLFW_KEY_A]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 0; strings[2].fretPressed = 2;
            strings[3].fretPressed = 2; strings[4].fretPressed = 1;   strings[5].fretPressed = 0;
        } else if (keyStates[GLFW_KEY_E]) {
            strings[0].fretPressed = 0; strings[1].fretPressed = 2; strings[2].fretPressed = 2;
            strings[3].fretPressed = 0; strings[4].fretPressed = 0; strings[5].fretPressed = 0;
        } else if (keyStates[GLFW_KEY_D]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = -1; strings[2].fretPressed = 0;
            strings[3].fretPressed = 2; strings[4].fretPressed = 3; strings[5].fretPressed = 1;
        } else if (keyStates[GLFW_KEY_G]) {
            strings[0].fretPressed = 3; strings[1].fretPressed = 1; strings[2].fretPressed = 0;
            strings[3].fretPressed = 0; strings[4].fretPressed = 3; strings[5].fretPressed = 3;
        } else if (keyStates[GLFW_KEY_B]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 2; strings[2].fretPressed = 4;
            strings[3].fretPressed = 4; strings[4].fretPressed = 3; strings[5].fretPressed = 2;
        } else if (keyStates[GLFW_KEY_F]) {
            strings[0].fretPressed = 1; strings[1].fretPressed = 3; strings[2].fretPressed = 3;
            strings[3].fretPressed = 1; strings[4].fretPressed = 1; strings[5].fretPressed = 1;
        } else if (keyStates[GLFW_KEY_C]) {
            strings[0].fretPressed = -1; strings[1].fretPressed = 3; strings[2].fretPressed = 5;
            strings[3].fretPressed = 5; strings[4].fretPressed = 4; strings[5].fretPressed = 3;
        }
    }
}

void onetimeBtnPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keyStates[key] = true;
        else if (action == GLFW_RELEASE) { 
            resetChord();
            keyStates[key] = false; 
        }
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        endProgram("Program terminates!");
        exit(0);
    }
}

void mousePressCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        glfwSetCursor(window, cursorPressed);
        isPressedLeft = button == GLFW_MOUSE_BUTTON_LEFT;
        isPressedRight = button == GLFW_MOUSE_BUTTON_RIGHT;
    } else if (action == GLFW_RELEASE) {
        glfwSetCursor(window, cursorReleased);

        isPressedLeft = false;
        isPressedRight = false;

        // right click reset
        lastHitFret = -1;
        lastHitStringName = "";

        // left click reset
        for (auto& string : strings) {
            string.hasBeenTriggered = false;
            string.fretPressed = 0;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    mouseXNDC = (float)(xpos / width) * 2.0f - 1.0f;
    mouseYNDC = 1.0f - (float)(ypos / height) * 2.0f;
}

int main()
{
    // glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // fullscreen window creation
    GLFWwindow* window = createFullScreenWindow("OpenGLuitar");
    //GLFWwindow* window = glfwCreateWindow(1280, 720, "test", NULL, NULL);
    if (window == NULL) return endProgram("Window did not create successfully.");
    glfwMakeContextCurrent(window);

    glfwGetFramebufferSize(window, &width, &height);
    aspectRatio = (float)width / height;
    
    // callback functions
    glfwSetKeyCallback(window, onetimeBtnPressCallback);
    glfwSetMouseButtonCallback(window, mousePressCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // glew
    if (glewInit() != GLEW_OK) return endProgram("GLEW did not initialize successfully.");

    // alpha channel for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // cursor
    cursorReleased = loadImageToCursor("res/textures/cursor.png");
    cursorPressed = loadImageToCursor("res/textures/cursor_pressed.png");
    glfwSetCursor(window, cursorReleased);
    
    // textures
    preprocessTexture(guitarTexture, "res/textures/guitar_no_strings.png");
    preprocessTexture(signatureTexture, "res/textures/signature.png");

    // shaders
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int stringShader = createShader("string.vert", "string.frag");
    unsigned int circleShader = createShader("circle.vert", "circle.frag");

    // audio
    AudioEngine::init();

    // objects position definitions
    float verticesGuitar[] = {
         -0.94, 0.6191, 0.0f, 1.0f,
         -0.94, -0.6191, 0.0f, 0.0f,
         0.94, -0.6191, 1.0f, 0.0f,
         0.94, 0.6191, 1.0f, 1.0f
    };

    float verticesSignature[] = {
        0.619167f, 0.94, 0.0, 1.0,
        0.619167f, 0.731436, 0.0, 0.0,
        0.94f, 0.731436, 1.0, 0.0,
        0.94f, 0.94, 1.0, 1.0
    };

    strings = {
        { -0.5800f,  0.0880f,  0.6600f,  0.0880f,  0.007f, 0.992f, 0.851f, 0.435f, "E"  },
        { -0.5800f,  0.0615f,  0.6600f,  0.0615f,  0.006f, 0.992f, 0.851f, 0.435f, "A"  },
        { -0.5800f,  0.0330f,  0.6600f,  0.0330f,  0.005f, 0.992f, 0.851f, 0.435f, "D"  },
        { -0.5800f,  0.0040f,  0.6600f,  0.0040f,  0.004f, 0.992f, 0.851f, 0.435f, "G"  },
        { -0.5800f, -0.0260f,  0.6600f, -0.0260f,  0.004f, 0.698f, 0.698f, 0.698f, "B"  },
        { -0.5800f, -0.0550f,  0.6600f, -0.0550f,  0.004f, 0.698f, 0.698f, 0.698f, "Eh" }
    };

    for (GuitarString& s : strings)
    {
        s.computeFretMiddles();
    }

    // static textures VAO inits
    unsigned int VAOguitar;
    unsigned int VAOsignature;
    formRectVAO(verticesGuitar, sizeof(verticesGuitar), VAOguitar);
    formRectVAO(verticesSignature, sizeof(verticesSignature), VAOsignature);

    // strings VAO init
    unsigned int VAOstrings;
    unsigned int vertexCount;
    formStringsVAO(strings, VAOstrings, vertexCount);

    // unit circle VAO init
    unsigned int VAOunitCircle;
    unsigned int unitCircleVertexCount;
    formUnitCircleVAO(VAOunitCircle, unitCircleVertexCount);

    // main loop
    glClearColor(0.5f, 0.6f, 1.0f, 1.0f);
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        drawRect(rectShader, VAOguitar, guitarTexture);
        drawRect(rectShader, VAOsignature, signatureTexture);

        detectChords();

        drawStrings(stringShader, VAOstrings, vertexCount);
        drawFretCircles(circleShader, VAOunitCircle, unitCircleVertexCount);

        glfwSwapBuffers(window);

        AudioEngine::collectGarbage();

        limitFPS();
    }

    glDeleteProgram(rectShader);
    glfwDestroyWindow(window);
    AudioEngine::shutdown();
    glfwTerminate();
    return 0;
}
