
// cv02.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


//funkce find_center_HSV
// samostatné vlákno img_process_code
//      dostane kameru
/*      read file
    structira img_data_s
        Point2f
        Mat frame - data snímku
    unique_pnt na img_data_s (muze na to ukazovat jen jedna promena)
        nidky se tak obě vlákna nedostanou naraz do toho
        zmena pointeru( frame.copyTo(img_data_local -> img_data_local->frame))


*/
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

#include <GL/glew.h> //pro jednoodušší práci s extentions 
#include <GL/wglew.h> 

#include <GLFW/glfw3.h> //knihovna pro zálkladní obsulu systému (klávesnice/myš)
#include <glm/glm.hpp>

#include <numeric>
#include <thread>
#include <vector>
#include <memory> //for smart pointers (unique_ptr)
#include <fstream>


void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*
    userParam);
void draw_cross_relative(cv::Mat& img, cv::Point2f center_relative, int size);
void draw_cross(cv::Mat& img, int x, int y, int size);

void image_processing(std::string string);
cv::Point2f find_center_Y(cv::Mat& frame);
cv::Point2f find_center_HSV(cv::Mat& frame);

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


void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}

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




//===================================================== MAIN =====================================================
int main()
{
    init_glfw();
    init_glew();

    glfwSetCursorPosCallback(globals.window, cursor_position_callback);
    glfwSetScrollCallback(globals.window, scroll_callback);
    glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
    glfwSetKeyCallback(globals.window, key_callback);

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
    while (!glfwWindowShouldClose(globals.window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        //Trojuhelnik
        {
            glUseProgram(prog_h);
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
    }


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
