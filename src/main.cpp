#include <glad/glad.h>
#include <GLFW/glfw3.h>

//#define STB_IMAGE_IMPLEMENTATION
//#define _CRT_SECURE_NO_WARNINGS
//#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "perlin.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

const unsigned int WINDOW_WIDTH = 1920;
const unsigned int WINDOW_HEIGHT = 1080;

float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

// Returns {r, g, b} in range [0.0, 1.0]
std::vector<float> hexToRGBf(const std::string& hex) {
    std::string clean = hex;

    // Remove leading '#' if present
    if (!clean.empty() && clean[0] == '#')
        clean.erase(0, 1);

    if (clean.size() != 6)
        throw std::runtime_error("Invalid hex color: must have 6 hex digits");

    // Parse R, G, B
    int r = std::stoi(clean.substr(0, 2), nullptr, 16);
    int g = std::stoi(clean.substr(2, 2), nullptr, 16);
    int b = std::stoi(clean.substr(4, 2), nullptr, 16);

    return { r / 255.0f, g / 255.0f, b / 255.0f };
}

int main()
{
    // glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Procedural Terrain", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int terrain_width = 2000, terrain_depth = 2000, terrain_height = 500;
    Perlin perlin(42, 0.0);

    std::vector<uint8_t> img(terrain_width * terrain_depth);

    //perlin.getHeatmap(img, terrain_width, terrain_depth, 250, 250);
    //std::cout << "1. Max img value: " << (int)(*std::max_element(img.begin(), img.end())) << std::endl;
    //perlin.create_png("terrain.png", terrain_width, terrain_depth, img);
    //perlin.applyGaussian(img, terrain_width, terrain_depth, 0.7);
    //std::cout << "2. Max img value: " << (int)(*std::max_element(img.begin(), img.end())) << std::endl;
    //perlin.create_png("terrain_gaussian.png", terrain_width, terrain_depth, img);

    perlin.getFractalNoise(img, terrain_width, terrain_depth, 250, 250, 5, 0.5, 2.0);
    //std::cout << "1. Max img value: " << (int)(*std::max_element(img.begin(), img.end())) << std::endl;
    perlin.create_png("terrain.png", terrain_width, terrain_depth, img);
    perlin.applyGaussian(img, terrain_width, terrain_depth, 0.7);
    //std::cout << "2. Max img value: " << (int)(*std::max_element(img.begin(), img.end())) << std::endl;
    perlin.create_png("terrain_gaussian.png", terrain_width, terrain_depth, img);

    Mesh mesh = perlin.getMesh(img, terrain_width, terrain_depth, terrain_height);
    GLMesh glmesh = perlin.uploadMesh(mesh);
    
    glm::vec3 highest_vertex = mesh.vertices.front();
    glm::vec3 lowest_vertex  = mesh.vertices.front();

    for (const auto& v : mesh.vertices) {
        if (v.y > highest_vertex.y) highest_vertex = v;
        if (v.y < lowest_vertex.y)  lowest_vertex = v;
    }

    std::cout << "Highest vertex: (" << highest_vertex.x << ", " << highest_vertex.y << ", " << highest_vertex.z << ")\n";
    std::cout << "Lowest vertex: ("  << lowest_vertex.x  << ", " << lowest_vertex.y  << ", " << lowest_vertex.z  << ")\n";

    unsigned int shader_id;

    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile("terrain.vs");
    std::ifstream fShaderFile("terrain.fs");
    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
    const char* charVertexCode = vertexCode.c_str();
    const char* charFragmentCode = fragmentCode.c_str();

    // compiling
    unsigned int vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &charVertexCode, NULL);
    glCompileShader(vertex);
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &charFragmentCode, NULL);
    glCompileShader(fragment);
    // shader Program
    shader_id = glCreateProgram();
    glAttachShader(shader_id, vertex);
    glAttachShader(shader_id, fragment);
    glLinkProgram(shader_id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glEnable(GL_DEPTH_TEST);

    glUniform1f(glGetUniformLocation(shader_id, std::string("uMinHeight").c_str()), lowest_vertex.y);
    glUniform1f(glGetUniformLocation(shader_id, std::string("uMaxHeight").c_str()), highest_vertex.y);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // render
        // ------
        //34a0a4
        //184e77
        auto color = hexToRGBf("184e77");
        glClearColor(color[0], color[1], color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_id);

        // Projection
        glm::mat4 proj = glm::perspective(glm::radians(fov), 
            float(WINDOW_WIDTH) / float(WINDOW_HEIGHT), 
            0.1f, 10000.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader_id, std::string("uProj").c_str()), 1, GL_FALSE, &proj[0][0]);

        // View
        glm::vec3 camTarget(0.0f, (lowest_vertex.y + lowest_vertex.y) / 2, 0.0f);
        glm::vec3 camPos(2100.0f, 1000.0f, 0.0f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(shader_id, std::string("uView").c_str()), 1, GL_FALSE, &view[0][0]);
        glUniform3f(glGetUniformLocation(shader_id, std::string("uCamPos").c_str()), camPos.x, camPos.y, camPos.z);

        // Model: rotate around center
        double t = glfwGetTime();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::rotate(model, float(t) * glm::radians(20.0f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(shader_id, std::string("uModel").c_str()), 1, GL_FALSE, &model[0][0]);

        glBindVertexArray(glmesh.vao);
        glDrawElements(GL_TRIANGLES, glmesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    perlin.destroy(glmesh);
    glfwTerminate();
    return 0;
}