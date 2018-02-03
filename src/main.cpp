
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <png.h>

#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <memory>

namespace Global
{
std::map<std::string, std::string> arguments;
GLFWwindow *mainWindow;
const int windowWidth = 800;
const int windowHeight = 600;
}

void Initizalize();
void Render();
void Cleanup();
void ParseArguments(int argc, char **argv);

void DumpArguments();

GLuint GetTextureFromPNG(std::string filename){
    png_infop pngInfo;
    
}

int main(int argc, char **argv)
{
    ParseArguments(argc, argv);
    DumpArguments();
    Initizalize();
    Render();
    Cleanup();
    return 0;
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

void Initizalize()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
    glViewport(0, 0, Global::windowWidth, Global::windowHeight);
}

void Render()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Cleanup()
{
    std::this_thread::sleep_for(std::chrono::seconds(3));
    glfwTerminate();
}