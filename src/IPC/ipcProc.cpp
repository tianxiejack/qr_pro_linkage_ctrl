#include "ipcProc.h"
#include "msg_id.h"
#include <sys/time.h>
#include <assert.h>
#include "CMessage.hpp"
#include <stdint.h>

CIPCProc* CIPCProc::pThis = 0;

CGlobalDate* CIPCProc::_GlobalDate = 0;

CIPCProc::CIPCProc()
{
	pthread_mutex_init(&mutex,NULL);
	CMD_AUTOCHECK fr_img_cmd_autocheck = {0,0,0,0};
	CMD_SENSOR fr_img_cmd_sensor={1};
	CMD_PinP fr_img_cmd_pinp = {0,0,0};
	CMD_TRK fr_img_cmd_trk = {0,0};
	CMD_SECTRK fr_img_cmd_sectrk = {0};
	CMD_ENH fr_img_cmd_enh = {0};
	CMD_MTD fr_img_cmd_mtd = {0};
	CMD_MMT fr_img_cmd_mmt = {0};
	CMD_MMTSELECT fr_img_cmd_mmtsel = {0};
	CMD_TRKDOOR fr_img_cmd_trkdoor = {1};
	CMD_SYNC422 fr_img_cmd_sync422 = {0};
	CMD_ZOOM fr_img_cmd_zoom = {0};
	SENDST fr_img_test = {0};	trackstatus = 0;
	trackposx=0.f;
	trackposy=0.f;
	exitThreadIPCRcv = false;
	exitGetShamDatainThrd = false;
	losttime = 3000;
}
CIPCProc::~CIPCProc()
{
	Destroy();

}

int CIPCProc::Create()
{
	pThis = this;
	OSA_thrCreate(&thrHandlPCRcv,
			IPC_childdataRcvn,
	                  0,
	                  0,
	                  this);
	_Message = CMessage::getInstance();
	_GlobalDate = CGlobalDate::Instance();
	 return 0;
}

int CIPCProc::Destroy()
{
	exitThreadIPCRcv=true;
	OSA_thrJoin(&thrHandlPCRcv);
	return 0;
}
IMGSTATUS * CIPCProc::getAvtStatSharedMem()
{
	ipc_status = ipc_getimgstatus_p();
	assert(ipc_status != NULL);
	return ipc_status;
}
OSDSTATUS *CIPCProc::getOSDSharedMem()
{
	ipc_OSD =ipc_getosdstatus_p();
	assert(ipc_OSD != NULL);
	return ipc_OSD;
}
UTCTRKSTATUS *CIPCProc::getUTCSharedMem()
{
	ipc_UTC = ipc_getutstatus_p();
	assert(ipc_UTC != NULL);
	return ipc_UTC;
}

LKOSDSTATUS* CIPCProc::getLKOSDShareMem()
{
	ipc_LKOSD = ipc_getlkosdstatus_p();
	assert(ipc_LKOSD != NULL);
	return ipc_LKOSD;
}

osdtext_t *CIPCProc::getOSDTEXTSharedMem()
{
	ipc_OSDTEXT =ipc_getosdtextstatus_p();
	assert(ipc_OSDTEXT != NULL);
	return ipc_OSDTEXT;
}
	
void CIPCProc::getIPCMsgProc()
{
	CMD_MOUSEPTZ mptz = {0};
	CMD_MTDTRKTIME mtdtime = {0};
	   int getx,gety;
  	   struct timeval tmp;
      	tmp.tv_sec = 0;
      	tmp.tv_usec = 10000;
      	while(!exitThreadIPCRcv){
         ipc_recvmsg(&fr_img_test,IPC_FRIMG_MSG);
        pthread_mutex_lock(&mutex);
        int chid = _GlobalDate->chid_camera;
        switch(fr_img_test.cmd_ID)
        {
            case trk:
                break;
            case sectrk:
                break;
            case enh:
                memcpy(&fr_img_cmd_enh, fr_img_test.param, sizeof(fr_img_cmd_enh));
                break;
            case mtd:
                memcpy(&fr_img_cmd_mtd, fr_img_test.param, sizeof(fr_img_cmd_mtd));
                break;
            case trkdoor:
                memcpy(&fr_img_cmd_trkdoor, fr_img_test.param, sizeof(fr_img_cmd_trkdoor));
                break;
            case mmt:
                memcpy(&fr_img_cmd_mmt, fr_img_test.param, sizeof(fr_img_cmd_mmt));
                break;
            case mmtselect:
                memcpy(&fr_img_cmd_mmtsel, fr_img_test.param, sizeof(fr_img_cmd_mmtsel));
                break;
            case read_shm_trkpos:
        		ipc_gettrack(&trackstatus,&trackposx,&trackposy);//get value from shared_memory
        		//printf("IPC  ==>  trackposx = %f,   trackposy = %f\n", trackposx, trackposy);
        		_Message->MSGDRIV_send(MSGID_IPC_INPUT_TRACKCTRL, 0);
        		break;

     		case sensor:
     			break;
     		case trktype:
     			break;
     		case mouseptz:
     			memcpy(&_GlobalDate->ipc_mouseptz, &fr_img_test.param, sizeof(cmd_mouseptz));
     			_Message->MSGDRIV_send(MSGID_EXT_INPUT_PLATCTRL, 0);
     			break;

     		case mtdredetect:		/*46:针对移动检测，跟丢后是否回到固定位置再次检测*/
     			if(_GlobalDate->ImgMtdStat)
     				_GlobalDate->mtdconfig.preset = fr_img_test.param[0];
     			printf("redetect   = %d\n" , _GlobalDate->mtdconfig.preset);
     			break;
     		case mtdoutput:			/*47:针对移动检测，是否报警输出*/
     			_GlobalDate->mtdconfig.warning = fr_img_test.param[0];
     			_Message->MSGDRIV_send(MSGID_IPC_Mtdoutput, 0);
     			break;
     		case mtdpolar:				/*48:针对移动检测，输出的信号极性*/
     			_GlobalDate->mtdconfig.high_low_level = fr_img_test.param[0];
     			_Message->MSGDRIV_send(MSGID_IPC_Mtdpolar, 0);
     		break;
     		case mtdtrktime:			/*49:针对移动检测，跟踪的持续时间*/
     			_GlobalDate->mtdconfig.trktime = fr_img_test.param[0] * 1000;
     			printf("trktime  =  %d \n", _GlobalDate->mtdconfig.trktime);
     		break;

     		case querypos:				/*Send Pos to img */
     			_Message->MSGDRIV_send(MSGID_IPC_QueryPos, 0);
     			break;

     		case speedloop:			/*Linkage :Control PTZ with speed loop in a thread  */
     			memset(&_GlobalDate->linkagePos, 0, sizeof(_GlobalDate->linkagePos));
     			memcpy(&_GlobalDate->linkagePos, &fr_img_test.param, sizeof(_GlobalDate->linkagePos));
     			OSA_semSignal(&_GlobalDate->m_semHndl_automtd);
     			//printf("[IPC] pos->pan = %d, pos->Tilt = %d \n", _GlobalDate->linkagePos.panPos, _GlobalDate->linkagePos.tilPos);
     			break;
     		case acqPosAndZoom:		 /*Linkage: Calls the recursive function(speedLoop) in the acq state */
     			memset(&_GlobalDate->linkagePos, 0, sizeof(_GlobalDate->linkagePos));
     			memcpy(&_GlobalDate->linkagePos, &fr_img_test.param, sizeof(_GlobalDate->linkagePos));
     			_Message->MSGDRIV_send(MSGID_IPC_INPUT_PosAndZoom, 0);
     			break;
     		case read_shm_osdtext:
     			_GlobalDate->recvOsdbuf.osdID = fr_img_test.param[0];
     			_Message->MSGDRIV_send(MSGID_IPC_INPUT_NameAndPos, 0);
     			break;
     		case ipcwordSize:
     			_GlobalDate->Host_Ctrl[wordSize] = fr_img_test.param[0];
     			_Message->MSGDRIV_send(MSGID_EXT_INPUT_fontsize, 0);
     			break;
    		case reset_swtarget_timer:
    			if(1 == _GlobalDate->mtdMode)
    				_Message->MSGDRIV_send(MSGID_IPC_INPUT_reset_swtarget_timer, 0);
    			break;
     		case mtdmode:
     			if(_GlobalDate->ImgMtdStat)
     				_GlobalDate->mtdMode = fr_img_test.param[0];
     			_Message->MSGDRIV_send(MSGID_EXT_INPUT_MTDMode, 0);
     			break;
     		case josctrl:
     			_GlobalDate->jos_params.menu = fr_img_test.param[32];
     			break;

		case setconfig:
			CMD_SETCONFIG recvconfig;
			memcpy(&recvconfig, fr_img_test.param, sizeof(recvconfig));
			ipc_Config_Write_Save(recvconfig);
			break;

		case ballbaud:
			memcpy(&_GlobalDate->m_uart_params[chid].balladdress, fr_img_test.param, sizeof(_GlobalDate->m_uart_params[chid].balladdress));
			memcpy(&_GlobalDate->m_uart_params[chid ].baudrate, &fr_img_test.param[4], sizeof(_GlobalDate->m_uart_params[chid].baudrate));
			_Message->MSGDRIV_send(MSGID_IPC_INPUT_ballparam, 0);
			break;

 		case enter_gridmap_view:
 			_GlobalDate->gridMap = fr_img_test.param[0];
 			break;

 		case storeMtdConfig:
 			pMtd = &cmd_mtd_config;
 			memcpy(pMtd, fr_img_test.param, sizeof(cmd_mtd_config));
 			_Message->MSGDRIV_send(MSGID_IPC_INPUT_MtdParams, 0);
 			break;

 		case storeDefaultWorkMode:
 			_GlobalDate->default_workMode = fr_img_test.param[0];
 			_Message->MSGDRIV_send(MSGID_IPC_INPUT_defaultWorkMode, 0);
 			break;

 		case jos_mouse_mode:
 			_GlobalDate->jos_params.ctrlMode = fr_img_test.param[0];/*jos:1 控球模式；mouse:2 鼠标模式*/
 			_GlobalDate->jos_params.workMode = ballctrl;
 			_Message->MSGDRIV_send(MSGID_EXT_INPUT_workModeSwitch, 0);
 			break;

	    default:
	        break;
        }
        pthread_mutex_unlock(&mutex);
     }
}

int  CIPCProc::ipcTrackCtrl(volatile unsigned char AvtTrkStat)
{
     memset(test.param,0,PARAMLEN);
	test.cmd_ID = trk;
	{
		test.param[0]=AvtTrkStat;
	    	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	}
	printf("IPC===>AvtTrkStat = %d\n", AvtTrkStat);
	return 0;
}

int CIPCProc::ipcSceneTrkCtrl(volatile unsigned char senceTrkStat)
{
    memset(test.param,0,PARAMLEN);
	test.cmd_ID = sceneTrk;
	test.param[0]=senceTrkStat;
	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	return 0;
}


int  CIPCProc::ipcMutilTargetDetecCtrl(volatile unsigned char ImgMmtStat)//1:open 0:close
{
      memset(test.param,0,PARAMLEN);
	test.cmd_ID = mmt;

	{
		test.param[0]=ImgMmtStat;
	    	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	    	//sThis->signalFeedBack(ACK_mmtStatus, 0, 0, 0);
	}

		return 0;
}

int  CIPCProc::ipcMutilTargetSelectCtrl(volatile unsigned char ImgMmtSelect)
{
      memset(test.param,0,PARAMLEN);
	test.cmd_ID = mmtselect;
	{
		test.param[0]=ImgMmtSelect;
	    	//ipc_sendmsg(&test,IPC_TOIMG_MSG);
	}
		return 0;
}

int CIPCProc::IpcMmtLockCtrl(int mmt_Select)
{
	memset(test.param,0,PARAMLEN);
	test.cmd_ID = mmtLock;
	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	//sThis->Change_avtStatus();
	//sThis->signalFeedBack(ACK_mmtSelectStatus, ACK_mmtSelect_value, mmt_Select, 0);
	return 0;
}

int CIPCProc::ipcImageEnhanceCtrl(volatile unsigned char ImgEnhStat) //1open 0close
{
      memset(test.param,0,PARAMLEN);
	test.cmd_ID = enh;
	{
		test.param[0]=ImgEnhStat;
	    	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	    	printf("enh is send \n");
	    	//sThis->Change_avtStatus();
	    	//sThis->signalFeedBack(ACK_EnhStatus, 0, 0, 0);
	}
		return 0;
}

int CIPCProc::ipcMoveTatgetDetecCtrl(volatile unsigned char ImgMtdStat)
{
      memset(test.param,0,PARAMLEN);
	test.cmd_ID = mtd;
	{
		test.param[0]=ImgMtdStat;
	    	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	    	//sThis->Change_avtStatus();
	    	//sThis->signalFeedBack(ACK_MtdStatus, 0, 0, 0);
	    	printf("MTDStat = %d\n", test.param[0]);

	}

		return 0;
}

int CIPCProc::ipcSecTrkCtrl(selectTrack *m_selcTrak)
{
	CMD_SECTRK cmd_sectrk;
    	memset(test.param,0,PARAMLEN);
	test.cmd_ID = sectrk;

	cmd_sectrk.SecAcqStat = m_selcTrak->SecAcqStat;
	cmd_sectrk.ImgPixelX =m_selcTrak->ImgPixelX;
	cmd_sectrk.ImgPixelY =m_selcTrak->ImgPixelY;
	memcpy(test.param, &cmd_sectrk, sizeof(cmd_sectrk));
	ipc_sendmsg(&test,IPC_TOIMG_MSG);

	return 0;
}

int CIPCProc::IpcSensorSwitch(volatile unsigned char ImgSenchannel)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = sensor;
	{
		test.param[0] = ImgSenchannel;
		ipc_sendmsg(&test, IPC_TOIMG_MSG);
		//sThis->Change_avtStatus();
		//sThis->signalFeedBack(ACK_mainVideoStatus, 0, 0, 0);
		printf("sensorchannel = %d\n", test.param[0]);
	}
	return 0;
}

int CIPCProc::IpcpinpCtrl(volatile unsigned char ImgPipStat)
{
	CMD_PinP cmd_pip;
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = pinp;
	{
		cmd_pip.ImgPicp = ImgPipStat;
		cmd_pip.PicpSensorStat = 1;
		cmd_pip.PicpZoomStat = 1;
		memcpy(test.param, &cmd_pip, sizeof(cmd_pip));
		ipc_sendmsg(&test, IPC_TOIMG_MSG);
		//sThis->signalFeedBack(ACK_picpStatus, 0, 0, 0);
	}
	return 0;
}

int CIPCProc::IpcAcqDoorCtrl(AcqBoxSize *BoxSize)
{
	memset(test.param, 0, PARAMLEN);
	AcqBoxWH m_acqBoxWH;
	test.cmd_ID = acqBox;
	{
		m_acqBoxWH.AimW = BoxSize->AcqBoxW[0];
		m_acqBoxWH.AimH = BoxSize->AcqBoxH[0];
		printf("IPC======>AimW = %d\n", m_acqBoxWH.AimW);
		printf("IPC======>AimH = %d\n", m_acqBoxWH.AimH);
		memcpy(test.param, &m_acqBoxWH, sizeof(m_acqBoxWH));
		ipc_sendmsg(&test, IPC_TOIMG_MSG);
		//sThis->Change_avtStatus();
	}
	return 0;
}

int CIPCProc::IpcIrisAndFocus(osd_triangle* osd_tri, char sign)
{
	memset(test.param, 0, PARAMLEN);
	CMD_triangle cmd_triangel;
	switch(sign)
	{
	case Exit:
		test.cmd_ID = exit_IrisAndFocus;
		break;
	case iris:
		test.cmd_ID = Iris;
		break;
	case Focus:
		test.cmd_ID = focus;
		break;
	}
	cmd_triangel.dir = osd_tri->dir;
	cmd_triangel.alpha = osd_tri->alpha;
	memcpy(test.param, &cmd_triangel, sizeof(cmd_triangel));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcFuncMenu(char sign)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = menu;
	test.param[0] = sign;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcConfig()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_config;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcConfigOSD()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_osd;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);

	return 0;
}

int CIPCProc::IpcConfigOSDTEXT()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_osdtext;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);

	return 0;
}

int CIPCProc::IpcConfigUTC()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_utctrk;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcSendLosttime()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = ipclosttime;
	memcpy(&test.param[0],&losttime , 4);
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCSendMtdFrame()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = mtdFrame;
	memcpy(&test.param, pMtd, sizeof(cmd_mtd_config));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCSendPos(int pan, int tilt, int zoom)
{
	LinkagePos cmd_linkagePos;
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = querypos;
	cmd_linkagePos.panPos = pan;
	cmd_linkagePos.tilPos = tilt;
	cmd_linkagePos.zoom = zoom;	//0;
	memcpy(&test.param,&cmd_linkagePos , sizeof(cmd_linkagePos));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::refreshPtzParam(int pan, int tilt, int zoom)
{
	LinkagePos cmd_linkagePos;
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = refreshptz;
	cmd_linkagePos.panPos = pan;
	cmd_linkagePos.tilPos = tilt;
	cmd_linkagePos.zoom = zoom;	//0;
	memcpy(&test.param,&cmd_linkagePos , sizeof(cmd_linkagePos));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCSendBallParam()
{
	memset(test.param, 0, PARAMLEN);
	int chid = _GlobalDate->chid_camera;
	test.cmd_ID = ballbaud;
	memcpy(test.param, &_GlobalDate->m_uart_params[chid].balladdress, sizeof(_GlobalDate->m_uart_params[chid].balladdress));
	memcpy(&test.param[4], &_GlobalDate->m_uart_params[chid].baudrate, sizeof(_GlobalDate->m_uart_params[chid].baudrate));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::ipcSendJosParams()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = josctrl;
	memcpy(test.param,&_GlobalDate->jos_params , sizeof(_GlobalDate->jos_params));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);

	return 0;
}

int CIPCProc::IpcWordFont(char font)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = ipcwordFont;
	test.param[0] = font;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcWordSize(char size)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = ipcwordSize;
	test.param[0] = size;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}



int CIPCProc::IPCConfigCamera()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_camera;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCMtdSwitch(volatile unsigned char ImgMtdStat, volatile unsigned char mtdMode)
{
	CMD_MTD cmd_mtd;
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = mtd;
	cmd_mtd.ImgMtdStat = ImgMtdStat;
	cmd_mtd.mtdMode = mtdMode;
	memcpy(test.param, &cmd_mtd, sizeof(cmd_mtd));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCMtdSelectCtrl(volatile unsigned char MtdSelect)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = mtdSelect;
	test.param[0] = MtdSelect;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IpcSwitchTarget()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = switchtarget;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
#if 0
	printf("\n");
	printf("\n");
	printf("#############  switch target    #############\n");
	printf("\n");
	printf("\n");
#endif
	return 0;
}
	
int CIPCProc::IpcElectronicZoom(int zoom)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = elecZoom;
	test.param[0] = zoom;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	//sThis->signalFeedBack(ACK_ElectronicZoomStatus, ACK_ElectronicZoom_value, zoom, 0);
	return 0;
}

int CIPCProc::IPCChannel_binding(int channel)
{
	memset(test.param, 0, PARAMLEN);
	return 0;
}

int CIPCProc::IPCAxisMove(int x, int y)
{
	memset(test.param, 0, PARAMLEN);
	CMD_POSMOVE cmd_axismove;
	test.cmd_ID = axismove;
	cmd_axismove.AvtMoveX = x;
	cmd_axismove.AvtMoveY = y;
	memcpy(test.param, &cmd_axismove, sizeof(cmd_axismove));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	//sThis->signalFeedBack(ACK_moveAxisStatus, ACK_moveAxis_value, x, y);
}


int CIPCProc::IPCpicp(int status, int pipChannel)
{
	memset(test.param, 0, PARAMLEN);
	CMD_PinP cmd_pip;
	test.cmd_ID = pinp;
	cmd_pip.ImgPicp = status;
	cmd_pip.PicpSensorStat = pipChannel;
	memcpy(test.param, &cmd_pip, sizeof(cmd_pip));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	//sThis->signalFeedBack(ACK_picpStatus, status, pipChannel, 0);
	return 0;

}
int CIPCProc::IPCAlgosdrect(int tmp)
{
	memset(test.param, 0, PARAMLEN);
	CMD_ALGOSDRECT cmd_algosdrect;
	test.cmd_ID = algosdrect;
	cmd_algosdrect.Imgalgosdrect = tmp;
	memcpy(test.param, &cmd_algosdrect, sizeof(cmd_algosdrect));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCchooseVideoChannel(int channel)
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = sensor;
	{
		test.param[0] = channel;
		ipc_sendmsg(&test, IPC_TOIMG_MSG);
		printf("IPC==>channel = %d \n", channel);
	}
	return 0;
}


int CIPCProc::IPCframeCtrl(int fps, int channel)
{
	memset(test.param, 0, PARAMLEN);
	return 0;
}

int CIPCProc::IPCcompression_quality(int quality, int channel)
{
	memset(test.param, 0, PARAMLEN);
	return 0;
}


int CIPCProc::IPCOsdWord(volatile osdbuffer_t &inosd)
{	
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = osdbuffer;
	memcpy(&test.param[0],(const char*)&inosd,sizeof(osdbuffer_t));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCBoresightPosCtrl(unsigned int x,unsigned int y)
{
	memset(test.param, 0, PARAMLEN);
	CMD_BoresightPos cmd_BoresightPos;
	test.cmd_ID = BoresightPos;
	{
		cmd_BoresightPos.BoresightPos_x = x;
		cmd_BoresightPos.BoresightPos_y = y;
		memcpy(test.param, &cmd_BoresightPos, sizeof(cmd_BoresightPos));
		ipc_sendmsg(&test, IPC_TOIMG_MSG);
	}
	return 0;
}

int CIPCProc::ipcAcqBoxCtrl(AcqBoxPos_ipc* m_acqPos)
{
	CMD_AcqBoxPos cmd_acqPos;
    	memset(test.param,0,PARAMLEN);
	test.cmd_ID = AcqPos;

	cmd_acqPos.AcqStat = m_acqPos->AcqStat;
	cmd_acqPos.BoresightPos_x = m_acqPos->BoresightPos_x;
	cmd_acqPos.BoresightPos_y = m_acqPos->BoresightPos_y;
	memcpy(test.param, &cmd_acqPos, sizeof(cmd_acqPos));
	ipc_sendmsg(&test,IPC_TOIMG_MSG);
	printf("cmd_acqPos.AcqStat = %d \n", cmd_acqPos.AcqStat);
	printf("x = %d   y  = %d \n", cmd_acqPos.BoresightPos_x, cmd_acqPos.BoresightPos_y);
	return 0;
}

int CIPCProc::IPCLKOSD()
{
	memset(test.param, 0, PARAMLEN);
	test.cmd_ID = read_shm_lkosd;
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

int CIPCProc::IPCResolution(CMD_IPCRESOLUTION tmp)
{
	memset(test.param, 0, PARAMLEN);
	CMD_IPCRESOLUTION cmd_resolution;
	test.cmd_ID = ipcresolution;
	cmd_resolution.resolution[ipc_eSen_CH0] = tmp.resolution[ipc_eSen_CH0];
	cmd_resolution.resolution[ipc_eSen_CH1] = tmp.resolution[ipc_eSen_CH1];
	cmd_resolution.resolution[ipc_eSen_CH2] = tmp.resolution[ipc_eSen_CH2];
	cmd_resolution.resolution[ipc_eSen_CH3] = tmp.resolution[ipc_eSen_CH3];
	cmd_resolution.resolution[ipc_eSen_CH4] = tmp.resolution[ipc_eSen_CH4];
	cmd_resolution.outputresol = tmp.outputresol;
	memcpy(test.param, &cmd_resolution, sizeof(cmd_resolution));
	ipc_sendmsg(&test, IPC_TOIMG_MSG);
	return 0;
}

void CIPCProc::ipc_Config_Write_Save(CMD_SETCONFIG config)
{
	memset(&_GlobalDate->avtSetting,0,sizeof(_GlobalDate->avtSetting));
	_GlobalDate->avtSetting.cmdBlock = config.block;
	_GlobalDate->avtSetting.cmdFiled = config.field;
	_GlobalDate->avtSetting.confitData = config.value;

	_GlobalDate->Host_Ctrl[config_Wblock]	=	_GlobalDate->avtSetting.cmdBlock;
	_GlobalDate->Host_Ctrl[config_Wfield]	=	_GlobalDate->avtSetting.cmdFiled;
	_GlobalDate->Host_Ctrl[config_Wvalue]	=	_GlobalDate->avtSetting.confitData;
	_Message->MSGDRIV_send(MSGID_EXT_INPUT_configWrite, 0);
	_Message->MSGDRIV_send(MSGID_EXT_INPUT_configWrite_Save, 0);
}
