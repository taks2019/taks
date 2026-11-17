#pragma once
#include <cstddef>
#include <cstdint>

namespace taks::RealModels {
    // Operates on the Android GLSurfaceView GL thread.
    // Vertices are genuine UV-mapped triangles, not generated primitive shapes.
    bool init();
    void shutdown();
    bool upload(int id,const std::uint8_t* bytes,std::size_t size,
                const std::uint32_t* argb,int width,int height);
    // Offline authored photo-textured geometry, positioned by the saved ecology.
    void render(const float* viewProjection,const float* worldNodes,
                const float* grass, const float* trees, bool fernApproved,
                float worldClock, float rain);
    bool complete();
    const char* errorMessage();
}
