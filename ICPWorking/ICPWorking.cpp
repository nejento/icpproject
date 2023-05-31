
// ICPWorking.cpp : This file contains the 'main' function. Program execution begins and ends there
//

// -- Includes -- 

#include <iostream>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>
#include <memory> //for smart pointers (unique_ptr)
#include <fstream>

// OpenCV
#include <opencv2/opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> //pro jednoodušší práci s extentions 
#include <GL/wglew.h> 

// GLFW toolkit
#include <GLFW/glfw3.h> //knihovna pro zálkladní obsulu systému (klávesnice/myš)

// OpenGL Math
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/common.hpp>


// -- Headers -- 

void init_glew(void);
void init_glfw(void);
void error_callback(int error, const char* description);
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*
    userParam);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


// -- Global variables, typedefs --

glm::vec4 color(1, 0, 0, 1);

typedef struct image_data {
    cv::Point2f center;
    cv::Mat frame;
} image_data;

std::unique_ptr<image_data> image_data_shared;

typedef struct s_globals {
    GLFWwindow* window;
    int height;
    int width;
    double app_start_time;
    cv::VideoCapture capture;
    bool fullscreen;
    int x = 0;
    int y = 0;
} s_globals;

s_globals globals;

std::mutex img_access_mutex;
bool image_proccessing_alive;

// -- Templated funkce -- 

// Templated: Inicializace GL extensions GLEW, použitelné PO vytvoření GL contextu
void init_glew(void)
{
    //
    // Initialize all valid GL extensions with GLEW.
    // Usable AFTER creating GL context!
    //
    {
        GLenum glew_ret;
        glew_ret = glewInit();
        if (glew_ret != GLEW_OK)
        {
            std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "GLEW successfully initialized to version: " << glewGetString(GLEW_VERSION) << std::endl;
        }
        // Platform specific. (Change to GLXEW or ELGEW if necessary.)
        glew_ret = wglewInit();
        if (glew_ret != GLEW_OK)
        {
            std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "WGLEW successfully initialized platform specific functions." << std::endl;
        }
    }

    if (glfwExtensionSupported("GL_KHR_debug"))
    {
        glDebugMessageCallback(MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);
        std::cout << "GL_DEBUG enabled." << std::endl;
    }


}

// Templated
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*
    userParam)
{
    auto const src_str = [source]() {
        switch (source)
        {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER: return "OTHER";
        default: return "Unknown";
        }
    }();
    auto const type_str = [type]() {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER: return "MARKER";
        case GL_DEBUG_TYPE_OTHER: return "OTHER";
        default: return "Unknown";
        }
    }();
    auto const severity_str = [severity]() {
        switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
        case GL_DEBUG_SEVERITY_LOW: return "LOW";
        case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
        case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
        default: return "Unknown";
        }
    }();
    std::cout << "[GL CALLBACK]: " <<
        "source = " << src_str <<
        ", type = " << type_str <<
        ", severity = " << severity_str <<
        ", ID = '" << id << '\'' <<
        ", message = '" << message << '\'' << std::endl;
}


// Templated
std::string textFileRead(const std::string fn) {
    std::ifstream file;
    file.exceptions(std::ifstream::badbit);
    std::stringstream ss;
    try {
        file.open(fn);
        std::string content;
        ss << file.rdbuf();
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << "Error opening file: " << fn <<
            std::endl;
        exit(EXIT_FAILURE);
    }
    return std::move(ss.str());
}

// Templated: Vypisuje informace o shaderu
std::string getShaderInfoLog(const GLuint obj) {
    int infologLength = 0;
    std::string s;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::vector<char> v(infologLength);
        glGetShaderInfoLog(obj, infologLength, NULL,
            v.data());
        s.assign(begin(v), end(v));
    }
    return s;
}

// Templated: Vypisuje informace o programu
std::string getProgramInfoLog(const GLuint obj) {
    int infologLength = 0;
    std::string s;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::vector<char> v(infologLength);
        glGetProgramInfoLog(obj, infologLength, NULL,
            v.data());
        s.assign(begin(v), end(v));
    }
    return s;
}

// Templated: Zachytávání chyb
void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}

// Templated: Uzavření a ukončení okna
static void finalize(int code)
{
    // ...

    // Close OpenGL window if opened and terminate GLFW  
    if (globals.window)
        glfwDestroyWindow(globals.window);
    glfwTerminate();

    // ...
}

// Editable: Nastavení GLFW
static void init_glfw(void)
{
    //
    // GLFW init.
    //

    // set error callback first
    glfwSetErrorCallback(error_callback);

    //initialize GLFW library
    int glfw_ret = glfwInit();
    if (!glfw_ret)
    {
        std::cerr << "GLFW init failed." << std::endl;
        finalize(EXIT_FAILURE);
    }

    // Shader based, modern OpenGL (3.3 and higher)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6); //Dal jsem šestku, protože jsme chads
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    globals.window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
    if (!globals.window)
    {
        std::cerr << "GLFW window creation error." << std::endl;
        finalize(EXIT_FAILURE);
    }

    // Get some GLFW info.
    {
        int major, minor, revision;

        glfwGetVersion(&major, &minor, &revision);
        std::cout << "Running GLFW " << major << '.' << minor << '.' << revision << std::endl;
        std::cout << "Compiled against GLFW " << GLFW_VERSION_MAJOR << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << std::endl;
    }

    glfwMakeContextCurrent(globals.window);                                       // Set current window.
    glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);      // Get window size.
    //glfwSwapInterval(0);                                                        // Set V-Sync OFF.
    glfwSwapInterval(1);                                                          // Set V-Sync ON.

    globals.app_start_time = glfwGetTime();                                       // Get start time.
}

// -- Input callbacky --

/*
* Editable: Key callback function
* @brief Function is called when key is pressed
* @param window Window that called the function
* @param key Key that was pressed
* @param scancode
* @param action Action that was performed
* @param mods
*/
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        std::cout << "WWWWWWWWWW" << '\n';
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        std::cout << "SSSSSSSSSS" << '\n';
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        std::cout << "AAAAAAAAAA" << '\n';
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        std::cout << "DDDDDDDDDD" << '\n';
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        color = glm::vec4(1, 0, 0, 1);
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
        color = glm::vec4(0, 1, 0, 1);
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        color = glm::vec4(0, 0, 1, 1);
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        if (globals.fullscreen) {
            glfwSetWindowMonitor(window, nullptr, globals.x, globals.y, 640, 480, 0);
            globals.fullscreen = false;
        }
        else {
            glfwGetWindowSize(window, &globals.x, &globals.y);
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            globals.fullscreen = true;
        }

}

/* Editable: Mouse button callback function
* @brief Function is called when mouse button is pressed
* @param window Window that called the function
* @param button Button that was pressed
* @param action Action that was performed
* @param mods
*/
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        std::cout << "MOUSE_RIGHT" << '\n';
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        std::cout << "MOUSE_LEFT" << '\n';
}

/* Editable: Scroll callback function
* @brief Function is called when mouse wheel is scrolled
* @param window Window that called the function
* @param xoffset
* @param yoffset
*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    std::cout << "x offset: " << xoffset << " , y offset: " << yoffset << '\n';
}

/* Editable: Cursor position callback function
* @brief Function is called when cursor is moved
* @param window Window that called the function
* @param xpos
* @param ypos
*/
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    std::cout << "x pos: " << xpos << " , y pos: " << ypos << '\n';
}

// -- Main --

//===================================================== MAIN =====================================================
// Editable: Hlavní funkce
int main()
{
    init_glfw(); //Init GLFW library
    init_glew(); //Init OpenGL Extension Wrangler Library

    glfwSetCursorPosCallback(globals.window, cursor_position_callback);
    glfwSetScrollCallback(globals.window, scroll_callback);
    glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
    glfwSetKeyCallback(globals.window, key_callback);

    //Enable Z buffer test (zajistí správné vykreslování objektů)
    glEnable(GL_DEPTH_TEST);

    //Enable back face culling (znemožní vykreslování vnitřních ploch)
    glEnable(GL_CULL_FACE);



    // create and use shaders
    GLuint VS_h, FS_h, prog_h;
    VS_h = glCreateShader(GL_VERTEX_SHADER);
    FS_h = glCreateShader(GL_FRAGMENT_SHADER);

    std::string VSsrc = textFileRead("resources/basic.vert");
    const char* VS_string = VSsrc.c_str();
    std::string FSsrc = textFileRead("resources/basic.frag");
    const char* FS_string = FSsrc.c_str();
    glShaderSource(VS_h, 1, &VS_string, NULL);
    glShaderSource(FS_h, 1, &FS_string, NULL);

    glCompileShader(VS_h);
    getShaderInfoLog(VS_h);
    glCompileShader(FS_h);
    getShaderInfoLog(FS_h);
    prog_h = glCreateProgram();
    glAttachShader(prog_h, VS_h);
    glAttachShader(prog_h, FS_h);
    glLinkProgram(prog_h);
    getProgramInfoLog(prog_h);
    glUseProgram(prog_h);


    //Init VAO 1
    struct vertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
    };
    std::vector<GLuint> indices = { 0,1,2 };
    //GL names for Array and Buffers Objects
    GLuint VAO1, VBO, EBO;
    // Generate the VAO and VBO 
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Bind VAO (set as the current)
    glBindVertexArray(VAO1);
    // Bind the VBO, set type as GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Fill-in data into the VBO
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
    // Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // Fill-in data into the EBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    // Set Vertex Attribute to explain OpenGL how to interpret the VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, position)));
    // Enable the Vertex Attribute 0 = position
    glEnableVertexAttribArray(0);
    // Set end enable Vertex Attribute 1 = Texture Coordinates
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, color)));
    glEnableVertexAttribArray(1);
    // Bind VBO and VAO to 0 to prevent unintended modification 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    

    // -- Vygenerujte data pro kolečko uprostřed obrazovky --

    //Druhý objekt VAO2
    //Init VAO 1
    struct vertex2 {
        glm::vec3 position;
    };
    std::vector<vertex> vertices2;
    std::vector<GLuint> indices2;
    //GL names for Array and Buffers Objects
    GLuint VAO2, VBO2, EBO2;
    // Generate the VAO and VBO 
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &EBO2);
    // Bind VAO (set as the current)
    glBindVertexArray(VAO2);
    // Bind the VBO, set type as GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    // Fill-in data into the VBO
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
    // Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
    // Fill-in data into the EBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    // Set Vertex Attribute to explain OpenGL how to interpret the VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex2, position)));
    // Enable the Vertex Attribute 0 = position
    glEnableVertexAttribArray(0);
    // Set end enable Vertex Attribute 1 = Texture Coordinates
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex2, color)));
    //glEnableVertexAttribArray(1);
    // Bind VBO and VAO to 0 to prevent unintended modification 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // -- Vygenerujte data pro kolečko uprostřed obrazovky z 100 000 vertexů --



    // Smyčka vyčištění a vykreslování
    double last_fps = glfwGetTime();
    int frame_cnt = 0;

    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

    // transformations
    // projection & viewport
    {

        float ratio = static_cast<float>(width) / height;

        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(60.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
            ratio,			     // Aspect Ratio. Depends on the size of your window.
            0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
            20000.0f              // Far clipping plane. Keep as little as possible.
        );

        //set uniform for shaders - projection matrix
        glUniformMatrix4fv(glGetUniformLocation(prog_h, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        // set visible area
        glViewport(0, 0, width, height);
    }

    while (!glfwWindowShouldClose(globals.window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Trojuhelnik
        {
            glUseProgram(prog_h);
            GLuint loc = glGetUniformLocation(prog_h, "color");
            
            //glUniform4f(loc, 1, 0, 0, 1);
            glUniform4fv(loc, 1, glm::value_ptr(color));

            // View matrix
            glm::mat4 v_m = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), //position of camera
                glm::vec3(0.0f, 0.0f, 0.0f), //where to look
                glm::vec3(0, 1, 0)  //UP direction
            );
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));

            
            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2.0, height / 2.0, 0.0));
            m_m = glm::scale(m_m, glm::vec3(5.0f));
            //m_m = glm::rotate(m_m, glm::radians(100.0f, ))

            // modify Model matrix and send to shaders
            // rotate slowly
            //m_m = glm::rotate(m_m, glm::radians(100.0f * (float)glfwGetTime()), glm::vec3(0.0f, 0.1f, 0.0f));
            
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            // =====================================================================================================


            glBindVertexArray(VAO1);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        //Kolečko
        {
            // Při odkomentování, bez úprav dle zadání, nebude fungovat samotné renderování trojúhelníku
            //glUseProgram(prog2_h); //jiný program, bude potřeba upravit¡ už nemáme barvu
            //glBindVertexArray(VAO2);
            //glDrawElements(GL_TRIANGLE_FAN, indices.size(), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(globals.window);
        glfwPollEvents();  
        

        //Frame counter
        frame_cnt++;
        {
            double now = glfwGetTime();
            if ((now - last_fps) > 1.0) {
                std::cout << frame_cnt << " FPS\r";
                frame_cnt = 0;
                last_fps = now;
            }
        
        }
    }


}
