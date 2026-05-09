#pragma once

#include "common.hpp"
#include <vector>
#include <cstdlib>
#include <iostream>
#include <cmath>

enum ParticleType {
    air,
    sand
};

class Map {
public:
    Map(int sizeX, int sizeY, SDL_GPUDevice* context, SDL_Window* window);
    ~Map();
    void quit();
    void update(double dt, int screen_width, int screen_height);
    void draw(SDL_Renderer* renderer, SDL_Window* window, int screen_width, int screen_height);
private:
    int sizeX, sizeY;
    int numParticles;
    char* particleMap;
    const float spawnSize = 20.0;
    SDL_GPUDevice* gpuDevice;
    SDL_GPUComputePipeline* computePipeline;
    SDL_GPUBuffer* particleBufferIn;
    SDL_GPUBuffer* particleBufferOut;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUGraphicsPipeline* drawPipeline;
    SDL_GPUTexture* particlesTexture;
    SDL_GPUSampler* sampler;
    int threadGroupsX;
    int threadGroupsY;
    const int threadsX = 64;
    const int threadsY = 1;
    const float GPUUpdateCooldown = 0.005;
    float GPUUpdateTimer = 0.0;
    void sendToGPU();
    void runComputeShader();
    void getFromGPU();
    void idxToCoord(int idx, int* outX, int* outY);
    void idxToCoord(int idx, float* outX, float* outY);
    int coordToIdx(int x, int y);
    int test = 0;
};