#pragma once
#include <glad/glad.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <vector>
#include <cstdint>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

struct GLMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
};

class Perlin {
public:
    explicit Perlin(uint64_t seed, double phase = 0.0);

    // Returns noise in [0,1]
    double getNoise(double x, double y);

    // Fills a width*height grayscale map (0..255)
    void getHeatmap(std::vector<uint8_t>& img,
        int width, int height,
        double scale_x, double scale_y);

    // Builds a grid mesh from a grayscale image
    Mesh getMesh(const std::vector<uint8_t>& img,
        int width, int height,
        double scale_height = 1.0f);

    // Upload the mesh to the OpenGl context
    GLMesh uploadMesh(const Mesh& m);

private:
    double   phase_;
    uint64_t seed_;

    inline double fade(double t) {
        return ((6 * t - 15) * t + 10) * t * t * t;
    }

    // linear interpolation
    inline double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }

    inline uint64_t hashing(int i, int j) {
        struct {
            int ii, jj;
        } coords{ i, j };
        return XXH64(&coords, sizeof(coords), seed_);
    }

    static inline double dot(double x1, double y1, double x2, double y2) {
        return x1 * x2 + y1 * y2;
    }
};
