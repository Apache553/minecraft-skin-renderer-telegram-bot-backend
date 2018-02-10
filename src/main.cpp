
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <memory>
#include <cstdlib>
#include <stdexcept>

namespace Global
{
std::map<std::string, std::string> arguments;
std::string vertexShaderPath = "default_vertex_shader.glsl";
std::string fragmentShaderPath = "default_fragment_shader.glsl";

GLFWwindow *mainWindow;

int windowWidth = 800;
int windowHeight = 600;

const int signatureLength = 4;

GLuint programHandle;
GLuint vertexShaderHandle;
GLuint fragmentShaderHandle;
}

void Initizalize();
void Render();
void Cleanup();
void ParseArguments(int argc, char **argv);
void ApplyArguments();
void DumpArguments();


int main(int argc, char **argv)
{
    ParseArguments(argc, argv);
    DumpArguments();
	ApplyArguments();
    Initizalize();
    Render();
    Cleanup();
    return 0;
}

void CheckOpenGLError()
{
	GLenum errorCode = glGetError();
	if (errorCode != GL_NO_ERROR)
	{
		std::cerr << "ERROR: glGetError() != GL_NO_ERROR" << std::endl;
		std::cerr << "ERROR glGetError() == " << errorCode << std::endl;
		exit(-1);
	}
}

GLuint GetTextureFromPNG(std::string filename) {
	std::FILE* filePtr = std::fopen(filename.c_str(), "rb");
	if (filePtr == nullptr)
	{
		std::cerr << "ERROR: Unable to open input file \'" << filename << '\'' << std::endl;
		exit(-1);
	}
	unsigned char signature[Global::signatureLength];
	if (std::fread(signature, 1, Global::signatureLength, filePtr) < Global::signatureLength)
	{
		std::cerr << "ERROR: Input file is not a valid png file!(Signature not long enough)" << std::endl;
		exit(-1);
	}
	if (png_sig_cmp(signature, 0, Global::signatureLength))
	{
		std::cerr << "ERROR: Input file is not a valid png file!(Signature not matches)" << std::endl;
		exit(-1);
	}
	auto pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr)
	{
		std::cerr << "ERROR: \'png_create_read_struct\' failed!" << std::endl;
		exit(-1);
	}
	auto pngInfoPtr = png_create_info_struct(pngPtr);
	if (pngInfoPtr == nullptr)
	{
		std::cerr << "ERROR: \'png_create_info_struct\' failed!" << std::endl;
		png_destroy_read_struct(&pngPtr, nullptr, nullptr);
		exit(-1);
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
		std::cerr << "ERROR: Unhandled exception!" << std::endl;
		exit(-1);
	}
	png_init_io(pngPtr, filePtr);
	png_set_sig_bytes(pngPtr, Global::signatureLength);
	png_read_info(pngPtr, pngInfoPtr);

	unsigned int imageWidth, imageHeight;
	int imageBitDepth, imageColorType, imageInterlaceMethod, imageCompressionMethod, imageFilterMethod;
	png_get_IHDR(pngPtr, pngInfoPtr, &imageWidth, &imageHeight, &imageBitDepth,
		&imageColorType, &imageInterlaceMethod, &imageCompressionMethod, &imageFilterMethod); // get image infomation

	std::cout << "INFO: input file is " << imageWidth << " * " << imageHeight << std::endl;

	if (imageColorType == PNG_COLOR_TYPE_RGB || imageColorType == PNG_COLOR_TYPE_RGBA)
	{
		std::cout << "INFO: input file is " << (imageColorType == PNG_COLOR_TYPE_RGB ? "RGB" : "RGBA") << " format." << std::endl;
	}

	if (imageColorType == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(pngPtr);		// force indexed-color image to rgb image
	}
	if (imageColorType == PNG_COLOR_TYPE_GRAY && imageBitDepth < 8)
	{
		png_set_expand_gray_1_2_4_to_8(pngPtr);		// force bit depth to 8 for gray124 image
	}
	if (imageBitDepth == 16)
	{
		png_set_strip_16(pngPtr);	// force bit depth to 16bit
	}
	if (png_get_valid(pngPtr, pngInfoPtr, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(pngPtr);	// transform indexed-color image's transparent color to alpha channel
	}
	if (imageColorType == PNG_COLOR_TYPE_GRAY || imageColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(pngPtr);	// force gray image to rgb image
	}
	if (imageInterlaceMethod != PNG_INTERLACE_NONE)
	{
		png_set_interlace_handling(pngPtr);
	}
	if (!(imageColorType&PNG_COLOR_MASK_ALPHA))
	{
		png_set_add_alpha(pngPtr, 0xffffffff, PNG_FILLER_AFTER);
	}
	png_read_update_info(pngPtr, pngInfoPtr);

	// generate a texture buffer for further usage
	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	// read & store data
	unsigned char *imageData = new unsigned char[imageWidth * imageHeight * 4];
	unsigned char**imageDataArray = new unsigned char*[imageHeight];
	for (size_t i = 0; i < imageHeight; ++i)
	{
		imageDataArray[i] = &imageData[i * imageWidth * 4];
	}
	png_read_image(pngPtr, imageDataArray);
	/* Debug output */
	for (size_t i = 0; i < imageHeight; ++i)
	{
		for (size_t j = 0; j < imageWidth; ++j)
		{
			for (size_t k = 0; k < 4; ++k)
			{
				std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned int>(imageData[i*imageWidth * 4 + j * 4 + k]) & 0xFF);
			}
			std::cout << ' ';
		}
		std::cout << std::endl;
	}
	/* Debug ouput */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	glFinish();
	png_read_end(pngPtr,pngInfoPtr);
	png_destroy_read_struct(&pngPtr, &pngInfoPtr,nullptr);
	std::fclose(filePtr);

	delete[] imageDataArray;
	delete[] imageData;

	return textureHandle;
}

void ParseArguments(int argc, char **argv)
{
    std::regex pattern("(.*?)=(.*)");
    for (int index = 1; index < argc; ++index)
    {
        std::cmatch matches;
        bool isFound = std::regex_match(argv[index], matches, pattern);
        if (isFound)
        {
            Global::arguments.insert(std::make_pair(matches[1], matches[2]));
        }
        else
        {
            std::cout << "Illegal argument: " << argv[index] << std::endl;
        }
    }
}

void DumpArguments()
{
    for (auto &argument : Global::arguments)
    {
        std::cout << argument.first << " : " << argument.second << std::endl;
    }
}

void ApplyArguments()
{
	if (Global::arguments.find("windowWidth") != Global::arguments.end())
	{
		char* ptr;
		Global::windowWidth=strtoul(Global::arguments["windowWidth"].c_str(), &ptr, 10);
	}
	if (Global::arguments.find("windowHeight") != Global::arguments.end())
	{
		char* ptr;
		Global::windowHeight = strtoul(Global::arguments["windowHeight"].c_str(), &ptr, 10);
	}
	if (Global::arguments.find("vertexShader") != Global::arguments.end())
	{
		Global::vertexShaderPath = Global::arguments["vertexShader"];
	}
	if (Global::arguments.find("fragmentShader") != Global::arguments.end())
	{
		Global::fragmentShaderPath = Global::arguments["fragmentShader"];
	}
}

void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	std::clog << "Source: ";
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		std::clog << "GL_DEBUG_SOURCE_API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::clog << "GL_DEBUG_SOURCE_WINDOW_SYSTEM"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::clog << "GL_DEBUG_SOURCE_SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::clog << "GL_DEBUG_SOURCE_THIRD_PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::clog << "GL_DEBUG_SOURCE_APPLICATION"; break;
	case GL_DEBUG_SOURCE_OTHER:
		std::clog << "GL_DEBUG_SOURCE_OTHER"; break;
	default:
		std::clog << "UNKNOWN_SOURCE"; break;
	}
	std::clog << '\n';

	std::clog << "Type: ";
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		std::clog << "GL_DEBUG_TYPE_ERROR"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::clog << "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::clog << "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::clog << "GL_DEBUG_TYPE_PERFORMANCE"; break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::clog << "GL_DEBUG_TYPE_PORTABILITY"; break;
	case GL_DEBUG_TYPE_MARKER:
		std::clog << "GL_DEBUG_TYPE_MARKER"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::clog << "GL_DEBUG_TYPE_PUSH_GROUP"; break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::clog << "GL_DEBUG_TYPE_POP_GROUP"; break;
	case GL_DEBUG_TYPE_OTHER:
		std::clog << "GL_DEBUG_TYPE_OTHER"; break;
	default:
		std::clog << "UNKNOWN_TYPE"; break;
	}
	std::clog << '\n';

	std::clog << "Severity: ";
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		std::clog << "GL_DEBUG_SEVERITY_HIGH"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::clog << "GL_DEBUG_SEVERITY_MEDIUM"; break;
	case GL_DEBUG_SEVERITY_LOW:
		std::clog << "GL_DEBUG_SEVERITY_LOW"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::clog << "GL_DEBUG_SEVERITY_NOTIFICATION"; break;
	default:
		std::clog << "UNKNOWN_SEVERITY"; break;
	}
	std::clog << '\n';

	std::clog.write(message, length);
	std::clog << std::endl;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

std::string GetFileContent(std::string filename,size_t bufferSize=1024)
{
	std::ifstream input(filename, std::ios::binary | std::ios::in);
	if (!input.is_open())
	{
		std::cerr << "ERROR: open file \'" << filename << "\' failed!" << std::endl;
		exit(-1);
	}
	std::string content;
	char* buffer = new char[bufferSize];
	while (!input.eof())
	{
		input.read(buffer, bufferSize);
		content.append(buffer, input.gcount());
	}
	delete[] buffer;
	return content;
}

std::string GetGLShaderLog(GLuint shaderHandle)
{
	int length;
	glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);
	if (glGetError() != GL_NO_ERROR)
	{
		std::cerr << "ERROR: Unable to get shader log length!" << std::endl;
		return std::string();
	}
	char* buffer = new char[length];
	int actualLength;
	glGetShaderInfoLog(shaderHandle, length, &actualLength, buffer);
	std::string infoLog;
	infoLog.assign(buffer, actualLength);
	delete[] buffer;
	return infoLog;
}

std::string GetGLProgramLog(GLuint programHandle)
{
	int length;
	glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &length);
	if (glGetError() != GL_NO_ERROR)
	{
		std::cerr << "ERROR: Unable to get program log length!" << std::endl;
		return std::string();
	}
	char* buffer = new char[length];
	int actualLength;
	glGetProgramInfoLog(programHandle, length, &actualLength, buffer);
	std::string infoLog;
	infoLog.assign(buffer, actualLength);
	delete[] buffer;
	return infoLog;
}

void SynthesizePipeline()
{
	int status;
	const char* _ref;
	Global::programHandle = glCreateProgram();
	// process vertex shader
	Global::vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	auto vertexShaderContent = GetFileContent(Global::vertexShaderPath);
	_ref = vertexShaderContent.c_str();
	glShaderSource(Global::vertexShaderHandle, 1, &_ref, nullptr);
	glCompileShader(Global::vertexShaderHandle);
	glGetShaderiv(Global::vertexShaderHandle, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to compile vertex shader" << std::endl;
		std::cerr << GetGLShaderLog(Global::vertexShaderHandle) << std::endl;
		exit(-1);
	}
	// process fragment shader
	Global::fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
	auto fragmentShaderContent = GetFileContent(Global::fragmentShaderPath);
	_ref = fragmentShaderContent.c_str();
	glShaderSource(Global::fragmentShaderHandle, 1, &_ref, nullptr);
	glCompileShader(Global::fragmentShaderHandle);
	glGetShaderiv(Global::fragmentShaderHandle, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to compile fragment shader" << std::endl;
		std::cerr << GetGLShaderLog(Global::fragmentShaderHandle) << std::endl;
		exit(-1);
	}
	// synthesize pipeline
	glAttachShader(Global::programHandle, Global::vertexShaderHandle);
	glAttachShader(Global::programHandle, Global::fragmentShaderHandle);
	glLinkProgram(Global::programHandle);
	glGetProgramiv(Global::programHandle, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to link fragment shader" << std::endl;
		std::cerr << GetGLProgramLog(Global::programHandle) << std::endl;
		exit(-1);
	}
	glUseProgram(Global::programHandle);
}

void Initizalize()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    Global::mainWindow = glfwCreateWindow(Global::windowWidth, Global::windowHeight, "Minecraft Skin Renderer", NULL, NULL);
    if (Global::mainWindow == NULL)
    {
        std::cerr << "Create main window failed!" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(Global::mainWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Initialize GLAD failed!" << std::endl;
        exit(-1);
    }

	glfwSetFramebufferSizeCallback(Global::mainWindow, framebufferSizeCallback);

    glViewport(0, 0, Global::windowWidth, Global::windowHeight);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(DebugCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	SynthesizePipeline();
}

void Render()
{
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	glFinish();
	float vertices[] = {
		1.0f,-1.0f,		1.0f,0.0f,
		-1.0f,-1.0f,	0.0f,0.0f,
		1.0f,1.0f,		1.0f,1.0f,
		-1.0f,1.0f,		0.0f,1.0f
	};
	GLuint vertexBufferHandle;
	GLuint vertexArrayHandle;
	GLuint textureHandle = GetTextureFromPNG("D:\\testpng.png");

	glGenBuffers(1, &vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &vertexArrayHandle);
	glBindVertexArray(vertexArrayHandle);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureHandle); 
	GLuint samplerLocation = glGetUniformLocation(Global::programHandle, "textureSampler");
	glUniform1i(samplerLocation, 1);

	while (!glfwWindowShouldClose(Global::mainWindow))
	{
		glBindVertexArray(vertexArrayHandle);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glFinish();
		glfwPollEvents();
	}
}

void Cleanup()
{
    glfwTerminate();
}