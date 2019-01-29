/*
 * platformparam.h
 *
 *  Created on: 2018年9月18日
 *      Author: jet
 */

#ifndef PLATFORMPARAM_H_
#define PLATFORMPARAM_H_

#include "sensorComp.h"
#include "deviceUser.h"
#include "platformFilter.h"
#include "Kalman.h"

const int VIRTUAL_DEVICE_COUNT	= 24;
const int  ANALOG_DEVICE_COUNT	= 16;
const int  SENSOR_COUNT	= 2;


#ifndef ONLYREAD
#	define ONLYREAD
#endif
typedef void * HPLTFILTER;
 const double PI = 3.14159269798;


typedef enum{
	DevUsr_AcqWinPosXInput =0,
	DevUsr_AcqWinPosYInput,
	DevUsr_AcqWinSizeXInput,
	DevUsr_AcqWinSizeYInput,
	DevUsr_TrkWinSizeXInput,
	DevUsr_TrkWinSizeYInput,
	DevUsr_AcqPltRateXInput,
	DevUsr_AcqPltRateYInput,
	DevUsr_AcqJoystickXInput,
	DevUsr_AcqJoystickYInput,
	DevUsr_AimpointOffsetXInput,
	DevUsr_AimpointOffsetYInput,
	DevUsr_AcqPositionDemandXInput,
	DevUsr_AcqPositionDemandYInput,
	DevUsr_Sensor1ZoomFeedbackInput,
	DevUsr_AimpointRefineXInput,
	DevUsr_AimpointRefineYInput,
	DevUsr_AcqPreprocessorManualOffsetInput,
	DevUsr_PltCoastXInput,
	DevUsr_PltCoastYInput,
	DevUsr_TrkPreprocessor,
	DevUsr_MAX
}eDevUsr;

typedef enum{
	AcqOutputType_Zero = 0,
	AcqOutputType_JoystickInput,
	AcqOutputType_ShapedAndGained,
	AcqOutputType_ShapedAndGainedAndIntegrated,
	AcqOutputType_DeterminedByPosition,
	AcqOutputType_ZeroInitFilter,
	AcqOutputType_DeterminedByIncomingPlatformData,
	AcqOutputType_Max
}eAcqOutputType;    //

typedef enum
{
	PlatStateType_Acquire = 0 ,
	PlatStateType_Trk_Lock = 1,
	PlatStateType_Trk_Break_Lock_1 = 2,
	PlatStateType_Trk_Break_Lock_2 = 3,
}eTrkStateType,ePlatStateType;

typedef enum{
	Bleed_Disable        = 0,
	Bleed_BrosightError  = 1,
	Bleed_PlatformDemand = 2
}ePlatfromCtrlBleed;
#pragma pack (4)

typedef struct{
	int iSensor;
	ONLYREAD int iTrkAlgState;
	ONLYREAD float fBoresightPositionX;
	ONLYREAD float fBoresightPositionY;
	ONLYREAD float fTargetBoresightErrorX;
	ONLYREAD float fTargetBoresightErrorY;
	ONLYREAD float fWindowPositionX;
	ONLYREAD float fWindowPositionY;
	ONLYREAD float fWindowSizeX;
	ONLYREAD float fWindowSizeY;
	ONLYREAD float fPlatformDemandX;
	ONLYREAD float fPlatformDemandY;

}PLATFORMCTRL_Output;

typedef struct{
	ONLYREAD int iTrkAlgState;
	ONLYREAD float fBoresightPositionX;
	ONLYREAD float fBoresightPositionY;
	ONLYREAD float fTargetBoresightErrorX;
	ONLYREAD float fTargetBoresightErrorY;
	ONLYREAD float fAcqWindowPositionX;
	ONLYREAD float fAcqWindowPositionY;
	ONLYREAD float fAcqWindowSizeX;
	ONLYREAD float fAcqWindowSizeY;
	ONLYREAD float fTargetSizeX;
	ONLYREAD float fTargetSizeY;

}PLATFORMCTRL_TrackerInput;

typedef  struct {
	void *object;
	ONLYREAD float devUsrInput[DevUsr_MAX];
	ONLYREAD float localInput[DevUsr_MAX];
	ONLYREAD float virtalInput[VIRTUAL_DEVICE_COUNT];
	ONLYREAD float analogInput[ANALOG_DEVICE_COUNT];
	ONLYREAD PLATFORMCTRL_TrackerInput trackerInput;
	ONLYREAD PLATFORMCTRL_Output output;
}PLATFORMCTRL_Interface;
#pragma pack ()

typedef PLATFORMCTRL_Interface * HPLTCTRL;
typedef SensorComp_CreateParams SensorParams;
typedef DeviceUser_CreateParams DeviceUserParam;
typedef PlatformFilter_CreateParams PlatformFilterParam;
typedef PlatformFilter_InitParams 	m_PlatformFilterParam;

#pragma pack (4)
typedef struct {
	float fOffset_X;
	float fOffset_Y;
	float fDeadband;
	float fCutpoint1;
	float fInputGain_X1;
	float fInputGain_Y1;
	float fCutpoint2;
	float fInputGain_X2;
	float fInputGain_Y2;
	float fPlatformAcquisitionModeGain_X;
	float fPlatformAcquisitionModeGain_Y;

	int   iShapingMode;
	float fPlatformIntegratingGain_X;
	float fPlatformIntegratingGain_Y;
	float fPlatformIntegratingDecayGain;
}JoystickRateDemandParam;

typedef struct
{
	SensorParams sensorParams[SENSOR_COUNT];
	DeviceUserParam deviceUsrParam[DevUsr_MAX];
	PlatformFilterParam platformFilterParam[2][2];
	m_PlatformFilterParam m__cfg_platformFilterParam[5];
	JoystickRateDemandParam joystickRateDemandParam;

	float scalarX;
	float scalarY;
	float demandMaxX[5];
	//float demandMinX[5];
	float demandMaxY[5];
	//float demandMinY[5];
	float deadbandX[5];
	float deadbandY[5];
	float driftX;
	float driftY;
	float Cfactorplatform;
	float OutputSwitch;
	float OutputPort;

	int bleedUsed;
	float bleedX[5];
	float bleedY[5];
	int iSensorInit;
	float fFov[SENSOR_COUNT];
	eAcqOutputType acqOutputType2;
	int acqOutputType;
	int bTrkWinFilter;
	int Kx[5];
	int Ky[5];
	float Error_X[5];
	float Error_Y[5];
	int Time[5];
	float BleedLimit_X[5];
	float BleedLimit_Y[5];
}configPlatParam;

typedef struct
{
	SensorParams sensorParams[SENSOR_COUNT];
	DeviceUserParam deviceUsrParam[DevUsr_MAX];
	PlatformFilterParam platformFilterParam[2][2];
	m_PlatformFilterParam m__cfg_platformFilterParam;
	JoystickRateDemandParam joystickRateDemandParam;
	float scalarX;
	float scalarY;
	float demandMaxX;
	float demandMinX;
	float demandMaxY;
	float demandMinY;
	float deadbandX;
	float deadbandY;
	float driftX;
	float driftY;
	int bleedUsed;
	float bleedX;
	float bleedY;
	int iSensorInit;
	float fFov[SENSOR_COUNT];
	eAcqOutputType acqOutputType2;
	int acqOutputType;
	int bTrkWinFilter;

	int Kx;
	int Ky;
	float Error_X;
	float Error_Y;
	int Time;

}PlatformCtrlParam;
#pragma pack ()
typedef PlatformCtrlParam PlatformCtrl_CreateParams;
typedef configPlatParam	configPlatParam_InitParams;
#pragma pack (4)
typedef struct
{
    HSENSORCOMP hSensor[SENSOR_COUNT];
    float fovX;//mRad
    float fovY;//mRad
    float width;//pix
    float height;//pix
    float curRateDemandX;
    float curRateDemandY;
    int lastPlatStat;
    HDEVUSR hDevUsers[DevUsr_MAX];
    float usrInputBak[DevUsr_MAX];

    HPLTFILTER filter[2][2];
    int iFilter;
    HPLTFILTER joystickIntegrator[2];
    PlatformFilterParam joystickIntegratorParam[2];
    HKalman hWinFilter;
    int iTrkAlgStateBak;
    int acqOutputTypeBak;
} PLATFORMCTRL_pri;

typedef struct{
	int uartDevice;
	int baud;
	int data;
	int check;
	int stop;
	int flow;
}Uart;

typedef struct
{
    PLATFORMCTRL_Interface inter;
    PlatformCtrlParam params;
    PLATFORMCTRL_pri privates;
}PlatformCtrl_Obj;
#pragma pack ()

#endif /* PLATFORMPARAM_H_ */
