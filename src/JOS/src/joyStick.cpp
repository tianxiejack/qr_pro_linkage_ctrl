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
	memset(&_GlobalDate->jos_params, 0, sizeof(_GlobalDate->jos_params));
	_GlobalDate->jos_params.ctrlMode = jos;
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
                	if(_GlobalDate->jos_params.ctrlMode == mouse)
                		procMouse_Axis(jse->number);
                	else
                		procJosEvent_Axis(jse->number);
                        break;
                case   JS_EVENT_BUTTON:
                	if(_GlobalDate->jos_params.ctrlMode == mouse)
                		procMouse_Button(jse->number);
                	else
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
	int zoom;
	static int  dirLimit;
	switch(mjosNum){
			case MSGID_INPUT_AXISX:
				if(_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit) == 0 && !_GlobalDate->jos_params.menu)
				{
					if(Xinit)
						 jse->value = 0;
					Xinit = false;
				_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX) = jse->value;
				josSendMsg(MSGID_EXT_INPUT_PLATCTRL);
				}
					break;
				case MSGID_INPUT_AXISY:
					if(_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit) == 0 && !_GlobalDate->jos_params.menu)
					{
						if(Yinit)
							 jse->value = 0;
						Yinit = false;
						_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = jse->value;
						josSendMsg(MSGID_EXT_INPUT_PLATCTRL);
					}
					else if(_GlobalDate->jos_params.menu)
					{
						_GlobalDate->jos_params.type = jos_Dir;
						if(jse->value < -10000)
						{
							if(!dirLimit)
								_GlobalDate->jos_params.jos_Dir = cursor_up;
							else
							_GlobalDate->jos_params.jos_Dir = 0;
							dirLimit = true;
						}
						else if(jse->value > 10000)
						{
							if(!dirLimit)
								_GlobalDate->jos_params.jos_Dir = cursor_down;
							else
							_GlobalDate->jos_params.jos_Dir = 0;
							dirLimit = true;
						}
						else if(jse->value == 0)
						{
							dirLimit = false;
							_GlobalDate->jos_params.type = 0;
							_GlobalDate->jos_params.jos_Dir = 0;
						}
						josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
					}
					else {
						Y_CtrlIrisAndFocus(jse->value);
						josSendMsg(MSGID_EXT_INPUT_IrisAndFocusAndExit);
					}
					break;
				case MSGID_INPUT__POVX:
					if(jse->value < 0)
						josSendMsg(MSGID_EXT_INPUT_workModeSwitch);
					else if(jse->value > 0)
					{
						_GlobalDate->jos_params.type = ctrlMode;
						if(_GlobalDate->jos_params.ctrlMode == mouse)
							_GlobalDate->jos_params.ctrlMode = jos;
						else if(_GlobalDate->jos_params.ctrlMode == jos)
							_GlobalDate->jos_params.ctrlMode = mouse;
						josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
						printf("_GlobalDate->jos_params.ctrlMode = %d \n", _GlobalDate->jos_params.ctrlMode);
					}
					else
						_GlobalDate->jos_params.type = 0;
					break;

				case MSGID_INPUT__POVY:
					if(jse->value == 0)
					{
						if(zoom)
						{
						_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 0;
						josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
						}
						else
						{
						_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 0;
						josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
						}

					}
					else if(jse->value < 0)
		    		{
						_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 1;
						josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
						zoom = 1;
		    		}
					else if(jse->value > 0)
					{
	    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 1;
	    				josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
	    				zoom = 0;
					}

					break;
				default:
					break;
	 }

}

void CJoystick::procMouse_Axis(UINT8  mjosNum)
{
	int zoom;
	switch(mjosNum){
				case MSGID_INPUT_AXISX:
					_GlobalDate->jos_params.type = cursor_move;
					_GlobalDate->jos_params.cursor_x = JosToWinX(jse->value, video_gaoqing);
					josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
					if(!jse->value)
						_GlobalDate->jos_params.type = 0;
					break;
				case MSGID_INPUT_AXISY:
					_GlobalDate->jos_params.type = cursor_move;
					_GlobalDate->jos_params.cursor_y = JosToWinY(jse->value, video_gaoqing);
						josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
						if(!jse->value)
							_GlobalDate->jos_params.type = 0;
					break;
				case MSGID_INPUT__POVX:
					if(jse->value < 0)
						josSendMsg(MSGID_EXT_INPUT_workModeSwitch);
					else if(jse->value > 0)
					{
						_GlobalDate->jos_params.type = ctrlMode;
						if(_GlobalDate->jos_params.ctrlMode == mouse)
							_GlobalDate->jos_params.ctrlMode = jos;
						else if((_GlobalDate->jos_params.ctrlMode == jos))
							_GlobalDate->jos_params.ctrlMode = mouse;
						josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
					}
					else
						_GlobalDate->jos_params.type = 0;
					break;

				case MSGID_INPUT__POVY:
					if(jse->value == 0)
					{
						if(zoom)
						{
							_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 0;
							josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
						}
						else
						{
							_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 0;
							josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
						}

					}
					else if(jse->value< 0)
		    		{
						_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 1;
						josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
						zoom = 1;
		    		}
					else if(jse->value > 0)
					{
	    				_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 1;
	    				josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
	    				zoom = 0;
					}

					break;
				default:
					break;
	 }

}

void CJoystick::ProcJosEvent_Button(UINT8  njosNum)
{
	_GlobalDate->jos_params.type = jos_button;
	switch (njosNum){
    		case 0x00:
    				if(jse->value == 1){
    					_GlobalDate->jos_params.jos_button = 1;
    					josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    				}
    				break;
    		case 0x01:
    			if(jse->value == 1){
    				_GlobalDate->jos_params.jos_button = 2;
    				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    				break;
    		case 0x02:
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.jos_button = 3;
    				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    				break;
    		case 0x03:
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.jos_button = 4;
					josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    			break;
    		case 0x04:
    			if(jse->value == 1){
    				_GlobalDate->jos_params.jos_button = 5;
    				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    			break;
    		case 0x05:
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.jos_button = 6;
    				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    			break;
    		case 0x06:
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.jos_button = 7;
					josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    			break;

    		case 0x07:
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.jos_button = 8;
    				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			}
    			break;

		case MSGID_INPUT_9:
			if(jse->value == 1){
				_GlobalDate->jos_params.jos_button = 9;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
			}
			break;

		case MSGID_INPUT_10:
			if(jse->value == 1){
				_GlobalDate->jos_params.jos_button = 0;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
			}
			break;
		case 10:
			if(jse->value == 1)
			{
				_GlobalDate->jos_params.type = enter;
				_GlobalDate->jos_params.enter = true;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
			}
			else
				_GlobalDate->jos_params.type = 0;
			break;
		case MSGID_INPUT_Menu:
			if(jse->value == 1)
			{
				_GlobalDate->jos_params.type = jos_menu;
				if(!_GlobalDate->jos_params.menu)
					_GlobalDate->jos_params.menu = true;
				else
					_GlobalDate->jos_params.menu = false;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
			}
			else
				_GlobalDate->jos_params.type = 0;
			break;
		default:
			break;
    }
}

void CJoystick::procMouse_Button(UINT8  njosNum)
{
	switch (njosNum) {
    		case 0x00:
				if(jse->value == 1){
				}
				break;
    		case 0x01:
    			if(jse->value == 1){
    			}
    				break;
    		case 0x02:
    			_GlobalDate->jos_params.type = mouse_button;
    			if(jse->value == 1)
    			{
    				_GlobalDate->jos_params.mouse_button = 3;
    				_GlobalDate->jos_params.mouse_state = true;
    			}
    			else{
    				_GlobalDate->jos_params.mouse_button = 3;
    				_GlobalDate->jos_params.mouse_state = false;
    			}
    			josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    				break;
    		case 0x03:
    			_GlobalDate->jos_params.type = mouse_button;
    			if(jse->value == 1)
    			{
    			_GlobalDate->jos_params.mouse_button = 4;
    			_GlobalDate->jos_params.mouse_state = true;
    			}
    			else{
    				_GlobalDate->jos_params.mouse_button = 4;
    				_GlobalDate->jos_params.mouse_state = false;
    			}
    			josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
    			break;
    		case 0x04:
    			if(jse->value == 1){

    			}
    			break;
    		case 0x05:
    			if(jse->value == 1){

    			}
    			break;
    		case 0x06:

    			break;

    		case 0x07:
    			if(jse->value == 1)
    			{

    			}
    			break;

		case MSGID_INPUT_9:
			if(jse->value == 1){

			}
			break;

		case MSGID_INPUT_10:
			if(jse->value == 1){

			}
			break;
		case MSGID_INPUT_Menu:
			if(jse->value == 1){

			}
			break;
		default:
			break;
    }
}

int CJoystick::JosToWinX(int x, int sensor)
{
	int m_WinX;
	if(video_pal == sensor)
		m_WinX = x/91 + ShowDPI[sensor][0]/2;
	else if((video_gaoqing0 == sensor)||(video_gaoqing == sensor)||(video_gaoqing2 == sensor)||(video_gaoqing3 == sensor))
	{
		m_WinX = x/33 + ShowDPI[sensor][0]/2;
		if(m_WinX > ShowDPI[sensor][0] - 10)
			m_WinX = ShowDPI[sensor][0] - 10;
		else if(m_WinX < 0)
			m_WinX = 0;
	}
	return m_WinX;
}

int CJoystick::JosToWinY(int y, int sensor)
{
	int m_WinY;
	if(video_pal == sensor)
		m_WinY = y/113 + ShowDPI[sensor][1]/2;
	else if((video_gaoqing0 == sensor)||(video_gaoqing == sensor)||(video_gaoqing2 == sensor)||(video_gaoqing3 == sensor))
	{
		m_WinY = y/59 + ShowDPI[sensor][1]/2;
		if(m_WinY > ShowDPI[sensor][1] - 10)
			m_WinY = ShowDPI[sensor][1] - 10;
		else if(m_WinY < 0)
			m_WinY =0;
	}
	return m_WinY;
}

int CJoystick::JosToMouseX(int x, int sensor)
{
	static int MouseX, x_bak;
	if(abs(x) > abs(x_bak))
	{
		MouseX = x/34 + ShowDPI[sensor][0]/2;
		x_bak = x;
	}
	if(MouseX > 1920)
		MouseX = 1920;
	else if(MouseX < 0)
		MouseX = 0;
	return MouseX;
}
int CJoystick::JosToMouseY(int y, int sensor)
{
	static int MouseY, y_bak;
	if(abs(y) < abs(y_bak))
	{
		MouseY = y_bak/60 + ShowDPI[sensor][1]/2;
		y_bak = y;
	}
	if(MouseY > 1080)
		MouseY = 1080;
	else if(MouseY < 0)
		MouseY = 0;
	return MouseY;
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

