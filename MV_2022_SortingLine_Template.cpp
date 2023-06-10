#include "stdafx.h"

#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <iomanip>
#include "Serial.h"
#include "MV_2022_SortingLine_Template.h"
#include <chrono>
#include <thread>

#include <queue>

#include "shape_detection.h"

// Setting for using Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
//typedef Pylon::CBaslerGigEInstantCamera Camera_t;
typedef Pylon::CInstantCamera Camera_t;

//using namespace Basler_GigECameraParams;
// Namespace for using pylon objects.
using namespace Pylon;
using namespace cv;
using namespace std;

// Namespace for using GenApi objects
using namespace GenApi;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 100;

struct SortingLineObject {
	int beginning_position;
	int end_position;
};

std::queue<SortingLineObject> objects_queue;

#define IN_OBJECT 1
#define OUT_OF_OBJECT 0
int scanning_status = OUT_OF_OBJECT;

#define WAITING_ON_OBJECT 1
#define IDLE 0
int acquisition_status = IDLE;

Rect cropBackgroundFromImage(Mat* imageRgb, Mat* imageGray, double background_threshold, int method);

//CPylonImage target;
//CImageFormatConverter fc;
Mat image;
CPylonImage image2;
Mat cv_img, gray;
tstring com = L"\\\\.\\COM2";//L"COM1";
char stop[] = "q";
tstring commPortName(com);
Serial serial(commPortName, 57600);
//unsigned char comp[832*832];
unsigned char* iluminationCompR, * iluminationCompG, * iluminationCompB;
Mat grayBackgroundImage;
unsigned long mavis_position_target = 1000000;
unsigned long mavis_position_current = 1500;
unsigned long mavis_obj_beginning_position = 0;
unsigned long mavis_obj_beginning_position_past = 1;
unsigned long mavis_obj_end_position = 0;
unsigned long mavis_obj_end_position_past = 1;
unsigned long mavis_obj_position_push = 0;
int mavis_inp = 0;
int in_processing = 0;
int pusher_selection = 0;

int mavis_position_current_high = 0;
int mavis_position_current_low = 0;
int com_lock_dummy;

unsigned long foto_camera_distance = 3000;

int main(int argc, char** argv)
{
	image = imread("mv.jpg", IMREAD_COLOR); // Read the file

	if (!image.data) // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return -1;
	}
	namedWindow("Machine Vision", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("Machine Vision", image); // Show our image inside it.

	namedWindow("Current Image", WINDOW_NORMAL); // Create a window for display.

	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized. 
	PylonInitialize();


	try
	{

		cout << "Creating Camera..." << endl;
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
		cout << "Camera Created." << endl;
		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		INodeMap& nodemap = camera.GetNodeMap();
		camera.Open();

		// Get camera device information.
		cout << "Camera Device Information" << endl
			<< "=========================" << endl;
		cout << "Vendor           : "
			<< CStringPtr(nodemap.GetNode("DeviceVendorName"))->GetValue() << endl;
		cout << "Model            : "
			<< CStringPtr(nodemap.GetNode("DeviceModelName"))->GetValue() << endl;
		cout << "Firmware version : "
			<< CStringPtr(nodemap.GetNode("DeviceFirmwareVersion"))->GetValue() << endl << endl;
		// Camera settings.
		cout << "Camera Device Settings" << endl
			<< "======================" << endl;


		// GENICAM standard - definisanje parametara kamere
		// Get the integer nodes describing the AOI.
		CIntegerPtr width = nodemap.GetNode("Width");
		CIntegerPtr height = nodemap.GetNode("Height");
		CIntegerPtr offsetX(nodemap.GetNode("OffsetX"));
		CIntegerPtr offsetY(nodemap.GetNode("OffsetY"));
		CEnumerationPtr TriggerMode(nodemap.GetNode("TriggerMode"));
		CEnumerationPtr ExposureMode(nodemap.GetNode("ExposureMode"));
		CFloatPtr ExposureTimeAbs(nodemap.GetNode("ExposureTimeAbs"));//kod starih kamera
		//CFloatPtr ExposureTime(nodemap.GetNode("ExposureTime"));//kod stare je ExposureTimeAbs

		//podesavanje parametara kamere
		int64_t newWidth = 1600;
		int64_t newHeight = 1200;
		offsetX->SetValue(0);
		offsetY->SetValue(0);
		width->SetValue(newWidth);
		height->SetValue(newHeight);
		TriggerMode->FromString("Off");//trigger je interni
		ExposureMode->FromString("Timed");//triger je odredjen internom vremenskom bazom
		ExposureTimeAbs->SetValue(300);//vreme ekspozicije // kod starih je ExposureTimeAbs

		//parametri koji odredjuju nacin akvizicije u kameri
		camera.MaxNumBuffer = 1;
		camera.OutputQueueSize = 1;
		//camera.StartGrabbing(GrabStrategy_UpcomingImage);
		camera.StartGrabbing(GrabStrategy_UpcomingImage);//GrabStrategy_OneByOne GrabStrategy_LatestImageOnly

		//podesavanje parametara sortirne linije
		serial.MavisSendComData(&com_lock_dummy, 7, 30);//brzina
		serial.MavisSendComData(&com_lock_dummy, 5, 0);//specijalno podesavanje sortirne linije - ne dirati
		serial.MavisSendComData(&com_lock_dummy, 13, 200);//sorter time
		// serial.MavisSendComData(&com_lock_dummy, 14, 7200);//sorter 1 position
		// serial.MavisSendComData(&com_lock_dummy, 15, 10200);//sorter 2 position
		// serial.MavisSendComData(&com_lock_dummy, 16, 13200);//sorter 3 position
		serial.MavisSendComData(&com_lock_dummy, 23, 2);//INIT
		SortingLineSetServoFeedback();//servo enkoder se koristi za povratnu spregu
		SortingLineLightOFF();//kontrola osvetljenja


		//inicijalizacija pocetka i kraja objekta 
		//u toku rada programa detekcija pocetka i kraja objekta se donosi diferenciranjem trenutne i prosle pozicije
		//stoga se inicijalne trenutne i prosle vrednosti izjednacavaju
		mavis_obj_beginning_position = SortingLineGetObjectBeginningPosition();
		mavis_obj_beginning_position_past = mavis_obj_beginning_position;
		mavis_obj_end_position = SortingLineGetObjectEndPosition();
		mavis_obj_end_position_past = mavis_obj_end_position;


		while (camera.IsGrabbing())
		{
			mavis_position_current = SortingLineGetCurrentPosition();//ocitavanje trenutne pozicije trake
			//procesiranje signala fotocelije koji ukazuju na postojanje objekta
			if (scanning_status == OUT_OF_OBJECT) {
				mavis_obj_beginning_position = SortingLineGetObjectBeginningPosition();
				if (mavis_obj_beginning_position != mavis_obj_beginning_position_past) {
					//pocetak novog objekta
					scanning_status = IN_OBJECT;
					mavis_obj_beginning_position_past = mavis_obj_beginning_position;
					cout << endl << "OBJECT BEGINNING AT " << mavis_obj_beginning_position << endl;
				}
			}
			else {//IN_OBJECT
				mavis_obj_end_position = SortingLineGetObjectEndPosition();
				if (mavis_obj_end_position != mavis_obj_end_position_past) {
					//kraj novog objekta
					scanning_status = OUT_OF_OBJECT;
					mavis_obj_end_position_past = mavis_obj_end_position;
					SortingLineObject new_object;
					new_object.beginning_position = mavis_obj_beginning_position;
					new_object.end_position = mavis_obj_end_position;
					objects_queue.push(new_object);
					cout << endl << "OBJECT END AT " << mavis_obj_end_position << endl;
					//cout << "queue size: " << objects_queue.size() << endl;
				}
			}

			//ispitivanje da li postoji novi objekat u QUEUE-u
			//ako postoji zadaje se pozicioniranje na sredinu snimljenog objekta
			//ako ne postoji azurira se zadata pozicija na 500 impulsa veca od trenutne
			if (objects_queue.size() > 0) {
				SortingLineObject new_object;
				new_object = objects_queue.front();
				//cout << "queue size: " << objects_queue.size() << endl;				
				mavis_position_target = ((new_object.beginning_position + new_object.end_position) / 2) + foto_camera_distance;
				acquisition_status = WAITING_ON_OBJECT;
				cout << "W";
			}
			else {
				mavis_position_target = mavis_position_current + 500;//zadata pozicija je uvek za 500 ispred trenutne - sargarepa i konj 
				cout << ".";
			}
			SortingLineSetTargetPosition(mavis_position_target);//postavlja se pozicija zaustavljanja izracunata u prethodnom koraku


			//ukoliko se ceka da objekat detektovan na traci dodje ispod kamere proverava se IN POSITION signal od sortirne linije
			if (acquisition_status == WAITING_ON_OBJECT) {
				if (SortingLineGetInPositionStatus() == 1) {//ako je zadata pozicija dostignuta, ide se na akviziciju
					cout << endl << "IMAGE ACQUISITION AT " << mavis_position_current << endl;
					SortingLineLightON();//ukljucivanje osvetljenja
					waitKey(500);
					/* Image should be processed and shapes detected after this line */
					AcquireImage(&camera);
					/* Image should be processed and shapes detected before this line */

					// SortingLineLightOFF();
					// waitKey(500);
					// AcquireImage(&camera);
					// SortingLineLasersRgbOnOff(1);
					// waitKey(500);
					// AcquireImage(&camera);
					// SortingLineLasersRgbOnOff(2);
					// waitKey(500);
					// AcquireImage(&camera);
					// SortingLineLasersRgbOnOff(4);
					// waitKey(500);
					// AcquireImage(&camera);
					// SortingLineLasersRgbOnOff(0);
					// SortingLineObject new_object;
					// new_object = objects_queue.front();
					// mavis_obj_position_push = (new_object.beginning_position + new_object.end_position) / 2;//odredjuje se sredina objekta

					// switch (pusher_selection) {//rotira u krug redni broj izbacivaca
					// case 0:
					// 	SortingLineSetPositionPusher1(mavis_obj_position_push);
					// 	pusher_selection = 1;
					// 	break;
					// case 1:
					// 	SortingLineSetPositionPusher2(mavis_obj_position_push);
					// 	pusher_selection = 2;
					// 	break;
					// case 2:
					// 	SortingLineSetPositionPusher3(mavis_obj_position_push);
					// 	pusher_selection = 0;
					// 	break;
					// }
					objects_queue.pop();//brise se objekat iz Queue-a
					acquisition_status = IDLE;
				}
			}

			//dodatni ispis
			cout << "current, start, end:" << mavis_position_current << std::setw(10) << mavis_obj_beginning_position << std::setw(10) << mavis_obj_end_position << std::setw(10) << std::endl;

			if (waitKey(30) >= 0) break;
		}
		camera.StopGrabbing();
	}
	catch (const GenericException& e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}
	// Releases all pylon resources. 
	PylonTerminate();
	return exitCode;

}

int AcquireImage(Camera_t* camera) {
	// This smart pointer will receive the grab result data.
	CGrabResultPtr ptrGrabResult;
	CPylonImage imagePylonTemp;
	static Mat rgbImage, grayImage;
	Mat rgbImageWithoutBackground;
	static Mat rgbFinalImage, grayFinalImage, absdifference, difference, rgbImageWithBackground, grayImageWithBackground;
	CImageFormatConverter fc;
	fc.OutputPixelFormat = PixelType_BGR8packed;
	double t = (double)getTickCount();
	camera->RetrieveResult(3000, ptrGrabResult, TimeoutHandling_ThrowException);//			
	// Image grabbed successfully?
	if (ptrGrabResult->GrabSucceeded())
	{
		//converto from Pylon to OpenCV image format		
		fc.Convert(imagePylonTemp, ptrGrabResult);
		rgbImageWithBackground = Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)imagePylonTemp.GetBuffer());
		imwrite("images/rgbImageWithBackground.bmp", rgbImageWithBackground);
		imshow("Current Image", rgbImageWithBackground); // Show our image inside it.

		/* Call my function to process the image */
		process_image_and_detect_shapes(rgbImageWithBackground);
	}
	else
	{
		cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
	}
	return 0;
}


int SortingLineSetTargetPosition(unsigned long position) {
	serial.MavisSendComData(&com_lock_dummy, 30, (position >> 16) & 0xFFFF);//komanda koja postavlja visih 16 bita 32-bitne zadate pozicije
	serial.MavisSendComData(&com_lock_dummy, 31, position & 0xFFFF);//komanda koja postavlja nizih 16 bita 32-bitne zadate pozicije
	return 0;
}


unsigned long SortingLineGetCurrentPosition(void) {
	unsigned long mavis_position_current;
	int mavis_position_current_high, mavis_position_current_low;
	serial.MavisGetComData(&com_lock_dummy, 30, &mavis_position_current_high);//komanda koja ocitava visih 16 bita 32-bitne trenutne pozicije
	serial.MavisGetComData(&com_lock_dummy, 31, &mavis_position_current_low);//komanda koja ocitava nizih 16 bita 32-bitne trenutne pozicije
	mavis_position_current = (mavis_position_current_high << 16) | mavis_position_current_low;//trenutna 32-bitna pozicija trake
	return mavis_position_current;
}

unsigned long SortingLineGetObjectBeginningPosition(void) {
	unsigned long mavis_position_obj_start;
	int mavis_position_current_high, mavis_position_current_low;
	serial.MavisGetComData(&com_lock_dummy, 34, &mavis_position_current_high);//komanda koja ocitava visih 16 bita 32-bitne pozicije pocetka objekta koji je detektovala fotocelija
	serial.MavisGetComData(&com_lock_dummy, 35, &mavis_position_current_low);//komanda koja ocitava nizih 16 bita 32-bitne pozicije pocetka objekta koji je detektovala fotocelija
	mavis_position_obj_start = (mavis_position_current_high << 16) | mavis_position_current_low;//trenutna 32-bitna pozicija pocetka objekta
	return mavis_position_obj_start;
}

unsigned long SortingLineGetObjectEndPosition(void) {
	unsigned long mavis_position_obj_stop;
	int mavis_position_current_high, mavis_position_current_low;
	serial.MavisGetComData(&com_lock_dummy, 36, &mavis_position_current_high);//komanda koja ocitava visih 16 bita 32-bitne pozicije pocetka objekta koji je detektovala fotocelija
	serial.MavisGetComData(&com_lock_dummy, 37, &mavis_position_current_low);//komanda koja ocitava nizih 16 bita 32-bitne pozicije pocetka objekta koji je detektovala fotocelija
	mavis_position_obj_stop = (mavis_position_current_high << 16) | mavis_position_current_low;//trenutna 32-bitna pozicija pocetka objekta
	return mavis_position_obj_stop;
}

int SortingLineGetInPositionStatus(void) {
	int mavis_inp;
	serial.MavisGetComData(&com_lock_dummy, 32, &mavis_inp);
	return mavis_inp;
}

int SortingLineSetPositionPusher1(unsigned long mavis_position_obj_push) {
	mavis_position_obj_push = mavis_position_obj_push + 7600;
	serial.MavisSendComData(&com_lock_dummy, 40, (mavis_position_obj_push >> 16) & 0xFFFF);//
	serial.MavisSendComData(&com_lock_dummy, 41, mavis_position_obj_push & 0xFFFF);//
	return 0;
}

int SortingLineSetPositionPusher2(unsigned long mavis_position_obj_push) {
	mavis_position_obj_push = mavis_position_obj_push + 11000;
	serial.MavisSendComData(&com_lock_dummy, 42, (mavis_position_obj_push >> 16) & 0xFFFF);//
	serial.MavisSendComData(&com_lock_dummy, 43, mavis_position_obj_push & 0xFFFF);//
	return 0;
}

int SortingLineSetPositionPusher3(unsigned long mavis_position_obj_push) {
	mavis_position_obj_push = mavis_position_obj_push + 14400;
	serial.MavisSendComData(&com_lock_dummy, 44, (mavis_position_obj_push >> 16) & 0xFFFF);//
	serial.MavisSendComData(&com_lock_dummy, 45, mavis_position_obj_push & 0xFFFF);//
	return 0;
}


int SortingLineLightON(void) {
	serial.MavisSendComData(&com_lock_dummy, 46, 1);
	return 0;
}

int SortingLineLightOFF(void) {
	serial.MavisSendComData(&com_lock_dummy, 46, 0);
	return 0;
}

int SortingLineLaserRedON(void) {
	serial.MavisSendComData(&com_lock_dummy, 47, 1);
	return 0;
}

int SortingLineLaserRedOFF(void) {
	serial.MavisSendComData(&com_lock_dummy, 47, 0);
	return 0;
}

int SortingLineLaserGreenON(void) {
	serial.MavisSendComData(&com_lock_dummy, 48, 1);
	return 0;
}

int SortingLineLaserGreenOFF(void) {
	serial.MavisSendComData(&com_lock_dummy, 48, 0);
	return 0;
}

int SortingLineLaserBlueON(void) {
	serial.MavisSendComData(&com_lock_dummy, 49, 1);
	return 0;
}

int SortingLineLaserBlueOFF(void) {
	serial.MavisSendComData(&com_lock_dummy, 49, 0);
	return 0;
}

int SortingLineLasersRgbOnOff(unsigned char rgb) {
	if ((rgb & 0x01) == 0x01) SortingLineLaserRedON();
	else SortingLineLaserRedOFF();
	if ((rgb & 0x02) == 0x02) SortingLineLaserGreenON();
	else SortingLineLaserGreenOFF();
	if ((rgb & 0x04) == 0x04) SortingLineLaserBlueON();
	else SortingLineLaserBlueOFF();
	return 0;
}

int SortingLineSetEncoderFeedback(void) {
	serial.MavisSendComData(&com_lock_dummy, 26, 0);
	return 0;
}

int SortingLineSetServoFeedback(void) {
	serial.MavisSendComData(&com_lock_dummy, 26, 1);
	return 0;
}

int SortingLineSetPositioningTolerance(unsigned int tolerance) {
	serial.MavisSendComData(&com_lock_dummy, 25, tolerance);
	return 0;
}
