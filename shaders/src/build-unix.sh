#!/bin/sh

# Compile the vertex shader
../../../Vulkan-LoaderAndValidationLayers/external/glslang/build/StandAlone/glslangValidator -V ./test.vert

# Compile the fragment shader
../../../Vulkan-LoaderAndValidationLayers/external/glslang/build/StandAlone/glslangValidator -V ./test.frag

mv ./vert.spv ../test.vert.spv
mv ./frag.spv ../test.frag.spv
