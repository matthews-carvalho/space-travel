#include <iostream>
#include <cstdlib>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

using namespace std;
using namespace glm;

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
const float LANE_WIDTH = WIDTH / 3.0f;

// Spaceship and comet definitions
struct Sprite
{
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions;
    float angle;

    void setupSprite(GLuint textureID, vec3 pos, vec3 dim)
    {
        texID = textureID;
        position = pos;
        dimensions = dim;
        angle = 0.0f;
    }
};

const GLchar *vertexShaderSource = "#version 400\n"
                                   "layout (location = 0) in vec3 position;\n"
                                   "layout (location = 1) in vec2 texc;\n"
                                   "uniform mat4 projection;\n"
                                   "uniform mat4 model;\n"
                                   "out vec2 texCoord;\n"
                                   "void main()\n"
                                   "{\n"
                                   
                                   "gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);\n"
                                   "texCoord = vec2(texc.s, 1.0 - texc.t);\n"
                                   "}\0";

const GLchar *fragmentShaderSource = "#version 400\n"
                                     "in vec2 texCoord;\n"
                                     "uniform sampler2D texBuffer;\n"
                                     "uniform vec2 offsetTex;\n"
                                     "out vec4 color;\n"
                                     "void main()\n"
                                     "{\n"
                                     "color = texture(texBuffer, texCoord + offsetTex);\n"
                                     "}\n\0";

// Global variables for spaceship, comets, and game state
Sprite spaceship;
Sprite comet;
bool gameOver = false;
int spaceshipLane = 1; // 0 = left, 1 = middle, 2 = right

// Function prototypes
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
int loadTexture(const string &filePath);
void drawSprite(Sprite spr, GLuint shaderID);
void moveSpaceship(int lane);
void resetComet();
void updateGame(float deltaTime);

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // Window settings
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Spaceship Game", nullptr, nullptr);
    if (!window)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // Set up spaceship and comet sprites
    GLuint spaceshipTexture = loadTexture("../textures/spaceship.png");
    GLuint cometTexture = loadTexture("../textures/asteroid.png");

    // Initial setup for the spaceship
    spaceship.setupSprite(spaceshipTexture, vec3(WIDTH / 2, 50, 0), vec3(50, 50, 1));

    // Initial setup for the comet (use resetComet() for consistency)
    resetComet();

    // Game loop
    while (!glfwWindowShouldClose(window) && !gameOver)
    {
        float currentFrame = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);

        cout << "Spaceship position: (" << spaceship.position.x << ", " << spaceship.position.y << ")" << endl;
        cout << "Comet position: (" << comet.position.x << ", " << comet.position.y << ")" << endl;

        // Update game logic and render
        updateGame(currentFrame);

        drawSprite(spaceship, 0); // Shader ID not set for simplicity
        drawSprite(comet, 0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_LEFT && spaceshipLane > 0)
        {
            moveSpaceship(spaceshipLane - 1);
        }
        if (key == GLFW_KEY_RIGHT && spaceshipLane < 2)
        {
            moveSpaceship(spaceshipLane + 1);
        }
    }
}

int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int loadTexture(const std::string &filePath)
{
    GLuint texID;
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        if (nrChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (nrChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture: " << filePath << std::endl;
        return -1;
    }

    return texID;
}

void drawSprite(Sprite spr, GLuint shaderID)
{
    glBindVertexArray(spr.VAO);
    glBindTexture(GL_TEXTURE_2D, spr.texID);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, spr.position);
    model = glm::rotate(model, glm::radians(spr.angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, spr.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void moveSpaceship(int lane)
{
    spaceshipLane = lane;
    spaceship.position.x = LANE_WIDTH / 2 + lane * LANE_WIDTH;
}

void resetComet()
{
    int lane = rand() % 3;
    comet.setupSprite(comet.texID, vec3(LANE_WIDTH / 2 + lane * LANE_WIDTH, HEIGHT + 50, 0), vec3(50, 50, 1));
}

void updateGame(float deltaTime)
{
    // Move comet down
    comet.position.y -= 300.0f * deltaTime;

    // Check for collision when comet is within the range of the spaceship
    if (comet.position.y < spaceship.position.y + spaceship.dimensions.y &&
        comet.position.y > spaceship.position.y - comet.dimensions.y &&
        comet.position.x == spaceship.position.x)
    {
        gameOver = true;
        cout << "Game Over!" << endl;
    }

    // Reset comet if it goes out of screen
    if (comet.position.y < -50)
    {
        resetComet();
    }
}