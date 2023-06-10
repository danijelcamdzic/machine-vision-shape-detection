/**
 * @file shape_detection.h
 *
 * @brief Contains defines and function declarations for shape detection
 */

#ifndef SHAPE_DETECTION_H
#define SHAPE_DETECTION_H

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
void process_image_and_detect_shapes(cv::Mat &image);
void process_contour(cv::Mat &image, std::vector<cv::Point> &contour);
std::string detect_shape(std::vector<cv::Point> &contour);

#endif /* SHAPE_DETECTION_H */
