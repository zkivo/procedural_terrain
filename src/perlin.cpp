#include "perlin.hpp"
#include <glad/glad.h>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define _CRT_SECURE_NO_WARNINGS
//#include "stb_image_write.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>
#include <array>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>


Perlin::Perlin(uint64_t seed, double phase) : seed_(seed), phase_(phase) {}

double Perlin::getNoise(double x, double y) {
    int x_index = (int)std::floor(x) & 255;
    int y_index = (int)std::floor(y) & 255;

    double xf = x - std::floor(x);
    double yf = y - std::floor(y);

    double u = fade(xf);
    double v = fade(yf);

    // the angle is calculated as (64bit hash / MAX_UINT64) * 2π
    // the first part gives us a value between 0 and 1
    // the second part gives an angle between 0 and 360 degrees (but in radians)
    double a_top_left     = (double)(hashing(x_index, y_index + 1)) / 18446744073709552000.0 * 2 * M_PI + phase_;
    double a_top_right    = (double)(hashing(x_index + 1, y_index + 1)) / 18446744073709552000.0 * 2 * M_PI + phase_;
    double a_bottom_left  = (double)(hashing(x_index, y_index)) / 18446744073709552000.0 * 2 * M_PI + phase_;
    double a_bottom_right = (double)(hashing(x_index + 1, y_index)) / 18446744073709552000.0 * 2 * M_PI + phase_;

    // below are unit vectors
    struct { double gx, gy; } grad_top_left     {cos(a_top_left), sin(a_top_left)};
    struct { double gx, gy; } grad_top_right    {cos(a_top_right), sin(a_top_right)};
    struct { double gx, gy; } grad_bottom_left  {cos(a_bottom_left), sin(a_bottom_left)};
    struct { double gx, gy; } grad_bottom_right {cos(a_bottom_right), sin(a_bottom_right)};

    double dot_top_left     = dot(grad_top_left.gx, grad_top_left.gy, xf , yf - 1);
    double dot_top_right    = dot(grad_top_right.gx, grad_top_right.gy, xf - 1, yf - 1);
    double dot_bottom_left  = dot(grad_bottom_left.gx, grad_bottom_left.gy, xf , yf);
    double dot_bottom_right = dot(grad_bottom_right.gx, grad_bottom_right.gy, xf - 1, yf);

    double left_lerp  = lerp(dot_bottom_left, dot_top_left, v);
    double right_lerp = lerp(dot_bottom_right, dot_top_right, v);
    double vertical_lerp = lerp(left_lerp, right_lerp, u);

    return (vertical_lerp + 1.0) * 0.5;
}

void Perlin::getHeatmap(std::vector<uint8_t>& img, int width, int height, double scale_x, double scale_y) {
    if (img.size() != width * height) img.resize(width * height);

    auto clamp_value = [](double value, double low, double high) {
        if (value < low) return low;
        if (value > high) return high;
        return value;
    };

    for (int row = 0; row < height; ++row) {
        double y = row / scale_y;
        for (int col = 0; col < width; ++col) {
            double x = col / scale_x;
            double h = this->getNoise(x, y);
            int v = (int)std::lround(clamp_value(h, 0.0, 1.0) * 255.0);
            img[row * width + col] = (uint8_t)v;
        }
    }
}

Mesh Perlin::getMesh(const std::vector<uint8_t>& img,
    int width,
    int height,
    double scale_height)
{
    Mesh mesh;
    if (width <= 1 || height <= 1) return mesh;
    if ((int)img.size() < width * height) return mesh;

    const int rows = height;
    const int cols = width;

    mesh.vertices.reserve(rows * cols);
    mesh.indices.reserve((rows - 1) * (cols - 1) * 6);

    auto get_position = [cols](int r, int c) {
        return r * cols + c;
    };

    auto u8_to_height = [](uint8_t v, double scale) {
        return (double(v) / 255.0f) * scale;
        };

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            glm::f64vec3 vertex;
            vertex.x = c - (cols / 2);
            vertex.y = (img[get_position(r, c)] / 255.0f * scale_height) - scale_height / 2;
            vertex.z = r - (rows / 2);
            mesh.vertices.push_back(vertex);
        }
    }

    for (int r = 0; r < rows - 1; ++r) {
        for (int c = 0; c < cols - 1; ++c) {
            unsigned int i0 = get_position(r, c);
            unsigned int i1 = get_position(r + 1, c);
            unsigned int i2 = get_position(r, c + 1);
            unsigned int i3 = get_position(r + 1, c + 1);
            // triangle 1
            mesh.indices.push_back(i0);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            //triangle 2
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i3);
        }
    }
    return mesh;
}

GLMesh Perlin::uploadMesh(const Mesh& m) {
    GLMesh g{};
    if (m.vertices.empty() || m.indices.empty()) return g;

    glGenVertexArrays(1, &g.vao);
    glGenBuffers(1, &g.vbo);
    glGenBuffers(1, &g.ebo);

    glBindVertexArray(g.vao);

    glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        m.vertices.size() * sizeof(glm::vec3),
        m.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        m.indices.size() * sizeof(unsigned int),
        m.indices.data(),
        GL_STATIC_DRAW);

    // layout(location = 0) => vec3 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);

    g.indexCount = (GLsizei)m.indices.size();
    return g;
}


//int test() {
//
//    bool GIF = false;
//
//    const int width  = 500; // number pixel in the x direction
//    const int height = 500; // number pixel in the y direction
//
//    const double scale_x = 100.0;  // pixels per unit in the x direction
//    const double scale_y = 100.0;  // pixels per unit in the y direction
//
//    if (GIF) std::filesystem::create_directories("GIF");
//    std::vector<uint8_t> img(width * height);
//
//    auto clamp_value = [](double value, double low, double high) {
//        if (value < low) return low;
//        if (value > high) return high;
//        return value;
//    };
//
//    int frame_idx = 0;
//    for (int deg = 0; deg <= 360; deg += 12) {
//        double phase_rad = deg * M_PI / 180.0;
//
//        Perlin perlin(42, phase_rad);
//
//        for (int row = 0; row < height; ++row) {
//            double y = row / scale_y;
//            for (int col = 0; col < width; ++col) {
//                double x = col / scale_x;
//                double h = perlin.getNoise(x, y);
//
//                // Map ~[-1,1] to [0,1] and clamp
//                double mapped = (h + 1.0) * 0.5;
//                int v = (int)std::lround(clamp_value(mapped, 0.0, 1.0) * 255.0);
//                img[row * width + col] = (uint8_t)v;
//            }
//        }
//
//        if (GIF) {
//            char filename[256];
//            std::snprintf(filename, sizeof(filename), "GIF/perlin_%03d.png", frame_idx++);
//            if (!stbi_write_png(filename, width, height, 1, img.data(), width)) {
//                std::cerr << "Failed to write " << filename << "\n";
//                return 1;
//            }
//            std::cout << "Saved " << filename << "\n";
//        }
//    }
//
//    if (GIF) {
//        std::cout << "All frames saved in ./GIF (0°..360° step 12°)\n";
//
//        std::cout << "Generating palette...\n";
//        int ret1 = std::system("ffmpeg -framerate 30 -i \"GIF/perlin_%03d.png\" -vf \"palettegen\" -y \"palette.png\"");
//
//        if (ret1 != 0) {
//            std::cerr << "FFmpeg palette generation failed (code " << ret1 << ")\n";
//            return 1;
//        }
//
//        std::cout << "Generating final GIF...\n";
//        int ret2 = std::system("ffmpeg -framerate 30 -i \"GIF/perlin_%03d.png\" -i \"palette.png\" -lavfi \"paletteuse\" -y \"perlin.gif\"");
//
//        if (ret2 != 0) {
//            std::cerr << "FFmpeg GIF creation failed (code " << ret2 << ")\n";
//            return 1;
//        }
//
//        std::cout << "perlin.gif successfully created!\n";
//        return 0;
//    } else {
//        if (!stbi_write_png("terrain.png", width, height, 1, img.data(), width)) {
//            std::cerr << "Failed to write PNG\n";
//            return 1;
//        }
//
//        std::cout << "Saved terrain.png (scalex=" << scale_x << ", scaley=" << scale_y << ")\n";
//        return 0;
//    }
//}
