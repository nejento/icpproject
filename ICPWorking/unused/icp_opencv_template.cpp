// icp.cpp 
// Author: JJ

#include <iostream>

// OpenCV 
#include <opencv2\opencv.hpp>


int main(int argc, char* argv[])
{
    int x, y;

    // load image
    cv::Mat scene = cv::imread("resources/lightbulb.jpg"); //can be JPG,PNG,GIF,TIFF,...

    if (scene.empty())
    {
        printf("not found\n");
        exit(1);
    }

    //process pixels
    for (y = 0; y < scene.rows; y++)
    {
        for (x = 0; x < scene.cols; x++)
        {
            cv::Vec3b pixel = scene.at<cv::Vec3b>(y, x);
            double Y = 0.299 * pixel[?] + 0.587 * pixel[?] + 0.114 * pixel[?];
        }
    }

    //show me!
    cv::namedWindow("scene", 0);
    cv::imshow("scene", scene);

    while (1) { cv::waitKey(1); } //infinite message loop with 1ms delay

    return 0;
}




// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
