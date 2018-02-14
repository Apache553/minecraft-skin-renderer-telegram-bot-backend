
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

#include "model.cpp"

struct PipelineInfo 
{
	GLuint vertexShaderHandle;
	GLuint fragmentShaderHandle;
	GLuint programHandle;
};

struct ImageData
{
	unsigned char *data;
	unsigned int width;
	unsigned int height;
	unsigned int bytePerPixel;
};

namespace Global
{
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

	GLFWwindow *mainWindow;

	int windowWidth = 800;
	int windowHeight = 600;

	const int signatureLength = 4;

	PipelineInfo modelPipelineInfo;
	PipelineInfo backgroundPipelineInfo;
}

ImageData GetImageDataFromPNG(std::string filename,bool flip=false) 
{
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
		png_set_add_alpha(pngPtr, 0x00000000, PNG_FILLER_AFTER);
	}
	png_read_update_info(pngPtr, pngInfoPtr);

	// generate a texture buffer for further usage

	// read & store data
	unsigned char *imageData = new unsigned char[imageWidth * imageHeight * 4];
	unsigned char**imageDataArray = new unsigned char*[imageHeight];
	if (flip)
	{
		for (size_t i = 0; i < imageHeight; ++i)
		{
			imageDataArray[i] = &imageData[(imageHeight - 1 - i) * imageWidth * 4];
		}
	}
	else
	{
		for (size_t i = 0; i < imageHeight; ++i)
		{
			imageDataArray[i] = &imageData[i * imageWidth * 4];
		}
	}
	png_read_image(pngPtr, imageDataArray);
	/* Debug output */
	//for (size_t i = 0; i < imageHeight; ++i)
	//{
	//	for (size_t j = 0; j < imageWidth; ++j)
	//	{
	//		for (size_t k = 0; k < 4; ++k)
	//		{
	//			std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned int>(imageData[i*imageWidth * 4 + j * 4 + k]) & 0xFF);
	//		}
	//		std::cout << ' ';
	//	}
	//	std::cout << std::endl;
	//}
	/* Debug ouput */
	png_read_end(pngPtr, pngInfoPtr);
	png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
	std::fclose(filePtr);

	delete[] imageDataArray;

	ImageData ret;
	ret.data = imageData;
	ret.width = imageWidth;
	ret.height = imageHeight;
	ret.bytePerPixel = 4;
	return ret;
}

GLuint GetTextureFromImage(const ImageData &image)
{
	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
	glFlush();
	return textureHandle;
}

void CopyAndFlipPixelHorizontally(ImageData &image, unsigned int srcX, unsigned int srcY, unsigned int sizeX, unsigned int sizeY, unsigned int targetX, unsigned int targetY)
{
	unsigned char *tmp = new unsigned char[sizeX*sizeY*image.bytePerPixel];
	unsigned char *tmp2 = new unsigned char[sizeX*sizeY*image.bytePerPixel];
	// read
	for (unsigned int deltaY = 0; deltaY < sizeY; ++deltaY)
	{
		for (unsigned int deltaX = 0; deltaX < sizeX; ++deltaX)
		{
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex)
			{
				tmp[deltaY*sizeX*image.bytePerPixel + deltaX * image.bytePerPixel + byteIndex] = 
					image.data[(srcY + deltaY)*image.width*image.bytePerPixel + (srcX + deltaX)*image.bytePerPixel + byteIndex];
			}
		}
	}
	// flip
	for (unsigned int Y = 0; Y < sizeY; ++Y)
	{
		for (unsigned int X = 0; X < sizeX; ++X)
		{
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex)
			{
				tmp2[Y*sizeX*image.bytePerPixel + (sizeX - X - 1)*image.bytePerPixel + byteIndex] = tmp[Y*sizeX*image.bytePerPixel + X * image.bytePerPixel + byteIndex];
			}
		}
	}
	// write back
	for (unsigned int deltaY = 0; deltaY < sizeY; ++deltaY)
	{
		for (unsigned int deltaX = 0; deltaX < sizeX; ++deltaX)
		{
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex)
			{
				image.data[(targetY + deltaY)*image.width*image.bytePerPixel + (targetX + deltaX)*image.bytePerPixel + byteIndex] = tmp2[deltaY*sizeX*image.bytePerPixel + deltaX * image.bytePerPixel + byteIndex];
			}
		}
	}

	delete[] tmp;
	delete[] tmp2;
}

void FilpImageVertically(ImageData &image)
{
	unsigned char*tmpImage = new unsigned char[image.bytePerPixel*image.height*image.width];
	for (unsigned int Y = 0; Y < image.height; ++Y)
	{
		std::memcpy(&tmpImage[Y*image.width*image.bytePerPixel], &image.data[(image.height - Y - 1)*image.width*image.bytePerPixel], image.bytePerPixel*image.width);
	}
	delete[] image.data;
	image.data = tmpImage;
}

ImageData ExtendSkin32x(ImageData &image,bool thinArm)
{
	if (image.height != 32 || image.width != 64)
	{
		std::cerr << "ERROR: Unable to extend skin image to 64x" << std::endl;
		exit(-1);
	}
	ImageData newImage;
	newImage.data = new unsigned char[64 * 64 * 4];
	newImage.height = 64;
	newImage.width = 64;
	newImage.bytePerPixel = 4;

	std::memset(newImage.data, 0, sizeof(unsigned char) * 64 * 64 * 4);
	std::memcpy(newImage.data, image.data, sizeof(unsigned char) * 4 * image.height * image.width);
	
	CopyAndFlipPixelHorizontally(newImage, 0, 16, 16, 16, 16, 48);
	if (thinArm)
	{
		CopyAndFlipPixelHorizontally(newImage, 40, 16, 14, 16, 32, 48);
	}
	else
	{
		CopyAndFlipPixelHorizontally(newImage, 40, 16, 16, 16, 32, 48);
	}
	return newImage;
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
		Global::windowWidth = strtoul(Global::arguments["windowWidth"].c_str(), &ptr, 10);
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
	if (Global::arguments.find("input") != Global::arguments.end())
	{
		Global::inputFilePath = Global::arguments["input"];
	}
	if (Global::arguments.find("output") != Global::arguments.end())
	{
		Global::outputFilePath = Global::arguments["output"];
	}
	if (Global::arguments.find("background") != Global::arguments.end())
	{
		Global::backgroundPath = Global::arguments["background"];
	}
	if (Global::arguments.find("bgVertexShader") != Global::arguments.end())
	{
		Global::bgVertexShaderPath = Global::arguments["bgVertexShader"];
	}
	if (Global::arguments.find("bgFragmentShader") != Global::arguments.end())
	{
		Global::bgFragmentShaderPath = Global::arguments["bgFragmentShader"];
	}
	if (Global::arguments.find("model") != Global::arguments.end())
	{
		Global::modelPath = Global::arguments["model"];
	}
	if (Global::arguments.find("modelConfig") != Global::arguments.end())
	{
		Global::modelConfigPath = Global::arguments["modelConfig"];
	}
}

std::string GetFileContent(std::string filename, size_t bufferSize = 1024)
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

PipelineInfo SynthesizePipeline(std::string vertexShaderPath,std::string fragmentShaderPath)
{

	int status;
	const char* _ref;
	PipelineInfo info;
	info.programHandle = glCreateProgram();
	// process vertex shader
	info.vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	auto vertexShaderContent = GetFileContent(vertexShaderPath);
	_ref = vertexShaderContent.c_str();
	glShaderSource(info.vertexShaderHandle, 1, &_ref, nullptr);
	glCompileShader(info.vertexShaderHandle);
	glGetShaderiv(info.vertexShaderHandle, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to compile vertex shader" << std::endl;
		std::cerr << GetGLShaderLog(info.vertexShaderHandle) << std::endl;
		exit(-1);
	}
	// process fragment shader
	info.fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
	auto fragmentShaderContent = GetFileContent(fragmentShaderPath);
	_ref = fragmentShaderContent.c_str();
	glShaderSource(info.fragmentShaderHandle, 1, &_ref, nullptr);
	glCompileShader(info.fragmentShaderHandle);
	glGetShaderiv(info.fragmentShaderHandle, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to compile fragment shader" << std::endl;
		std::cerr << GetGLShaderLog(info.fragmentShaderHandle) << std::endl;
		exit(-1);
	}
	// synthesize pipeline
	glAttachShader(info.programHandle, info.vertexShaderHandle);
	glAttachShader(info.programHandle, info.fragmentShaderHandle);
	glLinkProgram(info.programHandle);
	glGetProgramiv(info.programHandle, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cerr << "ERROR: Unable to link fragment shader" << std::endl;
		std::cerr << GetGLProgramLog(info.programHandle) << std::endl;
		exit(-1);
	}
	return info;
}

void glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	// ignore non-significant error/warning codes
	// if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
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

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(&glDebugOutput, nullptr);
	glViewport(0, 0, Global::windowWidth, Global::windowHeight);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	Global::modelPipelineInfo = SynthesizePipeline(Global::vertexShaderPath, Global::fragmentShaderPath);
	Global::backgroundPipelineInfo = SynthesizePipeline(Global::bgVertexShaderPath, Global::bgFragmentShaderPath);
}

void RenderBackground()
{
	float vertexInfo[] = {
		// position		textureCoord
		-1.0f,-1.0f,	0.0f,0.0f,
		 1.0f,-1.0f,	1.0f,0.0f,
		 1.0f, 1.0f,	1.0f,1.0f,
		-1.0f, 1.0f,	0.0f,1.0f
	};
	glUseProgram(Global::backgroundPipelineInfo.programHandle);
	GLuint subroutineIndex;
	if (Global::backgroundPath.empty())
	{
		subroutineIndex = glGetSubroutineIndex(Global::backgroundPipelineInfo.programHandle, GL_FRAGMENT_SHADER, "Plain");
	}
	else
	{
		subroutineIndex = glGetSubroutineIndex(Global::backgroundPipelineInfo.programHandle, GL_FRAGMENT_SHADER, "Image");
	}
	GLuint subroutineLocation = glGetSubroutineUniformLocation(Global::backgroundPipelineInfo.programHandle, GL_FRAGMENT_SHADER, "GetColor");
	GLsizei number;
	glGetProgramStageiv(Global::backgroundPipelineInfo.programHandle, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &number);
	GLuint* indices = new GLuint[number];
	indices[subroutineLocation] = subroutineIndex;
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, number, indices);
	delete[] indices;

	GLuint vertexArrayHandle;
	GLuint vertexArrayBufferHandle;
	GLuint textureHandle;

	glGenVertexArrays(1, &vertexArrayHandle);
	glGenBuffers(1, &vertexArrayBufferHandle);

	glBindVertexArray(vertexArrayHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexInfo), vertexInfo, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	if (!Global::backgroundPath.empty())
	{
		ImageData image = GetImageDataFromPNG(Global::backgroundPath, true);
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

	if (!Global::backgroundPath.empty())
	{
		glDeleteTextures(1, &textureHandle);
	}
}

inline void TransformVertices(std::vector<glm::vec4> &vertices, const glm::mat4& transformMatrix)
{
	for (auto& vertex : vertices)
	{
		vertex = transformMatrix * vertex;
	}
}

void RenderModel()
{
	auto models = LoadObjModel(Global::modelPath, 64, 64);
	auto config = LoadModelConfig(Global::modelConfigPath);
	glm::mat4 camaraMatrix = glm::lookAt(config.eyePosition, config.eyeTarget, config.eyeUpDirection);
	glm::mat4 projectMatrix = glm::perspective(glm::radians(60.0f), (float)(Global::windowWidth) / (float)(Global::windowHeight), 0.1f, 100.0f);

	// load render data
	GLuint vertexArrayHandle;
	GLuint vertexBufferHandle;
	GLuint textureHandle;

	glGenVertexArrays(1, &vertexArrayHandle);
	glGenBuffers(1, &vertexBufferHandle);

	std::vector<float> vertexData;
	std::vector<int> renderIndex;
	std::vector<int> renderCount;
	int faceCount = 0;

	for (auto& object : models)
	{
		const std::string& name = object.first;
		if (name.find("Attachment") == std::string::npos)
		{
			glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), config.origins[name]);
			for (auto& face : object.second.faces)
			{
				for (size_t i = 0; i < 4; ++i)
				{
					glm::vec4& raw_position = object.second.vertices[face.element[i].vertexIndex - 1];
					glm::vec4 position = transformMatrix * raw_position;
					glm::vec2& textureCoord = object.second.textureCoords[face.element[i].textureCoordIndex - 1];
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
	for (auto& object : models)
	{
		const std::string& name = object.first;
		if (name.find("Attachment") == std::string::npos)continue;
		std::string refName = name.substr(0, name.find("Attachment"));
		ObjModel& objectRef = models[refName];
		glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), config.origins[refName]);
		transformMatrix = glm::scale(transformMatrix, glm::vec3(config.attachmentScales[refName], config.attachmentScales[refName], config.attachmentScales[refName]));
		for (auto& face : objectRef.faces)
		{
			for (size_t i = 0; i < 4; ++i)
			{
				glm::vec4& raw_position = objectRef.vertices[face.element[i].vertexIndex - 1];
				glm::vec4 position = transformMatrix * raw_position;
				glm::vec2& textureCoord = object.second.textureCoords[face.element[i].textureCoordIndex - 1];
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


	glUseProgram(Global::modelPipelineInfo.programHandle);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(vertexArrayHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	
	ImageData image = GetImageDataFromPNG(Global::inputFilePath, true);
	if (image.width / image.height == 2)
	{
		FilpImageVertically(image);
		ImageData newImage = ExtendSkin32x(image, false);
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
	glUniform1i(samplerLocation, 0);

	GLuint viewMatrixUniform = glGetUniformLocation(Global::modelPipelineInfo.programHandle, "viewMatrix");
	GLuint projectMatrixUniform = glGetUniformLocation(Global::modelPipelineInfo.programHandle, "projectMatrix");
	glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(camaraMatrix));
	glUniformMatrix4fv(projectMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectMatrix));

	while (!glfwWindowShouldClose(Global::mainWindow))
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(vertexArrayHandle);
		glMultiDrawArrays(GL_TRIANGLE_FAN, renderIndex.data(), renderCount.data(), renderCount.size());
		glFlush();
		glfwPollEvents();
	}

}

void Render()
{
	RenderBackground();
	RenderModel();
}

void CleanupPipeline(PipelineInfo info)
{
	glDeleteShader(info.fragmentShaderHandle);
	glDeleteShader(info.vertexShaderHandle);
	glDeleteProgram(info.programHandle);
}

void SaveImage()
{
	// TODO
}

void Cleanup()
{
	CleanupPipeline(Global::backgroundPipelineInfo);
	CleanupPipeline(Global::modelPipelineInfo);
	glfwTerminate();
}

int main(int argc, char **argv)
{
	ParseArguments(argc, argv);
	DumpArguments();
	ApplyArguments();
	Initizalize();
	Render();
	SaveImage();
	Cleanup();
	return 0;
}
