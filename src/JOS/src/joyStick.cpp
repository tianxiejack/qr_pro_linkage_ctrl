#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "joyStick.h"
#include "app_status.h"

const int Jos_type = Jos_usb;
CGlobalDate* CJoystick::_GlobalDate = 0;
CStatusManager* CJoystick::_StatusManager = 0;

bool CJoystick::JosStart = true;
bool CJoystick::HKJosStart = true;
unsigned char jos_date[64] = {};


int controlBallComfortable(int zoom , int speed )
{
	int ret = speed;
	if( speed != 0)
	{
		if(zoom > 55000)
		{
			if(speed > 0)
			{
				if(speed <=25)
					ret = 1;
				else if(speed > 25 && speed < 45)
					ret = 2;
				else if(speed > 45)
					ret = 3;
			}
			else
			{
				if(speed >= -25)
					ret = -1;
				else if(speed < -25 && speed > -45)
					ret = -2;
				if(speed < -45)
					ret = -3;
			}
		}
		else if(zoom > 40000)
		{
			if(speed > 0)
			{
				if(speed <=25)
					ret = 1;
				else if(speed > 25 && speed < 45)
					ret = 3;
				else if(speed > 45)
					ret = 4;
			}
			else
			{
				if(speed >= -25)
					ret = -1;
				else if(speed < -25 && speed > -45)
					ret = -3;
				if(speed < -45)
					ret = -4;
			}
		}
		else if(zoom > 28000)
		{
			if(speed > 0)
			{
				if(speed <=25)
					ret = 1;
				else if(speed > 25 && speed < 45)
					ret = 3;
				else if(speed > 45)
					ret = 5;
			}
			else
			{
				if(speed >= -25)
					ret = -1;
				else if(speed < -25 && speed > -45)
					ret = -3;
				if(speed < -45)
					ret = -5;
			}
		}
	}
	else
		ret = 0;

	return ret;
}

CJoystick::CJoystick()
{
	jse = NULL;
	joystick_fd = -1;
	Cur_pressBut = false;
	_StatusManager = CStatusManager::Instance();
	_GlobalDate = CGlobalDate::Instance();
	_Message = CMessage::getInstance();
	memset(&_GlobalDate->jos_params, 0, sizeof(_GlobalDate->jos_params));
	_GlobalDate->jos_params.ctrlMode = mouse;
}

CJoystick::~CJoystick()
{
	Destroy();
	delete _GlobalDate;
	//delete _Message;
}

int  CJoystick::Create()
{
	if(Jos_type == Jos_usb)
	{
		int rv;
		user_device.idProduct = USB_PRODUCT_ID;
		user_device.idVendor =  USB_VENDOR_ID ;
		user_device.bInterfaceClass = LIBUSB_CLASS_HID ;
		user_device.bInterfaceSubClass = LIBUSB_CLASS_HID ;
		user_device.bmAttributes = LIBUSB_TRANSFER_TYPE_INTERRUPT ;
		user_device.dev = NULL;
		init_libusb();

		rv = get_device_descriptor(&dev_desc,&user_device);
		if(rv < 0) {
			printf("*** get_device_descriptor failed! \n");
			return -1;
		}
		printf("get_device_descriptor  rv = %d \n",rv);
		rv = get_device_endpoint(&dev_desc,&user_device);
		if(rv < 0) {
			printf("*** get_device_endpoint failed! rv:%d \n",rv);
			return -1;
		}
		printf("get_device_endpoint  rv = %d \n",rv);
		g_usb_handle = libusb_open_device_with_vid_pid(NULL, user_device.idVendor, user_device.idProduct);
		if(g_usb_handle == NULL) {
			printf("*** Permission denied or Can not find the USB board (Maybe the USB driver has not been installed correctly), quit!\n");
			return -1;
		}
			rv = libusb_claim_interface(g_usb_handle,user_device.bInterfaceNumber);
			if(rv < 0) {
				rv = libusb_detach_kernel_driver(g_usb_handle,user_device.bInterfaceNumber);
				if(rv < 0) {
					printf("*** libusb_detach_kernel_driver failed! rv%d\n",rv);
					return -1;
				}
				rv = libusb_claim_interface(g_usb_handle,user_device.bInterfaceNumber);
				if(rv < 0)
				{
					printf("*** libusb_claim_interface failed! rv%d\n",rv);
					return -1;
				}
			}

		USB_run();
	}
	else if(Jos_type == Jos_dev)
	{
		 open_joystick();
		  if (joystick_fd < 0)
			return -1;
		 jse = new joy_event;
		 Run();
	}
	 return 0;
}

int  CJoystick::Destroy()
{
	if(Jos_type == Jos_usb)
	{
		HKJosStart = false;
		OSA_thrDelete(&HK_thrJoy);
		libusb_close(g_usb_handle);
		libusb_release_interface(g_usb_handle,user_device.bInterfaceNumber);
		libusb_free_device_list(user_device.devs, 1);
		libusb_exit(NULL);
	}
	else if(Jos_type == Jos_dev){
		JosStart = false;
		OSA_thrDelete(&m_thrJoy);
		Stop();
	}

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

int CJoystick::init_libusb(void)
{
	/*1. init libusb lib*/
	int rv = 0;

	rv = libusb_init(NULL);
	if(rv < 0) {
		printf("*** initial USB lib failed! \n");
		return -1;
	}
	return rv;
}


int CJoystick::get_device_descriptor(struct libusb_device_descriptor *dev_desc,struct userDevice *user_device)
{
	/*2.get device descriptor*/
	int rv = -2;
	ssize_t cnt;
	int i = 0;

	libusb_device **devs;
	libusb_device *dev;

	cnt = libusb_get_device_list(NULL, &devs); //check the device number
	if (cnt < 0)
		return (int) cnt;

	while ((dev = devs[i++]) != NULL) {
		rv = libusb_get_device_descriptor(dev,dev_desc);
		if(rv < 0) {
			printf("*** libusb_get_device_descriptor failed! i:%d \n",i);
			return -1;
		}
		if(dev_desc->idProduct==user_device->idProduct &&dev_desc->idVendor==user_device->idVendor)
		{
			user_device->dev = dev;
			user_device->devs = devs;
			rv = 0;
			break;
		}
	}
	return rv;
}

int CJoystick::match_with_endpoint(const struct libusb_interface_descriptor * interface, struct userDevice *user_device)
{
	int i;
	int ret=0;
	for(i=0;i<interface->bNumEndpoints;i++)
	{
		if((interface->endpoint[i].bmAttributes&0x03)==user_device->bmAttributes)   //transfer type :bulk ,control, interrupt
		{
				if(interface->endpoint[i].bEndpointAddress&0x80)					//out endpoint & in endpoint
				{
					ret|=1;
					user_device->bInEndpointAddress = interface->endpoint[i].bEndpointAddress;
				}
				else
				{
					ret|=2;
					user_device->bOutEndpointAddress = interface->endpoint[i].bEndpointAddress;
				}
		}

	}
	if(ret==3)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int CJoystick::get_device_endpoint(struct libusb_device_descriptor *dev_desc,struct userDevice *user_device)
{
	/*3.get device endpoint that you need */
	int rv = -2;
	int i,j,k;
	struct libusb_config_descriptor *conf_desc;
	u_int8_t isFind = 0;
	for (i=0; i< dev_desc->bNumConfigurations; i++)
	{
		if(user_device->dev != NULL)
			rv = libusb_get_config_descriptor(user_device->dev,i,&conf_desc);
		if(rv < 0) {
			printf("*** libusb_get_config_descriptor failed! \n");
			return -1;
		}
		for (j=0; j< conf_desc->bNumInterfaces; j++)
		{
			for (k=0; k < conf_desc->interface[j].num_altsetting; k++)
			{
				if( conf_desc->interface[j].altsetting[k].bInterfaceClass==user_device->bInterfaceClass )
				{
					if(match_with_endpoint(&(conf_desc->interface[j].altsetting[k] ), user_device ))
					{
						user_device->bInterfaceNumber = conf_desc->interface[j].altsetting[k].bInterfaceNumber;
						libusb_free_config_descriptor(conf_desc);
						rv = 0;
						return rv;
					}
				}
			}
		}
	}
	return -2;  //don't find user device
}

int CJoystick::USB_run()
{
	int iRet;
	iRet = OSA_thrCreate(&HK_thrJoy, HK_josEventFunc , 0, 0, (void*)this);
	OSA_thrCreate(&HK_thrJoyMap, HK_josMapEventFunc , 0, 0, (void*)this);
		if(iRet != 0)
	printf(" [joystick_USB] jos thread create failed\n");
	return 0;
}

int CJoystick::HKJoystickProcess()
{
	int rv;
	int length;
	rv =  libusb_interrupt_transfer(g_usb_handle,user_device.bInEndpointAddress,jos_date,8,&length,10000000);
	if(rv < 0) {
		printf("*** bulk_transfer failed!   rv = %d \n",rv);
		return -1;
	}
	else
	{
		if(jos_date[usb_1_8] || jos_date[usb_enter] || jos_date[usb_special])
			Cur_pressBut = true;
		else if(jos_date[usb_X] || jos_date[usb_Y] || jos_date[usb_Z])
			Cur_pressBut = false;

    	if(_GlobalDate->jos_params.ctrlMode == mouse)
    	{
    			HK_procMouse_Axis(jos_date);
    			HK_procMouse_Button(jos_date);
    			HK_ProcJosEvent_Button(jos_date);
    			if(_GlobalDate->jos_params.menu && !Cur_pressBut)
    				HK_procJosEvent_Axis(jos_date);
    	}
    	else
    	{
    		if(!Cur_pressBut)
    			HK_procJosEvent_Axis(jos_date);
    		else
    			HK_ProcJosEvent_Button(jos_date);
    	}
	}
}

void CJoystick::HK_procJosEvent_Axis(unsigned char*  josNum)
{
	static int  dirLimit;
	if(_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_IrisAndFocusAndExit) == 0 && !_GlobalDate->jos_params.menu)
	{
		if(Xinit)
			josNum[usb_X] = 0;
		Xinit = false;
		HK_JosToSpeedX(josNum[usb_X]);

		if(Yinit)
			josNum[usb_Y] = 0;
		Yinit = false;
		HK_JosToSpeedY(josNum[usb_Y]);
	}
	else if(_GlobalDate->jos_params.menu && !_GlobalDate->gridMap)
	{
		_GlobalDate->jos_params.type = jos_Dir;
		if(josNum[usb_Y] > 0x77)
		{
			if(!dirLimit)
				_GlobalDate->jos_params.jos_Dir = cursor_up;
			else
				_GlobalDate->jos_params.jos_Dir = 0;
			dirLimit = true;
			josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		}
		else if(josNum[usb_Y] < 0x89 && josNum[usb_Y] > 0)
		{
			if(!dirLimit)
				_GlobalDate->jos_params.jos_Dir = cursor_down;
			else
				_GlobalDate->jos_params.jos_Dir = 0;
			dirLimit = true;
			josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		}
		else if( josNum[usb_Y]  == 0)
		{
			dirLimit = false;
			_GlobalDate->jos_params.jos_Dir = 0;
		}
	}
	else if(_GlobalDate->gridMap)
	{
		if(Xinit)
			josNum[usb_X] = 0;
		Xinit = false;
		HK_JosToSpeedX(josNum[usb_X]);

		if(Yinit)
			josNum[usb_Y] = 0;
		Yinit = false;
		HK_JosToSpeedY(josNum[usb_Y]);
#if 0
		if(_GlobalDate->gridMap)
			_GlobalDate->jos_params.ctrlMode = jos;
		else
			_GlobalDate->jos_params.ctrlMode = mouse;
#endif
	}
	/***********/
	static int zoom = 0;
	if(josNum[usb_Z] == 0)
	{
		if(zoom == 1)
		{
			_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 0;
			josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
		}
		else if(zoom == 2)
		{
			_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 0;
			josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
		}
		zoom = 0;
	}
	else if(josNum[usb_Z] <= 0x77 && josNum[usb_Z] > 0x11)
	{
		_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 1;
		josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
		zoom = 1;
	}
	else if(josNum[usb_Z] >= 0x89 && josNum[usb_Z] < 0xef )
	{
		_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 1;
		josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
		zoom = 2;
	}

}

void CJoystick::HK_procMouse_Axis(unsigned char*  MouseNum)
{
	/***********/
	static int zoom = 0;
	if(MouseNum[usb_Z] == 0)
	{
		if(zoom == 1)
		{
			_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 0;
			josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
		}
		else if(zoom == 2)
		{
			_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 0;
			josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
		}
		zoom = 0;

	}
	else if(MouseNum[usb_Z] <= 0x77 && MouseNum[usb_Z] > 0x11)
	{
		_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomShort ) = 1;
		josSendMsg(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL);
		zoom = 1;
	}
	else if(MouseNum[usb_Z] >= 0x89 && MouseNum[usb_Z] < 0xef )
	{
		_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_ZoomLong ) = 1;
		josSendMsg(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL);
		zoom = 2;
	}

}

void CJoystick::HK_ProcJosEvent_Button(unsigned char*  josNum)
{

	switch(josNum[usb_1_8])
	{
		case 0x01:
				_GlobalDate->jos_params.jos_button = 1;
				break;
		case 0x02:
				_GlobalDate->jos_params.jos_button = 2;
				break;
		case 0x04:
				_GlobalDate->jos_params.jos_button = 3;
				break;
		case 0x08:
				_GlobalDate->jos_params.jos_button = 4;
			break;
		case 0x10:
				_GlobalDate->jos_params.jos_button = 5;
			break;
		case 0x20:
				_GlobalDate->jos_params.jos_button = 6;
			break;
		case 0x40:
				_GlobalDate->jos_params.jos_button = 7;
			break;

		case 0x80:
				_GlobalDate->jos_params.jos_button = 8;
			break;
		case 0:
			break;
	}

	switch(josNum[usb_special])
	{
	case 0x01:
		_GlobalDate->jos_params.type = jos_button;
		_GlobalDate->jos_params.jos_button = 9;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		break;

	case 0x02:
		_GlobalDate->jos_params.type = jos_button;
		_GlobalDate->jos_params.jos_button = 0;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		break;

	case 0x10:
		if(_GlobalDate->calibration)
		{
			_GlobalDate->workMode = (_GlobalDate->workMode + 1)%3;

			switch(_GlobalDate->workMode)
			{
			case 0:
				_GlobalDate->workMode = manual_linkage;
				_GlobalDate->jos_params.ctrlMode = mouse;
				break;
			case 1:
				_GlobalDate->workMode = Auto_linkage;
				_GlobalDate->jos_params.ctrlMode = jos;
				break;
			case 2:
				_GlobalDate->workMode = ballctrl;
				if(_GlobalDate->jos_params.menu == true)
					_GlobalDate->jos_params.ctrlMode = mouse;
				else
					_GlobalDate->jos_params.ctrlMode = jos;
				break;
			}
			josSendMsg(MSGID_EXT_INPUT_workModeSwitch);

			struct timeval tmp;
			tmp.tv_sec = 0;
			tmp.tv_usec = 500000;
			select(0, NULL, NULL, NULL, &tmp);

			_GlobalDate->jos_params.type = ctrlMode;
		}
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		break;

	case 0x20:
		_GlobalDate->jos_params.type = jos_menu;
		if(!_GlobalDate->jos_params.menu)
		{
			_GlobalDate->jos_params.menu = true;
			if(_GlobalDate->workMode == ballctrl)
				_GlobalDate->jos_params.ctrlMode = mouse;
		}
		else
		{
			_GlobalDate->jos_params.menu = false;
			if(_GlobalDate->workMode == ballctrl)
				_GlobalDate->jos_params.ctrlMode = jos;
		}
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		break;
#if 0
	case 0x40:
		_GlobalDate->jos_params.type = ctrlMode;
		if(_GlobalDate->jos_params.ctrlMode == mouse)
			_GlobalDate->jos_params.ctrlMode = jos;
		else if((_GlobalDate->jos_params.ctrlMode == jos))
			_GlobalDate->jos_params.ctrlMode = mouse;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
		break;
#endif
	case 0:
		//_GlobalDate->jos_params.type = 0;
		break;

	default:
		break;
	}
	if(josNum[usb_1_8])
	{
		_GlobalDate->jos_params.type = jos_button;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
	}

	if(josNum[usb_enter] == 0x01)
	{
		_GlobalDate->jos_params.type = enter;
		_GlobalDate->jos_params.enter = true;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
	}

}

void CJoystick::HK_procMouse_Button(unsigned char*  MouseNum)
{
	static bool leftBut, rightBut;
		switch(MouseNum[usb_special])
		{
		case 0x04:
			if(leftBut)
			{
				_GlobalDate->jos_params.type = mouse_button;
				_GlobalDate->jos_params.mouse_button = 3;
				_GlobalDate->jos_params.mouse_state = false;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
				leftBut = false;
				//printf("左键抬起 \n");
			}
			else
			{
				_GlobalDate->jos_params.type = mouse_button;
				_GlobalDate->jos_params.mouse_button = 3;
				_GlobalDate->jos_params.mouse_state = true;
				leftBut = true;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
				//printf("左键按下 \n");
			}
			break;

		case 0x08:
			if(rightBut)
			{
				_GlobalDate->jos_params.type = mouse_button;
				_GlobalDate->jos_params.mouse_button = 4;
				_GlobalDate->jos_params.mouse_state = false;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
				rightBut = false;
				//printf("右键抬起 \n");
			}
			else
			{
				_GlobalDate->jos_params.type = mouse_button;
				_GlobalDate->jos_params.mouse_button = 4;
				_GlobalDate->jos_params.mouse_state = true;
				rightBut = true;
				josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
				//printf("右键按下 \n");
			}
			break;
#if 0
		case 0x40:
			_GlobalDate->jos_params.type = ctrlMode;
			if(_GlobalDate->jos_params.ctrlMode == mouse)
				_GlobalDate->jos_params.ctrlMode = jos;
			else if((_GlobalDate->jos_params.ctrlMode == jos))
				_GlobalDate->jos_params.ctrlMode = mouse;
			josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);
			break;
#endif
		case 0:

			break;

		default:
			break;
		}

}

void CJoystick::HK_JosToSpeedX(int X)
{
	int speed;
	switch(X)
	{
	case 0xef:
		speed = -10;
		break;

	case 0xde:
		speed = -15;
		break;

	case 0xcd:
		speed = -25;
		break;

	case 0xbc:
		speed = -35;
		break;

	case 0xab:
		speed = -45;
		break;

	case 0x9a:
		speed = -55;
		break;

	case 0x89:
		speed = -63;
		break;

	case 0x11:
		speed = 10;
		break;

	case 0x22:
		speed = 15;
		break;

	case 0x33:
		speed = 25;
		break;

	case 0x44:
		speed = 35;
		break;

	case 0x55:
		speed = 45;
		break;

	case 0x66:
		speed = 55;
		break;

	case 0x77:
		speed = 63;
		break;

	case 0x00:
		speed = 0;
		break;
	}
	speed = controlBallComfortable(_GlobalDate->rcv_zoomValue , speed );

	_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX) = speed;
	josSendMsg(MSGID_EXT_INPUT_PLATCTRL);

}

void CJoystick::HK_JosToSpeedY(int Y)
{
	int speed;
	switch(Y)
	{
	case 0xef:
		speed = -10;
		break;

	case 0xde:
		speed = -15;
		break;

	case 0xcd:
		speed = -25;
		break;

	case 0xbc:
		speed = -35;
		break;

	case 0xab:
		speed = -45;
		break;

	case 0x9a:
		speed = -55;
		break;

	case 0x89:
		speed = -63;
		break;

	case 0x11:
		speed = 10;
		break;

	case 0x22:
		speed = 15;
		break;

	case 0x33:
		speed = 25;
		break;

	case 0x44:
		speed = 35;
		break;

	case 0x55:
		speed = 45;
		break;

	case 0x66:
		speed = 55;
		break;

	case 0x77:
		speed = 63;
		break;

	case 0x00:
		speed = 0;
		break;
	}
	speed = controlBallComfortable(_GlobalDate->rcv_zoomValue , speed );

	_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = speed;
	josSendMsg(MSGID_EXT_INPUT_PLATCTRL);
}
int width = 1920, height = 1080;

void CJoystick::HK_JosToMouse(unsigned char x, unsigned char y)
{

	struct timeval tmp;
	int curX, curY;
	static int W = width/2;

	switch(x)
	{
	case 0xef:
	case 0x11:
		tmp.tv_sec = 0;
		tmp.tv_usec = 90000;
		break;

	case 0xde:
	case 0x22:
		tmp.tv_sec = 0;
		tmp.tv_usec = 60000;
		break;

	case 0xcd:
	case 0x33:
		tmp.tv_sec = 0;
		tmp.tv_usec = 50000;
		break;

	case 0xbc:
	case 0x44:
		tmp.tv_sec = 0;
		tmp.tv_usec = 40000;
		break;

	case 0xab:
	case 0x55:
		tmp.tv_sec = 0;
		tmp.tv_usec = 30000;
		break;

	case 0x9a:
	case 0x66:
		tmp.tv_sec = 0;
		tmp.tv_usec = 10000;
		break;

	case 0x89:
	case 0x77:
		tmp.tv_sec = 0;
		tmp.tv_usec = 10000;
		break;

	case 0x00:

		break;
	}


	if(x == 0xef)
		W -= 1;
	else if(x== 0x11)
		W += 1;
	else if(x == 0xde)
		W -= 2;
	else if(x == 0x22)
		W += 2;
	else if(x == 0xcd)
		W -= 4;
	else if(x == 0x33)
		W += 4;
	else if (x == 0xbc)
		W -= 5;
	else if(x == 0x44)
		W += 5;
	else if(x == 0xab)
		W -= 15;
	else if (x == 0x55)
		W += 15;
	else if(x == 0x9a)
		W -= 25;
	else if(x == 0x66)
		W += 25;
	else if(x == 0x89)
		W -= 45;
	else if(x == 0x77)
		W += 45;


		curX = W;
		if(curX > (width - 15) )
			W = curX = width - 15;
		else if(curX < 0)
			W = curX = 0;
		_GlobalDate->jos_params.cursor_x = curX;
		_GlobalDate->jos_params.type = cursor_move;


		switch(y)
		{
		case 0xef:
		case 0x11:
			tmp.tv_sec = 0;
			tmp.tv_usec = 90000;
			break;

		case 0xde:
		case 0x22:
			tmp.tv_sec = 0;
			tmp.tv_usec = 60000;
			break;

		case 0xcd:
		case 0x33:
			tmp.tv_sec = 0;
			tmp.tv_usec = 50000;
			break;

		case 0xbc:
		case 0x44:
			tmp.tv_sec = 0;
			tmp.tv_usec = 40000;
			break;

		case 0xab:
		case 0x55:
			tmp.tv_sec = 0;
			tmp.tv_usec = 30000;
			break;

		case 0x9a:
		case 0x66:
			tmp.tv_sec = 0;
			tmp.tv_usec = 10000;
			break;

		case 0x89:
		case 0x77:
			tmp.tv_sec = 0;
			tmp.tv_usec = 10000;
			break;

		case 0x00:

			break;
		}

		static int H = height/2;

		if(y == 0xef)
			H -= 1;
		else if(y == 0x11)
			H += 1;
		else if(y == 0xde)
			H -= 2;
		else if(y == 0x22)
			H += 2;
		else if(y == 0xcd)
			H -= 4;
		else if(y == 0x33)
			H += 4;
		else if(y == 0xbc)
			H -= 6;
		else if( y== 0x44)
			H += 6;
		else if(y == 0xab)
			H -= 12;
		else if(y == 0x55)
			H += 12;
		else if(y == 0x9a)
			H -= 20;
		else if(y == 0x66)
			H += 20;
		else if(y == 0x89)
			H -= 30;
		else if( y == 0x77)
			H += 30;

		curY = H;
		if(curY > (height - 20))
			H = curY = height - 20;
		else if(curY < 0)
			H = curY = 0;
		_GlobalDate->jos_params.cursor_y = curY;
		_GlobalDate->jos_params.type = cursor_move;
		josSendMsg(MSGID_IPC_INPUT_CTRLPARAMS);

		select(0, NULL, NULL, NULL, &tmp);

}

void CJoystick::HK_JosMap()
{
	if(_GlobalDate->jos_params.ctrlMode == mouse)
	{
		if(jos_date[usb_X] || jos_date[usb_Y])
			HK_JosToMouse(jos_date[usb_X], jos_date[usb_Y]);
	}
}

