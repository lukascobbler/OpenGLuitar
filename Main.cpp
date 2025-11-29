#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>

#include "Util.h"
#include "GuitarString.h"
#include "Audio.h"

#define NOMINMAX
#include <windows.h>

#define FPS 75

#define MAXIMUM_AMPLITUDE 0.008f
#define DECAY_RATE 2.1f

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

// textures
unsigned guitarTexture;
unsigned cursorNotPressedTexture;
unsigned signatureTexture;

// strings as a global variable
std::vector<GuitarString> strings;

// mouse related stuff
boolean isPressed;
GLFWcursor* cursorReleased;
GLFWcursor* cursorPressed;
double mouseX = 0.0, mouseY = 0.0;

// frame limiting
double lastTimeForRefresh;

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
        float dx = string.x1 - string.x0; float dy = string.y1 - string.y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0) continue;
        float px = -dy / len; float py = dx / len;
        float hx = px * string.thickness * 0.5f; float hy = py * string.thickness * 0.5f;

        float rectVertices[30] = {
            string.x0 - hx, string.y0 - hy, string.r, string.g, string.b,
            string.x1 - hx, string.y1 - hy, string.r, string.g, string.b,
            string.x1 + hx, string.y1 + hy, string.r, string.g, string.b,
            string.x0 - hx, string.y0 - hy, string.r, string.g, string.b,
            string.x1 + hx, string.y1 + hy, string.r, string.g, string.b,
            string.x0 + hx, string.y0 + hy, string.r, string.g, string.b
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

void drawRect(unsigned int rectShader, unsigned int VAOrect, unsigned int texture) {
    glUseProgram(rectShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(VAOrect);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawStrings(unsigned int stringShader, unsigned int VAO, 
    unsigned int vertexCount, int windowWidth, int windowHeight)
{
    float time = (float)glfwGetTime();
    float mouseXNDC = (float)(mouseX / windowWidth) * 2.0f - 1.0f;
    float mouseYNDC = 1.0f - (float)(mouseY / windowHeight) * 2.0f;

    std::vector<float> amplitudes;

    for (auto& string : strings)
    {
        bool trigger = false;

        if (isPressed && !string.hasBeenTriggered)
        {
            float distUp = pointLineDistance(mouseXNDC, mouseYNDC, string.x0, string.y0, string.x1, string.y1);
            if (distUp < string.thickness * 2.5f) {
                trigger = true;
                string.hasBeenTriggered = true;
            }
        }

        if (trigger)
        {
            AudioEngine::playNote(string.name, 0, 1);
            string.isVibrating = true;
            string.vibrationTime = 0.0f;
        }

        if (string.isVibrating)
        {
            string.vibrationTime += 0.016f;
            string.currentAmplitude = MAXIMUM_AMPLITUDE * (1.0f / (1.0f + DECAY_RATE * string.vibrationTime));
            if (string.currentAmplitude < 0.001f)
            {
                string.isVibrating = false;
                string.currentAmplitude = 0.0f;
            }
        }
        else
        {
            string.currentAmplitude = 0.0f;
        }

        amplitudes.push_back(string.currentAmplitude);
    }

    glUseProgram(stringShader);

    glUniform1fv(glGetUniformLocation(stringShader, "amplitudes"), amplitudes.size(), amplitudes.data());
    glUniform1f(glGetUniformLocation(stringShader, "time"), time);
    glUniform1f(glGetUniformLocation(stringShader, "frequency"), 200.0f);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glBindVertexArray(0);
}

void onetime_btn_press_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {

    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        endProgram("Program terminates!");
        exit(0);
    }
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwSetCursor(window, cursorPressed);
        isPressed = true;

    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        glfwSetCursor(window, cursorReleased);

        isPressed = false;
        for (auto& string : strings) {
            string.hasBeenTriggered = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // fullscreen window creation
    GLFWwindow* window = createFullScreenWindow("OpenGLuitar");
    //GLFWwindow* window = glfwCreateWindow(1280, 720, "test", NULL, NULL);
    if (window == NULL) return endProgram("Window did not create successfully.");
    glfwMakeContextCurrent(window);
    
    // callback functions
    glfwSetKeyCallback(window, onetime_btn_press_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
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
    
    preprocessTexture(guitarTexture, "res/textures/guitar_no_strings.png");
    preprocessTexture(signatureTexture, "res/textures/signature.png");

    // shaders
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    glUseProgram(rectShader);
    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

    unsigned int stringShader = createShader("string.vert", "string.frag");

    // audio
    AudioEngine::init();

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

    unsigned int VAOguitar;
    unsigned int VAOsignature;
    formRectVAO(verticesGuitar, sizeof(verticesGuitar), VAOguitar);
    formRectVAO(verticesSignature, sizeof(verticesSignature), VAOsignature);

    unsigned int VAOstrings;
    unsigned int vertexCount;

    formStringsVAO(strings, VAOstrings, vertexCount);

    glClearColor(0.5f, 0.6f, 1.0f, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        drawRect(rectShader, VAOguitar, guitarTexture);
        drawRect(rectShader, VAOsignature, signatureTexture);

        drawStrings(stringShader, VAOstrings, vertexCount, width, height);

        glfwSwapBuffers(window);

        AudioEngine::collectGarbage();

        limitFPS(); // todo mouse position detection problem with fps limiting 
                    // to 75 with busy waiting
    }

    glDeleteProgram(rectShader);
    glfwDestroyWindow(window);
    AudioEngine::shutdown();
    glfwTerminate();
    return 0;
}