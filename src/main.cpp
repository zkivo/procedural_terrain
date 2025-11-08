#include <glad/glad.h>
#include <GLFW/glfw3.h>

//#define STB_IMAGE_IMPLEMENTATION
//#define _CRT_SECURE_NO_WARNINGS
//#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "perlin.hpp"
#include "shader.h"

#include <iostream>
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
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
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

    glEnable(GL_DEPTH_TEST);
    Shader shader("mesh.vs", "mesh.fs");
    
    int terrain_width = 1000, terrain_depth = 1000, terrain_height = 500;
    Perlin perlin(42, 0.0);

    std::vector<uint8_t> img(terrain_width * terrain_depth);
    perlin.getHeatmap(img, terrain_width, terrain_depth, 500, 500);
    
    Mesh mesh = perlin.getMesh(img, terrain_width, terrain_depth, terrain_height);
    GLMesh glmesh = perlin.uploadMesh(mesh);
    
    shader.setFloat("uMaxY",  (float)(terrain_height)/2.0f);
    shader.setFloat("uMinY", -(float)(terrain_height)/2.0f);

    while (!glfwWindowShouldClose(window))
    {

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // per-frame time logic
        // --------------------
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

        shader.use();

        // Projection
        glm::mat4 proj = glm::perspective(glm::radians(fov), 
            float(WINDOW_WIDTH) / float(WINDOW_HEIGHT), 
            0.1f, 10000.0f);
        shader.setMat4("uProj", proj);

        // View: camera looks at the mesh center from +Z
        glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
        glm::vec3 camPos(1300.0f, 500.0f, 0.0f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
        shader.setMat4("uView", view);

        // Model: rotate around center
        double t = glfwGetTime();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::rotate(model, float(t) * glm::radians(20.0f), glm::vec3(0, 1, 0));
        shader.setMat4("uModel", model);

        glBindVertexArray(glmesh.vao);
        glDrawElements(GL_TRIANGLES, glmesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
