/**
 * @file main.cpp
 *
 * @brief Main program file for shape detection using OpenCV
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

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

    return 0;
}