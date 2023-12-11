#glslc ../shaders/simple.vert -o shaders/vert.spv
glslangValidator ../shaders/tri.vert -o shaders/vert.spv
#glslc ../shaders/simple.frag -o shaders/frag.spv
glslangValidator ../shaders/tri.frag -o shaders/frag.spv
