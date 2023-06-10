#ifndef __MV_2019_SortingLine_Template_H__
#define __MV_2019_SortingLine_Template_H__


#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Setting for using Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
//typedef Pylon::CBaslerGigEInstantCamera Camera_t;
typedef Pylon::CInstantCamera Camera_t;
//using namespace Basler_GigECameraParams;

// Namespace for using pylon objects.
using namespace Pylon;

int SortingLineSetTargetPosition(unsigned long position);
unsigned long SortingLineGetCurrentPosition (void);
unsigned long SortingLineGetObjectBeginningPosition (void);
unsigned long SortingLineGetObjectEndPosition (void);
int SortingLineGetInPositionStatus(void);
int SortingLineSetPositionPusher1 (unsigned long mavis_position_obj_push);
int SortingLineSetPositionPusher2 (unsigned long mavis_position_obj_push);
int SortingLineSetPositionPusher3 (unsigned long mavis_position_obj_push);
int SortingLineLightON(void);
int SortingLineLightOFF(void);
int SortingLineLaserRedON(void);
int SortingLineLaserRedOFF(void);
int SortingLineLaserGreenON(void);
int SortingLineLaserGreenOFF(void);
int SortingLineLaserBlueON(void);
int SortingLineLaserBlueOFF(void);
int SortingLineLasersRgbOnOff(unsigned char rgb);
int SortingLineSetEncoderFeedback(void);
int SortingLineSetServoFeedback(void);
int SortingLineSetPositioningTolerance(unsigned int tolerance);

int AcquireImage(Camera_t *camera);

#endif