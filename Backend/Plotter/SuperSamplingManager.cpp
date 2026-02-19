#include "SuperSamplingManager.h"
#include <algorithm>

namespace shmea {

SuperSamplingManager::SuperSamplingManager(unsigned int width, unsigned int height, unsigned int factor)
    : originalWidth(width),
      originalHeight(height),
      ssaaFactor(factor > 0 ? factor : 1)
{
    // Calculate supersampled dimensions
    ssaaWidth = originalWidth * ssaaFactor;
    ssaaHeight = originalHeight * ssaaFactor;
    
    // Initialize the buffer
    initializeBuffer();
}

SuperSamplingManager::~SuperSamplingManager() {
    // Image destructor will handle cleanup
}

void SuperSamplingManager::initializeBuffer() {
    // Allocate larger buffer for supersampled rendering
    ssaaImage.Allocate(ssaaWidth, ssaaHeight);

    // Fill with opaque black using bulk operation
    ssaaImage.SetAllPixels(RGBA(0, 0, 0, 0xFF));
}

void SuperSamplingManager::downsampleToOutput(Image& outputImage) {
    // Direct data access for both source and destination — bounds guaranteed by loop structure
    const RGBA* srcPixels = ssaaImage.getData();
    RGBA* dstPixels = outputImage.getData();
    unsigned int totalSamples = ssaaFactor * ssaaFactor;
    unsigned int fullAlpha = totalSamples * 255;
    float invTotalSamples = 1.0f / static_cast<float>(totalSamples);
    static const float inv255 = 1.0f / 255.0f;

    // For each pixel in the output image
    for (unsigned int y = 0; y < originalHeight; ++y) {
        for (unsigned int x = 0; x < originalWidth; ++x) {
            // Accumulate color values from the supersampled region
            unsigned int r = 0, g = 0, b = 0, a = 0;

            // Sample the NxN region from the supersampled image using direct access
            unsigned int baseX = x * ssaaFactor;
            unsigned int baseY = y * ssaaFactor;
            for (unsigned int sy = 0; sy < ssaaFactor; ++sy) {
                const RGBA* srcRow = srcPixels + (baseY + sy) * ssaaWidth + baseX;
                for (unsigned int sx = 0; sx < ssaaFactor; ++sx) {
                    const RGBA& pixel = srcRow[sx];
                    r += pixel.r;
                    g += pixel.g;
                    b += pixel.b;
                    a += pixel.a;
                }
            }

            RGBA& dst = dstPixels[y * originalWidth + x];

            // Fast path: all samples are fully opaque (common case for chart backgrounds)
            if (a == fullAlpha) {
                dst.r = static_cast<unsigned char>(r * invTotalSamples);
                dst.g = static_cast<unsigned char>(g * invTotalSamples);
                dst.b = static_cast<unsigned char>(b * invTotalSamples);
                dst.a = 255;
                continue;
            }

            // Handle case where alpha is zero
            if (a == 0) {
                dst = RGBA(0, 0, 0, 0);
                continue;
            }

            // Slow path: mixed alpha — need premultiply/unpremultiply
            // Re-accumulate with alpha premultiplication
            r = 0; g = 0; b = 0;
            for (unsigned int sy = 0; sy < ssaaFactor; ++sy) {
                const RGBA* srcRow = srcPixels + (baseY + sy) * ssaaWidth + baseX;
                for (unsigned int sx = 0; sx < ssaaFactor; ++sx) {
                    const RGBA& pixel = srcRow[sx];
                    float alphaFactor = pixel.a * inv255;
                    r += static_cast<unsigned int>(pixel.r * alphaFactor);
                    g += static_cast<unsigned int>(pixel.g * alphaFactor);
                    b += static_cast<unsigned int>(pixel.b * alphaFactor);
                }
            }

            // Normalize alpha channel
            float avgAlpha = a * invTotalSamples;
            float avgAlphaFactor = avgAlpha * inv255;
            if (avgAlphaFactor < 0.001f) {
                avgAlphaFactor = 0.001f;
            }

            // Unpremultiply alpha
            float invUnpremult = invTotalSamples / avgAlphaFactor;
            dst.r = static_cast<unsigned char>(std::min(255.0f, r * invUnpremult));
            dst.g = static_cast<unsigned char>(std::min(255.0f, g * invUnpremult));
            dst.b = static_cast<unsigned char>(std::min(255.0f, b * invUnpremult));
            dst.a = static_cast<unsigned char>(avgAlpha);
        }
    }
}

void SuperSamplingManager::setSuperSamplingFactor(unsigned int factor) {
    if (factor < 1) factor = 1; // Ensure factor is at least 1
    
    // Only reinitialize if the factor has changed
    if (factor != ssaaFactor) {
        ssaaFactor = factor;
        ssaaWidth = originalWidth * ssaaFactor;
        ssaaHeight = originalHeight * ssaaFactor;
        
        // Reinitialize supersampling buffer
        initializeBuffer();
    }
}

} // namespace shmea