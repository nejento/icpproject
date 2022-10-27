// ICPProject1.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//


// Spuštění programu: Ctrl+F5 nebo nabídka Ladit > Spustit bez ladění
// Ladění programu: F5 nebo nabídka Ladit > Spustit ladění

// Tipy pro zahájení práce:
//   1. K přidání nebo správě souborů použijte okno Průzkumník řešení.
//   2. Pro připojení ke správě zdrojového kódu použijte okno Team Explorer.
//   3. K zobrazení výstupu sestavení a dalších zpráv použijte okno Výstup.
//   4. K zobrazení chyb použijte okno Seznam chyb.
//   5. Pokud chcete vytvořit nové soubory kódu, přejděte na Projekt > Přidat novou položku. Pokud chcete přidat do projektu existující soubory kódu, přejděte na Projekt > Přidat existující položku.
//   6. Pokud budete chtít v budoucnu znovu otevřít tento projekt, přejděte na Soubor > Otevřít > Projekt a vyberte příslušný soubor .sln.

// C++ 
#include <iostream>
#include <chrono>
#include <numeric>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>


void draw_cross_relative(cv::Mat& img, cv::Point2f center_relative, int size);
void draw_cross(cv::Mat& img, int x, int y, int size);
cv::Point2f find_center_Y(cv::Mat& frame);
cv::Point2f find_center_HSV(cv::Mat& frame);

typedef struct s_globals {
    cv::VideoCapture capture;
} s_globals;

s_globals globals;

int main()
{
    //cv::Mat frame = cv::imread("resources/HSV-MAP.png");
    cv::Mat frame;
    globals.capture = cv::VideoCapture(cv::CAP_DSHOW);
    if (!globals.capture.isOpened())
    {
        std::cerr << "no camera" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        globals.capture.read(frame);
        if (frame.empty())
        {
            std::cerr << "Device closed (or video at the end)" << std::endl;
            break;
        }
        auto start = std::chrono::steady_clock::now();

        //cv::Point2f center_relative = find_center_Y(frame);
        cv::Point2f center_relative = find_center_HSV(frame);

        std::cout << "Stred zarovky relativne: " << center_relative << '\n';

        auto end = std::chrono::steady_clock::now();

        draw_cross_relative(frame, center_relative, 20);
        cv::namedWindow("frame");
        cv::imshow("frame", frame);

        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << "Elapsed time: " << elapsed_seconds.count() << "sec" << std::endl;

        cv::waitKey(1);

        //std::cout << "Hello World!";
    }
}

cv::Point2f find_center_HSV(cv::Mat& frame)
{
    cv::Mat frame_hsv;
    cv:cvtColor(frame, frame_hsv, cv::COLOR_BGR2HSV);

    //HSV range(0...180, 0...255, 0...255);
    cv::Scalar lower_bound(40, 80, 80);
    cv::Scalar upper_bound(70, 255, 255);
    cv::Mat frame_treshold;

    cv::inRange(frame_hsv, lower_bound, upper_bound, frame_treshold);

    cv::namedWindow("frametr");
    cv::imshow("frametr", frame_treshold);

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