#include "map.hpp"

Map::Map(int sizeX, int sizeY, SDL_GPUDevice* gpuDevice, SDL_Window* window) {
    this->sizeX = sizeX;
    this->sizeY = sizeY;
    this->numParticles = this->sizeX * this->sizeY;
    this->particleMap = (char*) std::calloc(this->numParticles, sizeof(char));

    //this->particleMap[2] = 1;

    this->gpuDevice = gpuDevice;

    if (this->numParticles % 4 != 0) {
        printf("numParticles not divisible by 4!!!");
        exit(1);
    }

    this->threadGroupsX = (this->numParticles / 4 + this->threadsX - 1) / this->threadsX;
    this->threadGroupsY = 1;

    // create shaders/pipeline

    SDL_GPUShader *vertexShader = LoadShader(this->gpuDevice, "vertShader.vert", 0, 0, 0, 0);
    if (vertexShader == NULL)
    {
        printf("Failed to create vertex shader!\n");
    }

    SDL_GPUShader *fragmentShader = LoadShader(this->gpuDevice, "fragShader.frag", 1, 0, 0, 0);
    if (fragmentShader == NULL)
    {
        printf("Failed to create fragment shader!\n");
    }


    SDL_GPUComputePipelineCreateInfo pipelineCreateInfo{
        .num_readwrite_storage_textures = 1,
        .num_readwrite_storage_buffers = 2,
        .threadcount_x = this->threadsX,
        .threadcount_y = this->threadsY,
        .threadcount_z = 1
    };

    this->computePipeline = CreateComputePipelineFromShader(
        gpuDevice,
        "compShader.comp",
        &pipelineCreateInfo
    );

    SDL_GPUColorTargetDescription colorTarget{
		.format = SDL_GetGPUSwapchainTextureFormat(this->gpuDevice, window)
	};

    SDL_GPUVertexBufferDescription vertexBufferDescription{
        .slot = 0,
        .pitch = sizeof(PositionTextureVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };

    SDL_GPUVertexAttribute vertexAttribute[]{{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0,
    }, {
        .location = 1,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .offset = sizeof(float) * 3
    }};

    SDL_GPUVertexInputState vertexInputState{
        .vertex_buffer_descriptions = &vertexBufferDescription,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertexAttribute,
        .num_vertex_attributes = 2,
    };
    
    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineInfo{
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.target_info = {
			.color_target_descriptions = &colorTarget,
			.num_color_targets = 1
		},
    };

    this->drawPipeline = SDL_CreateGPUGraphicsPipeline(this->gpuDevice, 
        &graphicsPipelineInfo
    );

    SDL_ReleaseGPUShader(this->gpuDevice, vertexShader);
    SDL_ReleaseGPUShader(this->gpuDevice, fragmentShader);

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

    // create texture/sampler

    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    SDL_GPUTextureCreateInfo textureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = this->sizeX,
        .height = this->sizeY,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    this->particlesTexture = SDL_CreateGPUTexture(this->gpuDevice, &textureCreateInfo);

    // Clear the texture to initialize it
    /* SDL_GPUCommandBuffer* initCmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);
    SDL_GPUColorTargetInfo colorTargetInfo{
        .texture = this->particlesTexture,
        .clear_color = {0.0f, 0.0f, 1.0f, 1.0f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false
    };
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        initCmdBuf,
        &colorTargetInfo,
        1,
        NULL
    );
    //SDL_BindGPUGraphicsPipeline(renderPass, this->drawPipeline);
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(initCmdBuf); */

    SDL_GPUSamplerCreateInfo samplerCreateInfo{
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT
    };

    sampler = SDL_CreateGPUSampler(this->gpuDevice, &samplerCreateInfo);

    // create vertex buffer

    SDL_GPUBufferCreateInfo vertexBufferCreateInfo {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(PositionTextureVertex) * 6
    };

    this->vertexBuffer = SDL_CreateGPUBuffer(
        this->gpuDevice,
        &vertexBufferCreateInfo
    );
    
    // transfer vertex data to GPU
    SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);

    SDL_GPUTransferBufferCreateInfo transBufferCreateInfo {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sizeof(PositionTextureVertex) * 6
    };

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
		this->gpuDevice,
        &transBufferCreateInfo
	);

	PositionTextureVertex* transferData = (PositionTextureVertex*)SDL_MapGPUTransferBuffer(
		this->gpuDevice,
		transferBuffer,
		false
	);

    transferData[0] = { -1, -1, 0, 0, 0 };
	transferData[1] = {  1, -1, 0, 1, 0 };
	transferData[2] = {  1,  1, 0, 1, 1 };
	transferData[3] = { -1, -1, 0, 0, 0 };
	transferData[4] = {  1,  1, 0, 1, 1 };
	transferData[5] = { -1,  1, 0, 0, 1 };

	SDL_UnmapGPUTransferBuffer(this->gpuDevice, transferBuffer);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

    SDL_GPUTransferBufferLocation transBufLocation {
        .transfer_buffer = transferBuffer,
        .offset = 0
	};

    SDL_GPUBufferRegion bufRegion {
        .buffer = this->vertexBuffer,
        .offset = 0,
        .size = sizeof(PositionTextureVertex) * 6
	};

	SDL_UploadToGPUBuffer(
		copyPass,
		&transBufLocation,
		&bufRegion,
		false
	);

	SDL_EndGPUCopyPass(copyPass);

	SDL_SubmitGPUCommandBuffer(cmdBuf);

	SDL_ReleaseGPUTransferBuffer(this->gpuDevice, transferBuffer);

    //this->sendToGPU();
    //this->runComputeShader();
}

Map::~Map() {
    std::free(particleMap);
}

void Map::quit() {
    SDL_ReleaseGPUBuffer(this->gpuDevice, this->particleBufferIn);
    SDL_ReleaseGPUBuffer(this->gpuDevice, this->particleBufferOut);
    SDL_ReleaseGPUComputePipeline(this->gpuDevice, this->computePipeline);
    SDL_ReleaseGPUGraphicsPipeline(this->gpuDevice, this->drawPipeline);
    SDL_ReleaseGPUTexture(this->gpuDevice, this->particlesTexture);
    SDL_ReleaseGPUSampler(this->gpuDevice, this->sampler);
}

void Map::update(double dt, int screen_width, int screen_height) {
    //printf("%i\n", this->test);
    this->test++;
    float rectSizeX = (float)screen_width / (float)sizeX;
    float rectSizeY = (float)screen_height / (float)sizeY;
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
    if (mouseButtonFlags & SDL_BUTTON_RMASK) {
        int coordX = std::floor(mouseX / rectSizeX);
        int coordY = std::floor(mouseY / rectSizeY);
        for (int x = coordX - this->spawnSize / 2.0f; x < coordX + this->spawnSize / 2.0f; x++) {
            for (int y = coordY - this->spawnSize / 2.0f; y < coordY + this->spawnSize / 2.0f; y++) {
                if (x >= 0 and x < this->sizeX and y >= 0 and y < this->sizeY) {
                    int idx = this->coordToIdx(x, y);
                    this->particleMap[idx] = ParticleType::air;
                }
            }
        }
    }
    this->sendToGPU();
    this->runComputeShader();
    this->getFromGPU();
    this->GPUUpdateTimer += dt;
    /* if (this->GPUUpdateTimer > this->GPUUpdateCooldown) {
        for (float i = this->GPUUpdateTimer; i > 0.0f; i -= this->GPUUpdateCooldown) {
            this->GPUUpdateTimer = 0.0;
            float LAST = SDL_GetTicks();
            this->sendToGPU();
            this->getFromGPU();
            float NOW = SDL_GetTicks();
            float GPUTime = ((NOW - LAST) * 0.001f);
        }
		//GPUTime *= 0.001;
        //printf("GPU time: %f\n", GPUTime);
    } */
}

void Map::draw(SDL_Renderer* renderer, SDL_Window* window, int screen_width, int screen_height) {
    SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);
    if (cmdBuf == NULL)
    {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
    }
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
    }
    if (swapchainTexture != NULL)
    {
        SDL_GPUColorTargetInfo colorTargetInfo{
            .texture = swapchainTexture,
            .clear_color = {0.0f, 0.0f, 1.0f, 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .cycle = false
        };
        colorTargetInfo.clear_color.a = 1;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
            cmdBuf,
            &colorTargetInfo,
            1,
            NULL
        );

        SDL_BindGPUGraphicsPipeline(renderPass, this->drawPipeline);
        SDL_GPUBufferBinding bufBinding{ .buffer = this->vertexBuffer, .offset = 0 };
        SDL_BindGPUVertexBuffers(renderPass, 0, &bufBinding, 1);
        SDL_GPUTextureSamplerBinding texSamplerBinding{ .texture = this->particlesTexture, .sampler = this->sampler };
        SDL_BindGPUFragmentSamplers(renderPass, 0, &texSamplerBinding, 1);
        SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);

        SDL_EndGPURenderPass(renderPass);
    }

    
    if (true) {
        SDL_SubmitGPUCommandBuffer(cmdBuf);
        //SDL_Delay(300);
    } else {
        SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf);
        if (fence == NULL) {
            SDL_Log("Error getting fence: %s", SDL_GetError());
        }
        SDL_WaitForGPUFences(this->gpuDevice, 1, &fence, 1);
        SDL_free(fence);
    }
}

void Map::sendToGPU() {
    SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);

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

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

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

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf);
    SDL_WaitForGPUFences(this->gpuDevice, 1, &fence, 1);
    SDL_free(fence);
}

void Map::runComputeShader() {
    // run the compute shader
    
    SDL_GPUCommandBuffer* computeCmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);
    
    SDL_GPUStorageBufferReadWriteBinding bufBindings[2] = {
        {.buffer = this->particleBufferOut},
        {.buffer = this->particleBufferIn}
    };
    
    SDL_GPUStorageTextureReadWriteBinding texBindings[1] = {
        {.texture = this->particlesTexture}
    };
    
    SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
        computeCmdBuf,
        texBindings,
        1,
        bufBindings,
        2
    );
    
    SDL_BindGPUComputePipeline(computePass, computePipeline);
    SDL_DispatchGPUCompute(computePass, this->threadGroupsX, this->threadGroupsY, 1);
    SDL_EndGPUComputePass(computePass);
    
    SDL_GPUFence* computeFence = SDL_SubmitGPUCommandBufferAndAcquireFence(computeCmdBuf);
    if (computeFence == NULL) {
        SDL_Log("Error getting fence: %s", SDL_GetError());
    }
    SDL_WaitForGPUFences(this->gpuDevice, 1, &computeFence, 1);
    SDL_free(computeFence);
}

void Map::getFromGPU() {
    SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(this->gpuDevice);

    // Download updated sand positions back to the CPU
    SDL_GPUTransferBufferCreateInfo transBufCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(char) * this->numParticles
    };
    
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        this->gpuDevice,
        &transBufCreateInfo
    );

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    
    SDL_GPUTransferBufferLocation transBufLocation{.transfer_buffer = transferBuffer, .offset = 0};
    SDL_GPUBufferRegion bufRegion{.buffer = this->particleBufferOut, .offset = 0, .size = sizeof(char) * this->numParticles};
    
    SDL_DownloadFromGPUBuffer(
        copyPass,
        &bufRegion,
        &transBufLocation
    );
    
    SDL_EndGPUCopyPass(copyPass);

    //SDL_Delay(100);
    
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf);
    if (fence == NULL) {
        SDL_Log("Error getting fence: %s", SDL_GetError());
    }
    SDL_WaitForGPUFences(this->gpuDevice, 1, &fence, 1);
    SDL_free(fence);
    
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