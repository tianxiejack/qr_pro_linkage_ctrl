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


const char  JS_EVENT_BUTTON = 0x01;
const char  JS_EVENT_AXIS = 0x02;
const char  JS_EVENT_INIT = 0x80;

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

class CJoystick : public CJosInterface {

public:
	CJoystick();
	~CJoystick();

	int Create();
	int  Destroy();
	int JosToWinX(int x, int sensor);
	int JosToWinY(int y, int sensor);

private:
	static CStatusManager* _StatusManager;
	static CGlobalDate* _GlobalDate;
	CMessage* _Message;
	joy_event *jse;
	static bool JosStart;
	int joystick_fd;
	const char*  joystick_Dev = "/dev/input/js0";

	int  Run();
	int  Stop();
	int open_joystick();
	int read_joystick_event(joy_event *jse);
	void JoystickProcess();
	void procJosEvent_Axis(UINT8  mjosNum );
	void procMouse_Axis(UINT8 mjosNum);
    void ProcJosEvent_Button(UINT8 njosNum);
    void procMouse_Button(UINT8 njosNum);
    void Y_CtrlIrisAndFocus(int value);

    void josSendMsg(int MsgId);

	OSA_ThrHndl m_thrJoy;
	static void *josEventFunc(void *context)
	{
		CJoystick *User = (CJoystick*)context;
		while(JosStart){
		User->JoystickProcess();
		}
		return  NULL;
	}

};


#endif /* JOSSTICK_H_ */
