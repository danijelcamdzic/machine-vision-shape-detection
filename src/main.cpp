/**
 * @file main.cpp
 *
 * @brief Main program file for shape detection using OpenCV
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

/* Image pre-processing parameters */
#ifndef GAUSSIAN_BLUR_SIZE
#define GAUSSIAN_BLUR_SIZE 5
#endif

/**
 * @brief Main function for for shape detection using OpenCV
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments. The second argument should be the path to the image file.
 * @return int Returns 0 if the program executed successfully, and 1 otherwise.
 */
int main(int argc, char** argv) {
    /* Ensure image path is provided */
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <image path>" << std::endl;
        return 1;       /**< Return as errors happened */
    }

    /* Load the original image */
    cv::Mat image = cv::imread(argv[1]);
    if(image.empty()) {
        std::cerr << "Failed to open image file: " << argv[1] << std::endl;
        return 1;
    }
    cv::imshow("Original Image", image);    /**< Show original image */
    cv::waitKey(0);

    /* Convert the image to grayscale */
    cv::Mat image_gray;
    cv::cvtColor(image, image_gray, cv::COLOR_BGR2GRAY);
    cv::imshow("Grayscale Image", image_gray);    /**< Show grayscale version of the image */
    cv::waitKey(0);

    /* Blur the image */
    cv::Mat image_blurred;
    cv::GaussianBlur(image_gray, image_blurred, cv::Size(GAUSSIAN_BLUR_SIZE, GAUSSIAN_BLUR_SIZE), 0);
    cv::imshow("Blurred Image", image_blurred);
    cv::waitKey(0);

    return 0;
}