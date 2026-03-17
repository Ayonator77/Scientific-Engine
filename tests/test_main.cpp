#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "geometry/Icosahedron.h"
#include "physics/SphParams.h"

#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "geometry/Icosahedron.h"
#include "physics/SphParams.h"

// ==============================================================================
// TEST ENVIRONMENT: HEADLESS OPENGL CONTEXT
// This spins up an invisible OpenGL context to allow VAO/VBO generation in tests.
// ==============================================================================
class HeadlessGraphicsEnvironment : public ::testing::Environment {
private:
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;

public:
    void SetUp() override {
        ASSERT_EQ(SDL_Init(SDL_INIT_VIDEO), 0) << "SDL Failed to initialize";
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        // Create an invisible window purely for the OpenGL context
        window = SDL_CreateWindow("V&V Headless", 0, 0, 100, 100, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        ASSERT_NE(window, nullptr) << "Headless window creation failed";

        context = SDL_GL_CreateContext(window);
        ASSERT_NE(context, nullptr) << "OpenGL context creation failed";

        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) << "GLAD failed to load";
    }

    void TearDown() override {
        if (context) SDL_GL_DeleteContext(context);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

// ==============================================================================
// V&V PHASE 1: ALGORITHMIC DETERMINISM
// ==============================================================================

// Replicate the GPU hash function to prove mathematical determinism on the CPU
uint32_t CalculateSpatialHash(int x, int y, int z, uint32_t tableSize) {
    return (uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u ^ uint32_t(z) * 83492791u) % tableSize;
}

TEST(SpatialHashingValidation, DeterminismAndBounds) {
    const uint32_t tableSize = 262144; // Standard power-of-two (2^18)
    
    // Test 1: Determinism (Identical inputs must produce identical outputs)
    uint32_t hash1 = CalculateSpatialHash(10, -5, 42, tableSize);
    uint32_t hash2 = CalculateSpatialHash(10, -5, 42, tableSize);
    EXPECT_EQ(hash1, hash2) << "Hash function is not deterministic!";

    // Test 2: Avalanche Effect (Adjacent physical cells should not heavily collide)
    uint32_t hash3 = CalculateSpatialHash(11, -5, 42, tableSize);
    EXPECT_NE(hash1, hash3) << "Hash function lacks avalanche properties!";
    
    // Test 3: Array Bounds Guarantee (Modulo must strictly constrain output)
    EXPECT_LT(hash1, tableSize) << "Hash output exceeded buffer size!";
    EXPECT_LT(hash3, tableSize) << "Hash output exceeded buffer size!";
}

// ==============================================================================
// V&V PHASE 2: GEOMETRIC INTEGRITY
// ==============================================================================
TEST(GeometryValidation, IcosahedronSubdivisionCounts) {
    // A base icosahedron mathematically guarantees 12 vertices and 20 faces (60 indices)
    Icosahedron baseIco; 
    EXPECT_EQ(baseIco.GetVertices().size(), 12) << "Base vertex count violated";
    EXPECT_EQ(baseIco.GetIndices().size(), 60) << "Base index count violated";

    // 1st Subdivision mathematically guarantees:
    // Vertices: V + E = 12 + 30 = 42
    // Faces: 4 * F = 80 (240 indices)
    Icosahedron sub1Ico;
    sub1Ico.Subdivide(1); // Call the subdivision method explicitly
    EXPECT_EQ(sub1Ico.GetVertices().size(), 42) << "Subdivision 1 vertex count violated";
    EXPECT_EQ(sub1Ico.GetIndices().size(), 240) << "Subdivision 1 index count violated";
}

// ==============================================================================
// V&V PHASE 3: PHYSICS PARAMETER SAFETY
// ==============================================================================
TEST(PhysicsValidation, SafeInitialization) {
    SphParams defaultParams;
    
    // Test to ensure default parameters do not trigger division-by-zero in compute shaders
    EXPECT_GT(defaultParams.target_density, 0.0f) << "Target density must be > 0";
    EXPECT_GT(defaultParams.smoothing_radius, 0.0f) << "Smoothing radius must be > 0";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Register the headless OpenGL context to spin up before tests run
    ::testing::AddGlobalTestEnvironment(new HeadlessGraphicsEnvironment());
    
    return RUN_ALL_TESTS();
}