#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include "Util.h"
#include "GuitarString.h"

#define NOMINMAX
#include <windows.h>

#define FPS 75

#define MAXIMUM_AMPLITUDE 0.005f
#define DECAY_RATE 2.0f

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

unsigned guitarTexture;
unsigned cursorNotPressedTexture;

GLFWcursor* cursorReleased;
GLFWcursor* cursorPressed;
boolean isPressed = false;
double mouseX = 0.0, mouseY = 0.0;

double lastTimeForRefresh;

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void formGuitarVAO(float* verticesRect, size_t rectSize, unsigned int& VAOrect) {
    // formiranje VAO-ova je izdvojeno u posebnu funkciju radi čitljivijeg koda u main funkciji

    // Podsetnik za atribute:
/*
    Jedan VAO se vezuje za jedan deo celokupnog skupa verteksa na sceni.
    Na primer, dobra praksa je da se jedan VAO vezuje za jedan VBO koji se vezuje za jedan objekat, odnosno niz temena koja opisuju objekat.

    VAO je pomoćna struktura koja opisuje kako se podaci u nizu objekta interpretiraju.
    U render-petlji, za crtanje određenog objekta, naredbom glBindVertexArray(nekiVAO) se određuje koji se objekat crta.

    Potrebno je definisati koje atribute svako teme u nizu sadrži, npr. pozicija na lokaciji 0 i boja na lokaciji 1.

    Ova konfiguracija je specifična za naš primer na vežbama i može se menjati za različite potrebe u projektu.


    Atribut se opisuje metodom glVertexAttribPointer(). Argumenti su redom:
        index - identifikacioni kod atributa, u verteks šejderu je povezan preko svojstva location (location = 0 u šejderu -> indeks tog atributa treba staviti isto 0 u ovoj naredbi)
        size - broj vrednosti unutar atributa (npr. za poziciju su x i y, odnosno 2 vrednosti; za boju r, g i b, odnosno 3 vrednosti)
        type - tip vrednosti
        normalized - da li je potrebno mapirati na odgovarajući opseg (mi poziciju već inicijalizujemo na opseg (-1, 1), a boju (0, 1), tako da nam nije potrebno)
        stride - koliko elemenata u nizu treba preskočiti da bi se došlo od datog atributa u jednom verteksu do istog tog atributa u sledećem verteksu
        pointer - koliko elemenata u nizu treba preskočiti od početka niza da bi se došlo do prvog pojavljivanja datog atributa
*/
// Četvorougao

    unsigned int VBOrect;
    glGenVertexArrays(1, &VAOrect);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOrect);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, rectSize, verticesRect, GL_STATIC_DRAW);

    // Atribut 0 (pozicija):
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (tekstura):
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void limitFPS() {
    while (glfwGetTime() < lastTimeForRefresh + 1.0 / FPS) {}
    lastTimeForRefresh += 1.0 / FPS;
}

void drawRect(unsigned int rectShader, unsigned int VAOrect) {
    glUseProgram(rectShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, guitarTexture);

    glBindVertexArray(VAOrect); // Podešavanje da se crta koristeći vertekse četvorougla
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Crtaju se trouglovi tako da formiraju četvorougao
}

void drawLines(unsigned int stringShader, unsigned int VAO, 
    unsigned int vertexCount, int windowWidth, int windowHeight, std::vector<Line2D>& lines)
{
    float time = (float)glfwGetTime();
    float mouseXNDC = (float)(mouseX / windowWidth) * 2.0f - 1.0f;
    float mouseYNDC = 1.0f - (float)(mouseY / windowHeight) * 2.0f;

    std::vector<float> amplitudes;

    for (auto& line : lines)
    {
        bool trigger = false;

        if (isPressed)
        {
            float distUp = pointLineDistance(mouseXNDC, mouseYNDC, line.x0, line.y0, line.x1, line.y1);
            if (distUp < line.thickness * 2.5f)
                trigger = true;

        }

        if (trigger)
        {
            line.isVibrating = true;
            line.vibrationTime = 0.0f;
        }

        if (line.isVibrating)
        {
            line.vibrationTime += 0.016f;
            line.currentAmplitude = MAXIMUM_AMPLITUDE * (1.0f / (1.0f + DECAY_RATE * line.vibrationTime));
            if (line.currentAmplitude < 0.001f)
            {
                line.isVibrating = false;
                line.currentAmplitude = 0.0f;
            }
        }
        else
        {
            line.currentAmplitude = 0.0f;
        }

        amplitudes.push_back(line.currentAmplitude);
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
    cursorReleased = loadImageToCursor("res/cursor.png");
    cursorPressed = loadImageToCursor("res/cursor_pressed.png");
    glfwSetCursor(window, cursorReleased);
    
    preprocessTexture(guitarTexture, "res/guitar_no_strings.png");

    // shaders
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    glUseProgram(rectShader);
    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

    unsigned int stringShader = createShader("string.vert", "string.frag");

    float verticesGuitar[] = {
         -0.94, 0.6191, 0.0f, 1.0f,// top left
         -0.94, -0.6191, 0.0f,  0.0f, // bottom left 
         0.94, -0.6191, 1.0f, 0.0f, // top right
         0.94, 0.6191, 1.0f, 1.0f, // bottom right 
    };

    std::vector<Line2D> lines = {
        { -0.5800f,  0.0880f,  0.6600f,  0.0880f,  0.007f, 0.992f, 0.851f, 0.435f },
        { -0.5800f,  0.0615f,  0.6600f,  0.0615f,  0.006f, 0.992f, 0.851f, 0.435f },
        { -0.5800f,  0.0330f,  0.6600f,  0.0330f,  0.005f, 0.992f, 0.851f, 0.435f },
        { -0.5800f,  0.0040f,  0.6600f,  0.0040f,  0.004f, 0.992f, 0.851f, 0.435f },
        { -0.5800f, -0.0260f,  0.6600f, -0.0260f,  0.004f, 0.698f, 0.698f, 0.698f },
        { -0.5800f, -0.0550f,  0.6600f, -0.0550f,  0.004f, 0.698f, 0.698f, 0.698f }
    };

    unsigned int VAOguitar;
    formGuitarVAO(verticesGuitar, sizeof(verticesGuitar), VAOguitar);

    unsigned int VAOstrings;
    unsigned int vertexCount;

    formLinesVAO(lines, VAOstrings, vertexCount);

    glClearColor(0.5f, 0.6f, 1.0f, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            
        }

        glClear(GL_COLOR_BUFFER_BIT);

        drawRect(rectShader, VAOguitar);

        drawLines(stringShader, VAOstrings, vertexCount, width, height, lines);

        glfwSwapBuffers(window);
        glfwPollEvents();

        limitFPS();
    }

    glDeleteProgram(rectShader);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}