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

/* Blurring */
#ifndef GAUSSIAN_BLUR_SIZE
#define GAUSSIAN_BLUR_SIZE 5        /**< The size of the blur window*/
#endif
/* Thresholding */
#ifndef BINARY_THRESHOLD_LOW
#define BINARY_THRESHOLD_LOW 220    /**< Everything below is 0 */
#endif
#ifndef BINARY_THRESHOLD_HIGH
#define BINARY_THRESHOLD_HIGH 255   /**< Everything above is 255 */
#endif
/* Canny edge detection */
#ifndef CANNY_THRESHOLD_1
#define CANNY_THRESHOLD_1 50        /**< First threshold for the hysteresis procedure */
#endif
#ifndef CANNY_THRESHOLD_2
#define CANNY_THRESHOLD_2 150       /**< Second threshold for the hysteresis procedure */
#endif
#ifndef CANNY_APERTURE_SIZE
#define CANNY_APERTURE_SIZE 3       /**< Aperture size for the Sobel operator*/
#endif

/* Function declaration */
std::string detect_shape(std::vector<cv::Point> &contour);

/**
 * @brief Main function for shape detection using OpenCV
 *
 * This function will:
 *               1. Load the image
 *               2. Convert it t ograyscale
 *               3. Blur it 
 *               4. Binarize it and invert its colors
 *               5. Use canny edge detection to detect edges
 *               6. Find contours
 *               7. Call the function to detect shapes
 *               8. Overlay the results on the original image
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
    cv::imshow("Grayscale Image", image_gray);      /**< Show grayscale version of the image */
    cv::waitKey(0);

    /* Blur the image */
    cv::Mat image_blurred;
    cv::GaussianBlur(image_gray, image_blurred, cv::Size(GAUSSIAN_BLUR_SIZE, GAUSSIAN_BLUR_SIZE), 0);
    cv::imshow("Blurred Image", image_blurred);     /**< Show blurred version of the image */
    cv::waitKey(0);

    /* Binarize and invert the image */
    cv::Mat image_bin;
    cv::threshold(image_blurred, image_bin, BINARY_THRESHOLD_LOW, BINARY_THRESHOLD_HIGH, cv::THRESH_BINARY_INV); 
    cv::imshow("Binary Inverted image", image_bin); /**< Show binary inverted version of the image */
    cv::waitKey(0);

    /* Apply Canny edge detection*/
    cv::Mat image_canny;
    cv::Canny(image_bin, image_canny, CANNY_THRESHOLD_1, CANNY_THRESHOLD_2, CANNY_APERTURE_SIZE);
    cv::imshow("Canny Image", image_canny);         /**< Show bcanny version of the image*/
    cv::waitKey(0);

    /* Find contours */
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(image_canny, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    /* Draw contours and detect shapes */
    for(auto &contour : contours) {
        /* Compute moments for the current contour */
        cv::Moments contourMoments = cv::moments(contour);
        
        /* Calculate the centroid of the contour */
        int centroidX = (int)(contourMoments.m10 / contourMoments.m00);
        int centroidY = (int)(contourMoments.m01 / contourMoments.m00);

        /* Detect the shape of the contour */
        std::string shape_name = detect_shape(contour);

        /* Draw the contour and the shape name */
        cv::drawContours(image, contours, -1, cv::Scalar(0, 255, 0), 2);
        cv::putText(image, shape_name, cv::Point(centroidX, centroidY), cv::FONT_HERSHEY_DUPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

        /* Show the original image with result overlays */
        cv::imshow("Original Image with Results", image);
        cv::waitKey(0);
    }

    return 0;
}

std::string detect_shape(std::vector<cv::Point> &contour) {
    std::string shape = "unknown";  

    /* Calculate the perimeter (total distance around the shape) */
    double perimeter = cv::arcLength(contour, true);  

    /* This will hold the simplified version of the shape */
    std::vector<cv::Point> shape_approx;  

    /* Simplify the shape, allowing it to be up to 2% different from the original */
    cv::approxPolyDP(contour, shape_approx, 0.03 * perimeter, true); 

    /* If the shape has 3 corners, it's a triangle */
    if (shape_approx.size() == 3) { 
        shape = "triangle"; 
    }
    /* If the shape has 4 corners, it could be a square or a rectangle */
    else if (shape_approx.size() == 4) {
        /* Get the smallest rectangle that can fit around the shape */
        cv::Rect boundingRect = cv::boundingRect(shape_approx); 

        /* Calculate the ratio of the width to the height */
        float aspect_ratio = (float) boundingRect.width / boundingRect.height; 

        /* If the aspect ratio is close to 1, it's a square; otherwise, it's a rectangle */
        shape = (aspect_ratio >= 0.95 && aspect_ratio <= 1.05) ? "square" : "rectangle"; 
    }
    /* If the shape has 5 corners, it's a pentagon */
    else if (shape_approx.size() == 5) { 
        shape = "pentagon"; 
    }
    /* If the shape has 6 corners, it's a hexagon */
    else if (shape_approx.size() == 6) { 
        shape = "hexagon"; 
    }
    /* If the shape has 10 corners, it's a star */
    else if (shape_approx.size() == 10) { 
        shape = "star"; 
    }
    /* If the shape has more corners or the shape is very rounded, it's a circle */
    else { 
        shape = "circle"; 
    }

    /* Return the name of the shape we've detected */
    return shape; 
}