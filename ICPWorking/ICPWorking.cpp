﻿#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

#include <GL/glew.h> //pro jednodušší práci s extentions 
#include <GL/wglew.h> 

#include <GLFW/glfw3.h> //knihovna pro zálkladní obsulu systému (klávesnice/myš)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <numeric>
#include <thread>
#include <vector>
#include <memory> //for smart pointers (unique_ptr)
#include <fstream>

#include "OBJloader.h"




void run_2D_raster_processing();
void draw_cross_relative(cv::Mat& img, cv::Point2f center_relative, int size);
void draw_cross(cv::Mat& img, int x, int y, int size);

void image_processing(std::string string);
cv::Point2f find_center_Y(cv::Mat& frame);
cv::Point2f find_center_HSV(cv::Mat& frame);


struct vertex {
    glm::vec3 position; // Vertex pos
    glm::vec3 color; // Color
    glm::vec2 texCoor; // Texture coordinates
    glm::vec3 normal; // Normal used for light reflectivity
};
GLuint PrepareVAO(std::vector<vertex> vertices, std::vector<GLuint> indices);

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
glm::vec4 color = { 1.0f, 1.0f, 1.0f, 0.0f };
std::mutex img_access_mutex;
bool image_proccessing_alive;
glm::vec3 player_position(0.0f, 0.0f, 0.2f);
glm::vec3 where_to_look(0.0f, 0.0f, 0.0f);
float cameraStep_size = 0.9;

int camera_movement_x = 0;
int camera_movement_z = 0;

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


void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}

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
        camera_movement_z = -1;
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        camera_movement_z = +1;
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        camera_movement_x = -1;
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        camera_movement_x = +1;

    if ((key == GLFW_KEY_W|| key == GLFW_KEY_S) && action == GLFW_RELEASE)
        camera_movement_z = 0;
    if ((key == GLFW_KEY_A || key == GLFW_KEY_D) && action == GLFW_RELEASE)
        camera_movement_x = 0;


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
static void finalize(int code)
{
    // ...

    // Close OpenGL window if opened and terminate GLFW  
    if (globals.window)
        glfwDestroyWindow(globals.window);
    glfwTerminate();

    // ...
}
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
    globals.window = glfwCreateWindow(800, 800, "Final project by Broz&Jacik", NULL, NULL);
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
    glfwMakeContextCurrent(globals.window);                                        // Set current window.
    glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);    // Get window size.
    //glfwSwapInterval(0);                                                        // Set V-Sync OFF.
    glfwSwapInterval(1);                                                        // Set V-Sync ON.


    globals.app_start_time = glfwGetTime();                                        // Get start time.
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        std::cout << "MOUSE_RIGHT" << '\n';
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        std::cout << "MOUSE_LEFT" << '\n';
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    std::cout << "x offset: " << xoffset << " , y offset: " << yoffset << '\n';
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    std::cout << "x pos: " << xpos << " , y pos: " << ypos << '\n';
}


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

void GenerateChessPattern(std::vector<vertex>& vertices_chess, cvflann::lsh::Bucket& indices_chess);

GLuint PrepareShaderProgram(std::string& vert_shader_path, std::string& frag_shader_path);

//===================================================== MAIN =====================================================
int main()
{
    //odkomentuj toto aby jsi spustil 2D raster tracker
    //run_2D_raster_processing();

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
    //prog_h set up
    GLuint prog_h = PrepareShaderProgram(vert_shader_path, frag_shader_path);

    std::string vert_shader_path_fan = "resources/basicFan.vert";
    std::string frag_shader_path_fan = "resources/basicFan.frag";
    GLuint prog_h2 = PrepareShaderProgram(vert_shader_path_fan, frag_shader_path_fan);

    std::string vert_shader_path_chess = "resources/basicChess.vert";
    std::string frag_shader_path_chess = "resources/basicChess.frag";
    GLuint prog_h_chess = PrepareShaderProgram(vert_shader_path_chess, frag_shader_path_chess);


    //init VAO1
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

    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

    float ratio = static_cast<float>(width) / height;

    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(60.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
        ratio,			     // Aspect Ratio. Depends on the size of your window.
        0.01f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        20000.0f             // Far clipping plane. Keep as little as possible.
    );

    //set uniform for shaders - projection matrix
    glUniformMatrix4fv(glGetUniformLocation(prog_h, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // set visible area
    glViewport(0, 0, width, height);
    
    double last_frame_time = glfwGetTime();
    

    std::vector<glm::vec3> vertices_car_subaru;
    std::vector<glm::vec2> uvs_car_subaru;
    std::vector<glm::vec3> normals_car_subaru;
    loadOBJ("resources/obj/_Subaru-Loyale.obj", vertices_car_subaru, uvs_car_subaru, normals_car_subaru);
        

    while(!glfwWindowShouldClose(globals.window)) {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        double delta_t = last_frame_time - glfwGetTime();
        last_frame_time = glfwGetTime();
        player_position.z += cameraStep_size * camera_movement_z * delta_t;
        player_position.x += cameraStep_size * camera_movement_x * delta_t;
        
        where_to_look.z += cameraStep_size * camera_movement_z * delta_t;
        where_to_look.x += cameraStep_size * camera_movement_x * delta_t;

        glm::mat4 v_m = glm::lookAt(player_position, //position of camera
            where_to_look,
            glm::vec3(0, 1, 0)  //UP direction
        );      
        //trojuhelnik
        {
            glUseProgram(prog_h);
            glBindVertexArray(VAO1);
            
            // Model Matrix
            glm::mat4 m_m = glm::identity<glm::mat4>();
            //m_m = glm::translate(m_m, glm::vec3(width / 2, height / 2, 0.0));
            //m_m = glm::scale(m_m, glm::vec3(500.0f));
            m_m = glm::rotate(m_m, glm::radians(50.0f * (float)glfwGetTime()), glm::vec3(0.3f, 0.1f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));
            glUniformMatrix4fv(glGetUniformLocation(prog_h, "uProj_M"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            // =====================================================================================================
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }
        
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
        }

        glfwSwapBuffers(globals.window);
        glfwPollEvents();
    }
    std::cout << "Program ended." << '\n';
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




//======================================================================================================================
//======================================================================================================================
//============================================2D-Raster-Processing======================================================
//======================================================================================================================
//======================================================================================================================
void run_2D_raster_processing() {
    cv::Mat frame = cv::imread("resources/HSV-MAP.png");
    globals.capture = cv::VideoCapture(cv::CAP_DSHOW);
    globals.capture = cv::VideoCapture("resources/video.mkv");
    if (!globals.capture.isOpened()) //pokud neni kamera otevřená
    {
        std::cerr << "no camera" << std::endl;
        exit(EXIT_FAILURE);
    }


    image_proccessing_alive = true;
    std::thread t1(image_processing, "something");

    //cv::Mat frame;
    cv::Point2f center_relative;
    std::unique_ptr<image_data> img_data_local_prt;
    while (true) {

        img_access_mutex.lock();
        if (image_data_shared) {
            img_data_local_prt = std::move(image_data_shared);
        }
        img_access_mutex.unlock();

        if (img_data_local_prt) {
            frame = img_data_local_prt->frame;
            center_relative = img_data_local_prt->center;
            draw_cross_relative(frame, center_relative, 20);
            cv::namedWindow("frame");
            cv::imshow("frame", frame);
            std::cout << "Stred relativne: " << center_relative << '\n';
            img_data_local_prt.reset();
        }
        cv::waitKey(60);

        if (!image_proccessing_alive) break;
    }
    t1.join();
    std::cout << "Program ended, threads were joined." << '\n';
}



void image_processing(std::string string) {

    while (true)
    {
        cv::Mat frame;
        globals.capture.read(frame);
        if (frame.empty())
        {
            std::cerr << "Device closed (or video at the end)" << std::endl;
            break;
        }

        //cv::Point2f center_relative = find_center_Y(frame);
        cv::Point2f center_relative = find_center_HSV(frame);

        img_access_mutex.lock();
        image_data_shared = std::make_unique<image_data>();
        image_data_shared->frame = frame;
        image_data_shared->center = center_relative;
        img_access_mutex.unlock();

        //auto end = std::chrono::steady_clock::now();
        //std::chrono::duration<double> elapsed_seconds = end - start;
        //std::cout << "Elapsed time: " << elapsed_seconds.count() << "sec" << std::endl;
        //std::cout << "Hello World!";

        cv::waitKey(1);
    }
    image_proccessing_alive = false;
}

cv::Point2f find_center_HSV(cv::Mat& frame)
{
    cv::Mat frame_hsv;
cv:cvtColor(frame, frame_hsv, cv::COLOR_BGR2HSV);

    //HSV range(0...180, 0...255, 0...255);
    //45-75 = green
    cv::Scalar lower_bound(45, 80, 80);
    cv::Scalar upper_bound(70, 255, 255);
    cv::Mat frame_treshold;

    cv::inRange(frame_hsv, lower_bound, upper_bound, frame_treshold);

    //cv::namedWindow("frametr");
    //cv::imshow("frametr", frame_treshold);

    std::vector<cv::Point> white_pixels;
    cv::findNonZero(frame_treshold, white_pixels);
    cv::Point white_reduced = std::reduce(white_pixels.begin(), white_pixels.end());

    cv::Point2f center_relative((float)white_reduced.x / white_pixels.size() / frame.cols, (float)white_reduced.y / white_pixels.size() / frame.rows);

    return center_relative;
}

cv::Point2f find_center_Y(cv::Mat& frame) {

    cv::Mat frame2;
    frame.copyTo(frame2);

    int sx = 0, sy = 0, sw = 0;

    for (int y = 0; y < frame.cols; y++)
    {
        for (int x = 0; x < frame.rows; x++)
        {
            cv::Vec3b pixel = frame.at<cv::Vec3b>(x, y); //BGR -> 2,1,0
            unsigned char Y = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];

            if (Y < 228)
            {
                Y = 0;
            }
            else
            {
                Y = 255;
                sx += x;
                sy += y;
                sw++;
            }

            frame2.at<cv::Vec3b>(x, y) = cv::Vec3b(Y, Y, Y);
        }
    }

    cv::Point2f center((float)sy / sw, (float)sx / sw);
    cv::Point2f center_relative((float)center.x / frame.cols, (float)center.y / frame.rows);
    //frame2.at<cv::Vec3b>(sx / sw, sy / sw) = cv::Vec3b(0, 0, 255);

    return center_relative;
}

void draw_cross(cv::Mat& img, int x, int y, int size)
{
    cv::Point p1(x - size / 2, y);
    cv::Point p2(x + size / 2, y);
    cv::Point p3(x, y - size / 2);
    cv::Point p4(x, y + size / 2);

    cv::line(img, p1, p2, CV_RGB(255, 0, 0), 3);
    cv::line(img, p3, p4, CV_RGB(255, 0, 0), 3);
}

void draw_cross_relative(cv::Mat& img, cv::Point2f center_relative, int size)
{
    center_relative.x = std::clamp(center_relative.x, 0.0f, 1.0f);
    center_relative.y = std::clamp(center_relative.y, 0.0f, 1.0f);
    size = std::clamp(size, 1, std::min(img.cols, img.rows));

    cv::Point2f center_absolute(center_relative.x * img.cols, center_relative.y * img.rows);

    cv::Point2f p1(center_absolute.x - size / 2, center_absolute.y);
    cv::Point2f p2(center_absolute.x + size / 2, center_absolute.y);
    cv::Point2f p3(center_absolute.x, center_absolute.y - size / 2);
    cv::Point2f p4(center_absolute.x, center_absolute.y + size / 2);

    cv::line(img, p1, p2, CV_RGB(0, 0, 255), 2);
    cv::line(img, p3, p4, CV_RGB(0, 0, 255), 2);
}

