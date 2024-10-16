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

// Window dimensions and lane width
const GLuint WIDTH = 800, HEIGHT = 600;
const float LANE_WIDTH = WIDTH / 3.0f;

// Sprite structure for objects like spaceship and comet
struct Sprite {
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions;
    float angle;

    // Initialize sprite with texture, position, and size
    void setupSprite(GLuint textureID, vec3 pos, vec3 dim) {
        this->texID = textureID;
        this->dimensions = dim;
        this->position = pos;

        // Define vertex data (position and texture coordinates)
        GLfloat vertices[] = {
            -0.5, -0.5, 0.0, 0.0, 0.0, // V0
            -0.5,  0.5, 0.0, 0.0, 1.0, // V1
             0.5, -0.5, 0.0, 1.0, 0.0, // V2
             0.5,  0.5, 0.0, 1.0, 1.0  // V3
        };

        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Setup position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // Setup texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // Unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

// Shader source code
const GLchar *vertexShaderSource = "#version 400\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec2 texc;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "out vec2 texCoord;\n"
    "void main() { gl_Position = projection * model * vec4(position, 1.0); texCoord = vec2(texc.s, 1.0 - texc.t); }\0";

const GLchar *fragmentShaderSource = "#version 400\n"
    "in vec2 texCoord;\n"
    "uniform sampler2D texBuffer;\n"
    "uniform vec2 offsetTex;\n"
    "out vec4 color;\n"
    "void main() { color = texture(texBuffer, texCoord + offsetTex); }\n\0";

// Global variables for spaceship, comet, and game state
Sprite spaceship, comet;
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

int main() {
    glfwInit(); // Initialize GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Space Travel", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback); // Register key input callback

    // Initialize OpenGL (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set up shader program
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Projection matrix
    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Load textures
    int spaceshipTexture = loadTexture("../textures/spaceship.png");
    int cometTexture = loadTexture("../textures/asteroid.png");

    // Setup spaceship and comet
    spaceship.setupSprite(spaceshipTexture, vec3(WIDTH / 2, 50, 0), vec3(50, 50, 1));
    comet.setupSprite(cometTexture, vec3(WIDTH / 2, 50, 0), vec3(50, 50, 1));
    resetComet(); // Initial comet setup

    // Main game loop
    while (!glfwWindowShouldClose(window) && !gameOver) {
        glfwPollEvents(); // Handle input events
        float currentFrame = glfwGetTime(); // Track time
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear screen

        updateGame(currentFrame); // Update game logic
        drawSprite(spaceship, shaderID); // Draw spaceship
        drawSprite(comet, shaderID);     // Draw comet

        glfwSwapBuffers(window); // Swap buffers
    }

    glfwTerminate(); // Clean up
    return 0;
}

// Handles keyboard input for moving spaceship between lanes
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_LEFT && spaceshipLane > 0) {
            moveSpaceship(spaceshipLane - 1); // Move left
        } else if (key == GLFW_KEY_RIGHT && spaceshipLane < 2) {
            moveSpaceship(spaceshipLane + 1); // Move right
        }
    }
}

// Sets up shaders (vertex and fragment shaders)
int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Loads texture from file using stb_image
int loadTexture(const std::string &filePath) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int imgWidth, imgHeight, nrChannels;
    unsigned char *data = stbi_load(filePath.c_str(), &imgWidth, &imgHeight, &nrChannels, 0);

    if (data) {
        if (nrChannels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << filePath << std::endl;
    }
    stbi_image_free(data);
    return texID;
}

// Draws a sprite using its position, size, and texture
void drawSprite(Sprite spr, GLuint shaderID) {
    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0f, 0.0f);
    glBindVertexArray(spr.VAO);
    glBindTexture(GL_TEXTURE_2D, spr.texID);

    mat4 model = mat4(1.0f);
    model = translate(model, spr.position);
    model = rotate(model, radians(spr.angle), vec3(0.0f, 0.0f, 1.0f));
    model = scale(model, spr.dimensions);

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Moves spaceship to the specified lane
void moveSpaceship(int lane) {
    spaceshipLane = lane;
    spaceship.position.x = LANE_WIDTH / 2 + lane * LANE_WIDTH;
}

// Resets comet to a random lane and off-screen position
void resetComet() {
    int lane = rand() % 3;
    comet.position = vec3(LANE_WIDTH / 2 + lane * LANE_WIDTH, HEIGHT + 50, 0);
}

// Updates game logic (comet movement, collision detection)
void updateGame(float deltaTime) {
    comet.position.y -= 0.3f * deltaTime; // Move comet down

    // Check for collision with spaceship
    if (comet.position.y < spaceship.position.y + spaceship.dimensions.y &&
        comet.position.y > spaceship.position.y - comet.dimensions.y &&
        comet.position.x == spaceship.position.x) {
        gameOver = true;
        cout << "Game Over!" << endl;
    }

    // Reset comet if it moves off the screen
    if (comet.position.y < -50) {
        resetComet();
    }
}