/* TODO
* - Vytvořit prostředí
* - Načítání objektů a textur
* - Zajistit pohyb kamery
* - Zajistit osvětlení
* - Přidat zvuk
*/

//=== Includes ===
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
#include <GL/glew.h> //pro jednodušší práci s extentions 
#include <GL/wglew.h> 

// GLFW toolkit
#include <GLFW/glfw3.h> //knihovna pro zálkladní obsulu systému (klávesnice/myš)

// OpenGL Math
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// Other Header files
#include "OBJloader.h"
#include "vertex.h"
#include "RealtimeRasterProcessing.h"

//=== Headers ===
void init_glew(void);
void init_glfw(void);
void error_callback(int error, const char* description);
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*
    userParam);


GLuint PrepareVAO(std::vector<vertex> vertices, std::vector<GLuint> indices);

typedef struct s_globals {
    GLFWwindow* window;
    float fov = 45;
    int height;
    int width;
    double app_start_time;
    cv::VideoCapture capture;
    bool fullscreen;
    int x = 0;
    int y = 0;
} s_globals;

s_globals globals;
glm::vec4 color = { 1.0f, 1.0f, 1.0f, 0.0f };

glm::vec3 cameraPosition(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUP(0.0f, 1.0f, 0.0f);
double delta_t = 0; //how much time has passed


//=== Templated ===
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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
//=== Templated ===
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
//=== Templated: Vypisuje informace o shaderu ===
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
//=== Templated: Vypisuje informace o programu ===
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
//=== Templated: Uzavření a ukončení okna ===
static void finalize(int code)
{
    // ...

    // Close OpenGL window if opened and terminate GLFW  
    if (globals.window)
        glfwDestroyWindow(globals.window);
    glfwTerminate();

    // ...
}
//=== Templated: Zachytávání chyb ===
void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}
//=== Templated: Inicializace GL extensions GLEW, použitelné PO vytvoření GL contextu ===
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

//=== Editable: Nastavení GLFW ===
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions <= this is the core profile
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    // original resolution 800x800
    globals.window = glfwCreateWindow(1000, 1000, "Final project by Broz&Jacik", NULL, NULL);
    if (!globals.window)
    {
        std::cerr << "GLFW window creation error." << std::endl;
        finalize(EXIT_FAILURE);
    }


    glfwSetInputMode(globals.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Get some GLFW info.
    {
        int major, minor, revision;

        glfwGetVersion(&major, &minor, &revision);
        std::cout << "Running GLFW " << major << '.' << minor << '.' << revision << std::endl;
        std::cout << "Compiled against GLFW " << GLFW_VERSION_MAJOR << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << std::endl;
    }
    glfwMakeContextCurrent(globals.window);                                        // Set current window.
    glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);       // Get window size.
    //glfwSwapInterval(0);                                                         // Set V-Sync OFF.
    glfwSwapInterval(1);                                                           // Set V-Sync ON.
    globals.app_start_time = glfwGetTime();                                        // Get start time.

}


//===================================================== MOVEMENT =====================================================

bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;

/* Editable: Key callback function
* @brief Function is called when key is pressed
* @param window Window that called the function
* @param key Key that was pressed
* @param scancode
* @param action Action that was performed
* @param mods
*/
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        color = { 0.0f, 0.0f, 1.0f, 0.0f };
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        color = { 1.0f, 0.0f, 0.0f, 0.0f };
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
        color = { 0.0f, 1.0f, 0.0f, 0.0f };
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        moveForward = true;
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        moveBackward = true;
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        moveLeft = true;
    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        moveRight = true;
    }

    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        moveForward = false;
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        moveBackward = false;
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        moveLeft = false;
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
        moveRight = false;
    }


    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        if (globals.fullscreen) {
            glfwSetWindowMonitor(window, nullptr, globals.x, globals.y, 800, 800, 0);
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

bool firstMouseMovement = true;
float last_mousePosition_X = 0;
float last_mousePosition_Y = 0;
float mouseSensitivity = 0.03f;
float yaw = 0;
float pitch = 0;


float clip(float n, float min, float max) {
    return std::max(min, std::min(n, max));
}

/* Editable: Cursor position callback function
* @brief Function is called when cursor is moved
* @param window Window that called the function
* @param xpos
* @param ypos
*/
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouseMovement) {
        firstMouseMovement = false;
        last_mousePosition_X = xpos;
        last_mousePosition_Y = ypos;
    }

    float mouseOffset_Y = ypos - last_mousePosition_Y; //reversed to prevent inverted mouse movement
    float mouseOffset_X = last_mousePosition_X - xpos;
    last_mousePosition_X = xpos;
    last_mousePosition_Y = ypos;

    yaw  += mouseOffset_X * mouseSensitivity;
    pitch += mouseOffset_Y * mouseSensitivity;
    pitch = clip(pitch, -89, 89);

    cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront.y = sin(glm::radians(pitch));
    cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(cameraFront);
    std::cout << "cameraFront: " << "x: " << cameraFront.x << " , y: " << cameraFront.y << "z: "<< cameraFront.z <<'\n';
}

//===================================================== END OF MOVEMENT =====================================================


void GenerateChessPattern(std::vector<vertex>& vertices_chess, cvflann::lsh::Bucket& indices_chess);

GLuint PrepareShaderProgram(std::string& vert_shader_path, std::string& frag_shader_path);

void HandleCameraMovement();

//===================================================== MAIN =====================================================
int main()
{
    
    //run_2D_raster_processing(); //odkomentuj toto aby jsi spustil 2D raster tracker

    init_glfw();
    init_glew();

    glfwSetCursorPosCallback(globals.window, cursor_position_callback);
    glfwSetScrollCallback(globals.window, scroll_callback);
    glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
    glfwSetKeyCallback(globals.window, key_callback);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glfwSwapInterval(1);//zapnutí Vsync

    std::string vert_shader_path = "resources/basic.vert";
    std::string frag_shader_path = "resources/basic.frag";
    GLuint prog_h = PrepareShaderProgram(vert_shader_path, frag_shader_path);

    std::string vert_shader_path_fan = "resources/basicFan.vert";
    std::string frag_shader_path_fan = "resources/basicFan.frag";
    GLuint prog_h2 = PrepareShaderProgram(vert_shader_path_fan, frag_shader_path_fan);

    std::string vert_shader_path_chess = "resources/basicChess.vert";
    std::string frag_shader_path_chess = "resources/basicChess.frag";
    GLuint prog_h_chess = PrepareShaderProgram(vert_shader_path_chess, frag_shader_path_chess);


    std::vector<vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f},{1.0f,0.0f,0.0f}}, //pozice + barva vrcholu
        {{0.5f, -0.5f, 0.0f},{0.0f,1.0f,0.0f}},
        {{0.0f, 0.5f, 0.0f},{0.0f,0.0f,1.0f}}
    };

    std::vector<GLuint> indices = {0, 1, 2};
    GLuint VAO1 = PrepareVAO(vertices, indices);
    
    //==================KRUH==================
    //vygenerujte data pro kolecko uprostred obrazovky (triangle fan) z 100 000 vertexu
    std::vector<vertex> vertices_kruh = {};
    std::vector<GLuint> indices_kruh = {};
    
    int vertexCount = 100000;
    float angleStep = 2*3.14159265358979323846f/ vertexCount;
    float magnitude = 0.5f;
    //generovani kruhu
    for (int i = 0; i < vertexCount; i++) {
        auto polar = std::polar(magnitude, angleStep * i);
        vertex temp_ver = {{polar._Val[0], polar._Val[1], 0.0f}};
        vertices_kruh.push_back(temp_ver);
        indices_kruh.push_back(i+1);
    }
    vertices_kruh.push_back({ {0.0f, 0.0f, 0.0f} });
    indices_kruh.push_back(0);
    
    
    GLuint VAO_kruh = PrepareVAO(vertices_kruh, indices_kruh);

    //==================SACHOVNICE==================

    //vygenerujte data pro kolecko uprostred obrazovky (triangle fan) z 100 000 vertexu
    std::vector<vertex> vertices_chess = {};
    std::vector<GLuint> indices_chess = {};
    GenerateChessPattern(vertices_chess, indices_chess);
    GLuint VAO_chess = PrepareVAO(vertices_chess, indices_chess);
    // set visible area
    double last_frame = glfwGetTime();
    
    //======================LOADED=MODEL=======================
    std::vector<vertex> vertices_subaru = {};
    std::vector<GLuint> indices_subaru = {};
    glm::vec3 scale = { 0.07,0.07,0.07 };
    loadOBJ("resources/obj/Frog/20436_Frog_v1 textured.obj", vertices_subaru, indices_subaru, scale);
    GLuint VAO_subaru = PrepareVAO(vertices_subaru, indices_subaru);
    std::string vert_shader_path_subaru = "resources/basic.vert";
    std::string frag_shader_path_subaru = "resources/basic.frag";
    GLuint prog_subaru = PrepareShaderProgram(vert_shader_path_subaru, frag_shader_path_subaru);

    //====================================
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    float ratio = static_cast<float>(width) / height;
    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(globals.fov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
        ratio,			     // Aspect Ratio. Depends on the size of your window.
        0.01f,               // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        100.0f               // Far clipping plane. Keep as little as possible.
    );
    //set uniform for shaders - projection matrix
    glUniformMatrix4fv(glGetUniformLocation(prog_subaru, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    last_frame = glfwGetTime();
    while(!glfwWindowShouldClose(globals.window)) {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float current_frame = glfwGetTime();
        delta_t = current_frame - last_frame;
        last_frame = current_frame;
       

        glm::mat4 view_m = glm::lookAt(
            cameraPosition, //from where
            cameraPosition+cameraFront,  //to where
            cameraUP  //UP direction
        );
        //===Movement================
        HandleCameraMovement();

        //std::cout << "Player position "<< "x: " << cameraPosition.x<< ", y: " << cameraPosition.y << ", z: " << cameraPosition.z << '\n';
        //subaru car
        {
            glUseProgram(prog_subaru);
            glBindVertexArray(VAO_subaru);

            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2, height / 2, 0.0));
            //m_m = glm::scale(m_m, glm::vec3(500.0f));
            //m_m = glm::rotate(m_m, glm::radians(50.0f * (float)glfwGetTime()), glm::vec3(0.0f, 0.5f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(prog_subaru, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_subaru, "uV_m"), 1, GL_FALSE, glm::value_ptr(view_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_subaru, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            // =====================================================================================================
            glDrawElements(GL_TRIANGLES, indices_subaru.size(), GL_UNSIGNED_INT, 0);
        }
        //podlaha
        {
            glUseProgram(prog_h_chess);
            glBindVertexArray(VAO_chess);

            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2, height / 2, 0.0));
            //m_m = glm::scale(m_m, glm::vec3(500.0f));
            //m_m = glm::rotate(m_m, glm::radians(50.0f * (float)glfwGetTime()), glm::vec3(0.3f, 0.1f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uV_m"), 1, GL_FALSE, glm::value_ptr(view_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            // =====================================================================================================
            glDrawElements(GL_TRIANGLES, indices_chess.size(), GL_UNSIGNED_INT, 0);
        }

        /*//trojuhelnik
        {
            glUseProgram(prog_h);
            glBindVertexArray(VAO1);
            
            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2, height / 2, 0.0));
            //m_m = glm::scale(m_m, glm::vec3(500.0f));
            m_m = glm::rotate(m_m, glm::radians(50.0f * (float)glfwGetTime()), glm::vec3(0.3f, 0.1f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uV_m"), 1, GL_FALSE, glm::value_ptr(view_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            // =====================================================================================================
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }*/
        /*
        //kruh
        {
            glUseProgram(prog_h2); // nutne upravit, uz nemame barvu => nový shared

            GLuint loc = glGetUniformLocation(prog_h2, "color");
            glUniform4fv(loc, 1, glm::value_ptr(color));

            

            glBindVertexArray(VAO_kruh);
            glDrawElements(GL_TRIANGLE_FAN, indices_kruh.size(), GL_UNSIGNED_INT, 0);
        }

        {
            glUseProgram(prog_h_chess);
            glBindVertexArray(VAO_chess);

            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2, height / 2, 0.0));
            //m_m = glm::scale(m_m, glm::vec3(5.0f));
            m_m = glm::rotate(m_m, glm::radians(100.0f * (float)glfwGetTime()), glm::vec3(0.0f, 0.1f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h_chess, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            // =====================================================================================================
            glDrawElements(GL_TRIANGLES, indices_chess.size(), GL_UNSIGNED_INT, 0);
        }*/

        glfwSwapBuffers(globals.window);
        glfwPollEvents();
    }
    std::cout << "Program ended." << '\n';
}

void HandleCameraMovement()
{
    float cameraSpeed = 2.5f * delta_t;
    //std::cout << "Delta = " << delta_t<< "\n";
    if(moveForward) cameraPosition += cameraSpeed * cameraFront;
    if(moveBackward)cameraPosition -= cameraSpeed * cameraFront;
    if(moveRight) {
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUP));
        cameraPosition += cameraRight * cameraSpeed;
    }
    if (moveLeft) {
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUP));
        cameraPosition -= cameraRight * cameraSpeed;
    }
}


//===================================================== END OF MAIN =====================================================
GLuint PrepareShaderProgram(std::string& vert_shader_path, std::string& frag_shader_path)
{
    //(FS = fragment shader, VS = vertex shader)
    GLuint prog_h;
    GLuint VS_h = glCreateShader(GL_VERTEX_SHADER);
    GLuint FS_h = glCreateShader(GL_FRAGMENT_SHADER);

    std::string VSsrc = textFileRead(vert_shader_path);
    const char* VS_string = VSsrc.c_str();
    std::string FSsrc = textFileRead(frag_shader_path);
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
    return prog_h;
}

GLuint PrepareVAO(std::vector<vertex> vertices, std::vector<GLuint> indices) {
    GLuint resultVAO = glCreateShader(GL_VERTEX_SHADER); //fust something to stop compile errors
    GLuint VBO, EBO;
    //GL names for Array and Buffers Objects
    // Generate the VAO and VBO
    glGenVertexArrays(1, &resultVAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Bind VAO (set as the current)
    glBindVertexArray(resultVAO);
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

    return resultVAO;
}
void GenerateChessPattern(std::vector<vertex>& vertices_chess, cvflann::lsh::Bucket& indices_chess)
{
    int min_x = -1;
    int max_x = 1;
    int min_y = -1;
    int max_y = 1;
    int ver_count = 0;

    float square_size = 0.1f;
    glm::vec3 color_chess;
    bool is_black = true;
    //generovani sachovnice
    for (float i = min_x; i < max_x; i += square_size) {
        is_black = !is_black;
        for (float j = min_y; j < max_y; j += square_size)
        {
            if (is_black) {
                color_chess = { 1.0f, 0.0f, 0.0f };
            }
            else color_chess = { 1.0f, 1.0f, 1.0f };
            is_black = !is_black;

            vertices_chess.push_back({ { i            , j, 0 }, color_chess });
            vertices_chess.push_back({ { i + square_size, j, 0 }, color_chess });
            vertices_chess.push_back({ { i, j + square_size, 0 }, color_chess });

            vertices_chess.push_back({ { i + square_size,j + square_size, 0 }, color_chess });
            vertices_chess.push_back({ { i, j + square_size, 0 }, color_chess });
            vertices_chess.push_back({ { i + square_size, j, 0 }, color_chess });

            for (int ver = 0; ver < 6; ver++) {
                indices_chess.push_back(ver_count++);
            }
        }
    }
}
