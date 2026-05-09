[[vk::image_format("rgba8")]]
RWTexture2D<float4> outImage : register(u0, space1);
RWStructuredBuffer<uint> sandPositionsOut : register(u1, space1);
RWStructuredBuffer<uint> sandPositionsIn : register(u2, space1);

const static int numThreads = 64;
const static float sizeX = 1000.0f;
const static float sizeY = 1000.0f;
const static int totalSize = 1000 * 1000;// sizeX * sizeY
const static int typeSizeRatio = 32.0f / 8.0f;
//const static int passesPerThread = ceil(totalSize / 4 / numThreads);
const static int outOfBounds = -1;
const static int air = 0;
const static int sand = 1;
const static float4 airColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
const static float4 sandColor = float4(0.8f, 1.0f, 0.2f, 1.0f);

/*
0    1    2    3       4    5    6    7
0                      1
0    1    2    3       0    1    2    3
xxxx xxxx xxxx xxxx    xxxx xxxx xxxx xxxx
*/

// Helper functions to extract 8-bit integers from a 32-bit integer
uint extract8Bit(uint packed, int index) {
    return (packed >> (index * 8)) & 0xFF;
}

uint pack8Bit(uint value0, uint value1, uint value2, uint value3) {
    return (value0 & 0xFF) | ((value1 & 0xFF) << 8) | ((value2 & 0xFF) << 16) | ((value3 & 0xFF) << 24);
}

int2 idxToCoord(int idx) {
    return int2(idx % sizeX, idx / sizeX);
}

int coordToIdx(int x, int y) {
    return y * sizeX + x;
}

int getValue(int idx) {
    // idx is in range 0-64 or -1 for OOB (for 8x8 grid)
    if (idx == outOfBounds) {
        return outOfBounds;
    }
    float superLocation = floor(float(idx) / typeSizeRatio);
    float superLocationIdx = float(idx) % typeSizeRatio;
    return extract8Bit(sandPositionsIn[superLocation], superLocationIdx);
}

int getBottomIdx(int idx) {
    if (idx + sizeX >= totalSize) {
        return outOfBounds;
    }
    return idx + sizeX;
}

int getTopIdx(int idx) {
    if (idx - sizeX < 0) {
        return outOfBounds;
    }
    return idx - sizeX;
}

int getBottomRightIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x >= sizeX - 1 || coords.y >= sizeY - 1) {
        return outOfBounds;
    }
    return idx + sizeX + 1;
}

int getTopRightIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x >= sizeX - 1 || coords.y <= 0) {
        return outOfBounds;
    }
    return idx - sizeX + 1;
}

int getBottomLeftIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x <= 0 || coords.y >= sizeY - 1) {
        return outOfBounds;
    }
    return idx + sizeX - 1;
}

int getTopLeftIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x <= 0 || coords.y <= 0) {
        return outOfBounds;
    }
    return idx - sizeX - 1;
}

int getRightIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x >= sizeX - 1) {
        return outOfBounds;
    }
    return idx + 1;
}

int getLeftIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x <= 0) {
        return outOfBounds;
    }
    return idx - 1;
}

int getDoubleLeftIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x <= 1) {
        return outOfBounds;
    }
    return idx - 2;
}

int getBottomDoubleLeftIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x <= 1 || coords.y >= sizeY - 1) {
        return outOfBounds;
    }
    return idx + sizeX - 2;
}

int getDoubleRightIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x >= sizeX - 2) {
        return outOfBounds;
    }
    return idx + 2;
}

int getTopDoubleRightIdx(int idx) {
    int2 coords = idxToCoord(idx);
    if (coords.x >= sizeX - 2 || coords.y <= 0) {
        return outOfBounds;
    }
    return idx - sizeX + 2;
}

int getCurrent(int idx) {
    return getValue(idx);
}

int getBottom(int idx) {
    return getValue(getBottomIdx(idx));
}

int getTop(int idx) {
    return getValue(getTopIdx(idx));
}

int getBottomRight(int idx) {
    return getValue(getBottomRightIdx(idx));
}

int getTopRight(int idx) {
    return getValue(getTopRightIdx(idx));
}

int getBottomLeft(int idx) {
    return getValue(getBottomLeftIdx(idx));
}

int getTopLeft(int idx) {
    return getValue(getTopLeftIdx(idx));
}

int getRight(int idx) {
    return getValue(getRightIdx(idx));
}

int getLeft(int idx) {
    return getValue(getLeftIdx(idx));
}

int getDoubleLeft(int idx) {
    return getValue(getDoubleLeftIdx(idx));
}

int getDoubleRight(int idx) {
    return getValue(getDoubleRightIdx(idx));
}

int getBottomDoubleLeft(int idx) {
    return getValue(getBottomDoubleLeftIdx(idx));
}

int getTopDoubleRight(int idx) {
    return getValue(getTopDoubleRightIdx(idx));
}

void addToImage(int particleType, int idx) {
    int2 coords = idxToCoord(idx);
    outImage[coords] = particleType == air ? airColor : sandColor;
}

//groupshared uint sharedData[totalSize];

[numthreads(numThreads, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint packed = GlobalInvocationID.x;
    int bitValues[typeSizeRatio];
    for (int packedIdx = 0; packedIdx < typeSizeRatio; packedIdx++) {
        uint idx = packed * typeSizeRatio + packedIdx;
        int filled = getCurrent(idx);
        /* filled = sand;
        bitValues[packedIdx] = filled; */
        //int2 coords = idxToCoord(idx);
        //outImage[coords] = float4(1.0f, 0.0f, 0.0f, 1.0f);
        if (filled == air) {
            int doubleRight = getDoubleRight(idx);
            int topDoubleRight = getTopDoubleRight(idx);
            filled = getTopRight(idx) == sand && getRight(idx) == sand && (doubleRight == sand || doubleRight == outOfBounds || topDoubleRight == sand || topDoubleRight == outOfBounds) ? sand : air;
            filled = getTopLeft(idx) == sand && getLeft(idx) == sand ? sand : filled;
            filled = getTop(idx) == sand ? sand : filled;
            bitValues[packedIdx] = filled;
            addToImage(filled, idx);
            continue;
        }
        if (filled == sand) {
            int doubleLeft = getDoubleLeft(idx);
            int bottomDoubleLeft = getBottomDoubleLeft(idx);
            filled = getBottomLeft(idx) == air && getLeft(idx) == air && (doubleLeft == air || doubleLeft == outOfBounds || bottomDoubleLeft == air || bottomDoubleLeft == outOfBounds) ? air : sand;
            filled = getBottomRight(idx) == air && getRight(idx) == air ? air : filled;
            filled = getBottom(idx) == air ? air : filled;
            bitValues[packedIdx] = filled;
            addToImage(filled, idx);
            continue;
        }
    }
    sandPositionsOut[packed] = pack8Bit(bitValues[0], bitValues[1], bitValues[2], bitValues[3]);

    /* for (int passNum = 0; passNum < passesPerThread; passNum++) {
        int packed = (GlobalInvocationID.x * passesPerThread + passNum) * typeSizeRatio;
        if (packed + 3 > totalSize) {
            break;
        }
        for (int packedIdx = 0; packedIdx < typeSizeRatio; packedIdx++) {
            int idx = packed + packedIdx;
            if (getCurrent(idx) == sand) {
                int dest = idx;
                int doubleLeft = getDoubleLeft(idx);
                int bottomDoubleLeft = getBottomDoubleLeft(idx);
                dest = getBottomLeft(idx) == air && getLeft(idx) == air && (doubleLeft == air || doubleLeft == outOfBounds || bottomDoubleLeft == air || bottomDoubleLeft == outOfBounds) ? getBottomLeftIdx(idx) : dest;
                dest = getBottomRight(idx) == air && getRight(idx) == air ? getBottomRightIdx(idx) : dest;
                dest = getBottom(idx) == air ? getBottomIdx(idx) : dest;
                //sharedData[idx] = air;
                sharedData[dest] = sand;
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    for (int passNum = 0; passNum < passesPerThread; passNum++) {
        int packed = (GlobalInvocationID.x * passesPerThread + passNum) * typeSizeRatio;
        if (packed + 3 > totalSize) {
            return;
        }
        sandPositionsOut[packed] = pack8Bit(sharedData[packed], sharedData[packed + 1], sharedData[packed + 2], sharedData[packed + 3]);
    } */
    /* int packed = GlobalInvocationID.x;
    int packedIdx = GlobalInvocationID.y;
    int idx = packed * typeSizeRatio + packedIdx; */
    //sandPositionsOut[packed] = pack8Bit(1, 1, 1, 1);

    // priority order: B, BR, BL
    /*
    possibilities:
    B  -> F (1)
       -> BR -> R  -> F (3)
                   -> BL -> L -> F (5)
                              -> E (5)
                         -> E (4)
             -> BL -> L  -> F (4)
                         -> E (4)
                   -> E (3)
    (1 + 3 + 5 + 5 + 4 + 4 + 4 + 3) / 8 = 3.625
    */
    /* if (getCurrent(idx) == sand) {
        int dest = idx;
        int doubleLeft = getDoubleLeft(idx);
        int bottomDoubleLeft = getBottomDoubleLeft(idx);
        dest = getBottomLeft(idx) == air && getLeft(idx) == air && (doubleLeft == air || doubleLeft == outOfBounds || bottomDoubleLeft == air || bottomDoubleLeft == outOfBounds) ? getBottomLeftIdx(idx) : dest;
        dest = getBottomRight(idx) == air && getRight(idx) == air ? getBottomRightIdx(idx) : dest;
        dest = getBottom(idx) == air ? getBottomIdx(idx) : dest;
        sharedData[idx] = air;
        sharedData[dest] = sand;
    }

    if (packedIdx == 0) {
        GroupMemoryBarrierWithGroupSync();
        sandPositionsOut[packed] = pack8Bit(sharedData[idx], sharedData[idx + 1], sharedData[idx + 2], sharedData[idx + 3]);
    } */
}