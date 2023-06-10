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
/* Define resize factor */
#ifndef RESIZE_FACTOR
#define RESIZE_FACTOR                   0.6     /**< Factor by which the image will be resized */
#endif
/* Define cropping percentages - should be values between 0 and 1 */
#ifndef CROP_PERCENT_TOP
#define CROP_PERCENT_TOP                0.02    /**< Percentage of the top of the image to crop */
#endif
#ifndef CROP_PERCENT_BOTTOM
#define CROP_PERCENT_BOTTOM             0.26    /**< Percentage of the bottom of the image to crop */
#endif
/* Blurring */
#ifndef GAUSSIAN_BLUR_SIZE
#define GAUSSIAN_BLUR_SIZE              15      /**< The size of the blur window */
#endif
/* Thresholding */
#ifndef BINARY_THRESHOLD_LOW
#define BINARY_THRESHOLD_LOW            35      /**< Everything below is 0 */
#endif
#ifndef BINARY_THRESHOLD_HIGH
#define BINARY_THRESHOLD_HIGH           255     /**< Everything above is 255 */
#endif
/* Canny edge detection */
#ifndef CANNY_THRESHOLD_1
#define CANNY_THRESHOLD_1               50      /**< First threshold for the hysteresis procedure */
#endif
#ifndef CANNY_THRESHOLD_2
#define CANNY_THRESHOLD_2               150     /**< Second threshold for the hysteresis procedure */
#endif
#ifndef CANNY_APERTURE_SIZE
#define CANNY_APERTURE_SIZE             5       /**< Aperture size for the Sobel operator*/
#endif
/* Define processing method */
#ifndef PROCESS_LONGEST_CONTOUR_ONLY
#define PROCESS_LONGEST_CONTOUR_ONLY    1     /**< Set to 1 to process only the longest contour, and 0 to process all contours */
#endif

/* Function declaration */
std::string detect_shape(std::vector<cv::Point> &contour);
void process_contour(cv::Mat &image, std::vector<cv::Point> &contour);

/**
 * @brief Main function for shape detection
 * 
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments. The second argument should be the path to the image file.
 * @return int Returns 0 if the program executed successfully, and 1 otherwise.
 */
int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <image path>" << std::endl;
        return 1;                                   /**< Return as errors happened */
    }

    /* Load the original image */
    cv::Mat image = cv::imread(argv[1]);
    if(image.empty()) {
        std::cerr << "Failed to open image file: " << argv[1] << std::endl;
        return 1;
    }
    cv::imshow("Original Image", image);           /**< Show original image */
    cv::waitKey(0);

    /* Resize the image */
    cv::Mat image_resized;
    cv::resize(image, image_resized, cv::Size(), RESIZE_FACTOR, RESIZE_FACTOR, cv::INTER_AREA);
    cv::imshow("Resized Image", image_resized);    /**< Show resized version of the image */
    cv::waitKey(0);

    /* Crop the image */
    int crop_rows_top = (int)(image_resized.rows * CROP_PERCENT_TOP);
    int crop_rows_bottom = (int)(image_resized.rows * (1 - CROP_PERCENT_BOTTOM));
    cv::Range row_range(crop_rows_top, crop_rows_bottom);
    cv::Range col_range(0, image_resized.cols);
    cv::Mat image_cropped = image_resized(row_range, col_range);
    cv::imshow("Cropped Image", image_cropped);     /**< Show cropped version of the image */
    cv::waitKey(0);

    /* Convert the image to grayscale */
    cv::Mat image_gray;
    cv::cvtColor(image_cropped, image_gray, cv::COLOR_BGR2GRAY);
    cv::imshow("Grayscale Image", image_gray);      /**< Show grayscale version of the image */
    cv::waitKey(0);

    /* Blur the image */
    cv::Mat image_blurred;
    cv::GaussianBlur(image_gray, image_blurred, cv::Size(GAUSSIAN_BLUR_SIZE, GAUSSIAN_BLUR_SIZE), 0);
    cv::imshow("Blurred Image", image_blurred);     /**< Show blurred version of the image */
    cv::waitKey(0);

    /* Binarize the image */
    cv::Mat image_bin;
    cv::threshold(image_blurred, image_bin, BINARY_THRESHOLD_LOW, BINARY_THRESHOLD_HIGH, cv::THRESH_BINARY); 
    cv::imshow("Binary image", image_bin);          /**< Show binary version of the image */
    cv::waitKey(0);

    /* Apply Canny edge detection*/
    cv::Mat image_canny;
    cv::Canny(image_bin, image_canny, CANNY_THRESHOLD_1, CANNY_THRESHOLD_2, CANNY_APERTURE_SIZE);
    cv::imshow("Canny Image", image_canny);         /**< Show Canny version of the image*/
    cv::waitKey(0);

    /* Find contours */
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy; 
    cv::findContours(image_canny, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

#if PROCESS_LONGEST_CONTOUR_ONLY
    if(contours.empty()) {
        std::cerr << "No contours found in the image" << std::endl;
        return 1;
    }

    /* Find the contour with the longest perimeter */
    auto longest_contour = std::max_element(contours.begin(), contours.end(), [](const std::vector<cv::Point>& contour1, const std::vector<cv::Point>& contour2) {
        return cv::arcLength(contour1, true) < cv::arcLength(contour2, true);
    });

    process_contour(image_cropped, *longest_contour);

#else
    for(size_t i = 0; i < contours.size(); i++) {
        /* If this contour has a parent contour, skip it (hierarchy is [next, previous, first child, parent]) */
        if(hierarchy[i][3] != -1) {
            continue;
        }

        process_contour(image_cropped, contours[i]);
    }
#endif

    return 0;
}

/**
 * @brief This function processes the contour and displays it on the image
 *
 * @param image An image over which to display hte contour
 * @param contour A reference to a std::vector of cv::Point that represents the contour.
 *
 * @return void
 */
void process_contour(cv::Mat &image, std::vector<cv::Point> &contour) {
    cv::Moments contourMoments = cv::moments(contour);
    int centroidX = (int)(contourMoments.m10 / contourMoments.m00);
    int centroidY = (int)(contourMoments.m01 / contourMoments.m00);

    std::string shape_name = detect_shape(contour);

    /* Draw the contour and the shape name */
    cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, 0, cv::Scalar(0, 255, 0), 2);
    cv::putText(image, shape_name, cv::Point(centroidX, centroidY), cv::FONT_HERSHEY_DUPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

    /* Show the original image with result overlays */
    cv::imshow("Original Image with Results", image);
    cv::waitKey(0);
}

/**
 * @brief This function detects the shape of a given contour based on the number of lines in it
 *
 * @param contour A reference to a std::vector of cv::Point that represents the contour.
 *
 * @return A std::string representing the detected shape.
 */
std::string detect_shape(std::vector<cv::Point> &contour) {
    std::string shape = "unknown";  

    /* Calculate the perimeter (total distance around the shape) */
    double perimeter = cv::arcLength(contour, true);  

    std::vector<cv::Point> shape_approx;  
    cv::approxPolyDP(contour, shape_approx, 0.03 * perimeter, true); 

    /* Detect the shape based on the number of lines in it*/
    if (shape_approx.size() == 3) { 
        shape = "triangle"; 
    }
    else if (shape_approx.size() == 4) {
        /* Get the smallest rectangle that can fit around the shape */
        cv::Rect boundingRect = cv::boundingRect(shape_approx); 

        float aspect_ratio = (float) boundingRect.width / boundingRect.height; 
        shape = (aspect_ratio >= 0.95 && aspect_ratio <= 1.05) ? "square" : "rectangle"; 
    }
    else if (shape_approx.size() == 5) { 
        shape = "pentagon"; 
    }
    else if (shape_approx.size() == 6) { 
        shape = "hexagon"; 
    }
    else { 
        /* Get the smallest rectangle that can fit around the shape */
        cv::Rect boundingRect = cv::boundingRect(shape_approx); 

        float aspect_ratio = (float) boundingRect.width / boundingRect.height; 
        shape = (aspect_ratio >= 0.9 && aspect_ratio <= 1.1) ? "circle" : "oval rectangle"; 
    }

    return shape; 
}
