#ifndef _DEVICE_USER_H
#define _DEVICE_USER_H

#include <string.h>

/* �����豸�û���� */
typedef void * HDEVUSR;

typedef enum {
	DEVUSER_InputSrcType_User = 0,
	DEVUSER_InputSrcType_Analog,
	DEVUSER_InputSrcType_Virtual
}eDEVUSER_InputSrcType;

typedef enum {
	DEVUSER_InputMethod_Absolute = 0,
	DEVUSER_InputMethod_Incremental
}eDEVUSER_InputMethod;

typedef enum {
	DEVUSER_GainType_Universal = 0,
	DEVUSER_GainType_Incremental
}eDEVUSER_GainType;

typedef struct
{
	int iIndex;

	eDEVUSER_InputSrcType inputSrcType;

	int iInputSrcNum;

	eDEVUSER_InputMethod inputMethod;

	float fGain;

	eDEVUSER_GainType gainType;

}DeviceUser_CreateParams;


class CDeviceUser{

public:
	CDeviceUser(){};
	~CDeviceUser(){};

static inline void DeviceUser_CreateParams_Init(DeviceUser_CreateParams *pPrm)
{
	memset(pPrm, 0, sizeof(DeviceUser_CreateParams));
	pPrm->fGain = 1.0f;
	pPrm->gainType = DEVUSER_GainType_Incremental;
}

int DeviceUser_GetIndex(HDEVUSR hDevUsr);

HDEVUSR DeviceUser_Create(DeviceUser_CreateParams *pPrm, void *pUser);

void DeviceUser_Delete(HDEVUSR hDevUsr);

void DeviceUser_Reset(HDEVUSR hDevUsr);

float DeviceUser_Get(HDEVUSR hDevUsr);

int DeviceUser_SetParam(HDEVUSR hDevUsr, DeviceUser_CreateParams *pPrm);

float PlatforCtrl_GetDevInputValue(void* handle, eDEVUSER_InputSrcType type, int iNumber);

};

#endif
