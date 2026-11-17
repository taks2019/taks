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
    void render(const float* viewProjection,const float* worldNodes);
    bool complete();
    const char* errorMessage();
}
