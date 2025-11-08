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

// settings
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

struct Bounds {
    glm::vec3 minP, maxP, center;
    float radius;
};

Bounds computeBounds(const Mesh& m) {
    Bounds b;
    if (m.vertices.empty()) {
        b.minP = b.maxP = b.center = glm::vec3(0.0f);
        b.radius = 1.0f;
        return b;
    }

    glm::vec3 mn(std::numeric_limits<float>::infinity());
    glm::vec3 mx(-std::numeric_limits<float>::infinity());
    for (auto& v : m.vertices) {
        mn = glm::min(mn, v);
        mx = glm::max(mx, v);
    }

    b.minP = mn;
    b.maxP = mx;
    b.center = 0.5f * (mn + mx);

    float r2 = 0.0f;
    for (auto& v : m.vertices) {
        float dist2 = glm::dot(v - b.center, v - b.center);
        r2 = std::max(r2, dist2);
    }
    b.radius = std::sqrt(r2);
    return b;
}

inline Bounds computeBoundsFromMesh(const Mesh& m) {
    Bounds b{};
    if (m.vertices.empty()) {
        b.minP = b.maxP = b.center = glm::vec3(0.0f);
        b.radius = 1.0f;
        return b;
    }

    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float minZ = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    float maxZ = -std::numeric_limits<float>::infinity();

    for (const glm::vec3& p : m.vertices) {
        if (p.x < minX) minX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.z < minZ) minZ = p.z;
        if (p.x > maxX) maxX = p.x;
        if (p.y > maxY) maxY = p.y;
        if (p.z > maxZ) maxZ = p.z;
    }

    b.minP = { minX, minY, minZ };
    b.maxP = { maxX, maxY, maxZ };
    b.center = 0.5f * (b.minP + b.maxP);

    // Bounding sphere radius around center (avoid glm::length2 if missing)
    float r2 = 0.0f;
    for (const glm::vec3& p : m.vertices) {
        glm::vec3 d = p - b.center;
        float d2 = glm::dot(d, d);   // squared distance
        if (d2 > r2) r2 = d2;
    }
    b.radius = std::sqrt(r2);
    return b;
}


float computeCameraDistanceToFit(float radius, float fovyDeg, float aspect) {
    float fovy = glm::radians(fovyDeg);
    float fovx = 2.0f * std::atan(std::tan(fovy * 0.5f) * aspect);
    float distV = radius / std::tan(fovy * 0.5f);
    float distH = radius / std::tan(fovx * 0.5f);
    return 1.05f * std::max(distV, distH); // 5% margin
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
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader shader("mesh.vs", "mesh.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------

    int terrain_width = 500, terrain_depth = 500;
    double scale_x = 100, scale_y = 100;
    Perlin perlin(42, 0.0);

    std::vector<uint8_t> img(terrain_width * terrain_depth);
    perlin.getHeatmap(img, terrain_width, terrain_depth, scale_x, scale_y, 42);
    
    Mesh mesh = perlin.getMesh(img, terrain_width, terrain_depth, 1.0, 500.0);
    GLMesh glmesh = perlin.uploadMesh(mesh);
    
    // Camera framing
    Bounds B = computeBoundsFromMesh(mesh);

    shader.setFloat("uMaxY", B.maxP.y);
    shader.setFloat("uMinY", B.minP.y);

    float camDist = 4.0f * B.radius;
    float aspect = float(WINDOW_WIDTH) / float(WINDOW_HEIGHT);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate shader
        shader.use();

        // Rebuild projection if aspect changes
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10000.0f);
        shader.setMat4("uProj", proj);

        // View: camera looks at the mesh center from +Z
        glm::vec3 camTarget = B.center;
        glm::vec3 camPos = camTarget + glm::vec3(0.0f, 0.5f * B.radius, camDist);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
        shader.setMat4("uView", view);


        // Model: rotate around center
        double t = glfwGetTime();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, B.center);
        model = glm::rotate(model, float(t) * glm::radians(20.0f), glm::vec3(0, 1, 0));
        //model = glm::translate(model, -B.center);
        shader.setMat4("uModel", model);

        shader.setMat4("uModel", model);

        //glUniform3f(glGetUniformLocation(prog, "uColor"), 0.85f, 0.90f, 1.00f);

        glBindVertexArray(glmesh.vao);
        glDrawElements(GL_TRIANGLES, glmesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}