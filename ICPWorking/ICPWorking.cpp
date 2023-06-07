/* TODO
* - Vzít movement ze starého projektu, fungoval imo lépe s voláním HandleCameraMovement
*/

//=== Includes ===
// C++ 
#include <iostream>
#include <chrono>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <memory> //for smart pointers (unique_ptr)
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>
#include <glm/ext.hpp>
//#include <irrklang/irrKlang.h>

// Other Header files
#include "OBJloader.h"


// === Headers ===
void init_glew(void);
void init_glfw(void);
void error_callback(int error, const char* description);
void finalize(int code);
void update_player_position();

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

std::string getProgramInfoLog(const GLuint obj);
std::string getShaderInfoLog(const GLuint obj);
std::string textFileRead(const std::string fn);

// create sound engine
/*
irrklang::ISoundEngine* engine = irrklang::createIrrKlangDevice();
*/

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

void setup_objects();
GLuint PrepareVAO(int index);

GLuint gen_tex(std::string filepath);
void make_shader(std::string vertex_shader, std::string fragment_shader, GLuint* shader);
void draw_textured(glm::mat4 m_m, glm::mat4 v_m, glm::mat4 projectionMatrix);
void draw_transparent(glm::mat4 m_m, glm::mat4 v_m, glm::mat4 projectionMatrix);

glm::vec3 check_collision(float x, float z);
std::array<bool, 3> check_objects_collisions(float x, float z);
void init_object_coords();



// === Globals ===
typedef struct image_data {
	cv::Point2f center;
	cv::Mat frame;
} image_data;

//std::unique_ptr<image_data> image_data_shared;

typedef struct s_globals {
	GLFWwindow* window;
	int height;
	int width;
	double app_start_time;
	cv::VideoCapture capture;
	bool fullscreen = false;
	int x = 0;
	int y = 0;
	double last_fps;
	float fov = 90.0f;
} s_globals;

s_globals globals;

//std::mutex img_access_mutex;
//bool image_proccessing_alive;

// player & position
glm::vec3 player_position(-10.0f, 1.0f, -10.0f);
glm::vec3 ball_position(0.0f, 0.0f, 0.0f);
glm::vec3 looking_position(10.0f, 1.0f, 10.0f);
glm::vec3 up(0, 1, 0);

GLfloat Yaw = -90.0f;
GLfloat Pitch = 0.0f;;
GLfloat Roll = 0.0f;

GLfloat lastxpos = 0.0f;
GLfloat lastypos = 0.0f;
#define array_cnt(a) ((unsigned int)(sizeof(a) / sizeof(a[0])))

// movement and sound help variables
int step_delay = 0;
bool ouch_ready = true;
int move_count = 0;


// === Asset Storage ===
// Vertex with color
/*
struct vertex {
	glm::vec3 position; // Vertex pos
	glm::vec3 color; // Color
	glm::vec3 normal; // Normal
};
*/

// Vertex with texture
struct tex_vertex {
	glm::vec3 position;
	glm::vec2 texcoord;
	glm::vec3 normal;
};

// Asset type
enum asset_type {
	asset_type_color,
	asset_type_texture
};

// Asset
struct asset {
	asset_type type;							// Asset type
	GLuint VAO;									// Vertex Array Object
	GLuint VBO;									// Vertex Buffer Object
	GLuint EBO;									// Element Buffer Object
	std::vector<vertex> vertex_array;			// Vertex Array
	std::vector<tex_vertex> tex_vertex_array;	// Vertex Array
	std::vector<GLuint> indices_array;			// Index Array
	glm::vec3 color;							// Color
	glm::vec3 scale;							// Scale
	glm::vec3 coord;							// Coordinates
};
const int n_assets = 17; // Počet inicializovaných objektů
asset assets[n_assets];  // Pole inicializovaných objektů

// Textures
GLuint texture_id[16];

// Objects with collisions
struct coords {
	float min_x;
	float max_x;
	float min_z;
	float max_z;
};
const int n_col_obj = 9;
std::vector<vertex> col_obj[n_col_obj];
coords objects_coords[n_col_obj];

GLuint prog_h, prog_tex, prog_transp;


// === Templated ===
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

//=== Templated: Čtení ze souboru ===
std::string textFileRead(const std::string fn) {
	std::ifstream file;
	file.exceptions(std::ifstream::badbit);
	std::stringstream ss;

	file.open(fn);
	if (file.is_open()) {
		std::string content;
		ss << file.rdbuf();
	}
	else {
		std::cerr << "Error opening file: " << fn << std::endl;
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

	exit(code);
	// ...
}

//=== Templated: Zachytávání chyb ===
void error_callback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

//=== Templated: Inicializace GL extensions GLEW, použitelné PO vytvoření GL contextu ===
void init_glew(void) {
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
}

//=== Editable: Nastavení GLFW ===
static void init_glfw(void)
{
	//
	// GLFW init.
	//

	// set error callback first
	glfwSetErrorCallback(error_callback);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);

	// assume ALL objects are non-transparent 
	glEnable(GL_CULL_FACE);

	// Transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//initialize GLFW library
	int glfw_ret = glfwInit();
	if (!glfw_ret)
	{
		std::cerr << "GLFW init failed." << std::endl;
		finalize(EXIT_FAILURE);
	}

	// Shader based, modern OpenGL (3.3 and higher)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // Living on the edge!
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	globals.window = glfwCreateWindow(800, 600, "Final Project Brož & Jacik", NULL, NULL);
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

	glfwMakeContextCurrent(globals.window);                                     // Set current window.
	glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);    // Get window size.
	//glfwSwapInterval(0);                                                      // Set V-Sync OFF.
	glfwSwapInterval(1);                                                        // Set V-Sync ON.


	globals.app_start_time = glfwGetTime();                                     // Get start time.
}


//=== Movement ===
/* Editable: Key callback function
* @brief: Callback function for keyboard input
* @param: window - window that received the event
* @param: key - key that was pressed or released
* @param: scancode - system-specific scancode of the key
* @param: action - GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT
* @param: mods - bit field describing which modifier keys were held down
* @return: none
*/


bool move_left_flag = false;
bool move_right_flag = false;
bool move_forward_flag = false;
bool move_backward_flag = false;

float delta_t = 0; // time between frames used for movement calculation
float last_frame = 0; // time of the last frame rendered

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		finalize(EXIT_SUCCESS);
	//glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W && action != GLFW_RELEASE)
		std::cout << 'W';
	if (key == GLFW_KEY_S && action != GLFW_RELEASE)
		std::cout << 'S';
	if (key == GLFW_KEY_A && action != GLFW_RELEASE)
		std::cout << 'A';
	if (key == GLFW_KEY_D && action != GLFW_RELEASE)
		std::cout << 'D';
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		if (globals.fullscreen) {
			glfwSetWindowMonitor(window, nullptr, globals.x, globals.y, 640, 480, 0);
			glViewport(0, 0, 640, 480);
			globals.fullscreen = false;
			glfwSetInputMode(globals.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			glfwGetWindowSize(window, &globals.x, &globals.y);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			glViewport(0, 0, mode->width, mode->height);
			globals.fullscreen = true;
			glfwSetInputMode(globals.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		move_forward_flag = true;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		move_backward_flag = true;
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		move_left_flag = true;
	if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		move_right_flag = true;
	}

	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		move_forward_flag = false;
	if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		move_backward_flag = false;
	if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		move_left_flag = false;
	if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
		move_right_flag = false;
	}
	std::cout << "Player position: " << player_position.x << " " << player_position.y << " " << player_position.z << " " << std::endl;
}

/* Editable: Mouse callback function - Pouze zaznamenáváme kliknutí, nereflektujeme žádné změny, jen vypisujeme v logu
* @brief: Callback function for mouse input
* @param: window - window that received the event
* @param: button - mouse button that was pressed or released
* @param: action - GLFW_PRESS or GLFW_RELEASE
* @param: mods - bit field describing which modifier keys were held down
* @return: none
*/
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		std::cout << "MOUSE_RIGHT" << '\n';
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		std::cout << "MOUSE_LEFT" << '\n';
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
		std::cout << "MOUSE_MIDDLE" << '\n';
}

/* Editable: Scroll callback function - Použité pro změnu FOV
* @brief: Callback function for mouse wheel scrolling
* @param: window - window that received the event
* @param: xoffset - scroll offset along the x-axis
* @param: yoffset - scroll offset along the y-axis
* @return: none
*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	std::cout << "mouse wheel(" << xoffset << ", " << yoffset << ")" << '\n';
	globals.fov += 10 * -yoffset;
	if (globals.fov > 170.0f) {
		globals.fov = 170.0f;
	}
	if (globals.fov < 20.0f) {
		globals.fov = 20.0f;
	}
}

/* Editable: Cursor position callback function - Použité pro otáčení postavy, tedy kamery
* @brief: Callback function for cursor position
* @param: window - window that received the event
* @param: xpos - new cursor x-coordinate, relative to the left edge of the content area
* @param: ypos - new cursor y-coordinate, relative to the top edge of the content area
* @return: none
*/
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//processMouseMovement(lastxpos-xpos, lastypos-ypos);
	std::cout << "cursor(" << xpos << ", " << ypos << ") " << '\n';

	Yaw += (xpos - lastxpos) / 5;
	Pitch += (lastypos - ypos) / 5;
	std::cout << "yp(" << Yaw << ", " << Pitch << ") " << '\n';

	if (true)
	{
		if (Pitch > 89.0f)
			Pitch = 89.0f;
		if (Pitch < -89.0f)
			Pitch = -89.0f;
	}

	looking_position.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	looking_position.y = sin(glm::radians(Pitch));
	looking_position.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

	lastxpos = xpos;
	lastypos = ypos;
}


//=== Main === 
int main()
{

	// === Random number generator ===
	std::mt19937_64 rng;
	// initialize the random number generator with time-dependent seed
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
	rng.seed(ss);
	// initialize a uniform distribution between 0 and 1
	std::uniform_real_distribution<double> unif(0, 1);


	init_glfw();
	init_glew();

	if (glfwExtensionSupported("GL_ARB_debug_output"))
	{
		glDebugMessageCallback(MessageCallback, 0);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		std::cout << "GL_DEBUG enabled." << std::endl;
	}

	// enable Z buffer test
	glEnable(GL_DEPTH_TEST);

	// ALL objects are non-transparent, cull back face of polygons 
	glEnable(GL_CULL_FACE);

	// create shaders
	std::cout << "BASIC SHADER" << '\n';
	make_shader("resources/my.vert", "resources/my.frag", &prog_h);
	glUseProgram(prog_h);

	std::cout << "TRANSPARENCY SHADER" << '\n';
	make_shader("resources/transparency.vert", "resources/transparency.frag", &prog_transp);

	std::cout << "TEXTURE SHADER" << '\n';
	make_shader("resources/texture.vert", "resources/texture.frag", &prog_tex);


	// load objects
	setup_objects();

	// set callbacks
	glfwSetCursorPosCallback(globals.window, cursor_position_callback);
	glfwSetScrollCallback(globals.window, scroll_callback);
	glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
	glfwSetKeyCallback(globals.window, key_callback);

	// frames and time
	int frame_cnt = 0;
	globals.last_fps = glfwGetTime();

	// transformations
	// projection & viewport
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

	float ratio = static_cast<float>(width) / height;

	// set visible area
	glViewport(0, 0, width, height);

	// Načtení textur
	texture_id[0] = gen_tex("resources/textures/box.png");
	texture_id[1] = gen_tex("resources/textures/concrete.png");
	texture_id[2] = gen_tex("resources/textures/brick_wall_texture.jpg");
	texture_id[3] = gen_tex("resources/textures/missing.png");
	texture_id[4] = gen_tex("resources/textures/mic_textura.jpg");

	// === Main Loop ===
	while (!glfwWindowShouldClose(globals.window)) {

		float current_frame = glfwGetTime();
		delta_t = current_frame - last_frame;
		last_frame = current_frame;

		glm::mat4 projectionMatrix = glm::perspective(
			glm::radians(globals.fov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
			ratio,			           // Aspect Ratio. Depends on the size of your window.
			0.1f,                      // Near clipping plane. Keep as big as possible, or you'll get precision issues.
			20000.0f                   // Far clipping plane. Keep as little as possible.
		);

		//set uniform for shaders - projection matrix
		glUniformMatrix4fv(glGetUniformLocation(prog_h, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		// set light color for shader
		glUniform4f(glGetUniformLocation(prog_h, "lightColor"), 1.0f, 1.0f, 1.0f, 1.0f);

		update_player_position(); //changing the movement of the player based on movement flags

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			glUseProgram(prog_h);

			// Model Matrix
			glm::mat4 m_m = glm::identity<glm::mat4>();

			// modify Model matrix and send to shaders
			m_m = glm::scale(m_m, glm::vec3(2.0f));

			// Předání do shaderu
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));

			// View matrix
			glm::mat4 v_m = glm::lookAt(
				player_position, //position of camera
				glm::vec3(player_position + looking_position), //where to look
				up  //UP direction
			);

			// set light pos in above player for shaders
			glUniform3f(glGetUniformLocation(prog_h, "lightPos"), player_position.x / 2, player_position.y / 2 + 1.0f, player_position.z / 2);
			// set camera pos for shaders
			glUniform3f(glGetUniformLocation(prog_h, "camPos"), player_position.x, player_position.y, player_position.z);

			// Předání do shaderu
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));

			// Use buffers
			// Objekty 1 - 5 jsou statické stěny a prostřední žlutý čtverec
			for (int i = 1; i < 10 ; i++) {
				glBindVertexArray(assets[i].VAO);
				glDrawElements(GL_TRIANGLES, assets[i].indices_array.size(), GL_UNSIGNED_INT, 0);
			}


			//chasing ball			
			float ball_speed = 8.0f * delta_t;
			glm::mat4 temp = m_m;
			glm::vec3 target_offset = glm::vec3(0,-0.175f,0);
			glm::vec3 direction_to_player = player_position*0.5f - ball_position + target_offset;
			direction_to_player = glm::normalize(direction_to_player);
			ball_position = ball_position + direction_to_player * ball_speed;
			//bal
			m_m = glm::translate(m_m, ball_position);
			m_m = glm::rotate(m_m, glm::radians(720.0f * (float)glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
			glBindVertexArray(assets[10].VAO);
			glDrawElements(GL_TRIANGLES, assets[10].indices_array.size(), GL_UNSIGNED_INT, 0);
			m_m = temp;


			// rotate glove and f
			temp = m_m;
			
			glm::vec3 glove_position = (player_position + looking_position) * 0.5f;
			glove_position.y = player_position.y; //ignore the y component of looking position
			m_m = glm::translate(m_m, glove_position); // move to the position of the player (camera)
			m_m = m_m = glm::rotate(m_m, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));//rotate so the palm of the hand is facing down
			
			float glove_angle = Yaw-90;
			m_m = m_m = glm::rotate(m_m, glm::radians(glove_angle), glm::vec3(0.0f, 0.0f, 1.0f)); //rotate the glove so it matches the direction the camera faces


			glm::vec3 gloveOffset = glm::vec3(0.01f, -2,1);
			m_m = glm::translate(m_m, gloveOffset); //apply offset to put the glove to the right position relative to the camera
			
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
			glBindVertexArray(assets[11].VAO);
			glDrawElements(GL_TRIANGLES, assets[11].indices_array.size(), GL_UNSIGNED_INT, 0);
			m_m = temp;

			// move teapot on edge
			temp = m_m;
			int edge = 21000;
			glm::vec3 change = (move_count < edge * 2) ?
				((move_count < edge) ? glm::vec3(move_count * 0.001f, 0, 0) : glm::vec3(edge * 0.001f, 0, move_count * 0.001f - edge * 0.001f)) :
				((move_count < edge * 3) ? glm::vec3(edge * 0.003f - move_count * 0.001f, 0, edge / 1000) : glm::vec3(0, 0, edge * 0.004f - move_count * 0.001f));
			m_m = glm::translate(m_m, change);
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
			glBindVertexArray(assets[12].VAO);
			glDrawElements(GL_TRIANGLES, assets[12].indices_array.size(), GL_UNSIGNED_INT, 0);
			m_m = temp;
			move_count++;
			if (move_count == edge * 4) {
				move_count = 0;
			}
			glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));

			// textured object draw
			draw_textured(m_m, v_m, projectionMatrix);
			// transparent object draw
			draw_transparent(m_m, v_m, projectionMatrix);

		}
		// swap buffers for rendering, catch and react to events
		glfwSwapBuffers(globals.window);
		glfwPollEvents();

		// frames a time
		frame_cnt++;
		double now = glfwGetTime();

		// display fps
		if ((now - globals.last_fps) > 1) {
			globals.last_fps = now;
			std::cout << "FPS: " << frame_cnt << std::endl;
			frame_cnt = 0;
		}
	}

	std::cout << "Program ended." << '\n';
	return (EXIT_SUCCESS);
}


glm::vec3 check_collision(float x, float z) {
	std::array<bool, 3> col = check_objects_collisions(x, z);
	if (col[0]) {
		//if object isn't in bounds of any object, move freely
		player_position.x = x;
		player_position.z = z;
		ouch_ready = true;
	}
	else {
		if (col[1]) {
			if (ouch_ready) {
				/*engine->play2D("resources/sounds/ouch.mp3");*/
				ouch_ready = false;
			}
			//if x step would not be in object bounds, move only on x axis
			player_position.x = x;
		}
		if (col[2]) {
			if (ouch_ready) {
				/*engine->play2D("resources/sounds/ouch.mp3");*/
				ouch_ready = false;
			}
			//if z step would not be in object bounds, move only on z axis
			player_position.z = z;
		}
	}
	if (step_delay == 0 && ouch_ready) { /*engine->play2D("resources/sounds/step1.mp3");*/ }
	if (step_delay == 8 && ouch_ready) { /*engine->play2D("resources/sounds/step2.mp3");*/ }
	if (step_delay++ == 16) { step_delay = 0; }
	return player_position;
}

std::array<bool, 3> check_objects_collisions(float x, float z) {
	std::array<bool, 3> col = { true, true, true };
	for (coords c : objects_coords) {
		//if x is in object bounds and z is in object bounds
		if (x > c.min_x && x < c.max_x && z > c.min_z && z < c.max_z) {
			col[0] = false;
			//if x step would be in object bounds
			if (player_position.x < c.min_x || player_position.x > c.max_x) {
				col[1] = false;
			}
			//if z step would be in object bounds
			if (player_position.z < c.min_z || player_position.z > c.max_z) {
				col[2] = false;
			}
			break;
		}
	}
	return col;
}

void init_object_coords() {
	//get min and max coords for objects (used in collision logic)
	for (int i = 0; i < n_col_obj; i++) {
		objects_coords[i].min_x = 999;
		objects_coords[i].max_x = -999;
		objects_coords[i].min_z = 999;
		objects_coords[i].max_z = -999;
		for (vertex v : col_obj[i]) {
			if (v.position[0] * 2 < objects_coords[i].min_x) {
				objects_coords[i].min_x = v.position[0] * 2;
			}
			if (v.position[0] * 2 > objects_coords[i].max_x) {
				objects_coords[i].max_x = v.position[0] * 2;
			}
			if (v.position[2] * 2 < objects_coords[i].min_z) {
				objects_coords[i].min_z = v.position[2] * 2;
			}
			if (v.position[2] * 2 > objects_coords[i].max_z) {
				objects_coords[i].max_z = v.position[2] * 2;
			}
		}
	}
}

/*
* Setup objects
*/
void setup_objects() {

	// === Textured objects ===

	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	// textured 0
	assets[0].type = asset_type_texture;
	assets[0].tex_vertex_array.push_back({ {-10.0f, -0.9f, -10.0f}, glm::vec2(-10.0f, -10.0f), up });
	assets[0].tex_vertex_array.push_back({ { -10.0f, -0.9f, 10.0f}, glm::vec2(-10.0f, 10.0f), up });
	assets[0].tex_vertex_array.push_back({ { 10.0f, -0.9f, 10.0f}, glm::vec2(10.0f, 10.0f), up });
	assets[0].tex_vertex_array.push_back({ {10.0f, -0.9f, 10.0f}, glm::vec2(10.0f, 10.0f), up });
	assets[0].tex_vertex_array.push_back({ {10.0f, -0.9f, -10.0f}, glm::vec2(10.0f, -10.0f), up });
	assets[0].tex_vertex_array.push_back({ {-10.0f, -0.9f, -10.0f}, glm::vec2(-10.0f, -10.0f), up });
	assets[0].indices_array = { 0, 1, 2 , 3, 4, 5};
	PrepareVAO(0);

	// textured 1
	assets[13].type = asset_type_texture;
	assets[13].tex_vertex_array.push_back({ { 10.0f, -1.0f, 10.0f }, glm::vec2(-10.0f, 10.0f), up });
	assets[13].tex_vertex_array.push_back({ { 10.0f, -1.0f, -10.0f}, glm::vec2(10.0f, 10.0f), up });
	assets[13].tex_vertex_array.push_back({ { 0.0f, -1.0f, 0.0f}, glm::vec2(0.0f, 0.0f), up });
	assets[13].indices_array = { 0, 1, 2 };
	PrepareVAO(13);

	// textured 2 brick wall
	assets[14].type = asset_type_texture;
	assets[14].tex_vertex_array.push_back({ { -11.0f, -1.0f, 11.0f}, glm::vec2(-1.0f, -1.0f), up });
	assets[14].tex_vertex_array.push_back({ { -11.0f, 9.0f, 11.0f}, glm::vec2(-1.0f, 1.0f), up });
	assets[14].tex_vertex_array.push_back({ { 11.0f, 9.0, 11.0f }, glm::vec2(1.0f, 1.0f), up });
	assets[14].tex_vertex_array.push_back({ { 11.0f, 9.0f, 11.0f}, glm::vec2(1.0f, 1.0f), up });
	assets[14].tex_vertex_array.push_back({ { 11.0f, -1.0f, 11.0f}, glm::vec2(1.0f, -1.0f), up });
	assets[14].tex_vertex_array.push_back({ { -11.0f, -1.0f, 11.0f }, glm::vec2(-1.0f, -1.0f), up});
	assets[14].indices_array = { 0, 1, 2, 3, 4, 5};

	PrepareVAO(14);

	// textured 3
	assets[15].type = asset_type_texture;
	assets[15].tex_vertex_array.push_back({ { -10.0f, -1.0f, 10.0f}, glm::vec2(-10.0f, 10.0f), up });
	assets[15].tex_vertex_array.push_back({ { 10.0f, -1.0f, 10.0f }, glm::vec2(10.0f, 10.0f), up });
	assets[15].tex_vertex_array.push_back({ { 0.0f, -1.0f, 0.0f}, glm::vec2(0.0f, 0.0f), up });
	assets[15].indices_array = { 0, 1, 2 };
	PrepareVAO(15);


	// === Colored objects ===
	int index;
	
	index = 1;
	assets[index].type = asset_type_color;
	//setup color, scale and coordinates for object
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 1 };
	assets[index].coord = { -10.5, -0.5, 10.5 };
	//load object from file
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	//setup vertex array
	PrepareVAO(1);

	index = 2;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 1 };
	assets[index].coord = { 10.5, -0.5, -10.5 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(2);

	index = 3;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 20 };
	assets[index].coord = { -10.5, -0.5, 0 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(3);

	index = 4;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 20 };
	assets[index].coord = { 10.5, -0.5, 0 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(4);

	index = 5;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 20, 1, 1 };
	assets[index].coord = { 0, -0.5, -10.5 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(5);

	index = 6;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 20, 1, 1 };
	assets[index].coord = { 0, -0.5, 10.5 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(6);

	index = 7;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 1 };
	assets[index].coord = { 10.5, -0.5, 10.5 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(7);

	index = 8;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.3, 0.3, 0.3 };
	assets[index].scale = { 1, 1, 1 };
	assets[index].coord = { -10.5, -0.5, -10.5 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(8);

	index = 9;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.7, 0.7, 0.0 };
	assets[index].scale = { 2, 1, 2 };
	assets[index].coord = { 0, -0.5, 0 };
	loadOBJ("resources/obj/cube.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(9);

	index = 10;
	//asset_type_texture
	assets[index].type = asset_type_color;
	assets[index].color = { 1, 1, 1 };
	assets[index].scale = { 0.2f, 0.2f, 0.2f };
	assets[index].coord = { 0, 0, 0 };
	loadOBJ("resources/obj/mic.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(10);

	index = 11;
	assets[index].type = asset_type_color;
	assets[index].color = { 1, 0.1, 0.1 };
	assets[index].scale = { 0.001, 0.001, 0.001 };
	assets[index].coord = { 0, 0, 0 };
	loadOBJ("resources/obj/work_glove.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(11);

	index = 12;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.1, 1.0, 0.1 };
	assets[index].scale = { 0.1, 0.1, 0.1 };
	assets[index].coord = { -10.5, 0.0, -10.5 };
	loadOBJ("resources/obj/teapot.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(12);

	index = 16;
	assets[index].type = asset_type_color;
	assets[index].color = { 0.1, 1.0, 0.1 };
	assets[index].scale = { 0.1, 0.1, 0.1 };
	assets[index].coord = { -9.5, 0.0, -9.5 };
	loadOBJ("resources/obj/teapot.obj", assets[index].vertex_array, assets[index].indices_array, assets[index].color, assets[index].scale, assets[index].coord);
	PrepareVAO(16);

	//choose objects with collisions
	int j = 0;
	for (int i : {1, 2, 3, 4, 5, 6, 7, 8, 9}) {
		col_obj[j] = assets[i].vertex_array;
		j++;
	}

	init_object_coords();
}

/* Generates a texture object from an image file
* filepath: path to the image file
* returns: the ID of the generated texture object
*/
GLuint gen_tex(std::string filepath)
{
	GLuint ID;
	cv::Mat image = cv::imread(filepath);

	// Generates an OpenGL texture object
	glGenTextures(1, &ID);

	// Assigns the texture to a Texture Unit
	glBindTexture(GL_TEXTURE_2D, ID);

	// Texture data alignment for transfer (single byte = basic, slow, but safe option; usually not necessary) 
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);
	int compressed;
	GLint internalformat, compressed_size;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
	/* if the compression has been successful */
	if (compressed == GL_TRUE)
	{
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &compressed_size);
		std::cout << "ORIGINAL: " << image.total() * image.elemSize() << " COMPRESSED: " << compressed_size << " INTERNAL FORMAT: " << internalformat << std::endl;
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Configures the way the texture repeats
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

	// Unbinds the OpenGL Texture object so that it can't accidentally be modified
	glBindTexture(GL_TEXTURE_2D, 0);

	return ID;
}

/* Prepares the VAO for the given asset
* @param index - the index of the asset in the assets array
* @return the VAO of the asset
*/
GLuint PrepareVAO(int index) {

	GLuint resultVAO = glCreateShader(GL_VERTEX_SHADER); //just something to stop compile errors

	// Generate the VAO and VBO
	glGenVertexArrays(1, &assets[index].VAO);
	glGenBuffers(1, &assets[index].VBO);
	glGenBuffers(1, &assets[index].EBO);
	// Bind VAO (set as the current)
	glBindVertexArray(assets[index].VAO);
	// Bind the VBO, set type as GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, assets[index].VBO);

	if (assets[index].type == asset_type_texture) 
	{
		// Fill-in data into the VBO
		glBufferData(GL_ARRAY_BUFFER, assets[index].tex_vertex_array.size() * sizeof(tex_vertex), assets[index].tex_vertex_array.data(), GL_DYNAMIC_DRAW);
		// Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assets[index].EBO);
		// Fill-in data into the EBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, assets[index].indices_array.size() * sizeof(GLuint), assets[index].indices_array.data(), GL_DYNAMIC_DRAW);
		// Set Vertex Attribute to explain OpenGL how to interpret the VBO
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)(0 + offsetof(tex_vertex, position)));
		// Enable the Vertex Attribute 0 = position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)(0 + offsetof(tex_vertex, texcoord)));
		glEnableVertexAttribArray(1);

		// TEST
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)(0 + offsetof(tex_vertex, normal)));
		glEnableVertexAttribArray(2);
	}
	else if (assets[index].type == asset_type_color)
	{
		// Fill-in data into the VBO
		glBufferData(GL_ARRAY_BUFFER, assets[index].vertex_array.size() * sizeof(vertex), assets[index].vertex_array.data(), GL_DYNAMIC_DRAW);
		// Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assets[index].EBO);
		// Fill-in data into the EBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, assets[index].indices_array.size() * sizeof(GLuint), assets[index].indices_array.data(), GL_DYNAMIC_DRAW);
		// Set Vertex Attribute to explain OpenGL how to interpret the VBO
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, position)));
		// Enable the Vertex Attribute 0 = position
		glEnableVertexAttribArray(0);
		// Set end enable Vertex Attribute 1 = Texture Coordinates
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, color)));
		glEnableVertexAttribArray(1);

		// TEST
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, normal)));
		glEnableVertexAttribArray(2);
	}

	// Bind VBO and VAO to 0 to prevent unintended modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return resultVAO;
}

void make_shader(std::string vertex_shader, std::string fragment_shader, GLuint* shader) {
	GLuint VS_h, FS_h, prog_h;
	VS_h = glCreateShader(GL_VERTEX_SHADER);
	FS_h = glCreateShader(GL_FRAGMENT_SHADER);

	// vert
	std::string VSsrc = textFileRead(vertex_shader);
	const char* VS_string = VSsrc.c_str();
	// frag
	std::string FSsrc = textFileRead(fragment_shader);
	const char* FS_string = FSsrc.c_str();
	glShaderSource(VS_h, 1, &VS_string, NULL);
	glShaderSource(FS_h, 1, &FS_string, NULL);

	// compile and use shaders
	glCompileShader(VS_h);
	getShaderInfoLog(VS_h);
	glCompileShader(FS_h);
	getShaderInfoLog(FS_h);
	prog_h = glCreateProgram();
	glAttachShader(prog_h, VS_h);
	glAttachShader(prog_h, FS_h);
	glLinkProgram(prog_h);
	getProgramInfoLog(prog_h);
	*shader = prog_h;

	// check if vertex shader, fragment shader compiled successfuly and program linked
	GLint success = 0;
	std::cout << "Success false = " << GL_FALSE << std::endl;
	glGetShaderiv(VS_h, GL_COMPILE_STATUS, &success);
	std::cout << "Vertex shader " << success << std::endl;
	glGetShaderiv(FS_h, GL_COMPILE_STATUS, &success);
	std::cout << "Fragment shader " << success << std::endl;
	glGetProgramiv(prog_h, GL_LINK_STATUS, &success);
	std::cout << "Program linking " << success << std::endl;
}

void draw_textured(glm::mat4 m_m, glm::mat4 v_m, glm::mat4 projectionMatrix) {
	glUseProgram(prog_tex);
	glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
	glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));
	glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	// set light color for shader
	glUniform4f(glGetUniformLocation(prog_tex, "lightColor"), 1.0f, 0.5f, 0.5f, 1.0f);
	// set light pos in above player for shaders
	glUniform3f(glGetUniformLocation(prog_tex, "lightPos"), player_position.x / 2, player_position.y / 2 + 1.0f, player_position.z / 2);
	// set camera pos for shaders
	glUniform3f(glGetUniformLocation(prog_tex, "camPos"), player_position.x, player_position.y, player_position.z);

	//set texture unit
	glActiveTexture(GL_TEXTURE0);

	//send texture unit number to FS
	glUniform1i(glGetUniformLocation(prog_tex, "tex0"), 0);

	// draw object using VAO (Bind+DrawElements+Unbind)
	glBindVertexArray(assets[0].VAO);
	glBindTexture(GL_TEXTURE_2D, texture_id[0]);
	glDrawElements(GL_TRIANGLES, assets[0].indices_array.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(assets[13].VAO);
	glBindTexture(GL_TEXTURE_2D, texture_id[1]);
	glDrawElements(GL_TRIANGLES, assets[13].indices_array.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(assets[14].VAO);
	glBindTexture(GL_TEXTURE_2D, texture_id[2]);
	glDrawElements(GL_TRIANGLES, assets[14].indices_array.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(assets[15].VAO);
	glBindTexture(GL_TEXTURE_2D, texture_id[3]);
	glDrawElements(GL_TRIANGLES, assets[15].indices_array.size(), GL_UNSIGNED_INT, 0);

	/*glBindVertexArray(assets[10].VAO);
	glBindTexture(GL_TEXTURE_2D, texture_id[4]);
	glDrawElements(GL_TRIANGLES, assets[10].indices_array.size(), GL_UNSIGNED_INT, 0);*/

	glUseProgram(prog_h);
}

void draw_transparent(glm::mat4 m_m, glm::mat4 v_m, glm::mat4 projectionMatrix) {
	glUseProgram(prog_transp);

	glEnable(GL_BLEND);			// enable blending
	glDisable(GL_CULL_FACE);	// no polygon removal
	glDepthMask(GL_FALSE);		// set Z to read-only

	glUniformMatrix4fv(glGetUniformLocation(prog_transp, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
	glUniformMatrix4fv(glGetUniformLocation(prog_transp, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));
	glUniformMatrix4fv(glGetUniformLocation(prog_transp, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	// set light color for shader
	glUniform4f(glGetUniformLocation(prog_transp, "lightColor"), 1.0f, 0.5f, 0.5f, 1.0f);
	// set light pos in above player for shaders
	glUniform3f(glGetUniformLocation(prog_transp, "lightPos"), player_position.x / 2, player_position.y / 2 + 1.0f, player_position.z / 2);
	// set camera pos for shaders
	glUniform3f(glGetUniformLocation(prog_transp, "camPos"), player_position.x, player_position.y, player_position.z);
	// set transparency for shaders
	glUniform1f(glGetUniformLocation(prog_transp, "transparency"), 0.5f);


	glBindVertexArray(assets[16].VAO);
	glDrawElements(GL_TRIANGLES, assets[16].indices_array.size(), GL_UNSIGNED_INT, 0);


	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glUseProgram(prog_h);
}

void update_player_position()
{
	float speed = 35.0f * delta_t;

	if (move_right_flag) {
		glm::vec3 xz = player_position + speed * glm::normalize(glm::cross(looking_position, up));
		player_position = check_collision(xz.x, xz.z);
	}
	if (move_left_flag) {
		glm::vec3 xz = player_position - speed * glm::normalize(glm::cross(looking_position, up));
		player_position = check_collision(xz.x, xz.z);
	}
	if (move_forward_flag) {
		glm::vec3 xz = player_position + speed * glm::normalize(looking_position);

		player_position = check_collision(xz.x, xz.z);
	}
	if (move_backward_flag) {
		glm::vec3 xz = player_position - speed * glm::normalize(looking_position);
		player_position = check_collision(xz.x, xz.z);
	}
}
