#!/bin/sh

BASE=`pwd`

$BASE/build/bin/MCSkinRenderer  \
    windowWidth=600 \
    windowHeight=800 \
    frameWidth=600 \
    frameHeight=800 \
    vertexShader=$BASE/shaders/model_vertex_shader.glsl \
    fragmentShader=$BASE/shaders/model_fragment_shader.glsl \
    input=$BASE/resource/tinylib.png \
    output=$BASE/output.png \
    background=$BASE/resource/testpng.png \
    bgVertexShader=$BASE/shaders/bg_vertex_shader.glsl \
    bgFragmentShader=$BASE/shaders/bg_fragment_shader_plain.glsl \
    outputVertexShader=$BASE/shaders/bg_vertex_shader.glsl \
    outputFragmentShader=$BASE/shaders/bg_fragment_shader_image.glsl \
    model=$BASE/resource/Steve.obj \
    modelConfig=$BASE/resource/default.mconf \
    thinArm=0 \
    keepWindow=1 \
    sizeMultiplier=2
