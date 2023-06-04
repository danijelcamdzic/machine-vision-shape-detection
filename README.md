# About

This repo represents the final project of the "Machine Vision" course. The project has a goal to detect and sort object shapes, which are found on the sorting line and recorded using an industrial camera. The project utilizes OpenCV library.

# Installing OpenCV

Install OpenCV on Fedora Linux using the following:

```bash
sudo dnf install opencv opencv-devel
```

To install OpenCV on a different operating system, look at the official documentation of the OpenCV library.

# Building the Project

To build the project use the following:

```bash
mkdir build
cd build/
cmake ..
make
```

# Run the Project

To run the project use the following

```bash
./shape-detection-app /path/your_image_file
```