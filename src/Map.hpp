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
    Map(int sizeX, int sizeY, SDL_GPUDevice* context);
    ~Map();
    void quit();
    void update(double dt, int screen_width, int screen_height);
    void draw(SDL_Renderer* renderer, int screen_width, int screen_height);
private:
    int sizeX, sizeY;
    int numParticles;
    char* particleMap;
    const float spawnSize = 10.0;
    SDL_GPUDevice* gpuDevice;
    SDL_GPUComputePipeline* computePipeline;
    SDL_GPUCommandBuffer* cmdBuf;
    SDL_GPUBuffer* particleBufferIn;
    SDL_GPUBuffer* particleBufferOut;
    int threadGroupsX;
    int threadGroupsY;
    const int threadsX = 64;
    const int threadsY = 1;
    const float GPUUpdateCooldown = 0.005;
    float GPUUpdateTimer = 0.0;
    void sendToGPU();
    void getFromGPU();
    void idxToCoord(int idx, int* outX, int* outY);
    void idxToCoord(int idx, float* outX, float* outY);
    int coordToIdx(int x, int y);
};