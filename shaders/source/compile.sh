# Requires shadercross CLI installed from SDL_shadercross
for filename in *.vert.hlsl; do
    if [ -f "$filename" ]; then
        shadercross.exe "$filename" -o "../compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross.exe "$filename" -o "../compiled/MSL/${filename/.hlsl/.msl}"
        shadercross.exe "$filename" -o "../compiled/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.frag.hlsl; do
    if [ -f "$filename" ]; then
        shadercross.exe "$filename" -o "../compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross.exe "$filename" -o "../compiled/MSL/${filename/.hlsl/.msl}"
        shadercross.exe "$filename" -o "../compiled/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.comp.hlsl; do
    echo "$filename"
    if [ -f "$filename" ]; then
        shadercross.exe "$filename" -o "../compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross.exe "$filename" -o "../compiled/MSL/${filename/.hlsl/.msl}"
        shadercross.exe "$filename" -o "../compiled/DXIL/${filename/.hlsl/.dxil}"
    fi
done