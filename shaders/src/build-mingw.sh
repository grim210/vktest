#!/bin/sh

# Compile the vertex shader
/c/VulkanSDK/1.0.21.0/Bin32/glslangValidator.exe -V ./test.vert

# Compile the fragment shader
/c/VulkanSDK/1.0.21.0/Bin32/glslangValidator.exe -V ./test.frag

mv ./vert.spv ../test.vert.spv
mv ./frag.spv ../test.frag.spv
