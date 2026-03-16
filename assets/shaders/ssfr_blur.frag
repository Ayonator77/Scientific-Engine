#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_colorTexture; // The raw, bumpy linear depth

void main() {
    float centerZ = texture(u_colorTexture, TexCoords).r;
    
    // If there is no water here, don't waste GPU cycles blurring empty space
    if (centerZ == 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec2 texelSize = 1.0 / textureSize(u_colorTexture, 0);
    float sum = 0.0;
    float weightSum = 0.0;
    
    float blurRadius = 10.0; // 7x7 kernel
    float spatialSig = 5.0; // How far the blur reaches spatially
    float depthSig = 0.15;   // How strict the depth cutoff is (prevents bleeding)

    for (float x = -blurRadius; x <= blurRadius; ++x) {
        for (float y = -blurRadius; y <= blurRadius; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float sampleZ = texture(u_colorTexture, TexCoords + offset).r;

            // Don't blur with the sky
            if (sampleZ == 0.0) continue;

            // Spatial weight (Standard Gaussian)
            float spatialWeight = exp(-(x*x + y*y) / (2.0 * spatialSig * spatialSig));
            
            // Depth weight (The "Bilateral" part - penalizes samples that are too far away)
            float depthDiff = centerZ - sampleZ;
            float depthWeight = exp(-(depthDiff * depthDiff) / (2.0 * depthSig * depthSig));

            float weight = spatialWeight * depthWeight;
            sum += sampleZ * weight;
            weightSum += weight;
        }
    }

    // Write the smoothed depth to the new texture!
    FragColor = vec4(sum / weightSum, 0.0, 0.0, 1.0);
}