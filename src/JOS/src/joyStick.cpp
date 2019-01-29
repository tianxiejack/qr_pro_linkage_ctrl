#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "joyStick.h"
#include "app_status.h"

CGlobalDate* CJoystick::_GlobalDate = 0;
CStatusManager* CJoystick::_StatusManager = 0;

bool CJoystick::JosStart = true;

CJoystick::CJoystick()
{
	jse = NULL;
	joystick_fd = -1;
	_StatusManager = CStatusManager::Instance();
	_GlobalDate = CGlobalDate::Instance();
	_Message = CMessage::getInstance();
}

CJoystick::~CJoystick()
{
	Destroy();
	delete _GlobalDate;
	//delete _Message;
}

int  CJoystick::Create()
{
	 open_joystick();
	  if (joystick_fd < 0)
	    return -1;
	 jse = new joy_event;
	 Run();
	 return 0;
}

int  CJoystick::Destroy()
{
	JosStart = false;
	OSA_thrDelete(&m_thrJoy);
	Stop();
	return 0;
}

int  CJoystick::Run()
{
	int iRet;
	iRet = OSA_thrCreate(&m_thrJoy, josEventFunc , 0, 0, (void*)this);
		if(iRet != 0)
	printf(" [joystick] jos thread create failed\n");
	return 0;
}

int  CJoystick::Stop()
{
    close(joystick_fd);
    return 0;
}

int CJoystick::open_joystick()
{
	 joystick_fd = open(joystick_Dev, O_RDONLY);

    if (joystick_fd < 0)
    {
    	fprintf(stdout," [joystick] jos_dev open failed\n");
        return joystick_fd;
    }
		return joystick_fd;
}

int CJoystick::read_joystick_event(joy_event *jse)
{
   int  bytes = read(joystick_fd, jse, sizeof(*jse));

    if (bytes == -1)
        return 0;
    else if (bytes == sizeof(*jse))
        return 1;
}

void CJoystick::JoystickProcess()
{
    int rc;
    if (rc = read_joystick_event(jse) == 1) {
        if((jse->type & JS_EVENT_INIT) != JS_EVENT_INIT)
        {
            jse->type &= ~JS_EVENT_INIT; /* ignore synthetic events */
			if(_GlobalDate->joystick_flag == 0)
			{
				return;
			}
            switch(jse->type){
                case   JS_EVENT_AXIS:
                        procJosEvent_Axis(jse->number);
                        break;
                case   JS_EVENT_BUTTON:
                        ProcJosEvent_Button(jse->number);
                        break;
                default:
                    printf("INFO: ERROR Jos Event, Can not excute here!!!\r\n");
                    break;
            }
        }
    }
}
bool Xinit = true;
bool Yinit = true;
void CJoystick::procJosEvent_Axis(UINT8  mjosNum)
{
	switch(mjosNum){
			case MSGID_INPUT_AXISX:
				if(_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit) == 0)
				{
					if(Xinit)
						 jse->value = 0;
					Xinit = false;
				_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX) = jse->value;
				josSendMsg(MSGID_EXT_INPUT_PLATCTRL);
				}
					break;
				case MSGID_INPUT_AXISY:
					if(_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit) == 0)
					{
						if(Yinit)
							 jse->value = 0;
						Yinit = false;
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = jse->value;
						josSendMsg(MSGID_EXT_INPUT_PLATCTRL);
					}
					else {
						Y_CtrlIrisAndFocus(jse->value);
						josSendMsg(MSGID_EXT_INPUT_IrisAndFocusAndExit);
					}
					break;
				case MSGID_INPUT__POVX:
					if(jse->value == 0)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_X) = 0;
					else if(jse->value == -32767)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_X) = 1;
					else if(jse->value == 32767)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_X) = 2;

						josSendMsg(MSGID_EXT_INPUT_AIMPOSXCTRL);
					break;

				case MSGID_INPUT__POVY:
					if(jse->value == 0)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_Y) = 0;
					else if(jse->value == -32767)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_Y) = 1;
					else if(jse->value == 32767)
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AIMPOS_Y) = 2;

						josSendMsg(MSGID_EXT_INPUT_AIMPOSYCTRL);

					break;
				default:
					break;
	 }

}

void CJoystick::ProcJosEvent_Button(UINT8  njosNum)
{
	switch (njosNum) {
    		case MSGID_INPUT_TrkCtrl:
    				if(jse->value == 1){
    					//_GlobalDate->Mtd_ExitMode = Manual;
    					josSendMsg(MSGID_EXT_INPUT_TRACKCTRL);
    				}
    				break;
    		case MSGID_INPUT_SensorCtrl:
    			if(jse->value == 1){
    				josSendMsg(MSGID_EXT_INPUT_SwitchSensor);
    			}
    				break;
    		case MSGID_INPUT_ZoomLong:
    				if(jse->value == 1)
    					_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 1;
    				else
    					_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 0;
    				josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
    				break;
    		case MSGID_INPUT_ZoomShort:
    			if(jse->value == 1)
    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 1;
    			else
    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 0;
    				josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
    			break;
    		case MSGID_INPUT_IrisAndFocusAndExit:
    			if(jse->value == 1){
    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit)  =  (_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit)  + 1) % 3;
    				josSendMsg(MSGID_EXT_INPUT_IrisAndFocusAndExit);
    			}
    			break;
    		case MSGID_INPUT_TrkSearch:
    			if(jse->value == 1)
    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_TrkSearch ) = 1;
    			else
    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_TrkSearch ) = 0;
    				josSendMsg(MSGID_EXT_INPUT_TRACKSEARCHCTRL);
    			break;
    		case MSGID_INPUT_FuncMenu:

    			break;

    		case MSGID_INPUT_AxisCtrl:
    			if(jse->value == 1)
    			{

    			}
    			break;

		case MSGID_INPUT_9:
			if(jse->value == 1){
				//josSendMsg(test_ptz_left);
				josSendMsg(MSGID_EXT_INPUT_MtdPreset);
			}
			break;

		case MSGID_INPUT_10:
			if(jse->value == 1){
				josSendMsg(MSGID_EXT_INPUT_CallPreset);
			}
			break;

		case MSGID_INPUT_Menu:
			if(jse->value == 1)
			{
				//josSendMsg(test_ptz_stop);
			}
			break;
		default:
			break;
    }
}

int CJoystick::JosToWinX(int x, int sensor)
{
	int m_WinX;
	if(video_pal == sensor){
		m_WinX = x/91 + ShowDPI[sensor][0]/2;
		return m_WinX;
	}else if((video_gaoqing0 == sensor)||(video_gaoqing == sensor)||(video_gaoqing2 == sensor)||(video_gaoqing3 == sensor)){
		m_WinX = x/34 + ShowDPI[sensor][0]/2;
		return m_WinX;
	}
}

int CJoystick::JosToWinY(int y, int sensor)
{
	int m_WinY;
	if(video_pal == sensor){
		m_WinY = y/113 + ShowDPI[sensor][1]/2;
		return m_WinY;
	}else if((video_gaoqing0 == sensor)||(video_gaoqing == sensor)||(video_gaoqing2 == sensor)||(video_gaoqing3 == sensor)){
		m_WinY = y/60 + ShowDPI[sensor][1]/2;
		return m_WinY;
	}
}

void CJoystick::Y_CtrlIrisAndFocus(int value)
{
	if(value < -10000)
		_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = -1;
	else if(value > 15000)
		_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = 1;
	else
		_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = 0;
}

void CJoystick::josSendMsg(int MsgId)
{
	_Message->MSGDRIV_send(MsgId, 0);
}

