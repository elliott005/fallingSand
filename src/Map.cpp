#include "map.hpp"

Map::Map(int sizeX, int sizeY, SDL_GPUDevice* gpuDevice) {
    this->sizeX = sizeX;
    this->sizeY = sizeY;
    this->numParticles = this->sizeX * this->sizeY;
    this->particleMap = (char*) std::calloc(this->numParticles, sizeof(char));

    this->particleMap[2] = 1;

    this->gpuDevice = gpuDevice;

    if (this->numParticles % 4 != 0) {
        printf("numParticles not divisible by 4!!!");
        exit(1);
    }

    this->threadGroupsX = (this->numParticles / 4 + this->threadsX - 1) / this->threadsX;
    this->threadGroupsY = 1;

    SDL_GPUComputePipelineCreateInfo pipelineCreateInfo{.num_readwrite_storage_buffers = 2, .threadcount_x = this->threadsX, .threadcount_y = this->threadsY, .threadcount_z = 1};

    this->computePipeline = CreateComputePipelineFromShader(
        gpuDevice,
        "compShader.comp",
        &pipelineCreateInfo
    );

    // Allocate GPU buffer for particle data
    
    SDL_GPUBufferCreateInfo bufferCreateInfoIn{.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ, .size = sizeof(char) * this->numParticles};
    
    this->particleBufferIn = SDL_CreateGPUBuffer(
        gpuDevice,
        &bufferCreateInfoIn
    );
    
    SDL_GPUBufferCreateInfo bufferCreateInfoOut{.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ, .size = sizeof(char) * this->numParticles};
    
    this->particleBufferOut = SDL_CreateGPUBuffer(
        gpuDevice,
        &bufferCreateInfoOut
    );
}

Map::~Map() {
    std::free(particleMap);
}

void Map::quit() {
    SDL_ReleaseGPUBuffer(this->gpuDevice, this->particleBufferIn);
    SDL_ReleaseGPUBuffer(this->gpuDevice, this->particleBufferOut);
    SDL_ReleaseGPUComputePipeline(this->gpuDevice, this->computePipeline);
}

void Map::update(double dt, int screen_width, int screen_height) {
    float rectSizeX = screen_width / sizeX;
    float rectSizeY = screen_height / sizeY;
    float mouseX, mouseY;
    SDL_MouseButtonFlags mouseButtonFlags = SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseButtonFlags & SDL_BUTTON_LMASK) {
        int coordX = std::floor(mouseX / rectSizeX);
        int coordY = std::floor(mouseY / rectSizeY);
        for (int x = coordX - this->spawnSize / 2.0f; x < coordX + this->spawnSize / 2.0f; x++) {
            for (int y = coordY - this->spawnSize / 2.0f; y < coordY + this->spawnSize / 2.0f; y++) {
                if (x >= 0 and x < this->sizeX and y >= 0 and y < this->sizeY) {
                    int idx = this->coordToIdx(x, y);
                    this->particleMap[idx] = ParticleType::sand;
                }
            }
        }
    }
    this->GPUUpdateTimer += dt;
    if (this->GPUUpdateTimer > this->GPUUpdateCooldown) {
        this->GPUUpdateTimer = 0.0;
        float LAST = SDL_GetTicks();
        this->sendToGPU();
        this->getFromGPU();
		float NOW = SDL_GetTicks();
		float GPUTime = ((NOW - LAST) * 0.001f);
		//GPUTime *= 0.001;
        //printf("GPU time: %f\n", GPUTime);
    }
}

void Map::draw(SDL_Renderer* renderer, int screen_width, int screen_height) {
    int rectSizeX = screen_width / sizeX;
    int rectSizeY = screen_height / sizeY;
    SDL_FRect rect;
    rect.w = rectSizeX;
    rect.h = rectSizeY;
    int totalSand = 0;
    for (int i = 0; i < this->numParticles; i++) {
        enum ParticleType t = (enum ParticleType)particleMap[i];
        if (t == ParticleType::air) {
            continue;
        }
        if (t == ParticleType::sand) {
            totalSand++;
            idxToCoord(i, &rect.x, &rect.y);
            rect.x *= rect.w;
            rect.y *= rect.h;
            SDL_SetRenderDrawColor(renderer, 0, 255, 100, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    //printf("total sand: %i\n", totalSand);
}

void Map::sendToGPU() {
    this->cmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);

    SDL_GPUTransferBufferCreateInfo transBufCreateInfo{.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size=sizeof(char) * this->numParticles};
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        this->gpuDevice,
        &transBufCreateInfo
	);
    
	char* transferData = (char*)SDL_MapGPUTransferBuffer(
        this->gpuDevice,
		transferBuffer,
		false
	);
    memcpy(transferData, this->particleMap, sizeof(char) * this->numParticles);

    SDL_UnmapGPUTransferBuffer(this->gpuDevice, transferBuffer);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(this->cmdBuf);

    SDL_GPUTransferBufferLocation transBufLocation{.transfer_buffer = transferBuffer, .offset = 0};
    SDL_GPUBufferRegion bufRegion{.buffer = this->particleBufferIn, .offset = 0, .size = sizeof(char) * this->numParticles};

    SDL_UploadToGPUBuffer(
		copyPass,
		&transBufLocation,
        &bufRegion,
		false
	);
    
    SDL_EndGPUCopyPass(copyPass);
    
    SDL_ReleaseGPUTransferBuffer(this->gpuDevice, transferBuffer);

    // run the compute shader

    SDL_GPUStorageBufferReadWriteBinding bufBindings[2] = {
        {.buffer = this->particleBufferIn},
        {.buffer = this->particleBufferOut}
    };

    SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
        cmdBuf,
        NULL,
        0,
        bufBindings,
        2
    );

    SDL_BindGPUComputePipeline(computePass, computePipeline);
    SDL_DispatchGPUCompute(computePass, this->threadGroupsX, this->threadGroupsY, 1);
    SDL_EndGPUComputePass(computePass);
    
    SDL_SubmitGPUCommandBuffer(this->cmdBuf);
}

void Map::getFromGPU() {
    this->cmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);

    // Download updated sand positions back to the CPU
    SDL_GPUTransferBufferCreateInfo transBufCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(char) * this->numParticles
    };
    
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        this->gpuDevice,
        &transBufCreateInfo
    );

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(this->cmdBuf);
    
    SDL_GPUTransferBufferLocation transBufLocation{.transfer_buffer = transferBuffer, .offset = 0};
    SDL_GPUBufferRegion bufRegion{.buffer = this->particleBufferOut, .offset = 0, .size = sizeof(char) * this->numParticles};
    
    SDL_DownloadFromGPUBuffer(
        copyPass,
        &bufRegion,
        &transBufLocation
    );
    
    SDL_EndGPUCopyPass(copyPass);
    
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(this->cmdBuf);
    SDL_WaitForGPUFences(this->gpuDevice, 1, &fence, 1);
    
    char* transferData = (char*)SDL_MapGPUTransferBuffer(
        this->gpuDevice,
        transferBuffer,
        true
    );
    
    memcpy(this->particleMap, transferData, sizeof(char) * this->numParticles);
    
    SDL_UnmapGPUTransferBuffer(this->gpuDevice, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(this->gpuDevice, transferBuffer);
}

void Map::idxToCoord(int idx, int* outX, int* outY) {
    *outX = idx % this->sizeX;
    *outY = idx / this->sizeX;
}

void Map::idxToCoord(int idx, float* outX, float* outY) {
    *outX = idx % this->sizeX;
    *outY = idx / this->sizeX;
}

int Map::coordToIdx(int x, int y) {
    return y * this->sizeX + x;
}