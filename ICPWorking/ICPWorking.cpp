
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
    cv::VideoCapture capture;
} s_globals;

s_globals globals;

std::mutex img_access_mutex;
bool image_proccessing_alive;

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
}

int main()
{
    //init_glew();
    //cv::Mat frame = cv::imread("resources/HSV-MAP.png");

    globals.capture = cv::VideoCapture(cv::CAP_DSHOW);
    //globals.capture = cv::VideoCapture("resources/video.mkv");
    if (!globals.capture.isOpened()) //pokud neni kamera otevřená 
    {
        std::cerr << "no camera" << std::endl;
        exit(EXIT_FAILURE);
    }


    image_proccessing_alive = true;
    std::thread t1(image_processing, "something");

    cv::Mat frame;
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
