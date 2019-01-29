#ifndef _PLATFORM_CONTROL_H
#define _PLATFORM_CONTROL_H

#include "stateCtrl.h"
#include "PlatformInterface.h"
#include "CMessage.hpp"
#include "globalDate.h"


class CplatFormControl : public CPlatformInterface{

public:
	CplatFormControl();
	~CplatFormControl();

HPLTCTRL PlatformCtrl_Create(PlatformCtrl_CreateParams *pPrm);

inline void PlatformCtrl_CreateParams_Init(PlatformCtrl_CreateParams *pPrm, configPlatParam_InitParams *m_Prm, View* m_Sensor);

void PlatformCtrl_Delete(HPLTCTRL handle);

int PlatformCtrl_TrackerInput(HPLTCTRL handle, PLATFORMCTRL_TrackerInput *pInput);

int PlatformCtrl_TrackerOutput(HPLTCTRL handle, PLATFORMCTRL_Output *pOutput);

int PlatformCtrl_VirtualInput(HPLTCTRL handle, int iIndex, float fValue);

protected:

private:
static CStatusManager* _stateManager;
static CGlobalDate* _GlobalDate;
CPlatformFilter* _Fiter;
CKalman_PTZ* _Kalman;
CDeviceUser* _DeviceUser;
CSensorComp* _Sensor;
CMessage* _Message;

int PlatformCtrl_AcqRateDemand(PlatformCtrl_Obj *pObj);
int PlatformCtrl_OutPlatformDemand(PlatformCtrl_Obj *pObj);
int PlatformCtrl_BuildDevUsrInput(PlatformCtrl_Obj *pObj);

void detectionFunc(PlatformCtrl_Obj *pObj);
void PlatformCtrl_ClassInit();
void BleedToPID_X(PlatformCtrl_Obj *pObj, float fTmpX, float fTmp);
void BleedToPID_Y(PlatformCtrl_Obj *pObj, float fTmpY, float fTmp);


};

#endif
