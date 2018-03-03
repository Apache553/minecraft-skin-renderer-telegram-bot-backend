
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>

struct PipelineInfo {
    GLuint vertexShaderHandle;
    GLuint fragmentShaderHandle;
    GLuint programHandle;
};

namespace Global {
std::map<std::string, std::string> arguments;

std::string vertexShaderPath;
std::string fragmentShaderPath;

std::string bgVertexShaderPath;
std::string bgFragmentShaderPath;

std::string inputFilePath;
std::string outputFilePath;
std::string backgroundPath;
std::string modelPath;
std::string modelConfigPath;

bool thinArm = false;
bool keepWindow = false;

GLFWwindow *mainWindow;

int windowWidth = 800;
int windowHeight = 600;

int frameWidth = 800;
int frameHeight = 600;

const int signatureLength = 4;

PipelineInfo modelPipelineInfo;
PipelineInfo backgroundPipelineInfo;
}  // namespace Global

#include "model.cpp"
#include "image.cpp"

GLuint GetTextureFromImage(const ImageData &image) {
    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
    glFlush();
    return textureHandle;
}

void SaveImage() {
    ImageData image;
    image.bytePerPixel = 3;
    image.width = Global::frameWidth;
    image.height = Global::frameHeight;
    // get data from OpenGL
    image.data = new unsigned char[image.height * image.width * image.bytePerPixel];
    glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data);
    // for (size_t i = 0; i < Global::frameWidth; ++i) {
    //     for (size_t byteId = 0; byteId < image.bytePerPixel; ++byteId) {
    //         std::cout << std::setw(2) << std::setfill('0') << std::hex
    //                   << (static_cast<unsigned int>(image.data[i * image.bytePerPixel + byteId]) & 0xFF);
    //     }
    //     std::cout << ' ';
    // }
    // std::cout << std::endl;
    // initialize output
    WriteImageDataToPNG(image, Global::outputFilePath, true);
    delete[] image.data;
}

void ParseArguments(int argc, char **argv) {
    std::regex pattern("(.*?)=(.*)");
    for (int index = 1; index < argc; ++index) {
        std::cmatch matches;
        bool isFound = std::regex_match(argv[index], matches, pattern);
        if (isFound) {
            Global::arguments.insert(std::make_pair(matches[1], matches[2]));
        } else {
            std::cout << "Illegal argument: " << argv[index] << std::endl;
        }
    }
}

void DumpArguments() {
    std::cout << "DEBUG: Dumping all command line arguments...\n#####\n";
    for (auto &argument : Global::arguments) {
        std::cout << argument.first << " = " << argument.second << std::endl;
    }
    std::cout << "#####\n";
}

void ApplyArguments() {
    if (Global::arguments.find("windowWidth") != Global::arguments.end()) {
        char *ptr;
        Global::windowWidth = strtoul(Global::arguments["windowWidth"].c_str(), &ptr, 10);
    }
    if (Global::arguments.find("windowHeight") != Global::arguments.end()) {
        char *ptr;
        Global::windowHeight = strtoul(Global::arguments["windowHeight"].c_str(), &ptr, 10);
    }
    if (Global::arguments.find("frameWidth") != Global::arguments.end()) {
        char *ptr;
        Global::frameWidth = strtoul(Global::arguments["frameWidth"].c_str(), &ptr, 10);
    }
    if (Global::arguments.find("frameHeight") != Global::arguments.end()) {
        char *ptr;
        Global::frameHeight = strtoul(Global::arguments["frameHeight"].c_str(), &ptr, 10);
    }
    if (Global::arguments.find("vertexShader") != Global::arguments.end()) {
        Global::vertexShaderPath = Global::arguments["vertexShader"];
    }
    if (Global::arguments.find("fragmentShader") != Global::arguments.end()) {
        Global::fragmentShaderPath = Global::arguments["fragmentShader"];
    }
    if (Global::arguments.find("input") != Global::arguments.end()) {
        Global::inputFilePath = Global::arguments["input"];
    }
    if (Global::arguments.find("output") != Global::arguments.end()) {
        Global::outputFilePath = Global::arguments["output"];
    }
    if (Global::arguments.find("background") != Global::arguments.end()) {
        Global::backgroundPath = Global::arguments["background"];
    }
    if (Global::arguments.find("bgVertexShader") != Global::arguments.end()) {
        Global::bgVertexShaderPath = Global::arguments["bgVertexShader"];
    }
    if (Global::arguments.find("bgFragmentShader") != Global::arguments.end()) {
        Global::bgFragmentShaderPath = Global::arguments["bgFragmentShader"];
    }
    if (Global::arguments.find("model") != Global::arguments.end()) {
        Global::modelPath = Global::arguments["model"];
    }
    if (Global::arguments.find("modelConfig") != Global::arguments.end()) {
        Global::modelConfigPath = Global::arguments["modelConfig"];
    }
    if (Global::arguments.find("thinArm") != Global::arguments.end()) {
        char *ptr;
        unsigned int value = strtoul(Global::arguments["thinArm"].c_str(), &ptr, 10);
        if (value == 0) {
            Global::thinArm = false;
        } else {
            Global::thinArm = true;
        }
    }
    if (Global::arguments.find("keepWindow") != Global::arguments.end()) {
        char *ptr;
        unsigned int value = strtoul(Global::arguments["keepWindow"].c_str(), &ptr, 10);
        if (value == 0) {
            Global::keepWindow = false;
        } else {
            Global::keepWindow = true;
        }
    }
}

std::string GetFileContent(const std::string &filename, const std::string &reason, size_t bufferSize = 1024) {
    std::ifstream input(filename, std::ios::binary | std::ios::in);
    if (!input.is_open()) {
        std::cerr << "ERROR: open file \'" << filename << "\' for \'" << reason << "\' failed!" << std::endl;
        exit(-1);
    }
    std::string content;
    char *buffer = new char[bufferSize];
    while (!input.eof()) {
        input.read(buffer, bufferSize);
        content.append(buffer, input.gcount());
    }
    delete[] buffer;
    return content;
}

std::string GetGLShaderLog(GLuint shaderHandle) {
    int length;
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "ERROR: Unable to get shader log length!" << std::endl;
        return std::string();
    }
    char *buffer = new char[length];
    int actualLength;
    glGetShaderInfoLog(shaderHandle, length, &actualLength, buffer);
    std::string infoLog;
    infoLog.assign(buffer, actualLength);
    delete[] buffer;
    return infoLog;
}

std::string GetGLProgramLog(GLuint programHandle) {
    int length;
    glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &length);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "ERROR: Unable to get program log length!" << std::endl;
        return std::string();
    }
    char *buffer = new char[length];
    int actualLength;
    glGetProgramInfoLog(programHandle, length, &actualLength, buffer);
    std::string infoLog;
    infoLog.assign(buffer, actualLength);
    delete[] buffer;
    return infoLog;
}

PipelineInfo SynthesizePipeline(std::string vertexShaderPath, std::string fragmentShaderPath) {
    int status;
    const char *_ref;
    PipelineInfo info;
    info.programHandle = glCreateProgram();
    // process vertex shader
    info.vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    auto vertexShaderContent = GetFileContent(vertexShaderPath, "vertex shader");
    _ref = vertexShaderContent.c_str();
    glShaderSource(info.vertexShaderHandle, 1, &_ref, nullptr);
    glCompileShader(info.vertexShaderHandle);
    glGetShaderiv(info.vertexShaderHandle, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        std::cerr << "ERROR: Unable to compile vertex shader" << std::endl;
        std::cerr << GetGLShaderLog(info.vertexShaderHandle) << std::endl;
        exit(-1);
    }
    // process fragment shader
    info.fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragmentShaderContent = GetFileContent(fragmentShaderPath, "fragment shader");
    _ref = fragmentShaderContent.c_str();
    glShaderSource(info.fragmentShaderHandle, 1, &_ref, nullptr);
    glCompileShader(info.fragmentShaderHandle);
    glGetShaderiv(info.fragmentShaderHandle, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        std::cerr << "ERROR: Unable to compile fragment shader" << std::endl;
        std::cerr << GetGLShaderLog(info.fragmentShaderHandle) << std::endl;
        exit(-1);
    }
    // synthesize pipeline
    glAttachShader(info.programHandle, info.vertexShaderHandle);
    glAttachShader(info.programHandle, info.fragmentShaderHandle);
    glLinkProgram(info.programHandle);
    glGetProgramiv(info.programHandle, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        std::cerr << "ERROR: Unable to link fragment shader" << std::endl;
        std::cerr << GetGLProgramLog(info.programHandle) << std::endl;
        exit(-1);
    }
    return info;
}

void Initizalize() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    std::cout << "INFO: window is " << Global::windowWidth << '*' << Global::windowHeight << std::endl;
    Global::mainWindow =
        glfwCreateWindow(Global::windowWidth, Global::windowHeight, "Minecraft Skin Renderer", NULL, NULL);
    if (Global::mainWindow == NULL) {
        std::cerr << "Create main window failed!" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(Global::mainWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Initialize GLAD failed!" << std::endl;
        exit(-1);
    }

    glViewport(0, 0, Global::windowWidth, Global::windowHeight);
    std::cout << "INFO: window framebuffer is " << Global::frameWidth << '*' << Global::frameHeight << std::endl;
    Global::modelPipelineInfo = SynthesizePipeline(Global::vertexShaderPath, Global::fragmentShaderPath);
    Global::backgroundPipelineInfo = SynthesizePipeline(Global::bgVertexShaderPath, Global::bgFragmentShaderPath);
}

void RenderBackground() {
    float vertexInfo[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                          1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};
    glUseProgram(Global::backgroundPipelineInfo.programHandle);

    GLuint vertexArrayHandle;
    GLuint vertexArrayBufferHandle;
    GLuint textureHandle;

    glGenVertexArrays(1, &vertexArrayHandle);
    glGenBuffers(1, &vertexArrayBufferHandle);

    glBindVertexArray(vertexArrayHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexInfo), vertexInfo, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    if (!Global::backgroundPath.empty()) {
        ImageData image = GetImageDataFromPNG(Global::backgroundPath, Global::signatureLength, true);
        textureHandle = GetTextureFromImage(image);
        delete[] image.data;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLuint samplerLocation = glGetUniformLocation(Global::backgroundPipelineInfo.programHandle, "textureSampler");
        glUniform1i(samplerLocation, 0);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glFlush();

    glDeleteBuffers(1, &vertexArrayBufferHandle);
    glDeleteVertexArrays(1, &vertexArrayHandle);

    if (!Global::backgroundPath.empty()) {
        glDeleteTextures(1, &textureHandle);
    }
}

inline void TransformVertices(std::vector<glm::vec4> &vertices, const glm::mat4 &transformMatrix) {
    for (auto &vertex : vertices) {
        vertex = transformMatrix * vertex;
    }
}

void RenderModel(unsigned int width, unsigned int height) {
    auto models = LoadObjModel(Global::modelPath, 64, 64);
    auto config = LoadModelConfig(Global::modelConfigPath);
    glm::mat4 camaraMatrix = glm::lookAt(config.eyePosition, config.eyeTarget, config.eyeUpDirection);
    glm::mat4 projectMatrix = glm::perspective(glm::radians(60.0f), (float)(width) / (float)(height), 0.1f, 100.0f);

    // load render data
    GLuint vertexArrayHandle;
    GLuint vertexBufferHandle;
    GLuint textureHandle;

	bool upscaleRGBA = false;
	
    glGenVertexArrays(1, &vertexArrayHandle);
    glGenBuffers(1, &vertexBufferHandle);

    std::vector<float> vertexData;
    std::vector<int> renderIndex;
    std::vector<int> renderCount;
    std::vector<float> attachVertexData;
    std::vector<int> attachRenderIndex;
    std::vector<int> attachRenderCount;
    int faceCount = 0;

    for (auto &object : models) {
        const std::string &name = object.first;
        if (name.find("Attachment") == std::string::npos) {
            glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), config.origins[name]);
            for (auto &face : object.second.faces) {
                for (size_t i = 0; i < 4; ++i) {
                    glm::vec4 &raw_position = object.second.vertices[face.element[i].vertexIndex - 1];
                    glm::vec4 position = transformMatrix * raw_position;
                    glm::vec2 &textureCoord = object.second.textureCoords[face.element[i].textureCoordIndex - 1];
                    vertexData.push_back(position.x);
                    vertexData.push_back(position.y);
                    vertexData.push_back(position.z);
                    vertexData.push_back(position.w);
                    vertexData.push_back(textureCoord.x);
                    vertexData.push_back(textureCoord.y);
                }
                renderIndex.push_back(faceCount * 4);
                renderCount.push_back(4);
                ++faceCount;
            }
        }
    }
    for (auto &object : models) {
        const std::string &name = object.first;
        if (name.find("Attachment") == std::string::npos) continue;
        std::string refName = name.substr(0, name.find("Attachment"));
        ObjModel &objectRef = models[refName];
        glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), config.origins[refName]);
        transformMatrix =
            glm::scale(transformMatrix, glm::vec3(config.attachmentScales[refName], config.attachmentScales[refName],
                                                  config.attachmentScales[refName]));
        for (auto &face : objectRef.faces) {
            for (size_t i = 0; i < 4; ++i) {
                glm::vec4 &raw_position = objectRef.vertices[face.element[i].vertexIndex - 1];
                glm::vec4 position = transformMatrix * raw_position;
                glm::vec2 &textureCoord = object.second.textureCoords[face.element[i].textureCoordIndex - 1];
                attachVertexData.push_back(position.x);
                attachVertexData.push_back(position.y);
                attachVertexData.push_back(position.z);
                attachVertexData.push_back(position.w);
                attachVertexData.push_back(textureCoord.x);
                attachVertexData.push_back(textureCoord.y);
            }
            attachRenderIndex.push_back(faceCount * 4);
            attachRenderCount.push_back(4);
            ++faceCount;
        }
    }

    glUseProgram(Global::modelPipelineInfo.programHandle);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vertexArrayHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    ImageData image = GetImageDataFromPNG(Global::inputFilePath, Global::signatureLength, true);
	upscaleRGBA = image.upscaleRGBA;
    if (image.width / image.height == 2) {
        FilpImageVertically(image);
        ImageData newImage = ExtendSkin32x(image, Global::thinArm);
        delete[] image.data;
        image = newImage;
        FilpImageVertically(image);
    }
    textureHandle = GetTextureFromImage(image);
    delete[] image.data;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GLuint samplerLocation = glGetUniformLocation(Global::modelPipelineInfo.programHandle, "textureSampler");
    GLuint transparentSwitchLocation =
        glGetUniformLocation(Global::modelPipelineInfo.programHandle, "disableTransparent");
	GLuint upscaleRGBAFlagLocation =
		glGetUniformLocation(Global::modelPipelineInfo.programHandle, "upscaleRGBA");

    glUniform1i(samplerLocation, 0);
	glUniform1i(upscaleRGBAFlagLocation, upscaleRGBA ? 1 : 0);

    GLuint viewMatrixUniform = glGetUniformLocation(Global::modelPipelineInfo.programHandle, "viewMatrix");
    GLuint projectMatrixUniform = glGetUniformLocation(Global::modelPipelineInfo.programHandle, "projectMatrix");
    glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(camaraMatrix));
    glUniformMatrix4fv(projectMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectMatrix));

    glClear(GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vertexArrayHandle);
    glUniform1i(transparentSwitchLocation, 1);
    glMultiDrawArrays(GL_TRIANGLE_FAN, renderIndex.data(), renderCount.data(), renderCount.size());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * attachVertexData.size(), attachVertexData.data(), GL_STATIC_DRAW);
    glUniform1i(transparentSwitchLocation, 0);
    glMultiDrawArrays(GL_TRIANGLE_FAN, renderIndex.data(), renderCount.data(), renderCount.size());
    glDisable(GL_BLEND);
    glFinish();

    glDeleteBuffers(1, &vertexBufferHandle);
    glDeleteTextures(1, &textureHandle);
    glDeleteVertexArrays(1, &vertexArrayHandle);
}

void Render() {
    if (Global::keepWindow) {
        RenderBackground();
        RenderModel(Global::windowWidth, Global::windowHeight);
        while (!glfwWindowShouldClose(Global::mainWindow)) {
            glfwPollEvents();
        }
    }

    GLuint framebufferHandle;
    GLuint renderbufferColorHandle;
    GLuint renderbufferDSHandle;

    GLint maxSamples;

    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    glGenFramebuffers(1, &framebufferHandle);
    glGenRenderbuffers(1, &renderbufferColorHandle);
    glGenRenderbuffers(1, &renderbufferDSHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbufferDSHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Global::frameWidth, Global::frameHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferDSHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbufferColorHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, Global::frameWidth, Global::frameHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbufferColorHandle);

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
    glViewport(0, 0, Global::frameWidth, Global::frameHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderBackground();
    RenderModel(Global::frameWidth, Global::frameHeight);

    SaveImage();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteRenderbuffers(1, &renderbufferColorHandle);
    glDeleteRenderbuffers(1, &renderbufferDSHandle);
    glDeleteFramebuffers(1, &framebufferHandle);
}

void CleanupPipeline(PipelineInfo info) {
    glDeleteShader(info.fragmentShaderHandle);
    glDeleteShader(info.vertexShaderHandle);
    glDeleteProgram(info.programHandle);
}

void Cleanup() {
    CleanupPipeline(Global::backgroundPipelineInfo);
    CleanupPipeline(Global::modelPipelineInfo);
    glfwTerminate();
}

int main(int argc, char **argv) {
    ParseArguments(argc, argv);
    DumpArguments();
    ApplyArguments();
    Initizalize();
    Render();
    Cleanup();
    return 0;
}
