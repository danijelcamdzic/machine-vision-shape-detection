/**
 * @file shape_detection.cpp
 *
 * @brief Contains function definitions for shape detection
 */

#include <iostream>
#include <string>
#include <vector>
#include "shape_detection.h"

/**
 * @brief Function for image processing and shape detection
 * 
 * @param image An image over which to display the contour
 * 
 * @return std::string shape_name
 */
std::string process_image_and_detect_shapes(cv::Mat &image) {
    if(image.empty()) {
        std::cerr << "Image is empty!" << std::endl;
        return "no shape";
    }
    //cv::imshow("Original Image", image);           /**< Show original image */
    //cv::waitKey(0);

    /* Resize the image */
    cv::Mat image_resized;
    cv::resize(image, image_resized, cv::Size(), RESIZE_FACTOR, RESIZE_FACTOR, cv::INTER_AREA);
    //cv::imshow("Resized Image", image_resized);    /**< Show resized version of the image */
    //cv::waitKey(0);

    /* Crop the image */
    int crop_rows_top = (int)(image_resized.rows * CROP_PERCENT_TOP);
    int crop_rows_bottom = (int)(image_resized.rows * (1 - CROP_PERCENT_BOTTOM));
    cv::Range row_range(crop_rows_top, crop_rows_bottom);
    cv::Range col_range(0, image_resized.cols);
    cv::Mat image_cropped = image_resized(row_range, col_range);
    //cv::imshow("Cropped Image", image_cropped);     /**< Show cropped version of the image */
    //cv::waitKey(0);

    /* Convert the image to grayscale */
    cv::Mat image_gray;
    cv::cvtColor(image_cropped, image_gray, cv::COLOR_BGR2GRAY);
    //cv::imshow("Grayscale Image", image_gray);      /**< Show grayscale version of the image */
    //cv::waitKey(0);

    /* Blur the image */
    cv::Mat image_blurred;
    cv::GaussianBlur(image_gray, image_blurred, cv::Size(GAUSSIAN_BLUR_SIZE, GAUSSIAN_BLUR_SIZE), 0);
    //cv::imshow("Blurred Image", image_blurred);     /**< Show blurred version of the image */
    //cv::waitKey(0);

    /* Binarize the image */
    cv::Mat image_bin;
    cv::threshold(image_blurred, image_bin, BINARY_THRESHOLD_LOW, BINARY_THRESHOLD_HIGH, cv::THRESH_BINARY); 
    //cv::imshow("Binary image", image_bin);          /**< Show binary version of the image */
    //cv::waitKey(0);

    /* Apply Canny edge detection*/
    cv::Mat image_canny;
    cv::Canny(image_bin, image_canny, CANNY_THRESHOLD_1, CANNY_THRESHOLD_2, CANNY_APERTURE_SIZE);
    //cv::imshow("Canny Image", image_canny);         /**< Show Canny version of the image*/
    //cv::waitKey(0);

    /* Find contours */
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy; 
    cv::findContours(image_canny, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

#if PROCESS_LONGEST_CONTOUR_ONLY
    if(contours.empty()) {
        std::cerr << "No contours found in the image" << std::endl;
        return "no shape";
    }

    /* Find the contour with the longest perimeter */
    auto longest_contour = std::max_element(contours.begin(), contours.end(), [](const std::vector<cv::Point>& contour1, const std::vector<cv::Point>& contour2) {
        return cv::arcLength(contour1, true) < cv::arcLength(contour2, true);
    });

    std::string shape_name = process_contour(image_cropped, *longest_contour);

#else
    for(size_t i = 0; i < contours.size(); i++) {
        /* If this contour has a parent contour, skip it (hierarchy is [next, previous, first child, parent]) */
        if(hierarchy[i][3] != -1) {
            continue;
        }

        std::string shape_name = process_contour(image_cropped, contours[i]);
    }
#endif
    
    return shape_name;
}

/**
 * @brief This function processes the contour and displays it on the image
 *
 * @param image An image over which to display the contour
 * @param contour A reference to a std::vector of cv::Point that represents the contour.
 *
 * @return std::string shape_name
 */
std::string process_contour(cv::Mat &image, std::vector<cv::Point> &contour) {
    cv::Moments contourMoments = cv::moments(contour);
    int centroidX = (int)(contourMoments.m10 / contourMoments.m00);
    int centroidY = (int)(contourMoments.m01 / contourMoments.m00);

    std::string shape_name = detect_shape(contour);

    /* Draw the contour and the shape name */
    cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, 0, cv::Scalar(0, 255, 0), 2);
    cv::putText(image, shape_name, cv::Point(centroidX, centroidY), cv::FONT_HERSHEY_DUPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

    /* Show the original image with result overlays */
    //cv::imshow("Original Image with Results", image);
    //cv::waitKey(0);
    
    return shape_name;
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
