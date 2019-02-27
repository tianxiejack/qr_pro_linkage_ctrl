/*
 * josStick.h
 *
 *  Created on: 2018年9月25日
 *      Author: jet
 */

#ifndef JOSSTICK_H_
#define JOSSTICK_H_

#include <iostream>
#include "osa_thr.h"
#include "josInterface.h"
#include "globalDate.h"
#include "CMessage.hpp"
#include "stateCtrl.h"
#include <cstdio>
#include <stdlib.h>
#include <string>
#include <libusb.h>


const char  JS_EVENT_BUTTON = 0x01;
const char  JS_EVENT_AXIS = 0x02;
const char  JS_EVENT_INIT = 0x80;

#define USB_VENDOR_ID 0x0483
#define USB_PRODUCT_ID 0x5710

#define BULK_ENDPOINT_OUT 1
#define BULK_ENDPOINT_IN  2

struct userDevice{
	uint16_t idVendor;
	uint16_t idProduct;
	uint8_t  bInterfaceClass;
	uint8_t  bInterfaceSubClass;
	uint8_t  bmAttributes;
	libusb_device *dev;
	libusb_device **devs;
	u_int8_t bInEndpointAddress;
	u_int8_t bOutEndpointAddress;
	uint8_t  bInterfaceNumber;
};

typedef int	INT32;
typedef short	INT16;
typedef char	INT8;
typedef unsigned int	UINT32;
typedef unsigned short	UINT16;
typedef unsigned char	UINT8;
typedef long		LONG;
typedef unsigned long	ULONG;

typedef struct js_event {
    UINT32 time;
    INT16 value;
    UINT8 type;
    UINT8 number;
}joy_event;

typedef enum{
	Jos_dev,
	Jos_usb
}JosType;

typedef enum{
	usb_1_8 = 0,
	usb_special,
	usb_enter,
	usb_X,
	usb_Y,
	usb_Z
}HK_JosParams;


class CJoystick : public CJosInterface {

public:
	CJoystick();
	~CJoystick();

	int Create();
	int  Destroy();
	int JosToWinX(int x, int sensor);
	int JosToWinY(int y, int sensor);

	int JosToMouseX(int x, int sensor);
	int JosToMouseY(int y, int sensor);

private:
	static CStatusManager* _StatusManager;
	static CGlobalDate* _GlobalDate;
	CMessage* _Message;
	joy_event *jse;
	static bool JosStart;
	static bool HKJosStart;
	int joystick_fd;
	const char*  joystick_Dev = "/dev/input/js0";


	libusb_device_handle* g_usb_handle;
	struct userDevice user_device;
	struct libusb_device_descriptor dev_desc;

	int  Run();
	int  Stop();
	int open_joystick();
	int read_joystick_event(joy_event *jse);
	void JoystickProcess();
	int HKJoystickProcess();
	void procJosEvent_Axis(UINT8  mjosNum );
	void procMouse_Axis(UINT8 mjosNum);
    void ProcJosEvent_Button(UINT8 njosNum);
    void procMouse_Button(UINT8 njosNum);
    void Y_CtrlIrisAndFocus(int value);
    void josSendMsg(int MsgId);

    int USB_run();
    int get_device_descriptor(struct libusb_device_descriptor *dev_desc,struct userDevice *user_device);
    int init_libusb(void);
    int get_device_endpoint(struct libusb_device_descriptor *dev_desc,struct userDevice *user_device);
    int match_with_endpoint(const struct libusb_interface_descriptor * interface, struct userDevice *user_device);
	void HK_procJosEvent_Axis(unsigned char*  josNum );
	void HK_procMouse_Axis(unsigned char*  MouseNum);
    void HK_ProcJosEvent_Button(unsigned char*  josNum);
    void HK_procMouse_Button(unsigned char*  MouseNum);
    void HK_JosToSpeedX(int X);
    void HK_JosToSpeedY(int Y);
	void HK_JosToMouse(unsigned char x, unsigned char y);
	void HK_JosMap();
	bool Cur_pressBut;


	OSA_ThrHndl m_thrJoy;
	static void *josEventFunc(void *context)
	{
		CJoystick *User = (CJoystick*)context;
		while(JosStart){
		User->JoystickProcess();
		}
		return  NULL;
	}

	OSA_ThrHndl HK_thrJoy;
	static void *HK_josEventFunc(void *context)
	{
		CJoystick *User = (CJoystick*)context;
		while(HKJosStart){
		User->HKJoystickProcess();
		}
		return  NULL;
	}

	OSA_ThrHndl HK_thrJoyMap;
	static void *HK_josMapEventFunc(void *context)
	{
		CJoystick *User = (CJoystick*)context;
		struct timeval thrJosMap;
		while(HKJosStart){
			thrJosMap.tv_sec = 0;
			thrJosMap.tv_usec = 10000;
			select(0, NULL, NULL, NULL, &thrJosMap);
			User->HK_JosMap();

		}
		return  NULL;
	}

};


#endif /* JOSSTICK_H_ */
