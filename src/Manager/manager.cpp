#include "manager.h"

const float jos_value = 32767;
const char up = 3;
const char down = 4;
const int Trk = 1;
const int Sensor = 2;
const int profileNum = 1922;
const int MtdAreaNum = 10;
static int  iAxis_X, iAxis_Y, m_valuex, m_valuey;
static int x_pos, x_speed, y_pos, y_speed;
extern View viewParam;
string parity[5], dev[5] ;

CManager* sThis = NULL;

CStatusManager* CManager::m_StateManager = 0;

CManager* CManager::pThis = 0;

CGlobalDate* CManager::m_GlobalDate = 0;

selectCh_t CGlobalDate::selectch = {1,1,1,1,1,0};

CManager::CManager() : cfg_value(profileNum)
{
	sThis = this;
	exitSpeedLoop_Linkage = true;
	creat();
}

CManager::~CManager()
{
	Message_destroy();
	Sem_destroy();
}

void CManager::creat()
{
	classObject_Creat();
	IPC_Creat();
	m_ipc->Create();
	m_Jos->Create();
	Sem_Creat();
	Message_Creat();
	configAvtFromFile();
	DxTimer_Create();
	Comm_Creat();
	MSGAPI_Init();
	Sem_Init();
	m_GlobalDate->chid_camera = 1;
	m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[m_GlobalDate->chid_camera];
	m_ptz->Create();
	m_ptzSpeed.create();
	Platform_Creat();
	m_GlobalDate->joystick_flag = 1;
	sensorChIdCreate();
	int x = 960;
	int y = 540;
	m_ipc->IPCBoresightPosCtrl(x, y);
	/*
	struct timeval tmp;
	tmp.tv_sec = 3;
	tmp.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tmp);
	*/
	m_GlobalDate->workMode = m_GlobalDate->default_workMode;
	usd_API_WORKMODEWITCH();
}

void CManager::Message_Creat()
{
	m_Message = CMessage::getInstance();
    _Handle = m_Message->MSGDRIV_create();
}

void CManager::Sem_Creat()
{
	OSA_semCreate(&m_GlobalDate->m_semHndl, 1, 0);
	OSA_semCreate(&m_GlobalDate->m_semHndl_s, 1, 0);
	OSA_semCreate(&m_GlobalDate->m_semHndl_socket, 1, 0);
	OSA_semCreate(&m_GlobalDate->m_semHndl_socket_s, 1, 0);
	OSA_semCreate(&m_GlobalDate->m_semHndl_automtd, 1, 0);
}

void CManager::Sem_Init()
{
	OSA_semSignal(&m_GlobalDate->m_semHndl_socket_s);
	OSA_semSignal(&m_GlobalDate->m_semHndl_s);
}

void CManager::classObject_Creat()
{
	pThis = this;
	m_StateManager = CStatusManager::Instance();
	m_GlobalDate = CGlobalDate::Instance();
	m_Platform = new CplatFormControl();
	m_ptz = new CPTZControl();
	m_ipc = new CIPCProc();
	m_Jos = new CJoystick();
}

void CManager::IPC_Creat()
{
    Ipc_init();
    int ret = Ipc_create();
    if(ret == -1)
    {
    	OSA_printf("[%s] ipc creat error \n", __func__);
    	return;
    }
}

void CManager::Platform_Creat()
{
	m_Platform->PlatformCtrl_CreateParams_Init(&m_cfgPlatParam, &m_InitParam, &viewParam);
	OSA_assert(m_plt == NULL);
	m_plt = m_Platform->PlatformCtrl_Create(&m_cfgPlatParam);
}

void CManager::Comm_Creat()
{
	CPortInterface *_UartProcess = PortFactory::createProduct(1);
	_UartProcess->getpM();
	_UartProcess->create();
	_UartProcess->run();

	CPortInterface *_NetWork = PortFactory::createProduct(2);
	_NetWork->getpM();
	_NetWork->create();
	_NetWork->run();
}

void CManager::GPIO_Create(unsigned int num, unsigned int mode)
{
	GPIO_create(num, mode);
}

void CManager::sensorChIdCreate()
{
	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	switch_CameraChid();
	int x = pObj->params.sensorParams[1].defaultBoresightPos_x;
	int y = pObj->params.sensorParams[1].defaultBoresightPos_y;
	m_ipc->IPCBoresightPosCtrl(x, y);
}

void CManager::DxTimer_Create()
{
	swtarget_id = dtimer.createTimer();
	dtimer.registerTimer(swtarget_id, Tcallback_Linkage, &swtarget_id);
}

void CManager::Tcallback_Linkage(void *p)
{
    int tid = *(int *)p;
	if(pThis->swtarget_id == tid)
	{
		pThis->m_ipc->IpcSwitchTarget();
	}
}

void CManager::destroy()
{
	thrDelete_LinkageSpeedLoop();
}

void CManager::Sem_destroy()
{
	OSA_semDelete(&m_GlobalDate->m_semHndl);
	OSA_semDelete(&m_GlobalDate->m_semHndl_s);
	OSA_semDelete(&m_GlobalDate->m_semHndl_socket);
	OSA_semDelete(&m_GlobalDate->m_semHndl_socket_s);
	OSA_semDelete(&m_GlobalDate->m_semHndl_automtd);
}

int CManager::GetIpcAddress(int type)
{
	m_ipc->ipc_status = m_ipc->getAvtStatSharedMem();
	switch(type)
	{
		case Trk:
			return  m_ipc->ipc_status->TrkStat;
		break;
		case Sensor:
			return  m_ipc->ipc_status->SensorStat;
		break;
	}
}

void CManager::Message_destroy()
{
	m_Message->MSGDRIV_destroy(_Handle);
}
void CManager::MSGAPI_Init()
{
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_PLATCTRL, MSGAPI_ExtInpuCtrl_AXIS, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_AxisCtrl, MSGAPI_ExtInpuCtrl_AxisCtrl, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_TRACKCTRL, MSGAPI_ExtInpuCtrl_TRACKCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_IrisAndFocusAndExit, MSGAPI_ExtInpuCtrl_IrisAndFocusAndExit, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_TRACKSEARCHCTRL, MSGAPI_ExtInpuCtrl_TRACKSEARCHCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_SwitchSensor, MSGAPI_ExtInpuCtrl_SwitchSensor, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_PRMBACK, MSGAPI_ExtInpuCtrl_PRMBACK, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_zoomSpeed, MSGAPI_ExtInpuCtrl_zoomSpeed, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL, MSGAPI_ExtInpuCtrl_ZOOMSHORTCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL, MSGAPI_ExtInpuCtrl_ZOOMLONGCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_IRISDOWNCTRL, MSGAPI_ExtInpuCtrl_IRISDOWNCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_IRISUPCTRL, MSGAPI_ExtInpuCtrl_IRISUPCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_FOCUSNEARCTRL, MSGAPI_ExtInpuCtrl_FOCUSNEARCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_FOCUSFARCHCTRL, MSGAPI_ExtInpuCtrl_FOCUSFARCHCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_configWrite, MSGAPI_ExtInpuCtrl_configWrite, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_config_Read, MSGAPI_ExtInpuCtrl_config_Read, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_configWrite_Save, MSGAPI_ExtInpuCtrl_configWrite_Save, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_MTDCTRL, MSGAPI_ExtInpuCtrl_MTDCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_MTDMode, MSGAPI_ExtInpuCtrl_MTDMode, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_MtdPreset, MSGAPI_EXT_INPUT_MtdPreset, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_CallPreset, MSGAPI_EXT_INPUT_CallPreset, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_setPan, MSGAPI_EXT_INPUT_setPan, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_setTilt, MSGAPI_EXT_INPUT_setTilt, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_setZoom, MSGAPI_EXT_INPUT_setZoom, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_queryPan, MSGAPI_EXT_INPUT_queryPan, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_queryTilt, MSGAPI_EXT_INPUT_queryTilt, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_queryZoom, MSGAPI_EXT_INPUT_queryZoom, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_workModeSwitch, MSGAPI_ExtInpuCtrl_WORKMODEWITCH, 0);

	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_setPreset, MSGAPI_EXT_INPUT_setPreset, 0);
	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_goToPreset, MSGAPI_EXT_INPUT_goToPreset, 0);
	m_Message->MSGDRIV_register(test_ptz_left, test_left, 0);
	m_Message->MSGDRIV_register(test_ptz_right, test_right, 0);
	m_Message->MSGDRIV_register(test_ptz_stop, test_stop, 0);

	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_reset_swtarget_timer, MSGAPI_IPC_INPUT_reset_swtarget_timer, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_TRACKCTRL, MSGAPI_IPC_INPUT_TRACKCTRL, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_chooseVideoChannel, MSGAPI_IPC_ChooseVideoChannel, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_Resolution, MSGAPI_IPC_Resolution, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_AxisMove, MSGAPI_IPC_AxisMove, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_Algosdrect, MSGAPI_IPC_Algosdrect, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_word, MSGAPI_IPC_word, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_WORDFONT, MSGAPI_IPC_WORDFONT, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_WORDSIZE, MSGAPI_IPC_WORDSIZE, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_AutoMtd, MSGAPI_IPC_AutoMtd, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_Mtdoutput, MSGAPI_IPC_Mtdoutput, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_Mtdpolar, MSGAPI_IPC_Mtdpolar, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_ballparam, MSGAPI_IPC_ballparam, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_UserOSD, MSGAPI_IPC_UserOSDSWITCH, 0);

	m_Message->MSGDRIV_register(MSGID_EXT_INPUT_fontsize, MSGAPI_IPC_INPUT_fontSize, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_QueryPos, MSGAPI_IPC_QueryPos, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_PosAndZoom, MSGAPI_IPC_INPUT_PosAndZoom, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_NameAndPos, MSGAPI_IPC_INPUT_VideoNameAndPos, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_CTRLPARAMS, MSGAPI_IPC_INPUT_CTRLPARAMS, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_defaultWorkMode, MSGAPI_IPC_INPUT_defaultWorkMode, 0);
	m_Message->MSGDRIV_register(MSGID_IPC_INPUT_MtdParams, MSGAPI_IPC_INPUT_MtdParams, 0);

}

void CManager::test_left(long p)
{
	pThis->m_ptz->ptzMove(3, 0x15);
}

void CManager::test_right(long p)
{
	pThis->m_ptz->ptzMove(4, 0x10);
}

void CManager::test_stop(long p)
{
	pThis->m_ptz->ptzMove(0, 0);
}

void CManager::MSGAPI_ExtInpuCtrl_AXIS(long p)
{
	pThis->usd_API_AXIS();
}

void CManager::MSGAPI_ExtInpuCtrl_AxisCtrl(long p)
{
	if(m_StateManager->isTwo_PlantForm_Acq() || m_StateManager->isTwo_PlantForm_Trk())
	{
		if(m_StateManager->isThree_CtrlPtz())
			m_StateManager->three_State_CtrlBox();
		else
			m_StateManager->three_State_CtrlPtz();
		int TrkStat = pThis->GetIpcAddress(Trk);
		pThis->AVTSpeedReset(!TrkStat, 1);
		m_StateManager->BoxCtrl_IPC(TrkStat);
		pThis->ExtInputCtrl_AcqBoxPos();
		pThis->usd_API_AXIS();
	}
}

void CManager::MSGAPI_ExtInpuCtrl_TRACKCTRL(long p)
{
	if(m_GlobalDate->Mtd_ExitMode == Manual)
	{
		if(!m_GlobalDate->Mtd_Manual_Limit)
			return ;
	}
	int TrkStat = pThis->GetIpcAddress(Trk);

	pThis->usd_API_StatusSwitch();
	if(TrkStat == 0)
	{
		m_StateManager->trk_State_TrkPIDInitX();
		m_StateManager->trk_State_TrkPIDInitY();
	}

	m_StateManager->BoxCtrl_IPC(!TrkStat);
	pThis->usd_API_Track();

	gettimeofday(&m_GlobalDate->PID_t, NULL);
	m_GlobalDate->time_stop = 0;
	m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
}

void CManager::MSGAPI_ExtInpuCtrl_IrisAndFocusAndExit(long p)
{
	m_StateManager->WorkMode_IrisAndFocus();
	pThis->usd_API_IrisAndFocusAndExit();
}

void CManager::MSGAPI_ExtInpuCtrl_TRACKSEARCHCTRL(long p)
{
	if(m_StateManager->isTwo_PlantForm_Trk())
		m_StateManager->three_State_Trksearch();
		pThis->usd_API_TrkSearch();
}

void CManager::MSGAPI_ExtInpuCtrl_SwitchSensor(long p)
{
	pThis->usd_API_SwitchSensor();
	pThis->Observer_Chid();
}

void CManager::MSGAPI_ExtInpuCtrl_PRMBACK(long p)
{
	pThis->usd_API_PRMBACK();
}

void CManager::MSGAPI_ExtInpuCtrl_zoomSpeed(long p)
{
	m_GlobalDate->m_CurrStat.m_zoomSpeed = m_GlobalDate->Host_Ctrl[zoomSpeed];
	pThis->MSGAPI_ExtInputCtrl_ZoomSpeed();
}

void CManager::MSGAPI_ExtInpuCtrl_ZOOMSHORTCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_ZoomShortStat= m_GlobalDate->EXT_Ctrl[Cmd_Mesg_ZoomShort];
	pThis->MSGAPI_ExtInputCtrl_ZoomShort();
}

void CManager::MSGAPI_ExtInpuCtrl_ZOOMLONGCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_ZoomLongStat = m_GlobalDate->EXT_Ctrl[Cmd_Mesg_ZoomLong];
	pThis->MSGAPI_ExtInputCtrl_ZoomLong();
}

void CManager::MSGAPI_ExtInpuCtrl_IRISDOWNCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_IrisDownStat=m_GlobalDate->EXT_Ctrl[Cmd_Mesg_IrisDown];
	pThis->MSGAPI_ExtInputCtrl_IrisDown();
}

void CManager::MSGAPI_ExtInpuCtrl_IRISUPCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_IrisUpStat= m_GlobalDate->EXT_Ctrl[Cmd_Mesg_IrisUp];
	pThis->MSGAPI_ExtInputCtrl_IrisUp();
}

void CManager::MSGAPI_ExtInpuCtrl_FOCUSNEARCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_FoucusNearStat = m_GlobalDate->EXT_Ctrl[Cmd_Mesg_FocusNear];
	pThis->MSGAPI_ExtInputCtrlFocusNear();
}

void CManager::MSGAPI_ExtInpuCtrl_FOCUSFARCHCTRL(long p)
{
	m_GlobalDate->m_CurrStat.m_FoucusFarStat = m_GlobalDate->EXT_Ctrl[Cmd_Mesg_FocusFar];
	pThis->MSGAPI_ExtInputCtrlFocusFar();
}

void CManager::MSGAPI_ExtInpuCtrl_configWrite(long p)
{
	int block = (int)m_GlobalDate->Host_Ctrl[config_Wblock];
	int field = (int)m_GlobalDate->Host_Ctrl[config_Wfield];
	float value = m_GlobalDate->Host_Ctrl[config_Wvalue];

	pThis->modifierAVTProfile( block, field, value,NULL);
	pThis->signalFeedBack(ACK_config_Write, ACK_config_Wblock, block, field);
	pThis->Observer_Action(block, field, value);
}
	
void CManager::MSGAPI_ExtInpuCtrl_config_Read(long p)
{
	while(m_GlobalDate->readConfigBuffer.size()){
		Read_config_buffer tmp = m_GlobalDate->readConfigBuffer.at(0);
		int block = tmp.block;
		int field = tmp.filed;
		if(m_GlobalDate->commode == 1)
			OSA_semWait(&m_GlobalDate->m_semHndl_s,OSA_TIMEOUT_FOREVER);
		else if(m_GlobalDate->commode == 2)
			OSA_semWait(&m_GlobalDate->m_semHndl_socket_s,OSA_TIMEOUT_FOREVER);
		pThis->answerRead( block, field);
		pThis->signalFeedBack(ACK_config_Read, ACK_config_Rblock, block, field);
		//printf(" [read] block = %d , field = %d \n", block, field);
		m_GlobalDate->readConfigBuffer.erase(m_GlobalDate->readConfigBuffer.begin());
	}
}

void CManager::MSGAPI_ExtInpuCtrl_configWrite_Save(long p)
{
	pThis->updataProfile();
}

void CManager::MSGAPI_ExtInpuCtrl_MTDCTRL(long p)
{
	//pThis->usd_API_MTDCTRL();
}

void CManager::MSGAPI_ExtInpuCtrl_MTDMode(long p)
{
	pThis->usd_API_MTDMode();
}

void CManager::MSGAPI_IPC_INPUT_TRACKCTRL(long p)
{
		pThis->usd_API_IPC_TRACKCTRL();
}

void CManager::MSGAPI_IPC_ChooseVideoChannel(long p)
{
	pThis->usd_API_ChooseVideoChannel();
}

void CManager::MSGAPI_IPC_Resolution(long p)
{
	pThis->usd_API_Resolution();
}

void CManager::MSGAPI_IPC_AxisMove(long p)
{
	pThis->usd_API_AxisMove();
}

void CManager::MSGAPI_IPC_Algosdrect(long p)
{
	pThis->usd_API_Algosdrect();
}

void CManager::MSGAPI_IPC_word(long p)
{
	pThis->usd_API_word();
}

void CManager::MSGAPI_IPC_WORDFONT(long p)
{
	pThis->usd_API_WORDFONT();
}

void CManager::MSGAPI_IPC_WORDSIZE(long p)
{
	pThis->usd_API_WORDSIZE();
}

void CManager::MSGAPI_IPC_AutoMtd(long p)
{
	pThis->reset_status_AutoMtd();
	int TrkStat = pThis->GetIpcAddress(Trk);
	pThis->usd_API_StatusSwitch();
	if(TrkStat == 0)
	{
		m_StateManager->trk_State_TrkPIDInitX();
		m_StateManager->trk_State_TrkPIDInitY();
	}
	if(!m_StateManager->isTwo_PlantForm_Mtd())
	{
		m_StateManager->BoxCtrl_IPC(!TrkStat);
		pThis->usd_API_Track();
	}

	gettimeofday(&m_GlobalDate->PID_t, NULL);
	m_GlobalDate->time_stop = 0;
	m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
}
void CManager::MSGAPI_IPC_Mtdoutput(long p)
{
	pThis->GPIO_Create(386, m_GlobalDate->mtdconfig.warning);
	printf(" [GPIO] 386 config is %d \n", m_GlobalDate->mtdconfig.warning);
}

void CManager::MSGAPI_IPC_Mtdpolar(long p)
{
	pThis->SwitchWarning(386, m_GlobalDate->mtdconfig.high_low_level);
	printf(" [GPIO] 386 polar is %d \n", m_GlobalDate->mtdconfig.high_low_level);
}

void CManager::MSGAPI_IPC_ballparam(long p)
{
	pThis->IPC_ballbaud();
}

void CManager::MSGAPI_IPC_UserOSDSWITCH(long p)
{
	pThis->m_ipc->ipc_OSD->osdUserShow = m_GlobalDate->osdUserSwitch;
	pThis->m_ipc->IpcConfigOSD();
}

void CManager::MSGAPI_IPC_INPUT_reset_swtarget_timer(long p)
{
	pThis->dtimer.resetTimer(pThis->swtarget_id,m_GlobalDate->mtdconfig.trktime);
}

void CManager::MSGAPI_EXT_INPUT_MtdPreset(long p)
{
	pThis->preset_Mtd();
}

void CManager::MSGAPI_EXT_INPUT_CallPreset(long p)
{
	pThis->m_ptz->ptzSetPos(m_GlobalDate->Mtd_Moitor_X, m_GlobalDate->Mtd_Moitor_Y);
}

void CManager::MSGAPI_EXT_INPUT_setPan(long p)
{
	pThis->usd_API_queryPos();
	int chid = m_GlobalDate->chid_camera;
	int pan = m_GlobalDate->m_camera_pos[chid].setpan;
	int Tilt = m_GlobalDate->Mtd_Moitor_Y;
	pThis->m_ptz->ptzSetPos(pan, Tilt);
}

void CManager::MSGAPI_EXT_INPUT_setTilt(long p)
{
	pThis->usd_API_queryPos();
	int chid = m_GlobalDate->chid_camera;
	int pan = m_GlobalDate->Mtd_Moitor_X;
	int Tilt = m_GlobalDate->m_camera_pos[chid].settilt;
	pThis->m_ptz->ptzSetPos(pan, Tilt);
}

void CManager::MSGAPI_EXT_INPUT_setZoom(long p)
{
	int chid = m_GlobalDate->chid_camera;
	int zoom = m_GlobalDate->m_camera_pos[chid].setzoom;
	pThis->m_ptz->setZoomPos(zoom);

}

void CManager::MSGAPI_EXT_INPUT_queryPan(long p)
{
	pThis->usd_API_queryPos();
	int pan = m_GlobalDate->Mtd_Moitor_X;
	pThis->signalFeedBack(ACK_pan, ACK_pan_value, pan, 0);
}

void CManager::MSGAPI_EXT_INPUT_queryTilt(long p)
{
	pThis->usd_API_queryPos();
	int Tilt = m_GlobalDate->Mtd_Moitor_Y;
	pThis->signalFeedBack(ACK_Tilt, ACK_tilt_value, Tilt, 0);
}

void CManager::MSGAPI_EXT_INPUT_queryZoom(long p)
{
	pThis->m_ptz->QueryZoom();
	int zoom = m_GlobalDate->rcv_zoomValue;
	pThis->signalFeedBack(ACK_zoom, ACK_zoom_value, zoom, 0);
}

void CManager::MSGAPI_ExtInpuCtrl_WORKMODEWITCH(long p)
{
	pThis->usd_API_WORKMODEWITCH();
}

void CManager::MSGAPI_EXT_INPUT_setPreset(long p)
{
	pThis->m_ptz->Preset(0x03, 1);
}

void CManager::MSGAPI_EXT_INPUT_goToPreset(long p)
{
	pThis->m_ptz->Preset(0x07, 1);
}

void CManager::MSGAPI_IPC_QueryPos(long p)
{
	m_GlobalDate->Sync_Query = 1;
	while(m_GlobalDate->Sync_Query)
	{
		pThis->m_ptz->QueryPos();
		pThis->m_ptz->QueryZoom();
		printf("query \n");
	}

		int pan = pThis->m_ptz->m_iPanPos;
		int Tilt = pThis->m_ptz->m_iTiltPos;
		int zoomValue =  pThis->m_ptz->m_iZoomPos;
		pThis->m_ipc->IPCSendPos(pan, Tilt, zoomValue);

}

void CManager::MSGAPI_IPC_INPUT_PosAndZoom(long p)
{
	unsigned int pan = m_GlobalDate->linkagePos.panPos;
	unsigned int Tilt = m_GlobalDate->linkagePos.tilPos;
	unsigned int zoom = m_GlobalDate->linkagePos.zoom;
	pThis->m_ptz->ptzSetPos(pan,Tilt);
	if(zoom>=2849 && zoom <= 65535)
		pThis->m_ptz->setZoomPos(zoom);
}

void CManager::MSGAPI_IPC_INPUT_VideoNameAndPos(long p)
{
	pThis->IPC_videoName_Pos();
}

void CManager::MSGAPI_IPC_INPUT_fontSize(long p)
{
	pThis->IPC_fontSize();
}

void CManager::MSGAPI_IPC_INPUT_CTRLPARAMS(long p)
{
	pThis->usd_API_ctrlParams();
}

void CManager::MSGAPI_IPC_INPUT_defaultWorkMode(long p)
{
	pThis->modifierAVTProfile(121, 0, m_GlobalDate->default_workMode, NULL);
	pThis->updataProfile();
}

void CManager::MSGAPI_IPC_INPUT_MtdParams(long p)
{
	pThis->cfg_value[848] = pThis->m_ipc->pMtd->targetNum;
	pThis->cfg_value[855] = pThis->m_ipc->pMtd->trackTime;
	pThis->cfg_value[850] = pThis->m_ipc->pMtd->maxArea;
	pThis->cfg_value[851] = pThis->m_ipc->pMtd->minArea;
	pThis->cfg_value[852] = pThis->m_ipc->pMtd->sensitivity;
	pThis->updataProfile();
}

void CManager::usd_API_AXIS()
{
	int sensor = GetIpcAddress(Sensor);
	if(m_StateManager->isTwo_PlantForm_Acq())
	{
		if(m_StateManager->isThree_CtrlBox())
			ExtInputCtrl_AcqBoxPos();
		else if(m_StateManager->isThree_CtrlPtz())
			ExtInputCtrl_AXIS();
	}
	else if(m_StateManager->isTwo_PlantForm_Trk())
	{
		if(m_StateManager->isThree_TrkSearch())
			ExtInputCtrl_SendSeaTrkPos(sensor);
	}
	else if( m_StateManager->isTwo_PlantForm_Mtd())
	{
		if( m_StateManager->isThree_MtdManual())
			ExtInputCtrl_AXIS();
	}
}

void CManager::usd_API_Track()
{
	int TrkStatus = GetIpcAddress(Trk);
/**reset  **/
	AVTSpeedReset(TrkStatus, 0);

	if(m_StateManager->isThree_CtrlBox())
		ExtInputCtrl_AcqBoxPos();
	printf("m_GlobalDate->Mtd_Manual_Limit = %d \n", m_GlobalDate->Mtd_Manual_Limit);
	if(m_StateManager->isThree_CtrlBox() && m_StateManager->isBOX_Trk()) //enter Trk  parameter is 1
	{
		if(m_GlobalDate->ThreeMode_bak == 1)
		{
			m_GlobalDate->Mtd_Limit = shield_AutoMtdDate;

			if(m_GlobalDate->Mtd_ExitMode == Manual)
			{
				if(!m_GlobalDate->Mtd_Manual_Limit)
					return ;
				else
					m_GlobalDate->Mtd_ExitMode = 10;
			}
			m_ipc->IPCMtdSelectCtrl(Selext);
		}

	}
	else
	{
		m_GlobalDate->avtStatus = !TrkStatus;
		signalFeedBack(ACK_avtStatus, 0, 0, 0);
		m_ipc->ipcTrackCtrl(!TrkStatus);
		m_GlobalDate->Mtd_Limit = recv_AutoMtdDate;
		m_GlobalDate->Mtd_Manual_Limit = 0;
#if 1
		if(m_StateManager->isTwo_PlantForm_Mtd() && m_StateManager->isThree_MtdManual())
		{
			struct timeval tmp;
			tmp.tv_sec = 0;
			tmp.tv_usec = 200000;
			select(0, NULL, NULL, NULL, &tmp);
			m_GlobalDate->ImgMtdStat = 1;
			m_GlobalDate->mtdMode = 0;
			usd_API_MTDMode();
		}
#endif
	}
}

void CManager::usd_API_StatusSwitch()
{
	if(m_StateManager->isTwo_PlantForm_Mtd())
	{
			m_GlobalDate->ThreeMode_bak = 1;  		/* When entering the track in mtd mode, record the three-level state*/
			m_StateManager->two_State_PlantForm_Trk();
			m_StateManager->three_State_CtrlBox();
	}
	else
	{
		if(m_StateManager->isTwo_PlantForm_Acq() || m_StateManager->isTwo_PlantForm_SceneTrk())
			m_StateManager->two_State_PlantForm_Trk();
		else if(m_StateManager->isTwo_PlantForm_Trk())
		{
			m_StateManager->two_State_PlantForm_Acq();
				if(m_StateManager->isThree_TrkSearch())
					m_StateManager->Quit_TrkSearch();
				if(m_GlobalDate->ThreeMode_bak == 1)
				{
#if 0
					m_StateManager->three_State_CtrlPtz();
					if(m_GlobalDate->Mtd_ExitMode == Manual)
					{
						m_GlobalDate->ImgMtdStat = m_GlobalDate->mtdMode = 0;
						m_GlobalDate->ThreeMode_bak = 0;
					}
#endif
					m_StateManager->two_State_PlantForm_Mtd();
					m_StateManager->three_State_MtdManual();
					m_GlobalDate->ThreeMode_bak = 0;
				}
		}
	}
}

void CManager::usd_API_IrisAndFocusAndExit()
{
	char sign = m_GlobalDate->EXT_Ctrl[MSGID_INPUT_IrisAndFocusAndExit];
	int up_down = m_GlobalDate->EXT_Ctrl[Cmd_Mesg_AXISY];

	if(up_down == -1)
	{
		m_GlobalDate->m_osd_triangel.dir = up;
		m_GlobalDate->m_osd_triangel.alpha = 255;
		uninit_time ();
	}
	else if(up_down == 1)
	{
		m_GlobalDate->m_osd_triangel.dir = down;
		m_GlobalDate->m_osd_triangel.alpha = 255;
		uninit_time ();
	}
	else
	{
		m_GlobalDate->m_osd_triangel.alpha = 0;
		init_sigaction();
		init_time();
	}

	if(sign == 0)
	{
		m_GlobalDate->m_osd_triangel.alpha = 0;
		up_down = 0;
		MSGAPI_ExtInputCtrl_Iris(up_down);
		MSGAPI_ExtInputCtrl_Focus(up_down);
	}

	m_ipc->IpcIrisAndFocus(&m_GlobalDate->m_osd_triangel, sign);

	switch(sign)
	{
		case iris:
			MSGAPI_ExtInputCtrl_Iris(up_down);
			break;
		case Focus:
			MSGAPI_ExtInputCtrl_Focus(up_down);
			break;
		default:
			break;
	}

}

void CManager::usd_API_TrkSearch()
{
	int sensor = GetIpcAddress(Sensor);
	int TrkStat = GetIpcAddress(Trk);
	if(TrkStat == 0)
	{
		m_GlobalDate->pParam.m_SecTrkStat= m_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_TrkSearch);
		return ;
	}

	iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
	iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);

	m_GlobalDate->pParam.m_SecTrkStat= m_GlobalDate->EXT_Ctrl.at(MSGID_INPUT_TrkSearch);

		if (m_GlobalDate->pParam.m_SecTrkStat==1){
			if( iAxis_X>=0)
				m_valuex =min(32640,iAxis_X);
			else
				m_valuex =max(-32640,iAxis_X);
			if( iAxis_Y>=0)
				m_valuey =min(32400,iAxis_Y);
			else
				m_valuey =max(-32400,iAxis_Y);
			m_GlobalDate->m_selectPara.SecAcqStat=m_GlobalDate->pParam.m_SecTrkStat;
			m_GlobalDate->m_selectPara.ImgPixelX = m_Jos->JosToWinX(m_valuex, sensor);
			m_GlobalDate->m_selectPara.ImgPixelY =	m_Jos->JosToWinY(m_valuey, sensor);
			m_ipc->ipcSecTrkCtrl(&m_GlobalDate->m_selectPara);
		 }else if(m_GlobalDate->pParam.m_SecTrkStat== 0){
			if( iAxis_X>=0)
				m_valuex =min(32640,iAxis_X);
			else
				m_valuex =max(-32640,iAxis_X);
			if( iAxis_Y>=0)
				m_valuey =min(32400,iAxis_Y);
			else
				m_valuey =max(-32400,iAxis_Y);
			m_GlobalDate->m_selectPara.SecAcqStat=m_GlobalDate->pParam.m_SecTrkStat;
			m_GlobalDate->m_selectPara.ImgPixelX = m_Jos->JosToWinX(m_valuex, sensor);
			m_GlobalDate->m_selectPara.ImgPixelY = m_Jos->JosToWinY(m_valuey, sensor);
			m_ipc->ipcSecTrkCtrl(&m_GlobalDate->m_selectPara);
			gettimeofday(&m_GlobalDate->PID_t, NULL);
			m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
			m_GlobalDate->time_stop = 0;
			m_GlobalDate->frame = 0;
		 }
		 else if(m_GlobalDate->pParam.m_SecTrkStat==3){
				iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
				iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);

			m_valuex = iAxis_X;
			m_valuey = iAxis_Y;
			if( iAxis_X<0)
				m_valuex =0;
			else  if(iAxis_X > 1920)
				m_valuex =1920;
			if( iAxis_Y<0)
				m_valuey =0;
			else  if(iAxis_Y > 1080)
				m_valuey =1080;
			m_GlobalDate->m_selectPara.SecAcqStat=1;
			if(video_pal == sensor){
				m_valuex = (int)((float)m_valuex*720.0/1920.0);
				m_valuey = (int)((float)m_valuey*576.0/1080.0);
			}
			m_GlobalDate->m_selectPara.ImgPixelX = m_valuex;
			m_GlobalDate->m_selectPara.ImgPixelY = m_valuey;
			m_ipc->ipcSecTrkCtrl(&m_GlobalDate->m_selectPara);
		}
		 else if (m_GlobalDate->pParam.m_SecTrkStat==4){
				iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
				iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);
			if( iAxis_X<0)
				m_valuex =0;
			else  if(iAxis_X > 1920)
				m_valuex =1920;
			if( iAxis_Y<0)
				m_valuey =0;
			else  if(iAxis_Y > 1080)
				m_valuey =1080;
			m_GlobalDate->m_selectPara.SecAcqStat=0;
			if(video_pal == sensor){
				m_valuex = (int)((float)m_valuex*720.0/1920.0);
				m_valuey = (int)((float)m_valuey*576.0/1080.0);
			}
			m_ipc->ipcSecTrkCtrl(&m_GlobalDate->m_selectPara);
			gettimeofday(&m_GlobalDate->PID_t, NULL);
			m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
			m_GlobalDate->time_stop = 0;
			m_GlobalDate->frame = 0;
		   }
}

void CManager::usd_API_ChooseVideoChannel()
{
	int channel = m_GlobalDate->Host_Ctrl[chooseVideoChannel];
	m_ipc->IPCchooseVideoChannel(channel);
}

void CManager::usd_API_SwitchSensor()
{
	int SensorStat = GetIpcAddress(Sensor);
	if(SensorStat != m_GlobalDate->selectch.idx)
		m_GlobalDate->selectch.idx = SensorStat;
	do{
			m_GlobalDate->selectch.idx++;
			m_GlobalDate->selectch.idx = m_GlobalDate->selectch.idx%5;
	}while(!m_GlobalDate->selectch.validCh[m_GlobalDate->selectch.idx]);
	unsigned char m_eSenserStat = m_GlobalDate->chid_camera = m_GlobalDate->selectch.idx;

	m_ipc->IpcSensorSwitch(m_eSenserStat);

}

void CManager::usd_API_Resolution()
{
	int block = (int)m_GlobalDate->Host_Ctrl[config_Wblock];
	int field = (int)m_GlobalDate->Host_Ctrl[config_Wfield];
	float value = m_GlobalDate->Host_Ctrl[config_Wvalue];
	modifierAVTProfile( block, field, value,NULL);

	signalFeedBack(ACK_config_Write, ACK_config_Wblock, block, field);
}

void CManager::usd_API_PRMBACK()
{
	int status , block;
	while(m_GlobalDate->defConfigBuffer.size()){
		//printf("size = %d \n", m_GlobalDate->defConfigBuffer.size());
		block = m_GlobalDate->defConfigBuffer.at(0);
		if(m_GlobalDate->commode == 1)
			OSA_semWait(&m_GlobalDate->m_semHndl_s,OSA_TIMEOUT_FOREVER);
		else if(m_GlobalDate->commode == 2)
			OSA_semWait(&m_GlobalDate->m_semHndl_socket_s,OSA_TIMEOUT_FOREVER);
		status = paramBackToDefault(block);
		signalFeedBack(ACK_param_todef, ACK_config_Rblock, block, 0);
		m_GlobalDate->defConfigBuffer.erase(m_GlobalDate->defConfigBuffer.begin());
}

}

void CManager::usd_API_AxisMove()
{
	int x = m_GlobalDate->Host_Ctrl[moveAxisX];
	int y = m_GlobalDate->Host_Ctrl[moveAxisY];
	m_ipc->IPCAxisMove(x, y);
}

void CManager::usd_API_Algosdrect()
{
	char tmp = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Algosdrect];
	m_ipc->IPCAlgosdrect(tmp);
}

void CManager::usd_API_word()
{
	m_ipc->IPCOsdWord(m_GlobalDate->recvOsdbuf);

	int block;
	if(m_GlobalDate->recvOsdbuf.osdID < 16)
		block = m_GlobalDate->recvOsdbuf.osdID + 7;
	else
		block = m_GlobalDate->recvOsdbuf.osdID + 13;

	modifierAVTProfile(block, 0, (float)m_GlobalDate->recvOsdbuf.ctrl,NULL);
	modifierAVTProfile(block, 1, (float)m_GlobalDate->recvOsdbuf.posx,NULL);
	modifierAVTProfile(block, 2, (float)m_GlobalDate->recvOsdbuf.posy,NULL);
	modifierAVTProfile(block, 3, 0,(char *)m_GlobalDate->recvOsdbuf.buf);
	modifierAVTProfile(block, 5, (float)m_GlobalDate->recvOsdbuf.color,NULL);
	modifierAVTProfile(block, 6, (float)m_GlobalDate->recvOsdbuf.alpha,NULL);
}

void CManager::IPC_videoName_Pos()
{
	int block, ID;
	m_ipc->ipc_OSDTEXT = m_ipc->getOSDTEXTSharedMem();

	ID = m_GlobalDate->recvOsdbuf.osdID;
	block = m_ipc->ipc_OSDTEXT->osdID[ID] + 13;
	printf("Id = %d , block = %d \n", m_GlobalDate->recvOsdbuf.osdID, block);

	modifierAVTProfile(block, 0, (float)m_ipc->ipc_OSDTEXT->ctrl[ID],NULL);
	modifierAVTProfile(block, 1, (float)m_ipc->ipc_OSDTEXT->posx[ID],NULL);
	modifierAVTProfile(block, 2, (float)m_ipc->ipc_OSDTEXT->posy[ID],NULL);
	modifierAVTProfile(block, 3, 0,(char *)m_ipc->ipc_OSDTEXT->buf[ID]);
	modifierAVTProfile(block, 5, (float)m_ipc->ipc_OSDTEXT->color[ID],NULL);
	modifierAVTProfile(block, 6, (float)m_ipc->ipc_OSDTEXT->alpha[ID],NULL);
	updataProfile();
}

void CManager::IPC_fontSize()
{
	int block = 53;
	int field = 1;
	float value = m_GlobalDate->Host_Ctrl[wordSize];
	modifierAVTProfile( block, field, value,NULL);
	updataProfile();
}

void CManager::IPC_ballbaud()
{
	int chId = m_GlobalDate->chid_camera;
	int block, filed;
	filed = 1;
	switch(chId)
	{
	case 0:
	 block = 52;
	break;
	case 1:
		block = 107;
		break;
	case 2:
		block = 108;
		break;
	case 3:
		block = 109;
		break;
	case 4:
		block = 110;
		break;
	}
	float address = m_GlobalDate->m_uart_params[chId].balladdress;
	float baud = m_GlobalDate->m_uart_params[chId].baudrate;
	modifierAVTProfile( block, filed, baud,NULL);
	modifierAVTProfile( block, filed+5, address,NULL);
	updataProfile();
}

void CManager::switch_speedchid()
{
	m_ptzSpeed.switchChid();
	usd_API_speedchid();
}

void CManager::preset_Mtd()
{
	m_GlobalDate->Mtd_Moitor = 1; //The flag used save default position
	m_ptz->QueryPos();
	int block = 55;
	int field = 0;
	float pan = m_GlobalDate->Mtd_Moitor_X;
	float Tilt = m_GlobalDate->Mtd_Moitor_Y;
	modifierAVTProfile( block, field, pan,NULL);
	modifierAVTProfile( block, field+1, Tilt,NULL);
	updataProfile();

}

void CManager::usd_API_WORDFONT()
{
	char font = m_GlobalDate->Host_Ctrl[wordType];
	m_ipc->IpcWordFont(font);

	int block = 53;
	int field = 0;
	float value = m_GlobalDate->Host_Ctrl[wordType];
	pThis->modifierAVTProfile( block, field, value,NULL);
}

void CManager::usd_API_WORDSIZE()
{
	char size = m_GlobalDate->Host_Ctrl[wordSize];
	m_ipc->IpcWordSize(size);

	int block = 53;
	int field = 1;
	float value = m_GlobalDate->Host_Ctrl[wordSize];
	pThis->modifierAVTProfile( block, field, value,NULL);
}

void CManager::usd_API_speedchid()
{
	int chid = m_GlobalDate->chid_camera;
	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;

	pObj->params.demandMaxX = m_InitParam.demandMaxX[chid];
	pObj->params.demandMaxY = m_InitParam.demandMaxY[chid];
	pObj->params.deadbandX = m_InitParam.deadbandX[chid];
	pObj->params.deadbandY = m_InitParam.deadbandY[chid];

	printf("pObj->params.bleedX = %f \n", pObj->params.bleedX);
	printf("pObj->params.bleedY = %f\n", pObj->params.bleedY);
	printf("pObj->params.demandMaxX = %f \n", pObj->params.demandMaxX);
	printf("pObj->params.demandMaxY = %f\n", pObj->params.demandMaxY);
	printf("pObj->params.deadbandX = %f\n", pObj->params.deadbandX);
	printf("pObj->params.deadbandY = %f\n", pObj->params.deadbandY);
}

void CManager::switch_PIDchid()
{
	int chId = m_GlobalDate->chid_camera;
	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	PlatformFilter_Obj* pPidObjx;
	PlatformFilter_Obj* pPidObjy;
	pPidObjx = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
	pPidObjy = (PlatformFilter_Obj*)pObj->privates.filter[0][1];

	pPidObjx->params.G = m_InitParam.m__cfg_platformFilterParam[chId].G;//kp
	pPidObjx->params.P0= m_InitParam.m__cfg_platformFilterParam[chId].P0;//Ki
	pPidObjx->params.P1= m_InitParam.m__cfg_platformFilterParam[chId].P1;//Kd
	pPidObjx->params.P2 = m_InitParam.m__cfg_platformFilterParam[chId].P2;//k

	pPidObjy->params.G =m_InitParam.m__cfg_platformFilterParam[chId].G2;//Kp
	pPidObjy->params.P0 = m_InitParam.m__cfg_platformFilterParam[chId].P02;//Ki
	pPidObjy->params.P1 = m_InitParam.m__cfg_platformFilterParam[chId].P12;//Kd
	pPidObjy->params.P2 = m_InitParam.m__cfg_platformFilterParam[chId].P22;//K

	pObj->params.Kx = m_InitParam.Kx[chId];
	pObj->params.Ky = m_InitParam.Ky [chId];
	pObj->params.Error_X = m_InitParam.Error_X[chId];
	pObj->params.Error_Y = m_InitParam.Error_Y [chId];
	pObj->params.Time = m_InitParam.Time[chId];
	pObj->params.bleedX = m_InitParam.BleedLimit_X[chId];
	pObj->params.bleedY = m_InitParam.BleedLimit_Y [chId];

}

void CManager::usd_API_queryPos()
{
	m_GlobalDate->sync_pos = 1;
	m_GlobalDate->Mtd_Moitor = 1; //The flag used save default position
	while(m_GlobalDate->sync_pos)
		m_ptz->QueryPos();
}

void CManager::usd_API_WORKMODEWITCH()
{
	switch(m_GlobalDate->workMode)
	{
	case 0x01:
		m_GlobalDate->ImgMtdStat = m_GlobalDate->mtdMode = 0;
		usd_API_MTDMode();
		m_GlobalDate->jos_params.workMode = manual_linkage;
		m_GlobalDate->jos_params.ctrlMode = mouse;
		break;
	case 0x02:
		m_GlobalDate->ImgMtdStat = 1;
		m_GlobalDate->mtdMode = 0;
		usd_API_MTDMode();
		m_GlobalDate->jos_params.workMode = Auto_linkage;
		m_GlobalDate->jos_params.ctrlMode = jos;
		break;
	case 0x03:
		m_GlobalDate->jos_params.workMode = ballctrl;
		if(m_GlobalDate->jos_params.menu == true)
			m_GlobalDate->jos_params.ctrlMode = mouse;
		else
			m_GlobalDate->jos_params.ctrlMode = jos;
		break;
	}
	m_GlobalDate->jos_params.type = workMode;
	usd_API_ctrlParams();
	m_ptz->ptzStop();
}

void CManager::usd_API_ctrlParams()
{
	m_ipc->ipcSendJosParams();
}

void CManager::Observer_Action(int block, int field, float value)
{
	switch(block)
	{
	case 23:
		switch(field)
		{
		case 0:
			m_ipc->ipc_OSD->osdChidIDShow[0] = (int)value;
			m_ipc->IpcConfigOSD();
			break;
		case 2:
			m_ipc->ipc_OSD->osdChidNmaeShow[0] = (int)value;
			m_ipc->IpcConfigOSD();
			break;
		case 3:
				m_GlobalDate->cameraSwitch[0] = value;
			break;
		case 4:
			m_GlobalDate->Host_Ctrl[config_Wblock]	=	23;
			m_GlobalDate->Host_Ctrl[config_Wfield]	=	4;
			m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution] = (int)value;
			usd_API_Resolution();
			break;
		case 5:
			m_GlobalDate->fovmode[0] = value;
			break;
		case 7:
			m_ipc->ipc_OSD->osdBoxShow[0] = (int)value;
			m_ipc->IpcConfigOSD();
			break;
		case 8:
			m_ipc->ipc_OSD->CROSS_draw_show[0] = (int)value;
			m_ipc->IpcConfigOSD();
			break;
		case 9:
			m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[0] = (int)value;
			m_ipc->IpcConfigUTC();
			break;
		}
		break;

		case 58:
			switch(field)
			{
			case 0:
				m_ipc->ipc_OSD->osdChidIDShow[1] = (int)value;
				m_ipc->IpcConfigOSD();
				break;
			case 2:
				m_ipc->ipc_OSD->osdChidNmaeShow[1] = (int)value;
				m_ipc->IpcConfigOSD();
				break;
			case 3:
					m_GlobalDate->cameraSwitch[1] = value;
				break;
			case 4:
				m_GlobalDate->Host_Ctrl[config_Wblock]	=	23;
				m_GlobalDate->Host_Ctrl[config_Wfield]	=	4;
				m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution1] = (int)value;
				usd_API_Resolution();
				break;
			case 5:
				m_GlobalDate->fovmode[1] = value;
				break;
			case 7:
				m_ipc->ipc_OSD->osdBoxShow[1] = value;
				m_ipc->IpcConfigOSD();
				break;
			case 8:
				m_ipc->ipc_OSD->CROSS_draw_show[1] = value;
				m_ipc->IpcConfigOSD();
				break;
			case 9:
				m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[1] = (int)value;
				m_ipc->IpcConfigUTC();
				break;
			}
			break;

			case 65:
				switch(field)
				{
				case 0:
					m_ipc->ipc_OSD->osdChidIDShow[2] = (int)value;
					m_ipc->IpcConfigOSD();
					break;
				case 2:
					m_ipc->ipc_OSD->osdChidNmaeShow[2] = (int)value;
					m_ipc->IpcConfigOSD();
					break;
				case 3:
						m_GlobalDate->cameraSwitch[2] = value;
					break;
				case 4:
					m_GlobalDate->Host_Ctrl[config_Wblock]	=	23;
					m_GlobalDate->Host_Ctrl[config_Wfield]	=	4;
					m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution2] = (int)value;
					usd_API_Resolution();
					break;
				case 5:
					m_GlobalDate->fovmode[2] = value;
					break;
				case 7:
					m_ipc->ipc_OSD->osdBoxShow[2] = value;
					m_ipc->IpcConfigOSD();
					break;
				case 8:
					m_ipc->ipc_OSD->CROSS_draw_show[2] = value;
					m_ipc->IpcConfigOSD();
					break;
				case 9:
					m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[2] = (int)value;
					m_ipc->IpcConfigUTC();
					break;
				}
				break;

				case 72:
					switch(field)
					{
					case 0:
						m_ipc->ipc_OSD->osdChidIDShow[3] = (int)value;
						m_ipc->IpcConfigOSD();
						break;
					case 2:
						m_ipc->ipc_OSD->osdChidNmaeShow[3] = (int)value;
						m_ipc->IpcConfigOSD();
						break;
					case 3:
							m_GlobalDate->cameraSwitch[3] = value;
						break;
					case 4:
						m_GlobalDate->Host_Ctrl[config_Wblock]	=	23;
						m_GlobalDate->Host_Ctrl[config_Wfield]	=	4;
						m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution3] = (int)value;
						usd_API_Resolution();
						break;
					case 5:
						m_GlobalDate->fovmode[3] = value;
						break;
					case 7:
						m_ipc->ipc_OSD->osdBoxShow[3] = value;
						m_ipc->IpcConfigOSD();
						break;
					case 8:
						m_ipc->ipc_OSD->CROSS_draw_show[3] = value;
						m_ipc->IpcConfigOSD();
						break;
					case 9:
						m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[3] = (int)value;
						m_ipc->IpcConfigUTC();
						break;
					}
					break;

					case 79:
						switch(field)
						{
						case 0:
							m_ipc->ipc_OSD->osdChidIDShow[4] = (int)value;
							m_ipc->IpcConfigOSD();
							break;
						case 2:
							m_ipc->ipc_OSD->osdChidNmaeShow[4] = (int)value;
							m_ipc->IpcConfigOSD();
							break;
						case 3:
								m_GlobalDate->cameraSwitch[4] = value;
							break;
						case 4:
							m_GlobalDate->Host_Ctrl[config_Wblock]	=	23;
							m_GlobalDate->Host_Ctrl[config_Wfield]	=	4;
							m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution4] = (int)value;
							usd_API_Resolution();
							break;
						case 5:
							m_GlobalDate->fovmode[4] = value;
							break;
						case 7:
							m_ipc->ipc_OSD->osdBoxShow[4] = value;
							m_ipc->IpcConfigOSD();
							break;
						case 8:
							m_ipc->ipc_OSD->CROSS_draw_show[4] = value;
							m_ipc->IpcConfigOSD();
							break;
						case 9:
							m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[4] = (int)value;
							m_ipc->IpcConfigUTC();
							break;
						}
						break;

						case 25:
							if(!field)
								m_GlobalDate->switchFovLV[0] = value;
							break;
						case 60:
							if(!field)
								m_GlobalDate->switchFovLV[1] = value;
							break;
						case 67:
							if(!field)
								m_GlobalDate->switchFovLV[2] = value;
							break;
						case 74:
							if(!field)
								m_GlobalDate->switchFovLV[3] = value;
							break;
						case 81:
							if(!field)
								m_GlobalDate->switchFovLV[4] = value;
							break;

						case 57:
							if(field == 3)
								m_GlobalDate->continueFovLv[0]= value;
							break;
						case 64:
							if(field == 3)
								m_GlobalDate->continueFovLv[1]= value;
							break;
						case 71:
							if(field == 3)
								m_GlobalDate->continueFovLv[2]= value;
							break;
						case 78:
							if(field == 3)
								m_GlobalDate->continueFovLv[3]= value;
							break;
						case 85:
							if(field == 3)
								m_GlobalDate->continueFovLv[4]= value;
							break;
	}
	switch(block)
	{
	case 23:
	case 58:
	case 65:
	case 72:
	case 79:
		if(field == 5)
			switch_CameraChid();
		break;
	case 25:
	case 60:
	case 67:
	case 74:
	case 81:
		if(!field)
			switch_CameraChid();
		break;
	case 57:
	case 64:
	case 71:
	case 78:
	case 85:
		if(field == 3)
		switch_CameraChid();
		break;
	}
}

void CManager::Observer_Chid()
{
	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;

	switch_speedchid();
	switch_PIDchid();
	switch_CameraChid();
	int x = pObj->params.sensorParams[1].defaultBoresightPos_x;
	int y = pObj->params.sensorParams[1].defaultBoresightPos_y;
	m_ipc->IPCBoresightPosCtrl(x, y);
	m_ipc->IpcConfigUTC();
}

void CManager::switch_CameraChid()
{
	char chid = m_GlobalDate->chid_camera;

	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	m_ipc->ipc_UTC->algOsdRect_Enable = m_GlobalDate->AutoBox[chid];
	pObj->params.sensorParams[1].fFovRatio = viewParam.radio[chid];
	usd_API_switchCameraFovAndLV(chid);
	pObj->privates.fovY = pObj->privates.fovX * pObj->params.sensorParams[1].fFovRatio;
}

void CManager::usd_API_switchCameraFovAndLV(char chid)
{
	const char fixFov = 0;
	const char switchFov = 1;
	const char continueFov = 2;
	char fovmode = m_GlobalDate->fovmode[chid];
	char swFovLV = m_GlobalDate->switchFovLV[chid];
	char conFovLV = m_GlobalDate->continueFovLv[chid];

	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;

	switch(fovmode)
	{
	case fixFov:
		pObj->privates.fovX=viewParam.level_Fov_fix[chid];
		pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_fix_x[chid];
		pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_fix_y[chid];
	break;

	case switchFov:
		switch(swFovLV)
		{
		case 1:
			pObj->privates.fovX=viewParam.level_Fov_switch1[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_switch_x1[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_switch_y1[chid];
			break;

		case 2:
			pObj->privates.fovX=viewParam.level_Fov_switch2[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_switch_x2[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_switch_y2[chid];
			break;

		case 3:
			pObj->privates.fovX=viewParam.level_Fov_switch3[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_switch_x3[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_switch_y3[chid];
			break;

		case 4:
			pObj->privates.fovX=viewParam.level_Fov_switch4[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_switch_x4[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_switch_y4[chid];
			break;

		case 5:
			pObj->privates.fovX=viewParam.level_Fov_switch5[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_switch_x5[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_switch_y5[chid];
			break;

		}
	break;

	case continueFov:
		switch(conFovLV)
		{

		case 1:
			pObj->privates.fovX=viewParam.level_Fov_continue1[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x1[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y1[chid];
			break;

		case 2:
			pObj->privates.fovX=viewParam.level_Fov_continue2[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x2[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y2[chid];
			break;

		case 3:
			pObj->privates.fovX=viewParam.level_Fov_continue3[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x3[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y3[chid];
			break;

		case 4:
			pObj->privates.fovX=viewParam.level_Fov_continue4[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x4[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y4[chid];
			break;

		case 5:
			pObj->privates.fovX=viewParam.level_Fov_continue5[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x5[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y5[chid];
			break;

		case 6:
			pObj->privates.fovX=viewParam.level_Fov_continue6[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x6[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y6[chid];
			break;

		case 7:
			pObj->privates.fovX=viewParam.level_Fov_continue7[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_x = viewParam.Boresightpos_continue_x7[chid];
			pObj->params.sensorParams[1].defaultBoresightPos_y = viewParam.Boresightpos_continue_y7[chid];
			break;

		}
	break;

	}
}

void CManager::thrCreate_LinkageSpeedLoop()
{
	exitSpeedLoop_Linkage = false;
	OSA_thrCreate(&Linkage_SpeedLoop, linkage_SpeedLoop, 0, 0, this);
}

void CManager::thrDelete_LinkageSpeedLoop()
{
	exitSpeedLoop_Linkage = true;
	OSA_semSignal(&m_GlobalDate->m_semHndl_automtd);
	OSA_thrDelete(&Linkage_SpeedLoop);
}

void CManager::usd_API_MTDMode()
{
	int TrkStat = GetIpcAddress(Trk);
	if(m_GlobalDate->ImgMtdStat)
	{
		m_StateManager->two_State_PlantForm_Mtd();
		if(m_GlobalDate->mtdMode == 0)
		{
			m_GlobalDate->mtdconfig.preset = 0;
			m_GlobalDate->Mtd_ExitMode = Manual;
			m_StateManager->three_State_MtdManual();
			if(TrkStat)
			{
				m_GlobalDate->ImgMtdStat = 0;
				m_StateManager->two_State_PlantForm_Mtd();
				usd_API_Track();
			}
			pThis->dtimer.stopTimer(pThis->swtarget_id);
			m_GlobalDate->MtdAutoLoop = false;
			AutoTimer = true;
		}
		else
		{
			m_GlobalDate->Mtd_ExitMode = Auto;
			m_GlobalDate->mtdconfig.preset = 1;

			if(TrkStat)
			{
				m_GlobalDate->ImgMtdStat = 0;
				m_ipc->ipcTrackCtrl(0);
				m_GlobalDate->Mtd_Limit = recv_AutoMtdDate;
				m_StateManager->two_State_PlantForm_Mtd();
			}
			else
			{
				m_StateManager->three_State_MtdAuto();
		if(exitSpeedLoop_Linkage)
			thrCreate_LinkageSpeedLoop();
		m_GlobalDate->MtdAutoLoop = true;
			}

		}
		AVTSpeedReset(1, 0);
		TrkStat = GetIpcAddress(Trk);
		if(m_GlobalDate->mtdMode && TrkStat)
		{
			m_GlobalDate->ImgMtdStat = 0;
			int TrkStat2;
			const char trk = 1;
			const char acq = 0;
			int stat = trk;

			for(; stat ==trk;)
			{
				TrkStat2 = GetIpcAddress(Trk);
				if(!TrkStat2)
					stat = acq;
			}

			if(!TrkStat2)
			{
				m_ipc->IPCMtdSwitch(m_GlobalDate->ImgMtdStat, m_GlobalDate->mtdMode);
				printf("first exit trk,then open mtd_auto\n");
			}
		}
		else
			m_ipc->IPCMtdSwitch(m_GlobalDate->ImgMtdStat, m_GlobalDate->mtdMode);
	}
	else
	{
		m_ipc->IPCMtdSwitch(m_GlobalDate->ImgMtdStat, m_GlobalDate->mtdMode);
		pThis->dtimer.stopTimer(pThis->swtarget_id);
		m_GlobalDate->MtdAutoLoop = false;
		AutoTimer = true;
	}

	if(!m_GlobalDate->ImgMtdStat)
	{
		m_StateManager->two_State_PlantForm_Acq();
		m_StateManager->three_State_CtrlPtz();
		m_GlobalDate->Mtd_ExitMode = 10;
	}

	for(int i = 0; i < 2; i++)
		m_ptz->ptzStop();
}

static int curOutput;
void CManager::usd_API_IPC_TRACKCTRL()
{
	int TrkStat;
	struct timeval tmp;
		TrkStat = GetIpcAddress(Trk);
		/*target lost Auto exit */
	m_GlobalDate->avtStatus = TrkStat;
	signalFeedBack(ACK_avtStatus, 0, 0, 0);

		if(TrkStat == 3)
		{
			printf(" Target  is  LOST   \n ");
			usd_API_StatusSwitch();
			m_StateManager->BoxCtrl_IPC(0);

			if(m_GlobalDate->Mtd_ExitMode == Auto)
			{
				if(m_GlobalDate->mtdconfig.preset == 1)
				{
					usd_API_Track();
					tmp.tv_sec = 0;
					tmp.tv_usec = 50000;
					select(0, NULL, NULL, NULL, &tmp);
					m_ptz->ptzSetPos(m_GlobalDate->Mtd_Moitor_X, m_GlobalDate->Mtd_Moitor_Y);
					printf("target lost waiting reset\n");
					m_GlobalDate->Mtd_Limit = recv_AutoMtdDate;
					m_ptz->ptzMove(0, 0);

					tmp.tv_sec = 1;
					tmp.tv_usec = 0;
					select(0, NULL, NULL, NULL, &tmp);
					m_ipc->IPCMtdSwitch(m_GlobalDate->ImgMtdStat, m_GlobalDate->mtdMode);
				}
			}
			else
				usd_API_Track();
		}
		m_pltInput.fTargetBoresightErrorX=m_ipc->trackposx;
		m_pltInput.fTargetBoresightErrorY=m_ipc->trackposy;
    	m_pltInput.iTrkAlgState= TrkStat;
    	m_Platform->PlatformCtrl_TrackerInput(m_plt, &m_pltInput);
    	m_Platform->PlatformCtrl_TrackerOutput(m_plt, &m_pltOutput);

		if(m_ptz != NULL)
		{
			m_ptz->m_iSetPanSpeed = m_ptzSpeed.GetPanSpeed((int)m_pltOutput.fPlatformDemandX);
			m_ptz->m_iSetTiltSpeed = m_ptzSpeed.GetTiltSpeed((int)m_pltOutput.fPlatformDemandY);
			curOutput++;
		}
		if(curOutput == 30)
		{
			curOutputParams();
			curOutput = 0;
		}
}

void CManager::reset_status_AutoMtd()
{
	if(m_GlobalDate->ImgMtdStat == 1)
	{
		m_StateManager->two_State_PlantForm_Mtd();
		if(m_GlobalDate->mtdMode == 1)
		{
			m_StateManager->three_State_MtdAuto();
			m_GlobalDate->Mtd_ExitMode = Auto;
		}
		else
		{
			m_StateManager->three_State_MtdManual();
			m_GlobalDate->Mtd_ExitMode = Manual;
		}
	}
	else
	{
		m_StateManager->two_State_PlantForm_Acq();
		m_StateManager->three_State_CtrlPtz();
	}
}

void CManager::refreshPtzParam()
{
	m_GlobalDate->sync_zoom = 1;
	while(m_GlobalDate->sync_zoom)
	{
		//m_ptz->QueryPos();
		m_ptz->QueryZoom();
	}
#if 0
		int pan = pThis->m_ptz->m_iPanPos;
		int Tilt = pThis->m_ptz->m_iTiltPos;
		int zoomValue =  pThis->m_ptz->m_iZoomPos;
		m_ipc->refreshPtzParam(pan, Tilt, zoomValue);
#endif
}
//
void CManager::ExtInputCtrl_AcqBoxPos()
{
	int TrkStat = GetIpcAddress(Trk);
	int sensor = GetIpcAddress(Sensor);

	iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
	iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);

	static int iAcqPos_x, iAcqPos_y;

		if( iAxis_X >=0)
			iAcqPos_x =min(32640,iAxis_X);
		else
			iAcqPos_x =max(-32640,iAxis_X);

		if( iAxis_Y >=0)
			iAcqPos_y =min(32400,iAxis_Y);
		else
			iAcqPos_y =max(-32400,iAxis_Y);

		m_GlobalDate->m_acqBox_Pos.AcqStat = m_StateManager->BOX_ipc_Interface();
		m_GlobalDate->m_acqBox_Pos.BoresightPos_x = m_Jos->JosToWinX(iAcqPos_x, sensor);
		m_GlobalDate->m_acqBox_Pos.BoresightPos_y = m_Jos->JosToWinY(iAcqPos_y, sensor);

		if(m_GlobalDate->m_acqBox_Pos.AcqStat == 0)
		{
			if((video_gaoqing0 == sensor)||(video_gaoqing == sensor)||(video_gaoqing2 == sensor)||(video_gaoqing3 == sensor))
			{
				m_GlobalDate->m_acqBox_Pos.BoresightPos_x = 960;
				m_GlobalDate->m_acqBox_Pos.BoresightPos_y = 540;
			}
			else if(video_pal == sensor)
			{
				m_GlobalDate->m_acqBox_Pos.BoresightPos_x = 360;
				m_GlobalDate->m_acqBox_Pos.BoresightPos_y = 288;
			}
		}

		if(TrkStat != 0)
			return ;
		gettimeofday(&m_GlobalDate->PID_t, NULL);
		m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
		m_GlobalDate->time_stop = 0;
		m_ipc->ipcAcqBoxCtrl(&m_GlobalDate->m_acqBox_Pos);

}

void CManager::ExtInputCtrl_AXIS()
{
	int TrkStat = GetIpcAddress(Trk);
	static int ipc_x, ipc_y;

	if(TrkStat == 0)
	{
		ipc_x = m_GlobalDate->ipc_mouseptz.mptzx;
		ipc_y = m_GlobalDate->ipc_mouseptz.mptzy;

		iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
		iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);
#if 0
	m_Platform->PlatformCtrl_VirtualInput(m_plt, DevUsr_AcqJoystickXInput, iAxis_X/jos_value);
	m_Platform->PlatformCtrl_VirtualInput(m_plt, DevUsr_AimpointRefineXInput, iAxis_X/jos_value);
	m_Platform->PlatformCtrl_VirtualInput(m_plt, DevUsr_AcqJoystickYInput, iAxis_Y/jos_value);
	m_Platform->PlatformCtrl_VirtualInput(m_plt, DevUsr_AimpointRefineYInput, iAxis_Y/jos_value);

     	 m_pltInput.iTrkAlgState= TrkStat;

     	if(ipc_x == 0 && ipc_y == 0){
     	 m_Platform->PlatformCtrl_TrackerInput(m_plt, &m_pltInput);
     	 m_Platform->PlatformCtrl_TrackerOutput(m_plt, &m_pltOutput);
     	}
#endif
		if(m_ptz != NULL){
			if(ipc_x == 0 && ipc_y == 0 ){
				m_ptz->m_iSetPanSpeed = iAxis_X;
				m_ptz->m_iSetTiltSpeed = iAxis_Y;
			}
			else{
				if(TrkStat == 0){
						m_ptz->m_iSetPanSpeed = ipc_x / 10;
						m_ptz->m_iSetTiltSpeed = -ipc_y / 8;
				}
			}
		}
	}
}

void CManager::ExtInputCtrl_SendSeaTrkPos(int sensor)
{
	iAxis_X = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX);
	iAxis_Y = m_GlobalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY);

	if (m_GlobalDate->pParam.m_SecTrkStat  == 1){
		if( iAxis_X>=0)
		    m_valuex =min(32640,iAxis_X);
		    else  m_valuex =max(-32640,iAxis_X);
		if( iAxis_Y>=0)
		    m_valuey =min(32400,iAxis_Y);
		    else  m_valuey =max(-32400,iAxis_Y);
		m_GlobalDate->m_selectPara.SecAcqStat=m_GlobalDate->pParam.m_SecTrkStat;
		m_GlobalDate->m_selectPara.ImgPixelX = m_Jos->JosToWinX(m_valuex, sensor);
		m_GlobalDate->m_selectPara.ImgPixelY=	m_Jos->JosToWinY(m_valuey, sensor);
		m_ipc->ipcSecTrkCtrl(&m_GlobalDate->m_selectPara);
		gettimeofday(&m_GlobalDate->PID_t, NULL);
		m_GlobalDate->time_start = m_GlobalDate->PID_t.tv_sec;
		m_GlobalDate->time_stop = 0;
		m_GlobalDate->frame = 0;
	 }
}

int CManager::readMtdDetectArea()
{
	string MtdArea;
	int configId_Max = MtdAreaNum;
	char  detectArea[30] = "detectArea_";
	MtdArea = "MtdDetectArea.yml";
	FILE *fp = fopen(MtdArea.c_str(), "rt");

	if(fp != NULL){
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len < 10)
			return  -1;
		else{
			FileStorage fr(MtdArea, FileStorage::READ);
			if(fr.isOpened()){
				for(int i=0; i<configId_Max; i++){
					sprintf(detectArea, "detectArea_%d", i);
					DetectArea_value[i] = (float)fr[detectArea];
					printf("DetectArea_value[%d] = %f \n", i, DetectArea_value[i]);
				}
			}
		}
	}
	return 0;
}


int CManager::setMtdDetectArea()
{
	string MtdArea;
	int configId_Max = MtdAreaNum;
	char  detectArea[30] = "detectArea_";
	MtdArea = "MtdDetectArea.yml";
	FILE *fp = fopen(MtdArea.c_str(), "rt");

	if(fp != NULL){
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len < 10)
			return  -1;
		else{
			FileStorage fr(MtdArea, FileStorage::WRITE);
			if(fr.isOpened()){
				for(int i=0; i<configId_Max; i++){
					sprintf(detectArea, "detectArea_%d", i);
					fr << detectArea << DetectArea_value[i];

				}
			}
		}
	}
	return 0;
}

int  CManager::configAvtFromFile()
{
	m_ipc->ipc_OSD = m_ipc->getOSDSharedMem();
	m_ipc->ipc_UTC = m_ipc->getUTCSharedMem();
	m_ipc->ipc_OSDTEXT = m_ipc->getOSDTEXTSharedMem();
	m_ipc->ipc_status = m_ipc->getAvtStatSharedMem();
	m_ipc->pMtd = &(m_ipc->cmd_mtd_config);

	string cfgAvtFile;
	int configId_Max = profileNum;
	char  cfg_avt[30] = "cfg_avt_";
	cfgAvtFile = "Profile.yml";
	FILE *fp = fopen(cfgAvtFile.c_str(), "rt");

	if(fp != NULL){
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len < 10)
			return  -1;
		else{
			FileStorage fr(cfgAvtFile, FileStorage::READ);
			if(fr.isOpened()){
				for(int i=0; i<configId_Max; i++){
					sprintf(cfg_avt, "cfg_avt_%d", i);
					cfg_value[i] = (float)fr[cfg_avt];
					//printf(" read cfg [%d] %f \n", i, cfg_value[i]);
				}

				{
					m_GlobalDate->default_workMode = (int)fr["cfg_avt_1920"];
				}

				{//jos
					m_InitParam.joystickRateDemandParam.fDeadband=(float)fr["cfg_avt_1"];
					m_InitParam.joystickRateDemandParam.fCutpoint1=(float)fr["cfg_avt_2"];
					m_InitParam.joystickRateDemandParam.fInputGain_X1=(float)fr["cfg_avt_3"];
					m_InitParam.joystickRateDemandParam.fInputGain_Y1=(float)fr["cfg_avt_4"];
					m_InitParam.joystickRateDemandParam.fCutpoint2=(float)fr["cfg_avt_5"];
					m_InitParam.joystickRateDemandParam.fInputGain_X2=(float)fr["cfg_avt_6"];
					m_InitParam.joystickRateDemandParam.fInputGain_Y2=(float)fr["cfg_avt_7"];

				}

				{//pid
				//通道１
					m_InitParam.m__cfg_platformFilterParam[0].G = (float)fr["cfg_avt_17"];//Kp
					m_InitParam.m__cfg_platformFilterParam[0].P0 = (float)fr["cfg_avt_18"];//Ki
					m_InitParam.m__cfg_platformFilterParam[0].P1 = (float)fr["cfg_avt_19"];//Kd
					m_InitParam.m__cfg_platformFilterParam[0].P2 = (float)fr["cfg_avt_20"];//K
					m_InitParam.m__cfg_platformFilterParam[0].G2 = (float)fr["cfg_avt_21"];//Kp
					m_InitParam.m__cfg_platformFilterParam[0].P02 = (float)fr["cfg_avt_22"];//Ki
					m_InitParam.m__cfg_platformFilterParam[0].P12 = (float)fr["cfg_avt_23"];//Kd
					m_InitParam.m__cfg_platformFilterParam[0].P22 = (float)fr["cfg_avt_24"];//K
				//通道2
					m_InitParam.m__cfg_platformFilterParam[1].G = (float)fr["cfg_avt_1441"];//Kp
					m_InitParam.m__cfg_platformFilterParam[1].P0 = (float)fr["cfg_avt_1442"];//Ki
					m_InitParam.m__cfg_platformFilterParam[1].P1 = (float)fr["cfg_avt_1443"];//Kd
					m_InitParam.m__cfg_platformFilterParam[1].P2 = (float)fr["cfg_avt_1444"];//K
					m_InitParam.m__cfg_platformFilterParam[1].G2 = (float)fr["cfg_avt_1445"];//Kp
					m_InitParam.m__cfg_platformFilterParam[1].P02 = (float)fr["cfg_avt_1446"];//Ki
					m_InitParam.m__cfg_platformFilterParam[1].P12 = (float)fr["cfg_avt_1447"];//Kd
					m_InitParam.m__cfg_platformFilterParam[1].P22 = (float)fr["cfg_avt_1448"];//K

				//通道3
					m_InitParam.m__cfg_platformFilterParam[2].G = (float)fr["cfg_avt_1473"];//Kp
					m_InitParam.m__cfg_platformFilterParam[2].P0 = (float)fr["cfg_avt_1474"];//Ki
					m_InitParam.m__cfg_platformFilterParam[2].P1 = (float)fr["cfg_avt_1475"];//Kd
					m_InitParam.m__cfg_platformFilterParam[2].P2 = (float)fr["cfg_avt_1476"];//K
					m_InitParam.m__cfg_platformFilterParam[2].G2 = (float)fr["cfg_avt_1477"];//Kp
					m_InitParam.m__cfg_platformFilterParam[2].P02 = (float)fr["cfg_avt_1478"];//Ki
					m_InitParam.m__cfg_platformFilterParam[2].P12 = (float)fr["cfg_avt_1479"];//Kd
					m_InitParam.m__cfg_platformFilterParam[2].P22 = (float)fr["cfg_avt_1480"];//K
				//通道4
					m_InitParam.m__cfg_platformFilterParam[3].G = (float)fr["cfg_avt_1505"];//Kp
					m_InitParam.m__cfg_platformFilterParam[3].P0 = (float)fr["cfg_avt_1506"];//Ki
					m_InitParam.m__cfg_platformFilterParam[3].P1 = (float)fr["cfg_avt_1507"];//Kd
				    m_InitParam.m__cfg_platformFilterParam[3].P2 = (float)fr["cfg_avt_1508"];//K
					m_InitParam.m__cfg_platformFilterParam[3].G2 = (float)fr["cfg_avt_1509"];//Kp
					m_InitParam.m__cfg_platformFilterParam[3].P02 = (float)fr["cfg_avt_1510"];//Ki
					m_InitParam.m__cfg_platformFilterParam[3].P12 = (float)fr["cfg_avt_1511"];//Kd
					m_InitParam.m__cfg_platformFilterParam[3].P22 = (float)fr["cfg_avt_1512"];//K

				//通道5
					m_InitParam.m__cfg_platformFilterParam[4].G = (float)fr["cfg_avt_1537"];//Kp
					m_InitParam.m__cfg_platformFilterParam[4].P0 = (float)fr["cfg_avt_1538"];//Ki
					m_InitParam.m__cfg_platformFilterParam[4].P1 = (float)fr["cfg_avt_1539"];//Kd
				    m_InitParam.m__cfg_platformFilterParam[4].P2 = (float)fr["cfg_avt_1540"];//K
					m_InitParam.m__cfg_platformFilterParam[4].G2 = (float)fr["cfg_avt_1541"];//Kp
					m_InitParam.m__cfg_platformFilterParam[4].P02 = (float)fr["cfg_avt_1542"];//Ki
					m_InitParam.m__cfg_platformFilterParam[4].P12 = (float)fr["cfg_avt_1543"];//Kd
				    m_InitParam.m__cfg_platformFilterParam[4].P22 = (float)fr["cfg_avt_1544"];//K
				}

				{
					//m_ptzSpeed.curve = (float)fr["cfg_avt_39"]; //new
					m_InitParam.acqOutputType = (int)fr["cfg_avt_40"];//....
					m_InitParam.OutputSwitch = (float)fr["cfg_avt_46"];//new
					m_InitParam.OutputPort= (float)fr["cfg_avt_47"];//new

					//PID比例调节器
					//通道１
					m_InitParam.Kx[0] = (int)fr["cfg_avt_752"];
					m_InitParam.Ky [0]= (int)fr["cfg_avt_753"];
					m_InitParam.Error_X[0] = (float)fr["cfg_avt_754"];
					m_InitParam.Error_Y [0]= (float)fr["cfg_avt_755"];
					m_InitParam.Time[0] = (float)fr["cfg_avt_756"];
					m_InitParam.BleedLimit_X[0] = (float)fr["cfg_avt_757"];
					m_InitParam.BleedLimit_Y [0]= (float)fr["cfg_avt_758"];

					//通道2
					m_InitParam.Kx[1] = (int)fr["cfg_avt_1456"];
					m_InitParam.Ky [1]= (int)fr["cfg_avt_1457"];
					m_InitParam.Error_X[1] = (float)fr["cfg_avt_1458"];
					m_InitParam.Error_Y [1]= (float)fr["cfg_avt_1459"];
					m_InitParam.Time [1]= (float)fr["cfg_avt_1460"];
					m_InitParam.BleedLimit_X[1] = (float)fr["cfg_avt_1461"];
					m_InitParam.BleedLimit_Y[1] = (float)fr["cfg_avt_1462"];
					//通道3
					m_InitParam.Kx[2] = (int)fr["cfg_avt_1488"];
					m_InitParam.Ky [2]= (int)fr["cfg_avt_1489"];
					m_InitParam.Error_X[2] = (float)fr["cfg_avt_1490"];
					m_InitParam.Error_Y [2]= (float)fr["cfg_avt_1491"];
					m_InitParam.Time [2]= (float)fr["cfg_avt_1492"];
					m_InitParam.BleedLimit_X[2] = (float)fr["cfg_avt_1493"];
					m_InitParam.BleedLimit_Y[2] = (float)fr["cfg_avt_1494"];

					//通道4
					m_InitParam.Kx[3] = (int)fr["cfg_avt_1520"];
					m_InitParam.Ky [3]= (int)fr["cfg_avt_1521"];
					m_InitParam.Error_X[3] = (float)fr["cfg_avt_1522"];
					m_InitParam.Error_Y [3]= (float)fr["cfg_avt_1523"];
					m_InitParam.Time [3]= (float)fr["cfg_avt_1524"];
					m_InitParam.BleedLimit_X[3] = (float)fr["cfg_avt_1525"];
					m_InitParam.BleedLimit_Y[3] = (float)fr["cfg_avt_1526"];
					//通道5
					m_InitParam.Kx[4] = (int)fr["cfg_avt_1552"];
					m_InitParam.Ky [4]= (int)fr["cfg_avt_1553"];
					m_InitParam.Error_X[4] = (float)fr["cfg_avt_1554"];
					m_InitParam.Error_Y [4]= (float)fr["cfg_avt_1555"];
					m_InitParam.Time [4]= (float)fr["cfg_avt_1556"];
					m_InitParam.BleedLimit_X[4] = (float)fr["cfg_avt_1557"];
					m_InitParam.BleedLimit_Y[4] = (float)fr["cfg_avt_1558"];
				}

				{
					m_ipc->ipc_UTC->occlusion_thred = (float)fr["cfg_avt_48"];//9--0
					m_ipc->ipc_UTC->retry_acq_thred = (float)fr["cfg_avt_49"];
					m_ipc->ipc_UTC->up_factor = (float)fr["cfg_avt_50"];
					m_ipc->ipc_UTC->res_distance = (int)fr["cfg_avt_51"];
					m_ipc->ipc_UTC->res_area = (int)fr["cfg_avt_52"];
					m_ipc->ipc_UTC->gapframe = (int)fr["cfg_avt_53"];
					m_ipc->ipc_UTC->enhEnable = (int)fr["cfg_avt_54"];
					m_ipc->ipc_UTC->cliplimit = (float)fr["cfg_avt_55"];
					m_ipc->ipc_UTC->dictEnable = (int)fr["cfg_avt_56"];
					m_ipc->ipc_UTC->moveX = (int)fr["cfg_avt_57"];
					m_ipc->ipc_UTC->moveY = (int)fr["cfg_avt_58"];
					m_ipc->ipc_UTC->moveX2 = (int)fr["cfg_avt_59"];
					m_ipc->ipc_UTC->moveY2 = (int)fr["cfg_avt_60"];
					m_ipc->ipc_UTC->segPixelX = (int)fr["cfg_avt_61"];
					m_ipc->ipc_UTC->segPixelY = (int)fr["cfg_avt_62"];
					//m_ipc->ipc_UTC->algOsdRect_Enable = (int)fr["cfg_avt_63"];  //9--15

					m_ipc->ipc_UTC->ScalerLarge = (int)fr["cfg_avt_64"];//10--0
					m_ipc->ipc_UTC->ScalerMid = (int)fr["cfg_avt_65"];
					m_ipc->ipc_UTC->ScalerSmall = (int)fr["cfg_avt_66"];
					m_ipc->ipc_UTC->Scatter = (int)fr["cfg_avt_67"];
					m_ipc->ipc_UTC->ratio = (float)fr["cfg_avt_68"];
					m_ipc->ipc_UTC->FilterEnable = (int)fr["cfg_avt_69"];
					m_ipc->ipc_UTC->BigSecEnable = (int)fr["cfg_avt_70"];
					m_ipc->ipc_UTC->SalientThred = (int)fr["cfg_avt_71"];
					m_ipc->ipc_UTC->ScalerEnable = (int)fr["cfg_avt_72"];
					m_ipc->ipc_UTC->DynamicRatioEnable = (int)fr["cfg_avt_73"];
					m_ipc->ipc_UTC->acqSize_width = (int)fr["cfg_avt_74"];
					m_ipc->ipc_UTC->acqSize_height = (int)fr["cfg_avt_75"];
					m_ipc->ipc_UTC->TrkAim43_Enable = (int)fr["cfg_avt_76"];
					m_ipc->ipc_UTC->SceneMVEnable = (int)fr["cfg_avt_77"];
					m_ipc->ipc_UTC->BackTrackEnable = (int)fr["cfg_avt_78"];
					m_ipc->ipc_UTC->bAveTrkPos = (int)fr["cfg_avt_79"]; //10--15

					m_ipc->ipc_UTC->fTau = (float)fr["cfg_avt_80"]; //11--0
					m_ipc->ipc_UTC->buildFrms = (int)fr["cfg_avt_81"];
					m_ipc->ipc_UTC->LostFrmThred = (int)fr["cfg_avt_82"];
					m_ipc->ipc_UTC->histMvThred = (float)fr["cfg_avt_83"];
					m_ipc->ipc_UTC->detectFrms = (int)fr["cfg_avt_84"];
					m_ipc->ipc_UTC->stillFrms = (int)fr["cfg_avt_85"];
					m_ipc->ipc_UTC->stillThred = (float)fr["cfg_avt_86"];
					m_ipc->ipc_UTC->bKalmanFilter = (int)fr["cfg_avt_87"];
					m_ipc->ipc_UTC->xMVThred = (float)fr["cfg_avt_88"];
					m_ipc->ipc_UTC->yMVThred = (float)fr["cfg_avt_89"];
					m_ipc->ipc_UTC->xStillThred = (float)fr["cfg_avt_90"];
					m_ipc->ipc_UTC->yStillThred = (float)fr["cfg_avt_91"];
					m_ipc->ipc_UTC->slopeThred = (float)fr["cfg_avt_92"];
					m_ipc->ipc_UTC->kalmanHistThred = (float)fr["cfg_avt_93"];
					m_ipc->ipc_UTC->kalmanCoefQ = (float)fr["cfg_avt_94"];
					m_ipc->ipc_UTC->kalmanCoefR = (float)fr["cfg_avt_95"];
				}

				{
					OSD_text* rOSD;
					rOSD = &m_OSD;

					rOSD->OSD_text_show[0] = (int)fr["cfg_avt_96"];
					rOSD->OSD_text_positionX[0]= (int)fr["cfg_avt_97"];//new
					rOSD->OSD_text_positionY[0]= (int)fr["cfg_avt_98"];//new
					rOSD->OSD_text_content[0]= (string)fr["cfg_avt_99"];//new
					rOSD->OSD_text_color[0]= (int)fr["cfg_avt_101"];
					rOSD->OSD_text_alpha[0] = (int)fr["cfg_avt_102"];

					m_ipc->ipc_OSDTEXT->ctrl[0] =(int)fr["cfg_avt_96"];
					m_ipc->ipc_OSDTEXT->posx[0] = (int)fr["cfg_avt_97"];
					m_ipc->ipc_OSDTEXT->posy[0] = (int)fr["cfg_avt_98"];
					string myrStr0 = (string)fr["cfg_avt_99"];
					myrStr0.copy((char *)m_ipc->ipc_OSDTEXT->buf[0],myrStr0.length(),0);
					m_ipc->ipc_OSDTEXT->color[0] = (int)fr["cfg_avt_101"];
					m_ipc->ipc_OSDTEXT->alpha[0] = (int)fr["cfg_avt_102"];
					m_ipc->ipc_OSDTEXT->osdID[0] = 0;

					rOSD->OSD_text_show[1] = (int)fr["cfg_avt_112"];
					rOSD->OSD_text_positionX[1]= (int)fr["cfg_avt_113"];//new
					rOSD->OSD_text_positionY[1]= (int)fr["cfg_avt_114"];//new
					rOSD->OSD_text_content[1]= (string)fr["cfg_avt_115"];//new
					rOSD->OSD_text_color[1]= (int)fr["cfg_avt_117"];
					rOSD->OSD_text_alpha[1] = (int)fr["cfg_avt_118"];

					m_ipc->ipc_OSDTEXT->ctrl[1] =(int)fr["cfg_avt_112"];
					m_ipc->ipc_OSDTEXT->posx[1] = (int)fr["cfg_avt_113"];
					m_ipc->ipc_OSDTEXT->posy[1] = (int)fr["cfg_avt_114"];
					string myrStr1 = (string)fr["cfg_avt_115"];
					myrStr1.copy((char *)m_ipc->ipc_OSDTEXT->buf[1],myrStr1.length(),0);
					m_ipc->ipc_OSDTEXT->color[1] = (int)fr["cfg_avt_117"];
					m_ipc->ipc_OSDTEXT->alpha[1] = (int)fr["cfg_avt_118"];
					m_ipc->ipc_OSDTEXT->osdID[1] = 1;
					
					rOSD->OSD_text_show[2] = (int)fr["cfg_avt_128"];
					rOSD->OSD_text_positionX[2]= (int)fr["cfg_avt_129"];//new
					rOSD->OSD_text_positionY[2]= (int)fr["cfg_avt_130"];//new
					rOSD->OSD_text_content[2]=(string) fr["cfg_avt_131"];//new
					rOSD->OSD_text_color[2]= (int)fr["cfg_avt_133"];
					rOSD->OSD_text_alpha[2] = (int)fr["cfg_avt_134"];

					m_ipc->ipc_OSDTEXT->ctrl[2] =(int)fr["cfg_avt_128"];
					m_ipc->ipc_OSDTEXT->posx[2] = (int)fr["cfg_avt_129"];
					m_ipc->ipc_OSDTEXT->posy[2] = (int)fr["cfg_avt_130"];
					string myrStr2 = (string)fr["cfg_avt_131"];
					myrStr2.copy((char *)m_ipc->ipc_OSDTEXT->buf[2],myrStr2.length(),0);
					m_ipc->ipc_OSDTEXT->color[2] = (int)fr["cfg_avt_133"];
					m_ipc->ipc_OSDTEXT->alpha[2] = (int)fr["cfg_avt_134"];
					m_ipc->ipc_OSDTEXT->osdID[2] = 2;
					
					rOSD->OSD_text_show[3] = (int)fr["cfg_avt_144"];
					rOSD->OSD_text_positionX[3]= (int)fr["cfg_avt_145"];//new
					rOSD->OSD_text_positionY[3]= (int)fr["cfg_avt_146"];//new
					rOSD->OSD_text_content[3]=(string) fr["cfg_avt_147"];//new
					rOSD->OSD_text_color[3]= (int)fr["cfg_avt_149"];
					rOSD->OSD_text_alpha[3] = (int)fr["cfg_avt_150"];

					m_ipc->ipc_OSDTEXT->ctrl[3] =(int)fr["cfg_avt_144"];
					m_ipc->ipc_OSDTEXT->posx[3] = (int)fr["cfg_avt_145"];
					m_ipc->ipc_OSDTEXT->posy[3] = (int)fr["cfg_avt_146"];
					string myrStr3 = (string)fr["cfg_avt_147"];
					myrStr3.copy((char *)m_ipc->ipc_OSDTEXT->buf[3],myrStr3.length(),0);
					m_ipc->ipc_OSDTEXT->color[3] = (int)fr["cfg_avt_149"];
					m_ipc->ipc_OSDTEXT->alpha[3] = (int)fr["cfg_avt_150"];
					m_ipc->ipc_OSDTEXT->osdID[3] = 3;
					
					rOSD->OSD_text_show[4] = (int)fr["cfg_avt_160"];
					rOSD->OSD_text_positionX[4]= (int)fr["cfg_avt_161"];//new
					rOSD->OSD_text_positionY[4]= (int)fr["cfg_avt_162"];//new
					rOSD->OSD_text_content[4]= (string)fr["cfg_avt_163"];//new
					rOSD->OSD_text_color[4]= (int)fr["cfg_avt_165"];
					rOSD->OSD_text_alpha[4] = (int)fr["cfg_avt_166"];

					m_ipc->ipc_OSDTEXT->ctrl[4] =(int)fr["cfg_avt_160"];
					m_ipc->ipc_OSDTEXT->posx[4] = (int)fr["cfg_avt_161"];
					m_ipc->ipc_OSDTEXT->posy[4] = (int)fr["cfg_avt_162"];
					string myrStr4 = (string)fr["cfg_avt_163"];
					myrStr4.copy((char *)m_ipc->ipc_OSDTEXT->buf[4],myrStr4.length(),0);
					m_ipc->ipc_OSDTEXT->color[4] = (int)fr["cfg_avt_165"];
					m_ipc->ipc_OSDTEXT->alpha[4] = (int)fr["cfg_avt_166"];
					m_ipc->ipc_OSDTEXT->osdID[4] = 4;
					
					rOSD->OSD_text_show[5] = (int)fr["cfg_avt_176"];
					rOSD->OSD_text_positionX[5]= (int)fr["cfg_avt_177"];//new
					rOSD->OSD_text_positionY[5]= (int)fr["cfg_avt_178"];//new
					rOSD->OSD_text_content[5]=(string) fr["cfg_avt_179"];//new
					rOSD->OSD_text_color[5]= (int)fr["cfg_avt_181"];
					rOSD->OSD_text_alpha[5] = (int)fr["cfg_avt_182"];

					m_ipc->ipc_OSDTEXT->ctrl[5] =(int)fr["cfg_avt_176"];
					m_ipc->ipc_OSDTEXT->posx[5] = (int)fr["cfg_avt_177"];
					m_ipc->ipc_OSDTEXT->posy[5] = (int)fr["cfg_avt_178"];
					string myrStr5 = (string)fr["cfg_avt_179"];
					myrStr5.copy((char *)m_ipc->ipc_OSDTEXT->buf[5],myrStr5.length(),0);
					m_ipc->ipc_OSDTEXT->color[5] = (int)fr["cfg_avt_181"];
					m_ipc->ipc_OSDTEXT->alpha[5] = (int)fr["cfg_avt_182"];
					m_ipc->ipc_OSDTEXT->osdID[5] = 5;
					
					rOSD->OSD_text_show[6] = (int)fr["cfg_avt_192"];
					rOSD->OSD_text_positionX[6]= (int)fr["cfg_avt_193"];//new
					rOSD->OSD_text_positionY[6]= (int)fr["cfg_avt_194"];//new
					rOSD->OSD_text_content[6]=(string) fr["cfg_avt_195"];//new
					rOSD->OSD_text_color[6]= (int)fr["cfg_avt_197"];
					rOSD->OSD_text_alpha[6] = (int)fr["cfg_avt_198"];
					
					m_ipc->ipc_OSDTEXT->ctrl[6] =(int)fr["cfg_avt_192"];
					m_ipc->ipc_OSDTEXT->posx[6] = (int)fr["cfg_avt_193"];
					m_ipc->ipc_OSDTEXT->posy[6] = (int)fr["cfg_avt_194"];
					string myrStr6 = (string)fr["cfg_avt_195"];
					myrStr6.copy((char *)m_ipc->ipc_OSDTEXT->buf[6],myrStr6.length(),0);
					m_ipc->ipc_OSDTEXT->color[6] = (int)fr["cfg_avt_197"];
					m_ipc->ipc_OSDTEXT->alpha[6] = (int)fr["cfg_avt_198"];
					m_ipc->ipc_OSDTEXT->osdID[6] = 6;
					
					rOSD->OSD_text_show[7] = (int)fr["cfg_avt_208"];
					rOSD->OSD_text_positionX[7]= (int)fr["cfg_avt_209"];//new
					rOSD->OSD_text_positionY[7]= (int)fr["cfg_avt_210"];//new
					rOSD->OSD_text_content[7]= (string)fr["cfg_avt_211"];//new
					rOSD->OSD_text_color[7]= (int)fr["cfg_avt_213"];
					rOSD->OSD_text_alpha[7] = (int)fr["cfg_avt_214"];

					m_ipc->ipc_OSDTEXT->ctrl[7] =(int)fr["cfg_avt_208"];
					m_ipc->ipc_OSDTEXT->posx[7] = (int)fr["cfg_avt_209"];
					m_ipc->ipc_OSDTEXT->posy[7] = (int)fr["cfg_avt_210"];
					string myrStr7 = (string)fr["cfg_avt_211"];
					myrStr7.copy((char *)m_ipc->ipc_OSDTEXT->buf[7],myrStr7.length(),0);
					m_ipc->ipc_OSDTEXT->color[7] = (int)fr["cfg_avt_213"];
					m_ipc->ipc_OSDTEXT->alpha[7] = (int)fr["cfg_avt_214"];
					m_ipc->ipc_OSDTEXT->osdID[7] = 7;
					
					rOSD->OSD_text_show[8] = (int)fr["cfg_avt_224"];
					rOSD->OSD_text_positionX[8]= (int)fr["cfg_avt_225"];//new
					rOSD->OSD_text_positionY[8]= (int)fr["cfg_avt_226"];//new
					rOSD->OSD_text_content[8]=(string) fr["cfg_avt_227"];//new
					rOSD->OSD_text_color[8]= (int)fr["cfg_avt_229"];
					rOSD->OSD_text_alpha[8] = (int)fr["cfg_avt_230"];
					
					m_ipc->ipc_OSDTEXT->ctrl[8] =(int)fr["cfg_avt_224"];
					m_ipc->ipc_OSDTEXT->posx[8] = (int)fr["cfg_avt_225"];
					m_ipc->ipc_OSDTEXT->posy[8] = (int)fr["cfg_avt_226"];
					string myrStr8 = (string)fr["cfg_avt_227"];
					myrStr8.copy((char *)m_ipc->ipc_OSDTEXT->buf[8],myrStr8.length(),0);
					m_ipc->ipc_OSDTEXT->color[8] = (int)fr["cfg_avt_229"];
					m_ipc->ipc_OSDTEXT->alpha[8] = (int)fr["cfg_avt_230"];
					m_ipc->ipc_OSDTEXT->osdID[8] = 8;
					
					rOSD->OSD_text_show[9] = (int)fr["cfg_avt_240"];
					rOSD->OSD_text_positionX[9]= (int)fr["cfg_avt_241"];//new
					rOSD->OSD_text_positionY[9]= (int)fr["cfg_avt_242"];//new
					rOSD->OSD_text_content[9]=(string) fr["cfg_avt_243"];//new
					rOSD->OSD_text_color[9]= (int)fr["cfg_avt_245"];
					rOSD->OSD_text_alpha[9] = (int)fr["cfg_avt_246"];
					
					m_ipc->ipc_OSDTEXT->ctrl[9] =(int)fr["cfg_avt_240"];
					m_ipc->ipc_OSDTEXT->posx[9] = (int)fr["cfg_avt_241"];
					m_ipc->ipc_OSDTEXT->posy[9] = (int)fr["cfg_avt_242"];
					string myrStr9 = (string)fr["cfg_avt_243"];
					myrStr9.copy((char *)m_ipc->ipc_OSDTEXT->buf[9],myrStr9.length(),0);
					m_ipc->ipc_OSDTEXT->color[9] = (int)fr["cfg_avt_245"];
					m_ipc->ipc_OSDTEXT->alpha[9] = (int)fr["cfg_avt_246"];
					m_ipc->ipc_OSDTEXT->osdID[9] = 9;

					rOSD->OSD_text_show[10] = (int)fr["cfg_avt_256"];
					rOSD->OSD_text_positionX[10]= (int)fr["cfg_avt_257"];//new
					rOSD->OSD_text_positionY[10]= (int)fr["cfg_avt_258"];//new
					rOSD->OSD_text_content[10]=(string) fr["cfg_avt_259"];//new
					rOSD->OSD_text_color[10]= (int)fr["cfg_avt_261"];
					rOSD->OSD_text_alpha[10] = (int)fr["cfg_avt_262"];

					m_ipc->ipc_OSDTEXT->ctrl[10] =(int)fr["cfg_avt_256"];
					m_ipc->ipc_OSDTEXT->posx[10] = (int)fr["cfg_avt_257"];
					m_ipc->ipc_OSDTEXT->posy[10] = (int)fr["cfg_avt_258"];
					string myrStr10 = (string)fr["cfg_avt_259"];
					myrStr10.copy((char *)m_ipc->ipc_OSDTEXT->buf[10],myrStr10.length(),0);
					m_ipc->ipc_OSDTEXT->color[10] = (int)fr["cfg_avt_261"];
					m_ipc->ipc_OSDTEXT->alpha[10] = (int)fr["cfg_avt_262"];
					m_ipc->ipc_OSDTEXT->osdID[10] = 10;

					rOSD->OSD_text_show[11] = (int)fr["cfg_avt_272"];
					rOSD->OSD_text_positionX[11]= (int)fr["cfg_avt_273"];//new
					rOSD->OSD_text_positionY[11]= (int)fr["cfg_avt_274"];//new
					rOSD->OSD_text_content[11]= (string)fr["cfg_avt_275"];//new
					rOSD->OSD_text_color[11]= (int)fr["cfg_avt_277"];
					rOSD->OSD_text_alpha[11] = (int)fr["cfg_avt_278"];
					
					m_ipc->ipc_OSDTEXT->ctrl[11] =(int)fr["cfg_avt_272"];
					m_ipc->ipc_OSDTEXT->posx[11] = (int)fr["cfg_avt_273"];
					m_ipc->ipc_OSDTEXT->posy[11] = (int)fr["cfg_avt_274"];
					string myrStr11 = (string)fr["cfg_avt_275"];
					myrStr11.copy((char *)m_ipc->ipc_OSDTEXT->buf[11],myrStr11.length(),0);
					m_ipc->ipc_OSDTEXT->color[11] = (int)fr["cfg_avt_277"];
					m_ipc->ipc_OSDTEXT->alpha[11] = (int)fr["cfg_avt_278"];
					m_ipc->ipc_OSDTEXT->osdID[11] = 11;
					
					rOSD->OSD_text_show[12] = (int)fr["cfg_avt_288"];
					rOSD->OSD_text_positionX[12]= (int)fr["cfg_avt_289"];//new
					rOSD->OSD_text_positionY[12]= (int)fr["cfg_avt_290"];//new
					rOSD->OSD_text_content[12]=(string) fr["cfg_avt_291"];//new
					rOSD->OSD_text_color[12]= (int)fr["cfg_avt_293"];
					rOSD->OSD_text_alpha[12] = (int)fr["cfg_avt_294"];

					m_ipc->ipc_OSDTEXT->ctrl[12] =(int)fr["cfg_avt_288"];
					m_ipc->ipc_OSDTEXT->posx[12] = (int)fr["cfg_avt_289"];
					m_ipc->ipc_OSDTEXT->posy[12] = (int)fr["cfg_avt_290"];
					string myrStr12 = (string)fr["cfg_avt_291"];
					myrStr12.copy((char *)m_ipc->ipc_OSDTEXT->buf[12],myrStr12.length(),0);
					m_ipc->ipc_OSDTEXT->color[12] = (int)fr["cfg_avt_293"];
					m_ipc->ipc_OSDTEXT->alpha[12] = (int)fr["cfg_avt_294"];
					m_ipc->ipc_OSDTEXT->osdID[12] = 12;

					rOSD->OSD_text_show[13] = (int)fr["cfg_avt_304"];
					rOSD->OSD_text_positionX[13]= (int)fr["cfg_avt_305"];//new
					rOSD->OSD_text_positionY[13]= (int)fr["cfg_avt_306"];//new
					rOSD->OSD_text_content[13]= (string)fr["cfg_avt_307"];//new
					rOSD->OSD_text_color[13]= (int)fr["cfg_avt_309"];
					rOSD->OSD_text_alpha[13] = (int)fr["cfg_avt_310"];
					
					m_ipc->ipc_OSDTEXT->ctrl[13] =(int)fr["cfg_avt_304"];
					m_ipc->ipc_OSDTEXT->posx[13] = (int)fr["cfg_avt_305"];
					m_ipc->ipc_OSDTEXT->posy[13] = (int)fr["cfg_avt_306"];
					string myrStr13 = (string)fr["cfg_avt_307"];
					myrStr13.copy((char *)m_ipc->ipc_OSDTEXT->buf[13],myrStr13.length(),0);
					m_ipc->ipc_OSDTEXT->color[13] = (int)fr["cfg_avt_309"];
					m_ipc->ipc_OSDTEXT->alpha[13] = (int)fr["cfg_avt_310"];
					m_ipc->ipc_OSDTEXT->osdID[13] = 13;
					
					rOSD->OSD_text_show[14] = (int)fr["cfg_avt_320"];
					rOSD->OSD_text_positionX[14]= (int)fr["cfg_avt_321"];//new
					rOSD->OSD_text_positionY[14]= (int)fr["cfg_avt_322"];//new
					rOSD->OSD_text_content[14]= (string)fr["cfg_avt_323"];//new
					rOSD->OSD_text_color[14]= (int)fr["cfg_avt_325"];
					rOSD->OSD_text_alpha[14] = (int)fr["cfg_avt_326"];
					
					m_ipc->ipc_OSDTEXT->ctrl[14] =(int)fr["cfg_avt_320"];
					m_ipc->ipc_OSDTEXT->posx[14] = (int)fr["cfg_avt_321"];
					m_ipc->ipc_OSDTEXT->posy[14] = (int)fr["cfg_avt_322"];
					string myrStr14 = (string)fr["cfg_avt_323"];
					myrStr14.copy((char *)m_ipc->ipc_OSDTEXT->buf[14],myrStr14.length(),0);
					m_ipc->ipc_OSDTEXT->color[14] = (int)fr["cfg_avt_325"];
					m_ipc->ipc_OSDTEXT->alpha[14] = (int)fr["cfg_avt_326"];
					m_ipc->ipc_OSDTEXT->osdID[14] = 14;
					
					rOSD->OSD_text_show[15] = (int)fr["cfg_avt_336"];
					rOSD->OSD_text_positionX[15]= (int)fr["cfg_avt_337"];//new
					rOSD->OSD_text_positionY[15]= (int)fr["cfg_avt_338"];//new
					rOSD->OSD_text_content[15]= (string)fr["cfg_avt_339"];//new
					rOSD->OSD_text_color[15]= (int)fr["cfg_avt_341"];
					rOSD->OSD_text_alpha[15] = (int)fr["cfg_avt_342"];

					m_ipc->ipc_OSDTEXT->ctrl[15] =(int)fr["cfg_avt_336"];
					m_ipc->ipc_OSDTEXT->posx[15] = (int)fr["cfg_avt_337"];
					m_ipc->ipc_OSDTEXT->posy[15] = (int)fr["cfg_avt_338"];
					string myrStr15 = (string)fr["cfg_avt_339"];
					myrStr15.copy((char *)m_ipc->ipc_OSDTEXT->buf[15],myrStr15.length(),0);
					m_ipc->ipc_OSDTEXT->color[15] = (int)fr["cfg_avt_341"];
					m_ipc->ipc_OSDTEXT->alpha[15] = (int)fr["cfg_avt_342"];
					m_ipc->ipc_OSDTEXT->osdID[15] = 15;
					
					rOSD->OSD_text_show[16] = (int)fr["cfg_avt_448"];
					rOSD->OSD_text_positionX[16]= (int)fr["cfg_avt_449"];//new
					rOSD->OSD_text_positionY[16]= (int)fr["cfg_avt_450"];//new
					rOSD->OSD_text_content[16]=(string) fr["cfg_avt_451"];//new
					rOSD->OSD_text_color[16]= (int)fr["cfg_avt_453"];
					rOSD->OSD_text_alpha[16] = (int)fr["cfg_avt_454"];

					m_ipc->ipc_OSDTEXT->ctrl[16] =(int)fr["cfg_avt_448"];
					m_ipc->ipc_OSDTEXT->posx[16] = (int)fr["cfg_avt_449"];
					m_ipc->ipc_OSDTEXT->posy[16] = (int)fr["cfg_avt_450"];
					string myrStr16 = (string)fr["cfg_avt_451"];
					myrStr16.copy((char *)m_ipc->ipc_OSDTEXT->buf[16],myrStr16.length(),0);
					m_ipc->ipc_OSDTEXT->color[16] = (int)fr["cfg_avt_453"];
					m_ipc->ipc_OSDTEXT->alpha[16] = (int)fr["cfg_avt_454"];
					m_ipc->ipc_OSDTEXT->osdID[16] = 16;

					rOSD->OSD_text_show[17] = (int)fr["cfg_avt_464"];
					rOSD->OSD_text_positionX[17]= (int)fr["cfg_avt_465"];//new
					rOSD->OSD_text_positionY[17]= (int)fr["cfg_avt_466"];//new
					rOSD->OSD_text_content[17]=(string) fr["cfg_avt_467"];//new
					rOSD->OSD_text_color[17]= (int)fr["cfg_avt_469"];
					rOSD->OSD_text_alpha[17] = (int)fr["cfg_avt_470"];

					m_ipc->ipc_OSDTEXT->ctrl[17] =(int)fr["cfg_avt_464"];
					m_ipc->ipc_OSDTEXT->posx[17] = (int)fr["cfg_avt_465"];
					m_ipc->ipc_OSDTEXT->posy[17] = (int)fr["cfg_avt_466"];
					string myrStr17 = (string)fr["cfg_avt_467"];
					myrStr17.copy((char *)m_ipc->ipc_OSDTEXT->buf[17],myrStr17.length(),0);
					m_ipc->ipc_OSDTEXT->color[17] = (int)fr["cfg_avt_469"];
					m_ipc->ipc_OSDTEXT->alpha[17] = (int)fr["cfg_avt_470"];
					m_ipc->ipc_OSDTEXT->osdID[17] = 17;

					rOSD->OSD_text_show[18] = (int)fr["cfg_avt_480"];
					rOSD->OSD_text_positionX[18]= (int)fr["cfg_avt_481"];//new
					rOSD->OSD_text_positionY[18]= (int)fr["cfg_avt_482"];//new
					rOSD->OSD_text_content[18]=(string) fr["cfg_avt_483"];//new
					rOSD->OSD_text_color[18]= (int)fr["cfg_avt_485"];
					rOSD->OSD_text_alpha[18] = (int)fr["cfg_avt_486"];

					m_ipc->ipc_OSDTEXT->ctrl[18] =(int)fr["cfg_avt_480"];
					m_ipc->ipc_OSDTEXT->posx[18] = (int)fr["cfg_avt_481"];
					m_ipc->ipc_OSDTEXT->posy[18] = (int)fr["cfg_avt_482"];
					string myrStr18 = (string)fr["cfg_avt_483"];
					myrStr18.copy((char *)m_ipc->ipc_OSDTEXT->buf[18],myrStr18.length(),0);
					m_ipc->ipc_OSDTEXT->color[18] = (int)fr["cfg_avt_485"];
					m_ipc->ipc_OSDTEXT->alpha[18] = (int)fr["cfg_avt_486"];
					m_ipc->ipc_OSDTEXT->osdID[18] = 18;

					rOSD->OSD_text_show[19] = (int)fr["cfg_avt_496"];
					rOSD->OSD_text_positionX[19]= (int)fr["cfg_avt_497"];//new
					rOSD->OSD_text_positionY[19]= (int)fr["cfg_avt_498"];//new
					rOSD->OSD_text_content[19]=(string) fr["cfg_avt_499"];//new
					rOSD->OSD_text_color[19]= (int)fr["cfg_avt_501"];
					rOSD->OSD_text_alpha[19] = (int)fr["cfg_avt_502"];

					m_ipc->ipc_OSDTEXT->ctrl[19] =(int)fr["cfg_avt_496"];
					m_ipc->ipc_OSDTEXT->posx[19] = (int)fr["cfg_avt_497"];
					m_ipc->ipc_OSDTEXT->posy[19] = (int)fr["cfg_avt_498"];
					string myrStr19 = (string)fr["cfg_avt_499"];
					myrStr19.copy((char *)m_ipc->ipc_OSDTEXT->buf[19],myrStr19.length(),0);
					m_ipc->ipc_OSDTEXT->color[19] = (int)fr["cfg_avt_501"];
					m_ipc->ipc_OSDTEXT->alpha[19] = (int)fr["cfg_avt_502"];
					m_ipc->ipc_OSDTEXT->osdID[19] = 19;

					rOSD->OSD_text_show[20] = (int)fr["cfg_avt_512"];
					rOSD->OSD_text_positionX[20]= (int)fr["cfg_avt_513"];//new
					rOSD->OSD_text_positionY[20]= (int)fr["cfg_avt_514"];//new
					rOSD->OSD_text_content[20]= (string)fr["cfg_avt_515"];//new
					rOSD->OSD_text_color[20]= (int)fr["cfg_avt_517"];
					rOSD->OSD_text_alpha[20] = (int)fr["cfg_avt_518"];

					m_ipc->ipc_OSDTEXT->ctrl[20] =(int)fr["cfg_avt_512"];
					m_ipc->ipc_OSDTEXT->posx[20] = (int)fr["cfg_avt_513"];
					m_ipc->ipc_OSDTEXT->posy[20] = (int)fr["cfg_avt_514"];
					string myrStr20 = (string)fr["cfg_avt_515"];
					myrStr20.copy((char *)m_ipc->ipc_OSDTEXT->buf[20],myrStr20.length(),0);
					m_ipc->ipc_OSDTEXT->color[20] = (int)fr["cfg_avt_517"];
					m_ipc->ipc_OSDTEXT->alpha[20] = (int)fr["cfg_avt_518"];
					m_ipc->ipc_OSDTEXT->osdID[20] = 20;

					rOSD->OSD_text_show[21] = (int)fr["cfg_avt_528"];
					rOSD->OSD_text_positionX[21]= (int)fr["cfg_avt_529"];//new
					rOSD->OSD_text_positionY[21]= (int)fr["cfg_avt_530"];//new
					rOSD->OSD_text_content[21]=(string) fr["cfg_avt_531"];//new
					rOSD->OSD_text_color[21]= (int)fr["cfg_avt_533"];
					rOSD->OSD_text_alpha[21] = (int)fr["cfg_avt_534"];

					m_ipc->ipc_OSDTEXT->ctrl[21] =(int)fr["cfg_avt_528"];
					m_ipc->ipc_OSDTEXT->posx[21] = (int)fr["cfg_avt_529"];
					m_ipc->ipc_OSDTEXT->posy[21] = (int)fr["cfg_avt_530"];
					string myrStr21 = (string)fr["cfg_avt_531"];
					myrStr21.copy((char *)m_ipc->ipc_OSDTEXT->buf[21],myrStr21.length(),0);
					m_ipc->ipc_OSDTEXT->color[21] = (int)fr["cfg_avt_533"];
					m_ipc->ipc_OSDTEXT->alpha[21] = (int)fr["cfg_avt_534"];
					m_ipc->ipc_OSDTEXT->osdID[21] = 21;

					rOSD->OSD_text_show[22] = (int)fr["cfg_avt_544"];
					rOSD->OSD_text_positionX[22]= (int)fr["cfg_avt_545"];//new
					rOSD->OSD_text_positionY[22]= (int)fr["cfg_avt_546"];//new
					rOSD->OSD_text_content[22]= (string)fr["cfg_avt_547"];//new
					rOSD->OSD_text_color[22]= (int)fr["cfg_avt_549"];
					rOSD->OSD_text_alpha[22] = (int)fr["cfg_avt_550"];

					m_ipc->ipc_OSDTEXT->ctrl[22] =(int)fr["cfg_avt_544"];
					m_ipc->ipc_OSDTEXT->posx[22] = (int)fr["cfg_avt_545"];
					m_ipc->ipc_OSDTEXT->posy[22] = (int)fr["cfg_avt_546"];
					string myrStr22 = (string)fr["cfg_avt_547"];
					myrStr22.copy((char *)m_ipc->ipc_OSDTEXT->buf[22],myrStr22.length(),0);
					m_ipc->ipc_OSDTEXT->color[22] = (int)fr["cfg_avt_549"];
					m_ipc->ipc_OSDTEXT->alpha[22] = (int)fr["cfg_avt_550"];
					m_ipc->ipc_OSDTEXT->osdID[22] = 22;

					rOSD->OSD_text_show[23] = (int)fr["cfg_avt_560"];
					rOSD->OSD_text_positionX[23]= (int)fr["cfg_avt_561"];//new
					rOSD->OSD_text_positionY[23]= (int)fr["cfg_avt_562"];//new
					rOSD->OSD_text_content[23]= (string)fr["cfg_avt_563"];//new
					rOSD->OSD_text_color[23]= (int)fr["cfg_avt_565"];
					rOSD->OSD_text_alpha[23] = (int)fr["cfg_avt_566"];

					m_ipc->ipc_OSDTEXT->ctrl[23] =(int)fr["cfg_avt_560"];
					m_ipc->ipc_OSDTEXT->posx[23] = (int)fr["cfg_avt_561"];
					m_ipc->ipc_OSDTEXT->posy[23] = (int)fr["cfg_avt_562"];
					string myrStr23 = (string)fr["cfg_avt_563"];
					myrStr23.copy((char *)m_ipc->ipc_OSDTEXT->buf[23],myrStr23.length(),0);
					m_ipc->ipc_OSDTEXT->color[23] = (int)fr["cfg_avt_565"];
					m_ipc->ipc_OSDTEXT->alpha[23] = (int)fr["cfg_avt_566"];
					m_ipc->ipc_OSDTEXT->osdID[23] = 23;

					rOSD->OSD_text_show[24] = (int)fr["cfg_avt_576"];
					rOSD->OSD_text_positionX[24]= (int)fr["cfg_avt_577"];//new
					rOSD->OSD_text_positionY[24]= (int)fr["cfg_avt_578"];//new
					rOSD->OSD_text_content[24]=(string) fr["cfg_avt_579"];//new
					rOSD->OSD_text_color[24]= (int)fr["cfg_avt_581"];
					rOSD->OSD_text_alpha[24] = (int)fr["cfg_avt_582"];

					m_ipc->ipc_OSDTEXT->ctrl[24] =(int)fr["cfg_avt_576"];
					m_ipc->ipc_OSDTEXT->posx[24] = (int)fr["cfg_avt_577"];
					m_ipc->ipc_OSDTEXT->posy[24] = (int)fr["cfg_avt_578"];
					string myrStr24 = (string)fr["cfg_avt_579"];
					myrStr24.copy((char *)m_ipc->ipc_OSDTEXT->buf[24],myrStr24.length(),0);
					m_ipc->ipc_OSDTEXT->color[24] = (int)fr["cfg_avt_581"];
					m_ipc->ipc_OSDTEXT->alpha[24] = (int)fr["cfg_avt_582"];
					m_ipc->ipc_OSDTEXT->osdID[24] = 24;

					rOSD->OSD_text_show[25] = (int)fr["cfg_avt_592"];
					rOSD->OSD_text_positionX[25]= (int)fr["cfg_avt_593"];//new
					rOSD->OSD_text_positionY[25]= (int)fr["cfg_avt_594"];//new
					rOSD->OSD_text_content[25]=(string) fr["cfg_avt_595"];//new
					rOSD->OSD_text_color[25]= (int)fr["cfg_avt_597"];
					rOSD->OSD_text_alpha[25] = (int)fr["cfg_avt_598"];

					m_ipc->ipc_OSDTEXT->ctrl[25] =(int)fr["cfg_avt_592"];
					m_ipc->ipc_OSDTEXT->posx[25] = (int)fr["cfg_avt_593"];
					m_ipc->ipc_OSDTEXT->posy[25] = (int)fr["cfg_avt_594"];
					string myrStr25 = (string)fr["cfg_avt_595"];
					myrStr25.copy((char *)m_ipc->ipc_OSDTEXT->buf[25],myrStr25.length(),0);
					m_ipc->ipc_OSDTEXT->color[25] = (int)fr["cfg_avt_597"];
					m_ipc->ipc_OSDTEXT->alpha[25] = (int)fr["cfg_avt_598"];
					m_ipc->ipc_OSDTEXT->osdID[25] = 25;

					rOSD->OSD_text_show[26] = (int)fr["cfg_avt_608"];
					rOSD->OSD_text_positionX[26]= (int)fr["cfg_avt_609"];//new
					rOSD->OSD_text_positionY[26]= (int)fr["cfg_avt_610"];//new
					rOSD->OSD_text_content[26]=(string) fr["cfg_avt_611"];//new
					rOSD->OSD_text_color[26]= (int)fr["cfg_avt_613"];
					rOSD->OSD_text_alpha[26] = (int)fr["cfg_avt_614"];

					m_ipc->ipc_OSDTEXT->ctrl[26] =(int)fr["cfg_avt_608"];
					m_ipc->ipc_OSDTEXT->posx[26] = (int)fr["cfg_avt_609"];
					m_ipc->ipc_OSDTEXT->posy[26] = (int)fr["cfg_avt_610"];
					string myrStr26 = (string)fr["cfg_avt_611"];
					myrStr26.copy((char *)m_ipc->ipc_OSDTEXT->buf[26],myrStr26.length(),0);
					m_ipc->ipc_OSDTEXT->color[26] = (int)fr["cfg_avt_613"];
					m_ipc->ipc_OSDTEXT->alpha[26] = (int)fr["cfg_avt_614"];
					m_ipc->ipc_OSDTEXT->osdID[26] = 26;
					
					rOSD->OSD_text_show[27] = (int)fr["cfg_avt_624"];
					rOSD->OSD_text_positionX[27]= (int)fr["cfg_avt_625"];//new
					rOSD->OSD_text_positionY[27]= (int)fr["cfg_avt_626"];//new
					rOSD->OSD_text_content[27]=(string) fr["cfg_avt_627"];//new
					rOSD->OSD_text_color[27]= (int)fr["cfg_avt_629"];
					rOSD->OSD_text_alpha[27] = (int)fr["cfg_avt_630"];
					
					m_ipc->ipc_OSDTEXT->ctrl[27] =(int)fr["cfg_avt_624"];
					m_ipc->ipc_OSDTEXT->posx[27] = (int)fr["cfg_avt_625"];
					m_ipc->ipc_OSDTEXT->posy[27] = (int)fr["cfg_avt_626"];
					string myrStr27 = (string)fr["cfg_avt_627"];
					myrStr27.copy((char *)m_ipc->ipc_OSDTEXT->buf[27],myrStr27.length(),0);
					m_ipc->ipc_OSDTEXT->color[27] = (int)fr["cfg_avt_629"];
					m_ipc->ipc_OSDTEXT->alpha[27] = (int)fr["cfg_avt_630"];
					m_ipc->ipc_OSDTEXT->osdID[27] = 27;
					
					rOSD->OSD_text_show[28] = (int)fr["cfg_avt_640"];
					rOSD->OSD_text_positionX[28]= (int)fr["cfg_avt_641"];//new
					rOSD->OSD_text_positionY[28]= (int)fr["cfg_avt_642"];//new
					rOSD->OSD_text_content[28]= (string)fr["cfg_avt_643"];//new
					rOSD->OSD_text_color[28]= (int)fr["cfg_avt_645"];
					rOSD->OSD_text_alpha[28] = (int)fr["cfg_avt_646"];

					m_ipc->ipc_OSDTEXT->ctrl[28] =(int)fr["cfg_avt_640"];
					m_ipc->ipc_OSDTEXT->posx[28] = (int)fr["cfg_avt_641"];
					m_ipc->ipc_OSDTEXT->posy[28] = (int)fr["cfg_avt_642"];
					string myrStr28 = (string)fr["cfg_avt_643"];
					myrStr28.copy((char *)m_ipc->ipc_OSDTEXT->buf[28],myrStr28.length(),0);
					m_ipc->ipc_OSDTEXT->color[28] = (int)fr["cfg_avt_645"];
					m_ipc->ipc_OSDTEXT->alpha[28] = (int)fr["cfg_avt_646"];
					m_ipc->ipc_OSDTEXT->osdID[28] = 28;

					rOSD->OSD_text_show[29] = (int)fr["cfg_avt_656"];
					rOSD->OSD_text_positionX[29]= (int)fr["cfg_avt_657"];//new
					rOSD->OSD_text_positionY[29]= (int)fr["cfg_avt_658"];//new
					rOSD->OSD_text_content[29]= (string)fr["cfg_avt_659"];//new
					rOSD->OSD_text_color[29]= (int)fr["cfg_avt_661"];
					rOSD->OSD_text_alpha[29] = (int)fr["cfg_avt_662"];
					
					m_ipc->ipc_OSDTEXT->ctrl[29] =(int)fr["cfg_avt_656"];
					m_ipc->ipc_OSDTEXT->posx[29] = (int)fr["cfg_avt_657"];
					m_ipc->ipc_OSDTEXT->posy[29] = (int)fr["cfg_avt_658"];
					string myrStr29 = (string)fr["cfg_avt_659"];
					myrStr29.copy((char *)m_ipc->ipc_OSDTEXT->buf[29],myrStr29.length(),0);
					m_ipc->ipc_OSDTEXT->color[29] = (int)fr["cfg_avt_661"];
					m_ipc->ipc_OSDTEXT->alpha[29] = (int)fr["cfg_avt_662"];
					m_ipc->ipc_OSDTEXT->osdID[29] = 29;
					
					rOSD->OSD_text_show[30] = (int)fr["cfg_avt_672"];
					rOSD->OSD_text_positionX[30]= (int)fr["cfg_avt_673"];//new
					rOSD->OSD_text_positionY[30]= (int)fr["cfg_avt_674"];//new
					rOSD->OSD_text_content[30]=(string) fr["cfg_avt_675"];//new
					rOSD->OSD_text_color[30]= (int)fr["cfg_avt_677"];
					rOSD->OSD_text_alpha[30] = (int)fr["cfg_avt_678"];
					
					m_ipc->ipc_OSDTEXT->ctrl[30] =(int)fr["cfg_avt_672"];
					m_ipc->ipc_OSDTEXT->posx[30] = (int)fr["cfg_avt_673"];
					m_ipc->ipc_OSDTEXT->posy[30] = (int)fr["cfg_avt_674"];
					string myrStr30 = (string)fr["cfg_avt_675"];
					myrStr30.copy((char *)m_ipc->ipc_OSDTEXT->buf[30],myrStr30.length(),0);
					m_ipc->ipc_OSDTEXT->color[30] = (int)fr["cfg_avt_677"];
					m_ipc->ipc_OSDTEXT->alpha[30] = (int)fr["cfg_avt_678"];
					m_ipc->ipc_OSDTEXT->osdID[30] = 30;
					
					rOSD->OSD_text_show[31] = (int)fr["cfg_avt_688"];
					rOSD->OSD_text_positionX[31]= (int)fr["cfg_avt_689"];//new
					rOSD->OSD_text_positionY[31]= (int)fr["cfg_avt_690"];//new
					rOSD->OSD_text_content[31]=(string) fr["cfg_avt_691"];//new
					rOSD->OSD_text_color[31]= (int)fr["cfg_avt_693"];
					rOSD->OSD_text_alpha[31] = (int)fr["cfg_avt_694"];

					m_ipc->ipc_OSDTEXT->ctrl[31] =(int)fr["cfg_avt_688"];
					m_ipc->ipc_OSDTEXT->posx[31] = (int)fr["cfg_avt_689"];
					m_ipc->ipc_OSDTEXT->posy[31] = (int)fr["cfg_avt_690"];
					string myrStr31 = (string)fr["cfg_avt_691"];
					myrStr31.copy((char *)m_ipc->ipc_OSDTEXT->buf[31],myrStr31.length(),0);
					m_ipc->ipc_OSDTEXT->color[31] = (int)fr["cfg_avt_693"];
					m_ipc->ipc_OSDTEXT->alpha[31] = (int)fr["cfg_avt_694"];
					m_ipc->ipc_OSDTEXT->osdID[31] = 31;

					m_ipc->ipc_OSD->ch0_acqRect_width =(int)fr["cfg_avt_704"];
					m_ipc->ipc_OSD->ch1_acqRect_width = (int)fr["cfg_avt_705"];
					m_ipc->ipc_OSD->ch2_acqRect_width = (int)fr["cfg_avt_706"];
					m_ipc->ipc_OSD->ch3_acqRect_width = (int)fr["cfg_avt_707"];
					m_ipc->ipc_OSD->ch4_acqRect_width = (int)fr["cfg_avt_708"];
					m_ipc->ipc_OSD->ch5_acqRect_width = (int)fr["cfg_avt_709"];
					m_ipc->ipc_OSD->ch0_acqRect_height = (int)fr["cfg_avt_710"];
					m_ipc->ipc_OSD->ch1_acqRect_height =(int)fr["cfg_avt_711"];
					m_ipc->ipc_OSD->ch2_acqRect_height = (int)fr["cfg_avt_712"];
					m_ipc->ipc_OSD->ch3_acqRect_height =(int)fr["cfg_avt_713"];
					m_ipc->ipc_OSD->ch4_acqRect_height = (int)fr["cfg_avt_714"];
					m_ipc->ipc_OSD->ch5_acqRect_height  =(int)fr["cfg_avt_715"];
					m_ipc->ipc_OSD->ch0_aim_width = (int)fr["cfg_avt_362"];
					m_ipc->ipc_OSD->ch1_aim_width = (int)fr["cfg_avt_922"];
					m_ipc->ipc_OSD->ch2_aim_width = (int)fr["cfg_avt_1034"];
					m_ipc->ipc_OSD->ch3_aim_width = (int)fr["cfg_avt_1146"];
					m_ipc->ipc_OSD->ch4_aim_width = (int)fr["cfg_avt_1258"];
					//m_ipc->ipc_OSD->ch5_aim_width = (int)fr["cfg_avt_725"];
					m_ipc->ipc_OSD->ch0_aim_height = (int)fr["cfg_avt_363"];
					m_ipc->ipc_OSD->ch1_aim_height = (int)fr["cfg_avt_923"];
					m_ipc->ipc_OSD->ch2_aim_height = (int)fr["cfg_avt_1035"];
					m_ipc->ipc_OSD->ch3_aim_height = (int)fr["cfg_avt_1147"];
					m_ipc->ipc_OSD->ch4_aim_height = (int)fr["cfg_avt_1259"];
					//m_ipc->ipc_OSD->ch5_aim_height = (int)fr["cfg_avt_731"];

					m_ipc->ipc_OSD->OSD_draw_color =(int)fr["cfg_avt_737"];
					m_ipc->ipc_OSD->CROSS_AXIS_WIDTH = (int)fr["cfg_avt_738"];
					m_ipc->ipc_OSD->CROSS_AXIS_HEIGHT = (int)fr["cfg_avt_739"];
					m_ipc->ipc_OSD->Picp_CROSS_AXIS_WIDTH = (int)fr["cfg_avt_740"];
					m_ipc->ipc_OSD->Picp_CROSS_AXIS_HEIGHT = (int)fr["cfg_avt_741"];
				}

				{
					/*creame params*/
					m_ipc->ipc_OSD->osdChidIDShow[0] = (int)fr["cfg_avt_352"];
					m_ipc->ipc_OSD->osdChidNmaeShow[0] = (int)fr["cfg_avt_354"];
					m_GlobalDate->cameraSwitch[0] = (int)fr["cfg_avt_355"];
					m_GlobalDate->AutoBox[0] = (int)fr["cfg_avt_361"];

					m_ipc->ipc_OSD->osdChidIDShow[1] = (int)fr["cfg_avt_912"];
					m_ipc->ipc_OSD->osdChidNmaeShow[1]= (int)fr["cfg_avt_914"];
					m_GlobalDate->cameraSwitch[1] = (int)fr["cfg_avt_915"];
					m_GlobalDate->AutoBox[1] = (int)fr["cfg_avt_921"];

					m_ipc->ipc_OSD->osdChidIDShow[2] = (int)fr["cfg_avt_1024"];
					m_ipc->ipc_OSD->osdChidNmaeShow[2]= (int)fr["cfg_avt_1026"];
					m_GlobalDate->cameraSwitch[2] = (int)fr["cfg_avt_1027"];
					m_GlobalDate->AutoBox[2] = (int)fr["cfg_avt_1033"];

					m_ipc->ipc_OSD->osdChidIDShow[3] = (int)fr["cfg_avt_1136"];
					m_ipc->ipc_OSD->osdChidNmaeShow[3]= (int)fr["cfg_avt_1138"];
					m_GlobalDate->cameraSwitch[3] = (int)fr["cfg_avt_1139"];
					m_GlobalDate->AutoBox[3] = (int)fr["cfg_avt_1145"];

					m_ipc->ipc_OSD->osdChidIDShow[4] = (int)fr["cfg_avt_1248"];
					m_ipc->ipc_OSD->osdChidNmaeShow[4]= (int)fr["cfg_avt_1250"];
					m_GlobalDate->cameraSwitch[4] = (int)fr["cfg_avt_1251"];
					m_GlobalDate->AutoBox[4] = (int)fr["cfg_avt_1257"];

/**********************************************************************/
					m_ipc->ipc_OSD->osdBoxShow[0] = (int)fr["cfg_avt_359"];
					m_ipc->ipc_OSD->osdBoxShow[1] = (int)fr["cfg_avt_919"];
					m_ipc->ipc_OSD->osdBoxShow[2] = (int)fr["cfg_avt_1031"];
					m_ipc->ipc_OSD->osdBoxShow[3] = (int)fr["cfg_avt_1143"];
					m_ipc->ipc_OSD->osdBoxShow[4] = (int)fr["cfg_avt_1255"];

					m_ipc->ipc_OSD->CROSS_draw_show[0] = (int)fr["cfg_avt_360"];
					m_ipc->ipc_OSD->CROSS_draw_show[1] = (int)fr["cfg_avt_920"];
					m_ipc->ipc_OSD->CROSS_draw_show[2] = (int)fr["cfg_avt_1032"];
					m_ipc->ipc_OSD->CROSS_draw_show[3] = (int)fr["cfg_avt_1144"];
					m_ipc->ipc_OSD->CROSS_draw_show[4] = (int)fr["cfg_avt_1256"];

/**********************************************************************/
					m_GlobalDate->fovmode[0]=(int)fr["cfg_avt_357"];
					m_GlobalDate->switchFovLV[0] = (int)fr["cfg_avt_384"];
					m_GlobalDate->continueFovLv[0] = (int)fr["cfg_avt_899"];

					m_GlobalDate->fovmode[1]=(int)fr["cfg_avt_917"];
					m_GlobalDate->switchFovLV[1] = (int)fr["cfg_avt_944"];
					m_GlobalDate->continueFovLv[1] = (int)fr["cfg_avt_1011"];

					m_GlobalDate->fovmode[2]=(int)fr["cfg_avt_1029"];
					m_GlobalDate->switchFovLV[2] = (int)fr["cfg_avt_1056"];
					m_GlobalDate->continueFovLv[2] = (int)fr["cfg_avt_1123"];

					m_GlobalDate->fovmode[3]=(int)fr["cfg_avt_1141"];
					m_GlobalDate->switchFovLV[3] = (int)fr["cfg_avt_1168"];
					m_GlobalDate->continueFovLv[3] = (int)fr["cfg_avt_1235"];

					m_GlobalDate->fovmode[4]=(int)fr["cfg_avt_1253"];
					m_GlobalDate->switchFovLV[4] = (int)fr["cfg_avt_1280"];
					m_GlobalDate->continueFovLv[4] = (int)fr["cfg_avt_1347"];
/**********************************************************************/

					/*通道1 固定视场*/
					float value;
					viewParam.radio[0]=(float)fr["cfg_avt_358"];
					value =(float)fr["cfg_avt_368"];
					value = value * (PI / 180) * 1000;
					viewParam.level_Fov_fix[0]= value;
					value =(float)fr["cfg_avt_369"];
					value = value * (PI / 180) * 1000;
					viewParam.vertical_Fov_fix[0]=value;
					viewParam.Boresightpos_fix_x[0]=(float)fr["cfg_avt_370"];
					viewParam.Boresightpos_fix_y[0]=(float)fr["cfg_avt_371"];
					/*通道１可切换视场*/
					value=(float)fr["cfg_avt_385"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch1[0]= value;
					value =(float)fr["cfg_avt_386"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch1[0]=value;
					viewParam.Boresightpos_switch_x1[0]=(float)fr["cfg_avt_387"];
					viewParam.Boresightpos_switch_y1[0]=(float)fr["cfg_avt_388"];

					value=(float)fr["cfg_avt_389"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch2[0]=value;
					value=(float)fr["cfg_avt_390"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch2[0]=value;
					viewParam.Boresightpos_switch_x2[0]=(float)fr["cfg_avt_391"];
					viewParam.Boresightpos_switch_y2[0]=(float)fr["cfg_avt_392"];
					value =(float)fr["cfg_avt_393"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch3[0]=value;
					value =(float)fr["cfg_avt_394"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch3[0]=value;
					viewParam.Boresightpos_switch_x3[0]=(float)fr["cfg_avt_395"];
					viewParam.Boresightpos_switch_y3[0]=(float)fr["cfg_avt_396"];
					value =(float)fr["cfg_avt_397"];
					value=value*(PI / 180) * 1000;
                                        viewParam.level_Fov_switch4[0]=value;
					value =(float)fr["cfg_avt_398"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch4[0]=value;
					viewParam.Boresightpos_switch_x4[0]=(float)fr["cfg_avt_399"];
					viewParam.Boresightpos_switch_y4[0]=(float)fr["cfg_avt_400"];

					value =(float)fr["cfg_avt_401"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch5[0]=value;
					value =(float)fr["cfg_avt_402"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch5[0]=value;
					viewParam.Boresightpos_switch_x5[0]=(float)fr["cfg_avt_403"];
					viewParam.Boresightpos_switch_y5[0]=(float)fr["cfg_avt_404"];
					/*通道１连续视场*/
					viewParam.zoombak1[0]=(float)fr["cfg_avt_416"];
					value =(float)fr["cfg_avt_417"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue1[0]=value;
					value =(float)fr["cfg_avt_418"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue1[0]=value;
					viewParam. Boresightpos_continue_x1[0]=(float)fr["cfg_avt_419"];
					viewParam.Boresightpos_continue_y1[0]=(float)fr["cfg_avt_420"];

					viewParam.zoombak2[0]=(float)fr["cfg_avt_421"];
					value=(float)fr["cfg_avt_422"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue2[0]=value;
					value=(float)fr["cfg_avt_423"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue2[0]=value;
					viewParam. Boresightpos_continue_x2[0]=(float)fr["cfg_avt_424"];
					viewParam.Boresightpos_continue_y2[0]=(float)fr["cfg_avt_425"];

					viewParam.zoombak3[0]=(float)fr["cfg_avt_426"];
					value =(float)fr["cfg_avt_427"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue3[0]=value;
					value =(float)fr["cfg_avt_428"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue3[0]=value;
					viewParam. Boresightpos_continue_x3[0]=(float)fr["cfg_avt_429"];
					viewParam.Boresightpos_continue_y3[0]=(float)fr["cfg_avt_430"];

					viewParam.zoombak4[0]=(float)fr["cfg_avt_431"];
					value =(float)fr["cfg_avt_880"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue4[0]=value;
					value =(float)fr["cfg_avt_881"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue4[0]=value;
					viewParam. Boresightpos_continue_x4[0]=(float)fr["cfg_avt_882"];
					viewParam.Boresightpos_continue_y4[0]=(float)fr["cfg_avt_883"];

					viewParam.zoombak5[0]=(float)fr["cfg_avt_884"];
					value =(float)fr["cfg_avt_885"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue5[0]=value;
					value =(float)fr["cfg_avt_886"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue5[0]=value;
				   	viewParam. Boresightpos_continue_x5[0]=(float)fr["cfg_avt_887"];
					viewParam.Boresightpos_continue_y5[0]=(float)fr["cfg_avt_888"];

					viewParam.zoombak6[0]=(float)fr["cfg_avt_889"];
					value =(float)fr["cfg_avt_890"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue6[0]=value;
					value =(float)fr["cfg_avt_891"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue6[0]=value;
					viewParam. Boresightpos_continue_x6[0]=(float)fr["cfg_avt_892"];
					viewParam.Boresightpos_continue_y6[0]=(float)fr["cfg_avt_893"];

					viewParam.zoombak7[0]=(float)fr["cfg_avt_894"];
					value =(float)fr["cfg_avt_895"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue7[0]=value;
					value =(float)fr["cfg_avt_896"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue7[0]=value;
					viewParam. Boresightpos_continue_x7[0]=(float)fr["cfg_avt_897"];
					viewParam.Boresightpos_continue_y7[0]=(float)fr["cfg_avt_898"];

	/*通道２固定视场*/

					viewParam.radio[1]=(float)fr["cfg_avt_918"];
					value =(float)fr["cfg_avt_928"];
					value = value* (PI / 180) * 1000;
					viewParam.level_Fov_fix[1]= value;
					value =(float)fr["cfg_avt_929"];
					value = value* (PI / 180) * 1000;
					viewParam.vertical_Fov_fix[1]=value;
					viewParam.Boresightpos_fix_x[1]=(float)fr["cfg_avt_930"];
					viewParam.Boresightpos_fix_y[1]=(float)fr["cfg_avt_931"];
					/*通道2可切换视场*/
					value =(float)fr["cfg_avt_945"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch1[1]= value;
					value =(float)fr["cfg_avt_946"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch1[1]=value;
					viewParam.Boresightpos_switch_x1[1]=(float)fr["cfg_avt_947"];
					viewParam.Boresightpos_switch_y1[1]=(float)fr["cfg_avt_948"];

					value =(float)fr["cfg_avt_949"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch2[1]=value;
					value =(float)fr["cfg_avt_950"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch2[1]=value;
					viewParam.Boresightpos_switch_x2[1]=(float)fr["cfg_avt_951"];
					viewParam.Boresightpos_switch_y2[1]=(float)fr["cfg_avt_952"];
					value=(float)fr["cfg_avt_953"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch3[1]=value;
					value =(float)fr["cfg_avt_954"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch3[1]= value;
					viewParam.Boresightpos_switch_x3[1]=(float)fr["cfg_avt_955"];
					viewParam.Boresightpos_switch_y3[1]=(float)fr["cfg_avt_956"];
					value=(float)fr["cfg_avt_957"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch4[1]=value;
					value=(float)fr["cfg_avt_958"];
					value=value*(PI / 180) * 1000;
                    viewParam.vertical_Fov_switch4[1]=value;
					viewParam.Boresightpos_switch_x4[1]=(float)fr["cfg_avt_959"];
					viewParam.Boresightpos_switch_y4[1]=(float)fr["cfg_avt_960"];
					value=(float)fr["cfg_avt_961"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch5[1]=value;
					value=(float)fr["cfg_avt_962"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch5[1]=value;
					viewParam.Boresightpos_switch_x5[1]=(float)fr["cfg_avt_963"];
					viewParam.Boresightpos_switch_y5[1]=(float)fr["cfg_avt_964"];
					/*通道2连续视场*/
					viewParam.zoombak1[1]=(float)fr["cfg_avt_976"];
					value=(float)fr["cfg_avt_977"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue1[1]=value;
					value=(float)fr["cfg_avt_978"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue1[1]=value;
					viewParam. Boresightpos_continue_x1[1]=(float)fr["cfg_avt_979"];
					viewParam.Boresightpos_continue_y1[1]=(float)fr["cfg_avt_980"];

					viewParam.zoombak2[1]=(float)fr["cfg_avt_981"];
					value=(float)fr["cfg_avt_982"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue2[1]=value;
					value=(float)fr["cfg_avt_983"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue2[1]= value;
					viewParam. Boresightpos_continue_x2[1]=(float)fr["cfg_avt_984"];
					viewParam.Boresightpos_continue_y2[1]=(float)fr["cfg_avt_985"];

					viewParam.zoombak3[1]=(float)fr["cfg_avt_986"];
					value=(float)fr["cfg_avt_987"];
					value=  value*(PI / 180) * 1000;
					viewParam.level_Fov_continue3[1]=  value;
					value=(float)fr["cfg_avt_988"];
					value=  value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue3[1]=value;
					viewParam. Boresightpos_continue_x3[1]=(float)fr["cfg_avt_989"];
					viewParam.Boresightpos_continue_y3[1]=(float)fr["cfg_avt_990"];

					viewParam.zoombak4[1]=(float)fr["cfg_avt_991"];
					value=(float)fr["cfg_avt_992"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue4[1]=value;
					value=(float)fr["cfg_avt_993"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue4[1]=value;
					viewParam. Boresightpos_continue_x4[1]=(float)fr["cfg_avt_994"];
					viewParam.Boresightpos_continue_y4[1]=(float)fr["cfg_avt_995"];

					viewParam.zoombak5[1]=(float)fr["cfg_avt_996"];
					value=(float)fr["cfg_avt_997"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue5[1]=value;
					value=(float)fr["cfg_avt_998"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue5[1]=value;
					viewParam. Boresightpos_continue_x5[1]=(float)fr["cfg_avt_999"];
					viewParam.Boresightpos_continue_y5[1]=(float)fr["cfg_avt_1000"];

					viewParam.zoombak6[1]=(float)fr["cfg_avt_1001"];
					value=(float)fr["cfg_avt_1002"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue6[1]=value;
					value=(float)fr["cfg_avt_1003"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue6[1]=value;
					viewParam. Boresightpos_continue_x6[1]=(float)fr["cfg_avt_1004"];
					viewParam.Boresightpos_continue_y6[1]=(float)fr["cfg_avt_1005"];

					viewParam.zoombak7[1]=(float)fr["cfg_avt_1006"];
					value=(float)fr["cfg_avt_1007"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue7[1]=value;
					value=(float)fr["cfg_avt_1008"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue7[1]=value;
					viewParam. Boresightpos_continue_x7[1]=(float)fr["cfg_avt_1009"];
					viewParam.Boresightpos_continue_y7[1]=(float)fr["cfg_avt_1010"];

					/*通道3固定视场*/

					viewParam.radio[2]=(float)fr["cfg_avt_1030"];
					value=(float)fr["cfg_avt_1042"];
					value =value* (PI / 180) * 1000;
					viewParam.level_Fov_fix[2]= value;
					value =(float)fr["cfg_avt_1043"];
					value = value * (PI / 180) * 1000;
					viewParam.vertical_Fov_fix[2]=value;
					viewParam.Boresightpos_fix_x[2]=(float)fr["cfg_avt_1044"];
					viewParam.Boresightpos_fix_y[2]=(float)fr["cfg_avt_1045"];

					/*通道3可切换视场*/
					value=(float)fr["cfg_avt_1057"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch1[2]=value;
					value =(float)fr["cfg_avt_1058"];
					value = value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch1[2]= value;
					viewParam.Boresightpos_switch_x1[2]=(float)fr["cfg_avt_1059"];
					viewParam.Boresightpos_switch_y1[2]=(float)fr["cfg_avt_1060"];

					value=(float)fr["cfg_avt_1061"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch2[2]=value;
					value =(float)fr["cfg_avt_1062"];
					value = value * (PI / 180) * 1000;
					viewParam.vertical_Fov_switch2[2]=value;
					viewParam.Boresightpos_switch_x2[2]=(float)fr["cfg_avt_1063"];
					viewParam.Boresightpos_switch_y2[2]=(float)fr["cfg_avt_1064"];

					value=(float)fr["cfg_avt_1065"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch3[2]=value;
					value =(float)fr["cfg_avt_1066"];
					value = value * (PI / 180) * 1000;
					viewParam.vertical_Fov_switch2[2]=value;
					viewParam.Boresightpos_switch_x3[2]=(float)fr["cfg_avt_1067"];
					viewParam.Boresightpos_switch_y3[2]=(float)fr["cfg_avt_1068"];

					value=(float)fr["cfg_avt_1069"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch4[2]=value;
					value =(float)fr["cfg_avt_1070"];
					value = value * (PI / 180) * 1000;
					viewParam.vertical_Fov_switch4[2]=value;
					viewParam.Boresightpos_switch_x4[2]=(float)fr["cfg_avt_1071"];
					viewParam.Boresightpos_switch_y4[2]=(float)fr["cfg_avt_1072"];

					value=(float)fr["cfg_avt_1073"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_switch5[2]=value;
					value=(float)fr["cfg_avt_1074"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_switch5[2]=value;
					viewParam.Boresightpos_switch_x5[2]=(float)fr["cfg_avt_1075"];
					viewParam.Boresightpos_switch_y5[2]=(float)fr["cfg_avt_1076"];
					/*通道3连续视场*/
					viewParam.zoombak1[2]=(int)fr["cfg_avt_1088"];

					value=(float)fr["cfg_avt_1089"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue1[2]=value;
					value=(float)fr["cfg_avt_1090"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue1[2]=value;
					viewParam. Boresightpos_continue_x1[2]=(float)fr["cfg_avt_1091"];
					viewParam.Boresightpos_continue_y1[2]=(float)fr["cfg_avt_1092"];

					viewParam.zoombak2[2]=(float)fr["cfg_avt_1093"];
					value=(float)fr["cfg_avt_1094"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue2[2]=value;
					value=(float)fr["cfg_avt_1095"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue2[2]=value;
					viewParam. Boresightpos_continue_x2[2]=(float)fr["cfg_avt_1096"];
					viewParam.Boresightpos_continue_y2[2]=(float)fr["cfg_avt_1097"];

					viewParam.zoombak3[2]=(float)fr["cfg_avt_1098"];
					value=(float)fr["cfg_avt_1099"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue3[2]=value;
					value=(float)fr["cfg_avt_1100"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue3[2]=value;
					viewParam. Boresightpos_continue_x3[2]=(float)fr["cfg_avt_1101"];
					viewParam.Boresightpos_continue_y3[2]=(float)fr["cfg_avt_1102"];

					viewParam.zoombak4[2]=(float)fr["cfg_avt_1103"];
					value=(float)fr["cfg_avt_1104"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue4[2]=value;
					value=(float)fr["cfg_avt_1105"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue4[2]=value;
					viewParam. Boresightpos_continue_x4[2]=(float)fr["cfg_avt_1106"];
					viewParam.Boresightpos_continue_y4[2]=(float)fr["cfg_avt_1107"];

					viewParam.zoombak5[2]=(float)fr["cfg_avt_1108"];
					value=(float)fr["cfg_avt_1109"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue5[2]=value;
					value=(float)fr["cfg_avt_1099"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue5[2]=value;
					viewParam. Boresightpos_continue_x5[2]=(float)fr["cfg_avt_1111"];
					viewParam.Boresightpos_continue_y5[2]=(float)fr["cfg_avt_1112"];

					viewParam.zoombak6[2]=(float)fr["cfg_avt_1113"];
					value=(float)fr["cfg_avt_1114"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue6[2]=value;
					value=(float)fr["cfg_avt_1099"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue6[2]=value;
					viewParam. Boresightpos_continue_x6[2]=(float)fr["cfg_avt_1116"];
					viewParam.Boresightpos_continue_y6[2]=(float)fr["cfg_avt_1117"];

					viewParam.zoombak7[2]=(float)fr["cfg_avt_1118"];
					value=(float)fr["cfg_avt_1119"];
					value=value*(PI / 180) * 1000;
					viewParam.level_Fov_continue7[2]=value;
					value=(float)fr["cfg_avt_1120"];
					value=value*(PI / 180) * 1000;
					viewParam.vertical_Fov_continue7[2]=value;
					viewParam. Boresightpos_continue_x7[2]=(float)fr["cfg_avt_1121"];
					viewParam.Boresightpos_continue_y7[2]=(float)fr["cfg_avt_1122"];

                    /*通道4固定视场*/
					  viewParam.radio[3]=(float)fr["cfg_avt_1142"];
					  value=(float)fr["cfg_avt_1152"];
					  value = value* (PI / 180) * 1000;
					  viewParam.level_Fov_fix[3]= value;
					  value =(float)fr["cfg_avt_1153"];
					  value = value * (PI / 180) * 1000;
					  viewParam.vertical_Fov_fix[3]=value;
					  viewParam.Boresightpos_fix_x[3]=(int)fr["cfg_avt_1154"];
					  viewParam.Boresightpos_fix_y[3]=(int)fr["cfg_avt_1155"];
					  /*通道4可切换视场*/
					  value=(float)fr["cfg_avt_1169"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch1[3]= value;
					  value =(float)fr["cfg_avt_1170"];
					  value = value * (PI / 180) * 1000;
					  viewParam.vertical_Fov_switch1[3]=value;
					  viewParam.Boresightpos_switch_x1[3]=(int)fr["cfg_avt_1171"];
					  viewParam.Boresightpos_switch_y1[3]=(int)fr["cfg_avt_1172"];

					  value =(float)fr["cfg_avt_1173"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch2[3]=value;
					  value =(float)fr["cfg_avt_1174"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch2[3]=value;
					  viewParam.Boresightpos_switch_x2[3]=(int)fr["cfg_avt_1175"];
					  viewParam.Boresightpos_switch_y2[3]=(int)fr["cfg_avt_1176"];

					  value=(float)fr["cfg_avt_1177"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch3[3]=value;
					  value=(float)fr["cfg_avt_1178"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch3[3]=value;
					  viewParam.Boresightpos_switch_x3[3]=(int)fr["cfg_avt_1179"];
					  viewParam.Boresightpos_switch_y3[3]=(int)fr["cfg_avt_1180"];

					  value=(float)fr["cfg_avt_1181"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch4[3]=value;
					  value=(float)fr["cfg_avt_1182"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch4[3]=value;
					  viewParam.Boresightpos_switch_x4[3]=(int)fr["cfg_avt_1183"];
					  viewParam.Boresightpos_switch_y4[3]=(int)fr["cfg_avt_1184"];

					  value =(float)fr["cfg_avt_1185"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch5[3]=value;
					  value =(float)fr["cfg_avt_1186"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch5[3]=value;
					  viewParam.Boresightpos_switch_x5[3]=(int)fr["cfg_avt_1187"];
					  viewParam.Boresightpos_switch_y5[3]=(int)fr["cfg_avt_1188"];

					  /*通道4连续视场*/
					  viewParam.zoombak1[3]=(int)fr["cfg_avt_1200"];
					  value=(float)fr["cfg_avt_1201"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue1[3]=value;
					  value=(float)fr["cfg_avt_1202"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue1[3]=value;
					  viewParam.Boresightpos_continue_x1[3]=(int)fr["cfg_avt_1203"];
					  viewParam.Boresightpos_continue_y1[3]=(int)fr["cfg_avt_1204"];

					  viewParam.zoombak2[3]=(int)fr["cfg_avt_1205"];
					  value=(float)fr["cfg_avt_1206"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue2[3]=value;
					  value=(float)fr["cfg_avt_1207"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue2[3]=value;
					  viewParam.Boresightpos_continue_x2[3]=(int)fr["cfg_avt_1208"];
					  viewParam.Boresightpos_continue_y2[3]=(int)fr["cfg_avt_1209"];

					  viewParam.zoombak3[3]=(int)fr["cfg_avt_1210"];
					  value=(float)fr["cfg_avt_1211"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue3[3]=value;
					  value=(float)fr["cfg_avt_1212"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue3[3]=value;
					  viewParam.Boresightpos_continue_x3[3]=(int)fr["cfg_avt_1213"];
					  viewParam.Boresightpos_continue_y3[3]=(int)fr["cfg_avt_1214"];

					  viewParam.zoombak4[3]=(int)fr["cfg_avt_1215"];
					  value=(float)fr["cfg_avt_1216"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue4[3]=value;
					  value=(float)fr["cfg_avt_1217"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue4[3]=value;
					  viewParam.Boresightpos_continue_x4[3]=(int)fr["cfg_avt_1218"];
					  viewParam.Boresightpos_continue_y4[3]=(int)fr["cfg_avt_1219"];


					  viewParam.zoombak5[3]=(int)fr["cfg_avt_1220"];
					  value=(float)fr["cfg_avt_1221"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue5[3]=value;
					  value=(float)fr["cfg_avt_1222"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue5[3]=value;
					  viewParam.Boresightpos_continue_x5[3]=(int)fr["cfg_avt_1123"];
					  viewParam.Boresightpos_continue_y5[3]=(int)fr["cfg_avt_1124"];

					  viewParam.zoombak6[3]=(int)fr["cfg_avt_1225"];
					  value=(float)fr["cfg_avt_1226"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue6[3]=value;
					  value=(float)fr["cfg_avt_1227"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue6[3]=value;
					  viewParam.Boresightpos_continue_x6[3]=(int)fr["cfg_avt_1228"];
					  viewParam.Boresightpos_continue_y6[3]=(int)fr["cfg_avt_1229"];

					  viewParam.zoombak7[3]=(int)fr["cfg_avt_1230"];
					  value=(float)fr["cfg_avt_1231"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue7[3]=value;
					  value=(float)fr["cfg_avt_1232"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue7[3]=value;
					  viewParam.Boresightpos_continue_x7[3]=(int)fr["cfg_avt_1233"];
					  viewParam.Boresightpos_continue_y7[3]=(int)fr["cfg_avt_1234"];

	  /*通道５*固定视场*/
					  viewParam.radio[4]=(float)fr["cfg_avt_1254"];
					  value=(float)fr["cfg_avt_1264"];
					  value=value* (PI / 180) * 1000;
					  viewParam.level_Fov_fix[4]= value;
					  value =(float)fr["cfg_avt_1265"];
					  value = value * (PI / 180) * 1000;
					  viewParam.vertical_Fov_fix[4]=value;
					  viewParam.Boresightpos_fix_x[4]=(int)fr["cfg_avt_1266"];
					  viewParam.Boresightpos_fix_y[4]=(int)fr["cfg_avt_1267"];
					  /*通道5可切换视场*/
					  value=(float)fr["cfg_avt_1281"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch1[4]= value;
					  value=(float)fr["cfg_avt_1282"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch1[4]=value;
					  viewParam.Boresightpos_switch_x1[4]=(int)fr["cfg_avt_1283"];
					  viewParam.Boresightpos_switch_y1[4]=(int)fr["cfg_avt_1284"];

					  value=(float)fr["cfg_avt_1285"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch2[4]=value;
					  value=(float)fr["cfg_avt_1286"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch2[4]=value;
					  viewParam.Boresightpos_switch_x2[4]=(int)fr["cfg_avt_1287"];
					  viewParam.Boresightpos_switch_y2[4]=(int)fr["cfg_avt_1288"];

					  value=(float)fr["cfg_avt_1289"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch3[4]=value;
					  value=(float)fr["cfg_avt_1290"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch3[4]=value;
					  viewParam.Boresightpos_switch_x3[4]=(int)fr["cfg_avt_1291"];
					  viewParam.Boresightpos_switch_y3[4]=(int)fr["cfg_avt_1292"];

					  value=(float)fr["cfg_avt_1293"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch4[4]=value;
					  value=(float)fr["cfg_avt_1294"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch4[4]=value;
					  viewParam.Boresightpos_switch_x4[4]=(int)fr["cfg_avt_1295"];
					  viewParam.Boresightpos_switch_y4[4]=(int)fr["cfg_avt_1296"];

					  value=(float)fr["cfg_avt_1297"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_switch5[4]=value;
					  value=(float)fr["cfg_avt_1298"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_switch5[4]=value;
					  viewParam.Boresightpos_switch_x5[4]=(int)fr["cfg_avt_1299"];
					  viewParam.Boresightpos_switch_y5[4]=(int)fr["cfg_avt_1300"];
					  /*通道5连续视场*/
					  viewParam.zoombak1[4]=(int)fr["cfg_avt_1312"];
					  value=(float)fr["cfg_avt_1313"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue1[4]=value;
					  value=(float)fr["cfg_avt_1314"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue1[4]=value;
					  viewParam.Boresightpos_continue_x1[4]= (int)fr["cfg_avt_1315"];
					  viewParam.Boresightpos_continue_y1[4]=(int)fr["cfg_avt_1316"];

					  viewParam.zoombak2[4]=(int)fr["cfg_avt_1317"];
					  value =(float)fr["cfg_avt_1318"];
					  value =value *(PI / 180) * 1000;
					  viewParam.level_Fov_continue2[4]=value;
					  value =(float)fr["cfg_avt_1319"];
					  value =value *(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue2[4]=(int)value;
					  viewParam.Boresightpos_continue_x2[4]=(int)fr["cfg_avt_1320"];
					  viewParam.Boresightpos_continue_y2[4]=(int)fr["cfg_avt_1321"];

					  viewParam.zoombak3[ 4]=(int)fr["cfg_avt_1322"];
					  value=(float)fr["cfg_avt_1323"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue3[4]=value;
					  value=(float)fr["cfg_avt_1324"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue3[4]=value;
					  viewParam.Boresightpos_continue_x3[4]=(int)fr["cfg_avt_1325"];
					  viewParam.Boresightpos_continue_y3[4]=(int)fr["cfg_avt_1326"];

					  viewParam.zoombak4[4]=(int)fr["cfg_avt_1327"];
					  value =(float)fr["cfg_avt_1328"];
					  value =value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue4[4]=value;
					  value =(float)fr["cfg_avt_1329"];
					  value =value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue4[4]=value;
					  viewParam.Boresightpos_continue_x4[4]=(int)fr["cfg_avt_1330"];
					  viewParam.Boresightpos_continue_y4[4]=(int)fr["cfg_avt_1331"];


					  viewParam.zoombak5[4]=(int)fr["cfg_avt_1332"];
					  value=(float)fr["cfg_avt_1333"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue5[4]=value;
					  value=(float)fr["cfg_avt_1334"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue5[4]=value;
					  viewParam.Boresightpos_continue_x5[4]=(int)fr["cfg_avt_1335"];
					  viewParam.Boresightpos_continue_y5[4]=(int)fr["cfg_avt_1336"];

					  viewParam.zoombak6[4]=(int)fr["cfg_avt_1337"];
					  value=(float)fr["cfg_avt_1338"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue6[4]=value;
					  value=(float)fr["cfg_avt_1339"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue6[4]=value;
					  viewParam.Boresightpos_continue_x6[4]=(int)fr["cfg_avt_1340"];
					  viewParam.Boresightpos_continue_y6[4]=(int)fr["cfg_avt_1341"];

					  viewParam.zoombak7[4]=(int)fr["cfg_avt_1342"];
					  value=(float)fr["cfg_avt_1343"];
					  value=value*(PI / 180) * 1000;
					  viewParam.level_Fov_continue7[4]=value;
					  value=(float)fr["cfg_avt_1344"];
					  value=value*(PI / 180) * 1000;
					  viewParam.vertical_Fov_continue7[4]=value;
					  viewParam.Boresightpos_continue_x7[4]=(int)fr["cfg_avt_1345"];
					  viewParam.Boresightpos_continue_y7[4]=(int)fr["cfg_avt_1346"];


#if 0
					string commdevice= (string)fr["cfg_avt_432"];
					commdevice.copy((char *)m_GlobalDate->m_uart_params.device, commdevice.length(),0);
					//uart.uartDevice=(string)fr["cfg_avt_432"];
					uart.baud=(int)fr["cfg_avt_433"];
					uart.data=(int)fr["cfg_avt_434"];
					uart.check=(int)fr["cfg_avt_435"];
					uart.stop=(int)fr["cfg_avt_436"];
					uart.flow=(int)fr["cfg_avt_437"];
#endif
				}


				{
					/*speed convert
					 * chid 1*/
					m_InitParam.demandMaxX[0]= (float)fr["cfg_avt_35"];
					m_InitParam.demandMaxY[0]=(float)fr["cfg_avt_36"];
					m_InitParam.deadbandX[0] = (float)fr["cfg_avt_37"];
					m_InitParam.deadbandY[0] = (float)fr["cfg_avt_38"];

					m_ptzSpeed.panSpeed[0].x0 = (int)fr["cfg_avt_768"];
					m_ptzSpeed.panSpeed[0].x1 = (int)fr["cfg_avt_769"];
					m_ptzSpeed.panSpeed[0].x2 = (int)fr["cfg_avt_770"];
					m_ptzSpeed.panSpeed[0].x3 = (int)fr["cfg_avt_771"];
					m_ptzSpeed.panSpeed[0].x4 = (int)fr["cfg_avt_772"];
					m_ptzSpeed.panSpeed[0].x5 = (int)fr["cfg_avt_773"];
					m_ptzSpeed.panSpeed[0].x6 = (int)fr["cfg_avt_774"];
					m_ptzSpeed.panSpeed[0].x7 = (int)fr["cfg_avt_775"];
					m_ptzSpeed.panSpeed[0].x8 = (int)fr["cfg_avt_776"];
					m_ptzSpeed.panSpeed[0].x9 = (int)fr["cfg_avt_786"];

					m_ptzSpeed.TiltSpeed[0].y0 = (int)fr["cfg_avt_777"];
					m_ptzSpeed.TiltSpeed[0].y1 = (int)fr["cfg_avt_778"];
					m_ptzSpeed.TiltSpeed[0].y2 = (int)fr["cfg_avt_779"];
					m_ptzSpeed.TiltSpeed[0].y3 = (int)fr["cfg_avt_780"];
					m_ptzSpeed.TiltSpeed[0].y4 = (int)fr["cfg_avt_781"];
					m_ptzSpeed.TiltSpeed[0].y5 = (int)fr["cfg_avt_782"];
					m_ptzSpeed.TiltSpeed[0].y6 = (int)fr["cfg_avt_783"];
					m_ptzSpeed.TiltSpeed[0].y7 = (int)fr["cfg_avt_784"];
					m_ptzSpeed.TiltSpeed[0].y8 = (int)fr["cfg_avt_785"];
					m_ptzSpeed.TiltSpeed[0].y9 = (int)fr["cfg_avt_787"];

					/*speed convert
					 * chid 2*/
					m_InitParam.demandMaxX[1]= (float)fr["cfg_avt_1588"];
					m_InitParam.demandMaxY[1]=(float)fr["cfg_avt_1589"];
					m_InitParam.deadbandX[1] = (float)fr["cfg_avt_1590"];
					m_InitParam.deadbandY[1] = (float)fr["cfg_avt_1591"];

					m_ptzSpeed.panSpeed[1].x0 = (int)fr["cfg_avt_1568"];
					m_ptzSpeed.panSpeed[1].x1 = (int)fr["cfg_avt_1569"];
					m_ptzSpeed.panSpeed[1].x2 = (int)fr["cfg_avt_1570"];
					m_ptzSpeed.panSpeed[1].x3 = (int)fr["cfg_avt_1571"];
					m_ptzSpeed.panSpeed[1].x4 = (int)fr["cfg_avt_1572"];
					m_ptzSpeed.panSpeed[1].x5 = (int)fr["cfg_avt_1573"];
					m_ptzSpeed.panSpeed[1].x6 = (int)fr["cfg_avt_1574"];
					m_ptzSpeed.panSpeed[1].x7 = (int)fr["cfg_avt_1575"];
					m_ptzSpeed.panSpeed[1].x8 = (int)fr["cfg_avt_1576"];
					m_ptzSpeed.panSpeed[1].x9 = (int)fr["cfg_avt_1586"];

					m_ptzSpeed.TiltSpeed[1].y0 = (int)fr["cfg_avt_1577"];
					m_ptzSpeed.TiltSpeed[1].y1 = (int)fr["cfg_avt_1578"];
					m_ptzSpeed.TiltSpeed[1].y2 = (int)fr["cfg_avt_1579"];
					m_ptzSpeed.TiltSpeed[1].y3 = (int)fr["cfg_avt_1580"];
					m_ptzSpeed.TiltSpeed[1].y4 = (int)fr["cfg_avt_1581"];
					m_ptzSpeed.TiltSpeed[1].y5 = (int)fr["cfg_avt_1582"];
					m_ptzSpeed.TiltSpeed[1].y6 = (int)fr["cfg_avt_1583"];
					m_ptzSpeed.TiltSpeed[1].y7 = (int)fr["cfg_avt_1584"];
					m_ptzSpeed.TiltSpeed[1].y8 = (int)fr["cfg_avt_1585"];
					m_ptzSpeed.TiltSpeed[1].y9 = (int)fr["cfg_avt_1587"];

					/*speed convert
					 * chid 3*/
					m_InitParam.demandMaxX[2]= (float)fr["cfg_avt_1620"];
					m_InitParam.demandMaxY[2]=(float)fr["cfg_avt_1621"];
					m_InitParam.deadbandX[2] = (float)fr["cfg_avt_1622"];
					m_InitParam.deadbandY[2] = (float)fr["cfg_avt_1623"];

					m_ptzSpeed.panSpeed[2].x0 = (int)fr["cfg_avt_1600"];
					m_ptzSpeed.panSpeed[2].x1 = (int)fr["cfg_avt_1601"];
					m_ptzSpeed.panSpeed[2].x2 = (int)fr["cfg_avt_1602"];
					m_ptzSpeed.panSpeed[2].x3 = (int)fr["cfg_avt_1603"];
					m_ptzSpeed.panSpeed[2].x4 = (int)fr["cfg_avt_1604"];
					m_ptzSpeed.panSpeed[2].x5 = (int)fr["cfg_avt_1605"];
					m_ptzSpeed.panSpeed[2].x6 = (int)fr["cfg_avt_1606"];
					m_ptzSpeed.panSpeed[2].x7 = (int)fr["cfg_avt_1607"];
					m_ptzSpeed.panSpeed[2].x8 = (int)fr["cfg_avt_1608"];
					m_ptzSpeed.panSpeed[2].x9 = (int)fr["cfg_avt_1618"];

					m_ptzSpeed.TiltSpeed[2].y0 = (int)fr["cfg_avt_1609"];
					m_ptzSpeed.TiltSpeed[2].y1 = (int)fr["cfg_avt_1610"];
					m_ptzSpeed.TiltSpeed[2].y2 = (int)fr["cfg_avt_1611"];
					m_ptzSpeed.TiltSpeed[2].y3 = (int)fr["cfg_avt_1612"];
					m_ptzSpeed.TiltSpeed[2].y4 = (int)fr["cfg_avt_1613"];
					m_ptzSpeed.TiltSpeed[2].y5 = (int)fr["cfg_avt_1614"];
					m_ptzSpeed.TiltSpeed[2].y6 = (int)fr["cfg_avt_1615"];
					m_ptzSpeed.TiltSpeed[2].y7 = (int)fr["cfg_avt_1616"];
					m_ptzSpeed.TiltSpeed[2].y8 = (int)fr["cfg_avt_1617"];
					m_ptzSpeed.TiltSpeed[2].y9 = (int)fr["cfg_avt_1619"];

					/*speed convert
					 * chid 4*/
					m_InitParam.demandMaxX[3]= (float)fr["cfg_avt_1652"];
					m_InitParam.demandMaxY[3]=(float)fr["cfg_avt_1653"];
					m_InitParam.deadbandX[3] = (float)fr["cfg_avt_1654"];
					m_InitParam.deadbandY[3] = (float)fr["cfg_avt_1655"];

					m_ptzSpeed.panSpeed[3].x0 = (int)fr["cfg_avt_1632"];
					m_ptzSpeed.panSpeed[3].x1 = (int)fr["cfg_avt_1633"];
					m_ptzSpeed.panSpeed[3].x2 = (int)fr["cfg_avt_1634"];
					m_ptzSpeed.panSpeed[3].x3 = (int)fr["cfg_avt_1635"];
					m_ptzSpeed.panSpeed[3].x4 = (int)fr["cfg_avt_1636"];
					m_ptzSpeed.panSpeed[3].x5 = (int)fr["cfg_avt_1637"];
					m_ptzSpeed.panSpeed[3].x6 = (int)fr["cfg_avt_1638"];
					m_ptzSpeed.panSpeed[3].x7 = (int)fr["cfg_avt_1639"];
					m_ptzSpeed.panSpeed[3].x8 = (int)fr["cfg_avt_1640"];
					m_ptzSpeed.panSpeed[3].x9 = (int)fr["cfg_avt_1650"];

					m_ptzSpeed.TiltSpeed[3].y0 = (int)fr["cfg_avt_1641"];
					m_ptzSpeed.TiltSpeed[3].y1 = (int)fr["cfg_avt_1642"];
					m_ptzSpeed.TiltSpeed[3].y2 = (int)fr["cfg_avt_1643"];
					m_ptzSpeed.TiltSpeed[3].y3 = (int)fr["cfg_avt_1644"];
					m_ptzSpeed.TiltSpeed[3].y4 = (int)fr["cfg_avt_1645"];
					m_ptzSpeed.TiltSpeed[3].y5 = (int)fr["cfg_avt_1646"];
					m_ptzSpeed.TiltSpeed[3].y6 = (int)fr["cfg_avt_1647"];
					m_ptzSpeed.TiltSpeed[3].y7 = (int)fr["cfg_avt_1648"];
					m_ptzSpeed.TiltSpeed[3].y8 = (int)fr["cfg_avt_1649"];
					m_ptzSpeed.TiltSpeed[3].y9 = (int)fr["cfg_avt_1651"];

					/*speed convert
					 * chid 5*/
					m_InitParam.demandMaxX[4]= (float)fr["cfg_avt_1684"];
					m_InitParam.demandMaxY[4]=(float)fr["cfg_avt_1685"];
					m_InitParam.deadbandX[4] = (float)fr["cfg_avt_1687"];
					m_InitParam.deadbandY[4] = (float)fr["cfg_avt_1688"];

					m_ptzSpeed.panSpeed[4].x0 = (int)fr["cfg_avt_1664"];
					m_ptzSpeed.panSpeed[4].x1 = (int)fr["cfg_avt_1665"];
					m_ptzSpeed.panSpeed[4].x2 = (int)fr["cfg_avt_1666"];
					m_ptzSpeed.panSpeed[4].x3 = (int)fr["cfg_avt_1667"];
					m_ptzSpeed.panSpeed[4].x4 = (int)fr["cfg_avt_1668"];
					m_ptzSpeed.panSpeed[4].x5 = (int)fr["cfg_avt_1669"];
					m_ptzSpeed.panSpeed[4].x6 = (int)fr["cfg_avt_1670"];
					m_ptzSpeed.panSpeed[4].x7 = (int)fr["cfg_avt_1671"];
					m_ptzSpeed.panSpeed[4].x8 = (int)fr["cfg_avt_1672"];
					m_ptzSpeed.panSpeed[4].x9 = (int)fr["cfg_avt_1682"];

					m_ptzSpeed.TiltSpeed[4].y0 = (int)fr["cfg_avt_1673"];
					m_ptzSpeed.TiltSpeed[4].y1 = (int)fr["cfg_avt_1674"];
					m_ptzSpeed.TiltSpeed[4].y2 = (int)fr["cfg_avt_1675"];
					m_ptzSpeed.TiltSpeed[4].y3 = (int)fr["cfg_avt_1676"];
					m_ptzSpeed.TiltSpeed[4].y4 = (int)fr["cfg_avt_1677"];
					m_ptzSpeed.TiltSpeed[4].y5 = (int)fr["cfg_avt_1678"];
					m_ptzSpeed.TiltSpeed[4].y6 = (int)fr["cfg_avt_1679"];
					m_ptzSpeed.TiltSpeed[4].y7 = (int)fr["cfg_avt_1680"];
					m_ptzSpeed.TiltSpeed[4].y8 = (int)fr["cfg_avt_1681"];
					m_ptzSpeed.TiltSpeed[4].y9 = (int)fr["cfg_avt_1683"];
				}

				{
					resolu.resolution[ipc_eSen_CH0] = (int)fr["cfg_avt_356"];
					resolu.resolution[ipc_eSen_CH1] = (int)fr["cfg_avt_916"];
					resolu.resolution[ipc_eSen_CH2] = (int)fr["cfg_avt_1028"];
					resolu.resolution[ipc_eSen_CH3] = (int)fr["cfg_avt_1140"];
					resolu.resolution[ipc_eSen_CH4] = (int)fr["cfg_avt_1252"];
					resolu.outputresol = (int)fr["cfg_avt_805"];
					for(int j = 0; j < 4; j++)
					{
						if((0 == resolu.resolution[j]) || (1 == resolu.resolution[j]))
						{
  							ShowDPI[j][0] = 1920;
					    	ShowDPI[j][1] = 1080;
						}
						else if((2 == resolu.resolution[j]) || (3 == resolu.resolution[j]))
						{
  							ShowDPI[j][0] = 1280;
					    	ShowDPI[j][1] = 720;
						}
					}
					if(0 == resolu.resolution[video_pal])
					{
  						ShowDPI[video_pal][0] = 720;
						ShowDPI[video_pal][1] = 576;
					}
					
					Enableresolution(resolu);
				}

				{
					/*output uart params/
					 * 通道１*/
					string myuartdevice= dev[0] = (string)fr["cfg_avt_816"];
					myuartdevice.copy((char *)m_GlobalDate->m_uart_params[0].device, myuartdevice.length(),0);
					m_GlobalDate->m_uart_params[0].baudrate = (int)fr["cfg_avt_817"];
					m_GlobalDate->m_uart_params[0].databits = (int)fr["cfg_avt_818"];
					string myuartparity = parity[0]= (string)fr["cfg_avt_819"];
					myuartparity.copy((char *)m_GlobalDate->m_uart_params[0].parity, myuartparity.length(),0);
					m_GlobalDate->m_uart_params[0].stopbits = (int)fr["cfg_avt_820"];
					m_GlobalDate->m_uart_params[0].balladdress= (int)fr["cfg_avt_822"];

					 /* 通道2*/
					string myuartdevice2= dev [1]= (string)fr["cfg_avt_1696"];
					myuartdevice2.copy((char *)m_GlobalDate->m_uart_params[1].device, myuartdevice2.length(),0);
					m_GlobalDate->m_uart_params[1].baudrate= (int)fr["cfg_avt_1697"];
					m_GlobalDate->m_uart_params[1].databits = (int)fr["cfg_avt_1698"];
					string myuartparity2 = parity[1] = (string)fr["cfg_avt_1699"];
					myuartparity2.copy((char *)m_GlobalDate->m_uart_params[1].parity, myuartparity2.length(),0);
					m_GlobalDate->m_uart_params[1].stopbits = (int)fr["cfg_avt_1700"];
					m_GlobalDate->m_uart_params[1].balladdress= (int)fr["cfg_avt_1702"];

					 /* 通道3*/
					string myuartdevice3= dev [2]= (string)fr["cfg_avt_1712"];
					myuartdevice3.copy((char *)m_GlobalDate->m_uart_params[2].device, myuartdevice3.length(),0);
					m_GlobalDate->m_uart_params[2].baudrate= (int)fr["cfg_avt_1713"];
					m_GlobalDate->m_uart_params[2].databits = (int)fr["cfg_avt_1714"];
					string myuartparity3 = parity[3] = (string)fr["cfg_avt_1715"];
					myuartparity3.copy((char *)m_GlobalDate->m_uart_params[2].parity, myuartparity3.length(),0);
					m_GlobalDate->m_uart_params[2].stopbits = (int)fr["cfg_avt_1716"];
					m_GlobalDate->m_uart_params[2].balladdress= (int)fr["cfg_avt_1718"];

					 /* 通道4*/
					string myuartdevice4= dev [3]= (string)fr["cfg_avt_1727"];
				    myuartdevice4.copy((char *)m_GlobalDate->m_uart_params[3].device, myuartdevice4.length(),0);
					m_GlobalDate->m_uart_params[3].baudrate= (int)fr["cfg_avt_1728"];
					m_GlobalDate->m_uart_params[3].databits = (int)fr["cfg_avt_1729"];
					string myuartparity4 = parity[3] = (string)fr["cfg_avt_1730"];
					myuartparity.copy((char *)m_GlobalDate->m_uart_params[3].parity, myuartparity4.length(),0);
					m_GlobalDate->m_uart_params[3].stopbits = (int)fr["cfg_avt_1731"];
					m_GlobalDate->m_uart_params[3].balladdress= (int)fr["cfg_avt_1733"];

					 /* 通道5*/
					string myuartdevice5= dev [4]= (string)fr["cfg_avt_1744"];
					myuartdevice5.copy((char *)m_GlobalDate->m_uart_params[4].device, myuartdevice5.length(),0);
					m_GlobalDate->m_uart_params[4].baudrate= (int)fr["cfg_avt_1745"];
					m_GlobalDate->m_uart_params[4].databits = (int)fr["cfg_avt_1746"];
					string myuartparity5 = parity[4] = (string)fr["cfg_avt_1747"];
					myuartparity.copy((char *)m_GlobalDate->m_uart_params[4].parity, myuartparity5.length(),0);
					m_GlobalDate->m_uart_params[4].stopbits = (int)fr["cfg_avt_1748"];
					m_GlobalDate->m_uart_params[4].balladdress= (int)fr["cfg_avt_1750"];


				}

				{
					m_ipc->ipc_OSD->OSD_text_font =(int)fr["cfg_avt_832"];
					m_ipc->ipc_OSD->OSD_text_size =(int)fr["cfg_avt_833"];
				}

				{
					//detect area
					m_GlobalDate->Mtd_Moitor_X = (int)fr["cfg_avt_864"];
					m_GlobalDate->Mtd_Moitor_Y = (int)fr["cfg_avt_865"];
				}

				{
					//Mtd
					m_ipc->pMtd->targetNum = (int)fr["cfg_avt_848"];
					m_ipc->pMtd->maxArea = (int)fr["cfg_avt_850"];
					m_ipc->pMtd->minArea = (int)fr["cfg_avt_851"];
					m_ipc->pMtd->sensitivity = (int)fr["cfg_avt_852"];
					m_ipc->pMtd->trackTime = (int)fr["cfg_avt_855"];
					m_GlobalDate->mtdconfig.trktime = m_ipc->pMtd->trackTime*1000;
				}


			}else{
				printf("Can not find YML. Please put this file into the folder of execute file\n");
				exit (-1);
			}
		}
	}else
		fclose(fp);

	m_ipc->ipc_OSD->OSD_draw_show = 1;
	m_ipc->ipc_OSD->osdUserShow = 1;
	m_ipc->IpcConfigOSDTEXT();
	m_ipc->IPCSendMtdFrame();
	m_ipc->IPCSendBallParam();
	m_ipc->IpcConfig();
}

void CManager::Enableresolution(CMD_IPCRESOLUTION resolu)
{
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution) = resolu.resolution[ipc_eSen_CH0];
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution1) = resolu.resolution[ipc_eSen_CH1];
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution2) = resolu.resolution[ipc_eSen_CH2];
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution3) = resolu.resolution[ipc_eSen_CH3];
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution4) = resolu.resolution[ipc_eSen_CH4];
	m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_OutputResol) = resolu.outputresol;
	Ipc_resolution();
}

void CManager::Ipc_resolution()
{
	CMD_IPCRESOLUTION resolu;
	resolu.resolution[ipc_eSen_CH0] = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution);
	resolu.resolution[ipc_eSen_CH1] = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution1);
	resolu.resolution[ipc_eSen_CH2] = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution2);
	resolu.resolution[ipc_eSen_CH3] = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution3);
	resolu.resolution[ipc_eSen_CH4] = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_Resolution4);
	resolu.outputresol = m_GlobalDate->EXT_Ctrl.at(Cmd_IPC_OutputResol);
    m_ipc->IPCResolution(resolu);
}

void CManager::modifierAVTProfile(int block, int field, float value,char *inBuf)
{
	m_ipc->ipc_OSD = m_ipc->getOSDSharedMem();
	m_ipc->ipc_UTC = m_ipc->getUTCSharedMem();

	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	PlatformFilter_Obj* pPidObj;

	OSD_text* wOSD;
	wOSD = &m_OSD;

	const char* str = "reboot";
	int check = ((block -1) * 16 + field);
printf("modifer============>check = %d  value = %f\n", check, value);
	cfg_value[check] = value;
	switch(check)
	{
		case 1:
			pObj->params.joystickRateDemandParam.fDeadband=(float)value;
			break;

		case 2:
			pObj->params.joystickRateDemandParam.fCutpoint1 = (float)value;
			break;

		case 3:
			pObj->params.joystickRateDemandParam.fInputGain_X1 = (float)value;
			break;

		case 4:
			pObj->params.joystickRateDemandParam.fInputGain_Y1 = (float)value;
			break;

		case 5:
			pObj->params.joystickRateDemandParam.fCutpoint2 = (float)value;
			break;

		case 6:
			pObj->params.joystickRateDemandParam.fInputGain_X2 = (float)value;
			break;

		case 7:
			pObj->params.joystickRateDemandParam.fInputGain_Y2 = (float)value;
			break;

		case 15:
			//debugPlatContrl.panVector = (float)value;
			//printf("%s:line %d     set panVector = %f\n",__func__,__LINE__,debugPlatContrl.panVector);
			break;

		case 17:
			break;
		case 1441:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			pPidObj->params.G = (float)value;
			printf("ppid->G=%f\n", pPidObj->params.G);
			break;
		case 1473:
		case 1505:
		case 1537:

			break;

		case 18:
			break;
		case 1442:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			pPidObj->params.P0= (float)value;//Ki
			printf("ppid->Ki=%f\n", pPidObj->params.P0);
			break;
		case 1474:
		case 1506:
		case 1538:

			break;

		case 19:
			break;
		case 1443:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			pPidObj->params.P1= (float)value;//Kd
			break;
		case 1475:
		case 1507:
		case 1539:

			break;

		case 20:
			break;
		case 1444:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			pPidObj->params.P2 = (float)value;//k
			break;
		case 1476:
		case 1508:
		case 1540:

			break;

		case 21:
			break;
		case 1445:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			pPidObj->params.G =(float) value;//Kp
			break;
		case 1477:
		case 1509:
		case 1541:

			break;

		case 22:
			break;
		case 1446:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			pPidObj->params.P0 = (float)value;//Ki
			break;
		case 1478:
		case 1510:
		case 1542:

			break;

		case 23:
			break;
		case 1447:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			pPidObj->params.P1 = (float)value;//Kd
			break;
		case 1479:
		case 1511:
		case 1543:

			break;

		case 24:
			break;
		case 1448:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			pPidObj->params.P2 = (float)value;//K
			break;
		case 1480:
		case 1512:
		case 1544:

			break;

		case 757:
			break;
		case 1461:
			pObj->params.bleedX = (float)value;
			break;
		case 1493:
		case 1525:
		case 1557:

			break;

		case 758:
			break;
		case 1462:
			pObj->params.bleedY = (float)value;
			break;
		case 1494:
		case 1526:
		case 1558:

			break;

		case 35:
			break;
		case 1588:
			pObj->params.demandMaxX = (float)value;
			break;
		case 1620:
		case 1652:
		case 1684:
			break;

		case 36:
			break;
		case 1589:
			pObj->params.demandMaxY = (float)value;
			break;
		case 1621:
		case 1653:
		case 1685:

				break;

		case 37:
			break;
		case 1590:
			pObj->params.deadbandX = (float) value;
			break;
		case 1622:
		case 1654:
		case 1686:

			break;

		case 38:
			break;
		case 1591:
			pObj->params.deadbandY = (float)value;
			break;
		case 1623:
		case 1655:
		case 1687:

			break;

		case 39:
			//m_ptzSpeed.curve =(float) value;   //&
			break;
		case 40:
			m_ipc->losttime = (int)value;
			break;

		case 46:
		//	pObj->params.OutputSwitch=(float) value;
				break;
		case 47:
		//	pObj->params.OutputPort=(float) value;
				break;

		case 48:
			m_ipc->ipc_UTC->occlusion_thred = (float)value;
			printf("--------48-------->m_ipc->ipc_UTC->occlusion_thred=%f",m_ipc->ipc_UTC->occlusion_thred);
			break;
		case 49:
			m_ipc->ipc_UTC->retry_acq_thred = (float)value;
			printf("--------49-------->m_ipc->ipc_UTC->retry_acq_thred=%f\n",m_ipc->ipc_UTC->retry_acq_thred);
			break;
		case 50:
			m_ipc->ipc_UTC->up_factor = (float)value;
			break;
		case 51:
			m_ipc->ipc_UTC->res_distance = (int)value;
			break;
		case 52:
			m_ipc->ipc_UTC->res_area = (int)value;
			break;
		case 53:
			m_ipc->ipc_UTC->gapframe = (int)value;
			break;
		case 54:
			m_ipc->ipc_UTC->enhEnable = (int)value;
			break;
		case 55:
			m_ipc->ipc_UTC->cliplimit = (float)value;
			break;
		case 56:
			m_ipc->ipc_UTC->dictEnable = (float)value;
			break;
		case 57:
			m_ipc->ipc_UTC->moveX  = (int)value;
			break;
		case 58:
			m_ipc->ipc_UTC->moveY = (int)value;
			break;
		case 59:
			m_ipc->ipc_UTC->moveX2 = (int)value;
			break;
		case 60:
			m_ipc->ipc_UTC->moveY2  = (int)value;
			break;
		case 61:
			m_ipc->ipc_UTC->segPixelX = (int)value;
			break;
		case 62:
			m_ipc->ipc_UTC->segPixelY  = (int)value;
			break;
		case 63:
			m_ipc->ipc_UTC->algOsdRect_Enable = (int)value;
			break;

		case 64:
			m_ipc->ipc_UTC->ScalerLarge = (int)value;
			printf("--------64-------->m_ipc->ipc_UTC->ScalerLarge=%d\n",m_ipc->ipc_UTC->ScalerLarge);
			break;
		case 65:
			m_ipc->ipc_UTC->ScalerMid = (int)value;
			printf("---------65------->m_ipc->ipc_UTC->ScalerMid=%d\n",m_ipc->ipc_UTC->ScalerMid);
			break;
		case 66:
			m_ipc->ipc_UTC->ScalerSmall = (int)value;
			break;
		case 67:
			m_ipc->ipc_UTC->Scatter  = (int)value;
			break;
		case 68:
			m_ipc->ipc_UTC->ratio = (float)value;
			break;
		case 69:
			m_ipc->ipc_UTC->FilterEnable = (int)value;
			break;
		case 70:
			m_ipc->ipc_UTC->BigSecEnable = (int)value;
			break;
		case 71:
			m_ipc->ipc_UTC->SalientThred = (int)value;
			break;
		case 72:
			m_ipc->ipc_UTC->ScalerEnable = (int)value;
			break;
		case 73:
			m_ipc->ipc_UTC->DynamicRatioEnable = (int)value;
			break;
		case 74:
			m_ipc->ipc_UTC->acqSize_width = (int)value;
			break;
		case 75:
			m_ipc->ipc_UTC->acqSize_height = (int)value;
			break;
		case 76:
			m_ipc->ipc_UTC->TrkAim43_Enable = (int)value;
			break;
		case 77:
			m_ipc->ipc_UTC->SceneMVEnable = (int)value;
			break;
		case 78:
			m_ipc->ipc_UTC->BackTrackEnable = (int)value;
			break;
		case 79:
			m_ipc->ipc_UTC->bAveTrkPos = (int)value;
			break;

		case 80:
			m_ipc->ipc_UTC->fTau = (float)value;
			printf("--------80----->m_ipc->ipc_UTC->fTau =%f\n",m_ipc->ipc_UTC->fTau );
			break;
		case 81:
			m_ipc->ipc_UTC->buildFrms = (float)value;
			break;
		case 82:
			m_ipc->ipc_UTC->LostFrmThred = (int)value;
			break;
		case 83:
			m_ipc->ipc_UTC->histMvThred = (float)value;
			break;
		case 84:
			m_ipc->ipc_UTC->detectFrms = (int)value;
			break;
		case 85:
			m_ipc->ipc_UTC->stillFrms = (int)value;
			break;
		case 86:
			m_ipc->ipc_UTC->stillThred = (int)value;
			break;
		case 87:
			m_ipc->ipc_UTC->bKalmanFilter  = (int)value;
			break;
		case 88:
			m_ipc->ipc_UTC->xMVThred = (float)value;
			break;
		case 89:
			m_ipc->ipc_UTC->yMVThred = (float)value;
			break;
		case 90:
			m_ipc->ipc_UTC->xStillThred = (float)value;
			break;
		case 91:
			m_ipc->ipc_UTC->yStillThred = (float)value;
			break;
		case 92:
			m_ipc->ipc_UTC->slopeThred = (float)value;
			break;
		case 93:
			m_ipc->ipc_UTC->kalmanHistThred = (float)value;
			break;
		case 94:
			m_ipc->ipc_UTC->kalmanCoefQ = (float)value;
			break;
		case 95:
			m_ipc->ipc_UTC->kalmanCoefR = (float)value;
			break;

		case 96:
			wOSD->OSD_text_show[0]  = (int)value;
			printf("change show :%d \n",wOSD->OSD_text_show[0]);
			break;
		case 97:
			wOSD->OSD_text_positionX[0]= (int)value;
			break;
		case 98:
			wOSD->OSD_text_positionY[0] = (int)value;
			break;
		case 99:
			wOSD->OSD_text_content[0] = inBuf;
			break;
		case 101:
			wOSD->OSD_text_color[0] = (int)value;
			break;
		case 102:
			wOSD->OSD_text_alpha[0] = (int)value;
			break;


		case 112:
			wOSD->OSD_text_show[1]=(int)value;
			break;
		case 113:
			wOSD->OSD_text_positionX[1]=(int)value;
			break;
		case 114:
			wOSD->OSD_text_positionY[1]=(int)value;
			break;
		case 115:
			wOSD->OSD_text_content[1] = inBuf;
			break;
		case 117:
			wOSD->OSD_text_color[1]=(int)value;
			break;
		case 118:
			wOSD->OSD_text_alpha[1]=(int)value;
			break;


		case 128:
			wOSD->OSD_text_show[2]=(int)value;
			break;
		case 129:
			wOSD->OSD_text_positionX[2]=(int)value;
			break;
		case 130:
			wOSD->OSD_text_positionY[2]=(int)value;
			break;
		case 131:
			wOSD->OSD_text_content[2] = inBuf;
			break;
		case 133:
			wOSD->OSD_text_color[2]=(int)value;
			break;
		case 134:
			wOSD->OSD_text_alpha[2]=(int)value;
			break;


		case 144:
			wOSD->OSD_text_show[3]=(int)value;
			break;
		case 145:
			wOSD->OSD_text_positionX[3]=(int)value;
			break;
		case 146:
			wOSD->OSD_text_positionY[3]=(int)value;
			break;
		case 147:
			wOSD->OSD_text_content[3] = inBuf;
			break;
		case 149:
			wOSD->OSD_text_color[3]=(int)value;
			break;
		case 150:
			wOSD->OSD_text_alpha[3]=(int)value;
			break;


		case 160:
			wOSD->OSD_text_show[4]=(int)value;
			break;
		case 161:
			wOSD->OSD_text_positionX[4]=(int)value;
			break;
		case 162:
			wOSD->OSD_text_positionY[4]=(int)value;
			break;
		case 163:
			wOSD->OSD_text_content[4] = inBuf;
			break;
		case 165:
			wOSD->OSD_text_color[4]=(int)value;
			break;
		case 166:
			wOSD->OSD_text_alpha[4]=(int)value;
			break;

		case 176:
			wOSD->OSD_text_show[5]=(int)value;
			break;
		case 177:
			wOSD->OSD_text_positionX[5]=(int)value;
			break;
		case 178:
			wOSD->OSD_text_positionY[5]=(int)value;
			break;
		case 179:
			wOSD->OSD_text_content[5] = inBuf;
			break;
		case 181:
			wOSD->OSD_text_color[5]=(int)value;
			break;
		case 182:
			wOSD->OSD_text_alpha[5]=(int)value;
			break;

		case 192:
			wOSD->OSD_text_show[6]=(int)value;
			break;
		case 193:
			wOSD->OSD_text_positionX[6]=(int)value;
			break;
		case 194:
			wOSD->OSD_text_positionY[6]=(int)value;
			break;
		case 195:
			wOSD->OSD_text_content[6] = inBuf;
			break;
		case 197:
			wOSD->OSD_text_color[6]=(int)value;
			break;
		case 198:
			wOSD->OSD_text_alpha[6]=(int)value;
			break;


		case 208:
			wOSD->OSD_text_show[7]=(int)value;
			break;
		case 209:
			wOSD->OSD_text_positionX[7]=(int)value;
			break;
		case 210:
			wOSD->OSD_text_positionY[7]=(int)value;
			break;
		case 211:
			wOSD->OSD_text_content[7] = inBuf;
			break;
		case 213:
			wOSD->OSD_text_color[7]=(int)value;
			break;
		case 214:
			wOSD->OSD_text_alpha[7]=(int)value;
			break;


		case 224:
			wOSD->OSD_text_show[8]=(int)value;
			break;
		case 225:
			wOSD->OSD_text_positionX[8]=(int)value;
			break;
		case 226:
			wOSD->OSD_text_positionY[8]=(int)value;
			break;
		case 227:
			wOSD->OSD_text_content[8] = inBuf;
			break;
		case 229:
			wOSD->OSD_text_color[8]=(int)value;
			break;
		case 230:
			wOSD->OSD_text_alpha[8]=(int)value;
			break;

		case 240:
			wOSD->OSD_text_show[9]=(int)value;
			break;
		case 241:
			wOSD->OSD_text_positionX[9]=(int)value;
			break;
		case 242:
			wOSD->OSD_text_positionY[9]=(int)value;
			break;
		case 243:
			wOSD->OSD_text_content[9] = inBuf;
			break;
		case 245:
			wOSD->OSD_text_color[9]=(int)value;
			break;
		case 246:
			wOSD->OSD_text_alpha[9]=(int)value;
			break;

		case 256:
			wOSD->OSD_text_show[10]=(int)value;
			break;
		case 257:
			wOSD->OSD_text_positionX[10]=(int)value;
			break;
		case 258:
			wOSD->OSD_text_positionY[10]=(int)value;
			break;
		case 259:
			wOSD->OSD_text_content[10] = inBuf;
			break;
		case 261:
			wOSD->OSD_text_color[10]=(int)value;
			break;
		case 262:
			wOSD->OSD_text_alpha[10]=(int)value;
			break;

		case 272:
			wOSD->OSD_text_show[11]=(int)value;
			break;
		case 273:
			wOSD->OSD_text_positionX[11]=(int)value;
			break;
		case 274:
			wOSD->OSD_text_positionY[11]=(int)value;
			break;
		case 275:
			wOSD->OSD_text_content[11]= inBuf;
			break;
		case 277:
			wOSD->OSD_text_color[11]=(int)value;
			break;
		case 278:
			wOSD->OSD_text_alpha[11]=(int)value;
			break;

		case 288:
			wOSD->OSD_text_show[12]=(int)value;
			break;
		case 289:
			wOSD->OSD_text_positionX[12]=(int)value;
			break;
		case 290:
			wOSD->OSD_text_positionY[12]=(int)value;
			break;
		case 291:
			wOSD->OSD_text_content[12] = inBuf;
			break;
		case 293:
			wOSD->OSD_text_color[12]=(int)value;
			break;
		case 294:
			wOSD->OSD_text_alpha[12]=(int)value;
			break;

		case 304:
			wOSD->OSD_text_show[13]=(int)value;
			break;
		case 305:
			wOSD->OSD_text_positionX[13]=(int)value;
			break;
		case 306:
			wOSD->OSD_text_positionY[13]=(int)value;
			break;
		case 307:
			wOSD->OSD_text_content[13] = inBuf;
			break;
		case 309:
			wOSD->OSD_text_color[13]=(int)value;
			break;
		case 310:
			wOSD->OSD_text_alpha[13]=(int)value;
			break;



		case 320:
			wOSD->OSD_text_show[14]=(int)value;
			break;
		case 321:
			wOSD->OSD_text_positionX[14]=(int)value;
			break;
		case 322:
			wOSD->OSD_text_positionY[14]=(int)value;
			break;
		case 323:
			wOSD->OSD_text_content[14] = inBuf;
			break;
		case 325:
			wOSD->OSD_text_color[14]=(int)value;
			break;
		case 326:
			wOSD->OSD_text_alpha[14]=(int)value;
			break;


		case 336:
			wOSD->OSD_text_show[15]=(int)value;
			break;
		case 337:
			wOSD->OSD_text_positionX[15]=(int)value;
			break;
		case 338:
			wOSD->OSD_text_positionY[15]=(int)value;
			break;
		case 339:
			wOSD->OSD_text_content[15] = inBuf;
			break;
		case 341:
			wOSD->OSD_text_color[15]=(int)value;
			break;
		case 342:
			wOSD->OSD_text_alpha[15]=(int)value;
			break;
		case 357:
			//viewParam.FovMode[0]=(int)value;
			break;
		case 358:
			viewParam.radio[0]=(float)value;
			pObj->params.sensorParams[1].fFovRatio=(float)value;
			break;
		case 370:
			value = value * (PI/180) * 1000;
			pObj->privates.fovX=(float)value;
			viewParam.level_Fov_fix[0]= (float)value;
			break;
		case 371:
			value = value * (PI/180) * 1000;
			pObj->privates.fovY=(float)value;
			viewParam.vertical_Fov_fix[0]= (float)value;
			break;
		case 372:
			viewParam.Boresightpos_fix_x[0]=(int)value;
			pObj->params.sensorParams[1].defaultBoresightPos_x = (int)value;
			break;
		case 373:
			viewParam.Boresightpos_fix_y[0]=(int)value;
			pObj->params.sensorParams[1].defaultBoresightPos_y = (int)value;
			break;

			/*通道１可切换视场*/
		case 385:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_switch1[0]= value;
			break;
		case 386:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_switch1[0]=value;
			break;
		case 387:
			viewParam.Boresightpos_switch_x1[0]=(int)value;
			break;
		case 388:
			viewParam.Boresightpos_switch_y1[0]=(int)value;
			break;
		case 389:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_switch2[0]=(int)value;
			break;
		case 390:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_switch2[0]=value;
			break;
		case 391:
			viewParam.Boresightpos_switch_x2[0]=(int)value;
			break;
		case 392:
			viewParam.Boresightpos_switch_y2[0]=(int)value;
			break;
		case 393:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_switch3[0]=value;
			break;
		case 394:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_switch3[0]=value;
			break;
		case 395:
			viewParam.Boresightpos_switch_x3[0]=(int)value;
			break;
		case 396:
			viewParam.Boresightpos_switch_y3[0]=(int)value;
			break;
		case 397:
			value = value * (PI/180) * 1000;
		    viewParam.level_Fov_switch4[0]=value;
		    break;
		case 398:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_switch4[0]=value;
			break;
		case 399:
			viewParam.Boresightpos_switch_x4[0]=(int)value;
			break;
		case 400:
			viewParam.Boresightpos_switch_y4[0]=(int)value;
			break;
		case 401:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_switch5[0]=value;
			break;
		case 402:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_switch5[0]=value;
			break;
		case 403:
			viewParam.Boresightpos_switch_x5[0]=(int)value;
			break;
		case 404:
			viewParam.Boresightpos_switch_y5[0]=(int)value;
			break;
			/*通道１连续视场*/
		case 416:
			viewParam.zoombak1[0]=(int)value;
			break;
		case 417:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_continue1[0]=value;
			break;
		case 418:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_continue1[0]=value;
			break;
		case 419:
			viewParam. Boresightpos_continue_x1[0]=(int)value;
			break;
		case 420:
			viewParam.Boresightpos_continue_y1[0]=(int)value;
			break;
		case 421:
			viewParam.zoombak2[0]=(int)value;
			break;
		case 422:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_continue2[0]=value;
			break;
		case 423:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_continue2[0]=value;
			break;
		case 424:
			viewParam. Boresightpos_continue_x2[0]=(int)value;
			break;
		case 425:
			viewParam.Boresightpos_continue_y2[0]=(int)value;
			break;
		case 426:
			viewParam.zoombak3[0]=(int)value;
			break;
		case 427:
			value = value * (PI/180) * 1000;
			viewParam.level_Fov_continue3[0]=value;
			break;
		case 428:
			value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_continue3[0]=value;
			break;
		case 429:
			viewParam. Boresightpos_continue_x3[0]=(int)value;
			break;
		case 430:
			viewParam.Boresightpos_continue_y3[0]=(int)value;
			break;
		case 431:
		   viewParam.zoombak4[0]=(int)value;
		   break;
		case 880:
			value = value * (PI/180) * 1000;
		   viewParam.level_Fov_continue4[0]=value;
		   break;
		   case 881:
			   value = value * (PI/180) * 1000;
		   viewParam.vertical_Fov_continue4[0]=value;
		   break;
		   case 882:
		   viewParam. Boresightpos_continue_x4[0]=(int)value;
		   break;
		   case 883:
		   viewParam.Boresightpos_continue_y4[0]=(int)value;
		   break;
		   case 884:
		   viewParam.zoombak5[0]=(int)value;
		   break;
		   case 885:
			   value = value * (PI/180) * 1000;
	       viewParam.level_Fov_continue5[0]=value;
	       break;
		   case 886:
			   value = value * (PI/180) * 1000;
     		viewParam.vertical_Fov_continue5[0]=value;
     		break;
		   case 887:
		   	viewParam. Boresightpos_continue_x5[0]=(int)value;
		   	break;
		   case 888:
			viewParam.Boresightpos_continue_y5[0]=(int)value;
			break;
		   case 889:
			viewParam.zoombak6[0]=(int)value;
			break;
		   case 890:
			   value = value * (PI/180) * 1000;
			viewParam.level_Fov_continue6[0]=value;
			break;
		   case 891:
			   value = value * (PI/180) * 1000;
			viewParam.vertical_Fov_continue6[0]=value;
			break;
		   case 892:
			viewParam. Boresightpos_continue_x6[0]=(int)value;
			break;
		   case 893:
			viewParam.Boresightpos_continue_y6[0]=(int)value;
			break;
		   case 894:
			viewParam.zoombak7[0]=(int)value;
			break;
		   case 895:
			   value = value * (PI/180) * 1000;
			viewParam.level_Fov_continue7[0]=value;
			break;
		   case 896:
			viewParam.vertical_Fov_continue7[0]=value;
			break;
		   case 897:
			viewParam. Boresightpos_continue_x7[0]=(int)value;
			break;
		   case 898:
			viewParam.Boresightpos_continue_y7[0]=(int)value;
			break;

	   /*通道2固定视场*/
		   case 918:
			   viewParam.radio[1]=(float)value;
			   pObj->params.sensorParams[1].fFovRatio=(float)value;
			   break;
		   case 928:
			   value = value * (PI/180) * 1000;
			   pObj->privates.fovX=(float)value;
			   viewParam.level_Fov_fix[1]= (float)value;
			   break;
		   case 929:
			   value = value * (PI/180) * 1000;
			   pObj->privates.fovY=(float)value;
			   viewParam.vertical_Fov_fix[1]= (float)value;
			   break;
		   case 930:
			   viewParam.Boresightpos_fix_x[1] =(float)value;
			   pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
			   printf(" bPos x \n");
			   break;
		   case 931:
			   viewParam.Boresightpos_fix_y[1] =(int)value;
			   pObj->params.sensorParams[1].defaultBoresightPos_y = (int)value;
			   printf(" bPos y \n");
			   break;
/*通道2可切换视场*/

           case 945:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_switch1[1]= (float)value;
               break;
           case 946:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_switch1[1]= (float)value;
               break;
           case 947:
               viewParam.Boresightpos_switch_x1[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 948:
              viewParam.Boresightpos_switch_y1[1] =(float)value;
              pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;
           case 949:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_switch2[1]= (float)value;
               break;
           case 950:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_switch2[1]= (float)value;
               break;
           case 951:
               viewParam.Boresightpos_switch_x2[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 952:
               viewParam.Boresightpos_switch_y2[1]  =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;
           case 953:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_switch3[1]= (float)value;
               break;
           case 954:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_switch3[1]= (float)value;
               break;
           case 955:
               viewParam.Boresightpos_switch_x3[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 956:
               viewParam.Boresightpos_switch_y3[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;
           case 957:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_switch4[1]= (float)value;
               break;
           case 958:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_switch4[1]= (float)value;
               break;
           case 959:
               viewParam.Boresightpos_switch_x4[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 960:
               viewParam.Boresightpos_switch_y4[1]  =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;
           case 961:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_switch5[1]= (float)value;
               break;
           case 962:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_switch5[1]= (float)value;
               break;
           case 963:
               viewParam.Boresightpos_switch_x5[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 964:
               viewParam.Boresightpos_switch_y5[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 976:
               viewParam.zoombak1[1]=(float)value;
               break;
           case 977:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue1[1]= (float)value;
               break;
           case 978:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue1[1]= (float)value;
               break;
           case 979:
               viewParam. Boresightpos_continue_x1[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 980:
              viewParam.Boresightpos_continue_y1[1] =(float)value;
              pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 981:
               viewParam.zoombak2[1]=(float)value;
               break;
           case 982:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue2[1]= (float)value;
               break;
           case 983:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue2[1]= (float)value;
               break;
           case 984:
               viewParam. Boresightpos_continue_x2[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 985:
               viewParam.Boresightpos_continue_y2[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 986:
               viewParam.zoombak3[1]=(float)value;
               break;
           case 987:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue3[1]= (float)value;
               break;
           case 988:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue3[1]= (float)value;
               break;
           case 989:
               viewParam. Boresightpos_continue_x3[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 990:
               viewParam.Boresightpos_continue_y3[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 991:
               viewParam.zoombak4[1]=(float)value;
               break;
           case 992:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue4[1]= (float)value;
               break;
           case 993:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue4[1]= (float)value;
               break;
           case 994:
               viewParam. Boresightpos_continue_x4[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 995:
               viewParam.Boresightpos_continue_y4[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 996:
               viewParam.zoombak5[1]=(float)value;
               break;
           case 997:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue5[1]= (float)value;
               break;
           case 998:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue5[1]= (float)value;
               break;
           case 999:
               viewParam. Boresightpos_continue_x5[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 1000:
               viewParam.Boresightpos_continue_y5[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 1001:
               viewParam.zoombak6[1]=(float)value;
               break;
           case 1002:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue6[1]= (float)value;
               break;
           case 1003:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue6[1]= (float)value;
               break;
           case 1004:
               viewParam. Boresightpos_continue_x6[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 1005:
               viewParam.Boresightpos_continue_y6[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

           case 1006:
               viewParam.zoombak7[1]=(float)value;
               break;
           case 1007:
               value = value * (PI/180) * 1000;
               pObj->privates.fovX=(float)value;
               viewParam.level_Fov_continue7[1]= (float)value;
               break;
           case 1008:
               value = value * (PI/180) * 1000;
               pObj->privates.fovY=(float)value;
               viewParam.vertical_Fov_continue7[1]= (float)value;
               break;
           case 1009:
               viewParam. Boresightpos_continue_x7[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_x = (float)value;
               break;
           case 1010:
               viewParam.Boresightpos_continue_y7[1] =(float)value;
               pObj->params.sensorParams[1].defaultBoresightPos_y = (float)value;
               break;

		case 432:
			uart.uartDevice=(int)value;
			break;
		case 433:
			uart.baud=(int)value;
			break;
		case 434:
			uart.data=(int)value;
			break;
		case 435:
			uart.check=(int)value;
			break;
		case 436:
			uart.stop=(int)value;
			break;
		case 437:
			uart.flow=(int)value;
			break;


		case 448:
			wOSD->OSD_text_show[16]=(int)value;
			break;
		case 449:
			wOSD->OSD_text_positionX[16]=(int)value;
			break;
		case 450:
			wOSD->OSD_text_positionY[16]=(int)value;
			break;
		case 451:
			wOSD->OSD_text_content[16] = inBuf;
			break;
		case 453:
			wOSD->OSD_text_color[16]=(int)value;
			break;
		case 454:
			wOSD->OSD_text_alpha[16]=(int)value;
			break;



		case 464:
			wOSD->OSD_text_show[17]=(int)value;
			break;
		case 465:
			wOSD->OSD_text_positionX[17]=(int)value;
			break;
		case 466:
			wOSD->OSD_text_positionY[17]=(int)value;
			break;
		case 467:
			wOSD->OSD_text_content[17] = inBuf;
			break;
		case 469:
			wOSD->OSD_text_color[17]=(int)value;
			break;
		case 470:
			wOSD->OSD_text_alpha[17]=(int)value;
			break;

		case 480:
			wOSD->OSD_text_show[18]=(int)value;
			break;
		case 481:
			wOSD->OSD_text_positionX[18]=(int)value;
			break;
		case 482:
			wOSD->OSD_text_positionY[18]=(int)value;
			break;
		case 483:
			wOSD->OSD_text_content[18] = inBuf;
			break;
		case 485:
			wOSD->OSD_text_color[18]=(int)value;
			break;
		case 486:
			wOSD->OSD_text_alpha[18]=(int)value;
			break;


		case 496:
			wOSD->OSD_text_show[19]=(int)value;
			break;
		case 497:
			wOSD->OSD_text_positionX[19]=(int)value;
			break;
		case 498:
			wOSD->OSD_text_positionY[19]=(int)value;
			break;
		case 499:
			wOSD->OSD_text_content[19] = inBuf;
			break;
		case 501:
			wOSD->OSD_text_color[19]=(int)value;
			break;
		case 502:
			wOSD->OSD_text_alpha[19]=(int)value;
			break;


		case 512:
			wOSD->OSD_text_show[20]=(int)value;
			break;
		case 513:
			wOSD->OSD_text_positionX[20]=(int)value;
			break;
		case 514:
			wOSD->OSD_text_positionY[20]=(int)value;
			break;
		case 515:
			wOSD->OSD_text_content[20] = inBuf;
			break;
		case 517:
			wOSD->OSD_text_color[20]=(int)value;
			break;
		case 518:
			wOSD->OSD_text_alpha[20]=(int)value;
			break;



		case 528:
			wOSD->OSD_text_show[21]=(int)value;
			break;
		case 529:
			wOSD->OSD_text_positionX[21]=(int)value;
			break;
		case 530:
			wOSD->OSD_text_positionY[21]=(int)value;
			break;
		case 531:
			wOSD->OSD_text_content[21] = inBuf;
			break;
		case 533:
			wOSD->OSD_text_color[21]=(int)value;
			break;
		case 534:
			wOSD->OSD_text_alpha[21]=(int)value;
			break;


		case 544:
			wOSD->OSD_text_show[22]=(int)value;
			break;
		case 545:
			wOSD->OSD_text_positionX[22]=(int)value;
			break;
		case 546:
			wOSD->OSD_text_positionY[22]=(int)value;
			break;
		case 547:
			wOSD->OSD_text_content[22] = inBuf;
			break;
		case 549:
			wOSD->OSD_text_color[22]=(int)value;
			break;
		case 550:
			wOSD->OSD_text_alpha[22]=(int)value;
			break;


		case 560:
			wOSD->OSD_text_show[23]=(int)value;
			break;
		case 561:
			wOSD->OSD_text_positionX[23]=(int)value;
			break;
		case 562:
			wOSD->OSD_text_positionY[23]=(int)value;
			break;
		case 563:
			wOSD->OSD_text_content[23] = inBuf;
			break;
		case 565:
			wOSD->OSD_text_color[23]=(int)value;
			break;
		case 566:
			wOSD->OSD_text_alpha[23]=(int)value;
			break;



		case 576:
			wOSD->OSD_text_show[24]=(int)value;
			break;
		case 577:
			wOSD->OSD_text_positionX[24]=(int)value;
			break;
		case 578:
			wOSD->OSD_text_positionY[24]=(int)value;
			break;
		case 579:
			wOSD->OSD_text_content[24] = inBuf;
			break;
		case 581:
			wOSD->OSD_text_color[24]=(int)value;
			break;
		case 582:
			wOSD->OSD_text_alpha[24]=(int)value;
			break;


		case 592:
			wOSD->OSD_text_show[25]=(int)value;
			break;
		case 593:
			wOSD->OSD_text_positionX[25]=(int)value;
			break;
		case 594:
			wOSD->OSD_text_positionY[25]=(int)value;
			break;
		case 595:
			wOSD->OSD_text_content[25] = inBuf;
			break;
		case 597:
			wOSD->OSD_text_color[25]=(int)value;
			break;
		case 598:
			wOSD->OSD_text_alpha[25]=(int)value;
			break;

		case 608:
			wOSD->OSD_text_show[26]=(int)value;
			break;
		case 609:
			wOSD->OSD_text_positionX[26]=(int)value;
			break;
		case 610:
			wOSD->OSD_text_positionY[26]=(int)value;
			break;
		case 611:
			wOSD->OSD_text_content[26] = inBuf;
			break;
		case 613:
			wOSD->OSD_text_color[26]=(int)value;
			break;
		case 614:
			wOSD->OSD_text_alpha[26]=(int)value;
			break;

		case 624:
			wOSD->OSD_text_show[27]=(int)value;
			break;
		case 625:
			wOSD->OSD_text_positionX[27]=(int)value;
			break;
		case 626:
			wOSD->OSD_text_positionY[27]=(int)value;
			break;
		case 627:
			wOSD->OSD_text_content[27] = inBuf;
			break;
		case 629:
			wOSD->OSD_text_color[27]=(int)value;
			break;
		case 630:
			wOSD->OSD_text_alpha[27]=(int)value;
			break;

		case 640:
			wOSD->OSD_text_show[28]=(int)value;
			break;
		case 641:
			wOSD->OSD_text_positionX[28]=(int)value;
			break;
		case 642:
			wOSD->OSD_text_positionY[28]=(int)value;
			break;
		case 643:
			wOSD->OSD_text_content[28] = inBuf;
			break;
		case 645:
			wOSD->OSD_text_color[28]=(int)value;
			break;
		case 646:
			wOSD->OSD_text_alpha[28]=(int)value;
			break;


		case 656:
			wOSD->OSD_text_show[29]=(int)value;
			break;
		case 657:
			wOSD->OSD_text_positionX[29]=(int)value;
			break;
		case 658:
			wOSD->OSD_text_positionY[29]=(int)value;
			break;
		case 659:
			wOSD->OSD_text_content[29] = inBuf;
			break;
		case 661:
			wOSD->OSD_text_color[29]=(int)value;
			break;
		case 662:
			wOSD->OSD_text_alpha[29]=(int)value;
			break;

		case 672:
			wOSD->OSD_text_show[30]=(int)value;
			break;
		case 673:
			wOSD->OSD_text_positionX[30]=(int)value;
			break;
		case 674:
			wOSD->OSD_text_positionY[30]=(int)value;
			break;
		case 675:
			wOSD->OSD_text_content[30] = inBuf;
			break;
		case 677:
			wOSD->OSD_text_color[30]=(int)value;
			break;
		case 678:
			wOSD->OSD_text_alpha[30]=(int)value;
			break;


		case 688:
			wOSD->OSD_text_show[31]=(int)value;
			break;
		case 689:
			wOSD->OSD_text_positionX[31]=(int)value;
			break;
		case 690:
			wOSD->OSD_text_positionY[31]=(int)value;
			break;
		case 691:
			wOSD->OSD_text_content[31] = inBuf;
			break;
		case 693:
			wOSD->OSD_text_color[31]=(int)value;
			break;
		case 694:
			wOSD->OSD_text_alpha[31]=(int)value;
			break;





		case 704:
			m_ipc->ipc_OSD->ch0_acqRect_width =(int)value;
			break;
		case 705:
			m_ipc->ipc_OSD->ch1_acqRect_width = (int)value;
			break;
		case 706:
			m_ipc->ipc_OSD->ch2_acqRect_width = (int)value;
			break;
		case 707:
			m_ipc->ipc_OSD->ch3_acqRect_width = (int)value;
			break;
		case 708:
			m_ipc->ipc_OSD->ch4_acqRect_width = (int)value;
			break;
		case 709:
			m_ipc->ipc_OSD->ch5_acqRect_width = (int)value;
			break;
		case 710:
			m_ipc->ipc_OSD->ch0_acqRect_height = (int)value;
			break;
		case 711:
			m_ipc->ipc_OSD->ch1_acqRect_height =(int) value;
			break;
		case 712:
			m_ipc->ipc_OSD->ch2_acqRect_height = (int)value;
			break;
		case 713:
			m_ipc->ipc_OSD->ch3_acqRect_height = (int)value;
			break;
		case 714:
			m_ipc->ipc_OSD->ch4_acqRect_height = (int)value;
			break;
		case 715:
			m_ipc->ipc_OSD->ch5_acqRect_height  = (int)value;
			break;


		case 362:
			m_ipc->ipc_OSD->ch0_aim_width = (int)value;
			break;
		case 922:
			m_ipc->ipc_OSD->ch1_aim_width = (int)value;
			break;
		case 1034:
			m_ipc->ipc_OSD->ch2_aim_width =(int) value;
			break;
		case 1146:
			m_ipc->ipc_OSD->ch3_aim_width = (int)value;
			break;
		case 1258:
			m_ipc->ipc_OSD->ch4_aim_width = (int)value;
			break;
			/*
		case 725:
			m_ipc->ipc_OSD->ch5_aim_width = (int)value;
			break;
			*/
		case 363:
			m_ipc->ipc_OSD->ch0_aim_height = (int)value;
			break;
		case 923:
			m_ipc->ipc_OSD->ch1_aim_height = (int)value;
			break;
		case 1035:
			m_ipc->ipc_OSD->ch2_aim_height = (int)value;
			break;
		case 1147:
			m_ipc->ipc_OSD->ch3_aim_height = (int)value;
			break;
		case 1259:
			m_ipc->ipc_OSD->ch4_aim_height = (int)value;
			break;
			/*
		case 731:
			m_ipc->ipc_OSD->ch5_aim_height = (int)value;
			break;
			*/


		case 736:
			m_ipc->ipc_OSD->CROSS_draw_show[0] = (int)value;
			break;
		case 737:
			m_ipc->ipc_OSD->OSD_draw_color = (int)value;
			break;
		case 738:
			m_ipc->ipc_OSD->CROSS_AXIS_WIDTH = (int)value;
			break;
		case 739:
			m_ipc->ipc_OSD->CROSS_AXIS_HEIGHT = (int)value;
			break;
		case 740:
			m_ipc->ipc_OSD->Picp_CROSS_AXIS_WIDTH = (int)value;
			break;
		case 741:
			m_ipc->ipc_OSD->Picp_CROSS_AXIS_HEIGHT = (int)value;
			break;

		case 752:
			break;
		case 1456:
			pObj->params.Kx = (int)value;
			break;
		case 1488:
		case 1520:
		case 1552:

			break;

		case 753:
			break;
		case 1457:
			pObj->params.Ky = (int)value;
			break;
	    case 1489:
		case 1521:
		case 1553:

			break;

		case 754:
			break;
		case 1458:
			pObj->params.Error_X = (float)value;
			break;
		case 1490:
		case 1522:
		case 1554:

			break;

		case 755:
			break;
		case 1459:
			pObj->params.Error_Y = (float)value;
			break;
		case 1491:
		case 1523:
		case 1555:

			break;

		case 756:
			break;
		case 1460:
			pObj->params.Time = (float)value;
			break;
		case 1492:
		case 1524:
		case 1556:

			break;

		case 768:
			break;
		case 1568:
			m_ptzSpeed.SpeedLevelPan[0] = (int)value;
			break;
		case 1600:
		case 1632:
		case 1664:

			break;

		case 769:
			break;
		case 1569:
			m_ptzSpeed.SpeedLevelPan[1] = (int)value;
			break;
		case 1601:
		case 1633:
		case 1665:

			break;

		case 770:
			break;
		case 1570:
			m_ptzSpeed.SpeedLevelPan[2] = (int)value;
			break;
		case 1602:
		case 1634:
		case 1666:

			break;

		case 771:
			break;
		case 1571:
			m_ptzSpeed.SpeedLevelPan[3] = (int)value;
			break;
		case 1603:
		case 1635:
		case 1667:

			break;

		case 772:
			break;
		case 1572:
			m_ptzSpeed.SpeedLevelPan[4] = (int)value;
			break;
		case 1604:
		case 1636:
		case 1668:

			break;

		case 773:
			break;
		case 1573:
			m_ptzSpeed.SpeedLevelPan[5] = (int)value;
			break;
		case 1605:
		case 1637:
		case 1669:

			break;

		case 774:
			break;
		case 1574:
			m_ptzSpeed.SpeedLevelPan[6] = (int)value;
			break;
		case 1606:
		case 1638:
		case 1670:

			break;

		case 775:
			break;
		case 1575:
			break;
		case 1607:
		case 1639:
		case 1671:

			break;

		case 776:
			break;
		case 1576:
			m_ptzSpeed.SpeedLevelPan[8] = (int)value;
			break;
		case 1608:
		case 1640:
		case 1672:

			break;

		case 786:
			break;
		case 1586:
			m_ptzSpeed.SpeedLevelPan[9] = (int)value;
			break;
		case 1618:
		case 1650:
		case 1682:

			break;

		case 777:
			break;
		case 1577:
			m_ptzSpeed.SpeedLeveTilt[0] = (int)value;
			break;
		case 1609:
		case 1641:
		case 1673:

			break;

		case 778:
			break;
		case 1578:
			m_ptzSpeed.SpeedLeveTilt[1] = (int)value;
			break;
		case 1610:
		case 1642:
		case 1674:

			break;

		case 779:
			break;
		case 1579:
			m_ptzSpeed.SpeedLeveTilt[2] = (int)value;
			break;
		case 1611:
		case 1643:
		case 1675:

			break;

		case 780:
			break;
		case 1580:
			m_ptzSpeed.SpeedLeveTilt[3] = (int)value;
			break;
		case 1612:
		case 1644:
		case 1676:

			break;

		case 781:
			break;
		case 1581:
			m_ptzSpeed.SpeedLeveTilt[4] = (int)value;
			break;
		case 1613:
		case 1645:
		case 1677:

			break;

		case 782:
			break;
		case 1582:
			m_ptzSpeed.SpeedLeveTilt[5] = (int)value;
			break;
		case 1614:
		case 1646:
		case 1678:

			break;

		case 783:
			break;
		case 1583:
			m_ptzSpeed.SpeedLeveTilt[6] = (int)value;
			break;
		case 1615:
		case 1647:
		case 1679:

			break;

		case 784:
			break;
		case 1584:
			m_ptzSpeed.SpeedLeveTilt[7] = (int)value;
			break;
		case 1616:
		case 1648:
		case 1680:

			break;

		case 785:
			break;
		case 1585:
			m_ptzSpeed.SpeedLeveTilt[8] = (int)value;
			break;
		case 1617:
		case 1649:
		case 1681:

			break;

		case 787:
			break;
		case 1587:
			m_ptzSpeed.SpeedLeveTilt[9] = (int)value;
			break;
		case 1619:
		case 1651:
		case 1683:

			break;

		case 356:
			cfg_value[800] = (float)m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution];
			cfg_value[801] = (float)m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution1];
			cfg_value[802] = (float)m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution2];
			cfg_value[803] = (float)m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution3];
			cfg_value[804] = (float)m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution4];
			resolu.resolution[ipc_eSen_CH0] = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution];
			resolu.resolution[ipc_eSen_CH1] = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution1];
			resolu.resolution[ipc_eSen_CH2] = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution2];
			resolu.resolution[ipc_eSen_CH3] = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution3];
			resolu.resolution[ipc_eSen_CH4] = m_GlobalDate->EXT_Ctrl[Cmd_IPC_Resolution4];
			updataProfile();
		    system(str);
			break;

		case 855:
			m_GlobalDate->mtdconfig.trktime = (int)value*1000;
			break;

		case 856:
			m_GlobalDate->mtdconfig.warning = (int)value;
			break;

		case 857:
			m_GlobalDate->mtdconfig.high_low_level = (int)value;
			break;

		case 864:
			//detect area
			m_GlobalDate->Mtd_Moitor_X = (int)value;
			break;

		case 865:
			m_GlobalDate->Mtd_Moitor_Y = (int)value;
			break;

		default:
			break;
	}

	if(check >= 704 && check <= 741)
		m_ipc->IpcConfigOSD();
	else if(check >=48 && check <= 95)
		m_ipc->IpcConfigUTC();
	else if(check == 40)
		m_ipc->IpcSendLosttime();

	int x = pObj->params.sensorParams[1].defaultBoresightPos_x;
	int y = pObj->params.sensorParams[1].defaultBoresightPos_y;
	switch(check)
	{
	case 362:
	case 363:
	case 922:
	case 923:
	case 1034:
	case 1035:
	case 1146:
	case 1147:
	case 1258:
	case 1259:
		m_ipc->IpcConfigOSD();
		break;
	case 930:
	case 931:
	case 947:
	case 948:
	case 951:
	case 952:
	case 955:
	case 956:
	case 959:
	case 960:
	case 963:
	case 964:
	case 979:
	case 980:
	case 984:
	case 985:
	case 989:
	case 990:
	case 994:
	case 995:
	case 999:
	case 1000:
	case 1004:
	case 1005:
	case 1009:
	case 1010:
		m_ipc->IPCBoresightPosCtrl(x, y);
		break;
	}

	signalFeedBack( ACK_config_Write,  0,  0,  0);

}

int CManager::paramBackToDefault(int blockId)
{
 	bool allFlag = false;
	m_ipc->ipc_OSD = m_ipc->getOSDSharedMem();
	m_ipc->ipc_UTC = m_ipc->getUTCSharedMem();

	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	PlatformFilter_Obj* pPidObj;

	OSD_text* rOSD;
	rOSD = &m_OSD;

	if(blockId == 255)
		allFlag = true;

	string cfgAvtFile;
	int configId_Max =848;
	char  cfg_avt[20] = "cfg_avt_";
	cfgAvtFile = "Profile.yml";
	FILE *fp = fopen(cfgAvtFile.c_str(), "rt");

	if(fp != NULL){
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len < 10)
			return  -1;
		else{
			FileStorage fr(cfgAvtFile, FileStorage::READ);
			if(fr.isOpened()){
				for(int i=0; i<configId_Max; i++){
					sprintf(cfg_avt, "cfg_avt_%d", i);
					cfg_value[i] = (float)fr[cfg_avt];
					//printf(" read cfg [%d] %f \n", i, cfg_value[i]);
				}
				if(0x1 == blockId || allFlag)
				{
					pObj->params.joystickRateDemandParam.fDeadband	=	(float)fr["cfg_avt_1"];
					pObj->params.joystickRateDemandParam.fCutpoint1		=	(float)fr["cfg_avt_2"];
					pObj->params.joystickRateDemandParam.fInputGain_X1	=	(float)fr["cfg_avt_3"];
					pObj->params.joystickRateDemandParam.fInputGain_Y1	=	(float)fr["cfg_avt_4"];
					pObj->params.joystickRateDemandParam.fCutpoint2		=	(float)fr["cfg_avt_5"];
					pObj->params.joystickRateDemandParam.fInputGain_X2	=	(float)fr["cfg_avt_6"];
					pObj->params.joystickRateDemandParam.fInputGain_Y2	=	(float)fr["cfg_avt_7"];

				}
				if(0x2 == blockId || allFlag)
				{
					pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
					pPidObj->params.G = (float)fr["cfg_avt_17"];//Kp
					pPidObj->params.P0 = (float)fr["cfg_avt_18"];//Ki
					pPidObj->params.P1 = (float)fr["cfg_avt_19"];//Kd
					pPidObj->params.P2 = (float)fr["cfg_avt_20"];//K
					printf("backToDefparm G : %f \n",pPidObj->params.G);
					pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
					pPidObj->params.G = (float)fr["cfg_avt_21"];//Kp
					pPidObj->params.P0 = (float)fr["cfg_avt_22"];//Ki
					pPidObj->params.P1 = (float)fr["cfg_avt_23"];//Kd
					pPidObj->params.P2 = (float)fr["cfg_avt_24"];//K

				}
				if(0x3 == blockId || allFlag)
				{
					pObj->params.bleedX = (float)fr["cfg_avt_33"];
					pObj->params.bleedY = (float)fr["cfg_avt_34"];
					pObj->params.demandMaxX= (float)fr["cfg_avt_35"];
					pObj->params.demandMaxY=(float)fr["cfg_avt_36"];
					pObj->params.deadbandX = (float)fr["cfg_avt_37"];
					pObj->params.deadbandY = (float)fr["cfg_avt_38"];
					//m_ptzSpeed.curve = (float)fr["cfg_avt_39"]; //new
					//pObj->params.acqOutputType = (int)fr["cfg_avt_40"];//....
					//pObj->params.OutputSwitch = (float)fr["cfg_avt_46"];//new
					//pObj->params.OutputPort= (float)fr["cfg_avt_47"];//new

				}
				if(0x4 == blockId || allFlag)
				{

					m_ipc->ipc_UTC->occlusion_thred = (float)fr["cfg_avt_48"];//9--0
					m_ipc->ipc_UTC->retry_acq_thred = (float)fr["cfg_avt_49"];
					m_ipc->ipc_UTC->up_factor = (float)fr["cfg_avt_50"];
					m_ipc->ipc_UTC->res_distance = (int)fr["cfg_avt_51"];
					m_ipc->ipc_UTC->res_area = (int)fr["cfg_avt_52"];
					m_ipc->ipc_UTC->gapframe = (int)fr["cfg_avt_53"];
					m_ipc->ipc_UTC->enhEnable = (int)fr["cfg_avt_54"];
					m_ipc->ipc_UTC->cliplimit = (float)fr["cfg_avt_55"];
					m_ipc->ipc_UTC->dictEnable = (int)fr["cfg_avt_56"];
					m_ipc->ipc_UTC->moveX = (int)fr["cfg_avt_57"];
					m_ipc->ipc_UTC->moveY = (int)fr["cfg_avt_58"];
					m_ipc->ipc_UTC->moveX2 = (int)fr["cfg_avt_59"];
					m_ipc->ipc_UTC->moveY2 = (int)fr["cfg_avt_60"];
					m_ipc->ipc_UTC->segPixelX = (int)fr["cfg_avt_61"];
					m_ipc->ipc_UTC->segPixelY = (int)fr["cfg_avt_62"];
					m_ipc->ipc_UTC->algOsdRect_Enable = (int)fr["cfg_avt_63"];  //9--15
				}
				if(0x5 == blockId || allFlag)
				{
					m_ipc->ipc_UTC->ScalerLarge = (int)fr["cfg_avt_64"];//10--0
					m_ipc->ipc_UTC->ScalerMid = (int)fr["cfg_avt_65"];
					m_ipc->ipc_UTC->ScalerSmall = (int)fr["cfg_avt_66"];
					m_ipc->ipc_UTC->Scatter = (int)fr["cfg_avt_67"];
					m_ipc->ipc_UTC->ratio = (float)fr["cfg_avt_68"];
					m_ipc->ipc_UTC->FilterEnable = (int)fr["cfg_avt_69"];
					m_ipc->ipc_UTC->BigSecEnable = (int)fr["cfg_avt_70"];
					m_ipc->ipc_UTC->SalientThred = (int)fr["cfg_avt_71"];
					m_ipc->ipc_UTC->ScalerEnable = (int)fr["cfg_avt_72"];
					m_ipc->ipc_UTC->DynamicRatioEnable = (int)fr["cfg_avt_73"];
					m_ipc->ipc_UTC->acqSize_width = (int)fr["cfg_avt_74"];
					m_ipc->ipc_UTC->acqSize_height = (int)fr["cfg_avt_75"];
					m_ipc->ipc_UTC->TrkAim43_Enable = (int)fr["cfg_avt_76"];
					m_ipc->ipc_UTC->SceneMVEnable = (int)fr["cfg_avt_77"];
					m_ipc->ipc_UTC->BackTrackEnable = (int)fr["cfg_avt_78"];
					m_ipc->ipc_UTC->bAveTrkPos = (int)fr["cfg_avt_79"]; //10--15

				}
				if(0x6 == blockId || allFlag)
				{
					m_ipc->ipc_UTC->fTau = (float)fr["cfg_avt_80"]; //11--0
					m_ipc->ipc_UTC->buildFrms = (int)fr["cfg_avt_81"];
					m_ipc->ipc_UTC->LostFrmThred = (int)fr["cfg_avt_82"];
					m_ipc->ipc_UTC->histMvThred = (float)fr["cfg_avt_83"];
					m_ipc->ipc_UTC->detectFrms = (int)fr["cfg_avt_84"];
					m_ipc->ipc_UTC->stillFrms = (int)fr["cfg_avt_85"];
					m_ipc->ipc_UTC->stillThred = (float)fr["cfg_avt_86"];
					m_ipc->ipc_UTC->bKalmanFilter = (int)fr["cfg_avt_87"];
					m_ipc->ipc_UTC->xMVThred = (float)fr["cfg_avt_88"];
					m_ipc->ipc_UTC->yMVThred = (float)fr["cfg_avt_89"];
					m_ipc->ipc_UTC->xStillThred = (float)fr["cfg_avt_90"];
					m_ipc->ipc_UTC->yStillThred = (float)fr["cfg_avt_91"];
					m_ipc->ipc_UTC->slopeThred = (float)fr["cfg_avt_92"];
					m_ipc->ipc_UTC->kalmanHistThred = (float)fr["cfg_avt_93"];
					m_ipc->ipc_UTC->kalmanCoefQ = (float)fr["cfg_avt_94"];
					m_ipc->ipc_UTC->kalmanCoefR = (float)fr["cfg_avt_95"];

				}
				if(0x7 == blockId || allFlag)
				{
					rOSD->OSD_text_show[0] = (int)fr["cfg_avt_96"];
					rOSD->OSD_text_positionX[0]= (int)fr["cfg_avt_97"];//new
					rOSD->OSD_text_positionY[0]= (int)fr["cfg_avt_98"];//new
					rOSD->OSD_text_content[0]= (string)fr["cfg_avt_99"];//new
					rOSD->OSD_text_color[0]= (int)fr["cfg_avt_101"];
					rOSD->OSD_text_alpha[0] = (int)fr["cfg_avt_102"]; //m_ipc->ipc_OSD->

				}
				if(0x8 == blockId || allFlag)
				{
					rOSD->OSD_text_show[1] = (int)fr["cfg_avt_112"];
					rOSD->OSD_text_positionX[1]= (int)fr["cfg_avt_113"];//new
					rOSD->OSD_text_positionY[1]= (int)fr["cfg_avt_114"];//new
					rOSD->OSD_text_content[1]= (string)fr["cfg_avt_115"];//new
					rOSD->OSD_text_color[1]= (int)fr["cfg_avt_117"];
					rOSD->OSD_text_alpha[1] = (int)fr["cfg_avt_118"];
				}
				if(0x9 == blockId || allFlag)
				{
					rOSD->OSD_text_show[2] = (int)fr["cfg_avt_128"];
					rOSD->OSD_text_positionX[2]= (int)fr["cfg_avt_129"];//new
					rOSD->OSD_text_positionY[2]= (int)fr["cfg_avt_130"];//new
					rOSD->OSD_text_content[2]=(string) fr["cfg_avt_131"];//new
					rOSD->OSD_text_color[2]= (int)fr["cfg_avt_133"];
					rOSD->OSD_text_alpha[2] = (int)fr["cfg_avt_134"];
				}
				if(0xa == blockId || allFlag)
				{
					rOSD->OSD_text_show[3] = (int)fr["cfg_avt_144"];
					rOSD->OSD_text_positionX[3]= (int)fr["cfg_avt_145"];//new
					rOSD->OSD_text_positionY[3]= (int)fr["cfg_avt_146"];//new
					rOSD->OSD_text_content[3]=(string) fr["cfg_avt_147"];//new
					rOSD->OSD_text_color[3]= (int)fr["cfg_avt_149"];
					rOSD->OSD_text_alpha[3] = (int)fr["cfg_avt_150"];
				}
				if(0xb == blockId || allFlag)
				{
					rOSD->OSD_text_show[4] = (int)fr["cfg_avt_160"];
					rOSD->OSD_text_positionX[4]= (int)fr["cfg_avt_161"];//new
					rOSD->OSD_text_positionY[4]= (int)fr["cfg_avt_162"];//new
					rOSD->OSD_text_content[4]= (string)fr["cfg_avt_163"];//new
					rOSD->OSD_text_color[4]= (int)fr["cfg_avt_165"];
					rOSD->OSD_text_alpha[4] = (int)fr["cfg_avt_166"];
				}
				if(0xc == blockId || allFlag)
				{
					rOSD->OSD_text_show[5] = (int)fr["cfg_avt_176"];
					rOSD->OSD_text_positionX[5]= (int)fr["cfg_avt_177"];//new
					rOSD->OSD_text_positionY[5]= (int)fr["cfg_avt_178"];//new
					rOSD->OSD_text_content[5]=(string) fr["cfg_avt_179"];//new
					rOSD->OSD_text_color[5]= (int)fr["cfg_avt_181"];
					rOSD->OSD_text_alpha[5] = (int)fr["cfg_avt_182"];
				}
				if(0xd == blockId || allFlag)
				{
					rOSD->OSD_text_show[6] = (int)fr["cfg_avt_192"];
					rOSD->OSD_text_positionX[6]= (int)fr["cfg_avt_193"];//new
					rOSD->OSD_text_positionY[6]= (int)fr["cfg_avt_194"];//new
					rOSD->OSD_text_content[6]=(string) fr["cfg_avt_195"];//new
					rOSD->OSD_text_color[6]= (int)fr["cfg_avt_197"];
					rOSD->OSD_text_alpha[6] = (int)fr["cfg_avt_198"];
				}
				if(0xe == blockId || allFlag)
				{
					rOSD->OSD_text_show[7] = (int)fr["cfg_avt_208"];
					rOSD->OSD_text_positionX[7]= (int)fr["cfg_avt_209"];//new
					rOSD->OSD_text_positionY[7]= (int)fr["cfg_avt_210"];//new
					rOSD->OSD_text_content[7]= (string)fr["cfg_avt_211"];//new
					rOSD->OSD_text_color[7]= (int)fr["cfg_avt_213"];
					rOSD->OSD_text_alpha[7] = (int)fr["cfg_avt_214"];
				}
				if(0xf == blockId || allFlag)
				{
					rOSD->OSD_text_show[8] = (int)fr["cfg_avt_224"];
					rOSD->OSD_text_positionX[8]= (int)fr["cfg_avt_225"];//new
					rOSD->OSD_text_positionY[8]= (int)fr["cfg_avt_226"];//new
					rOSD->OSD_text_content[8]=(string) fr["cfg_avt_227"];//new
					rOSD->OSD_text_color[8]= (int)fr["cfg_avt_229"];
					rOSD->OSD_text_alpha[8] = (int)fr["cfg_avt_230"];
				}
				if(0x10 == blockId || allFlag)
				{
					rOSD->OSD_text_show[9] = (int)fr["cfg_avt_240"];
					rOSD->OSD_text_positionX[9]= (int)fr["cfg_avt_241"];//new
					rOSD->OSD_text_positionY[9]= (int)fr["cfg_avt_242"];//new
					rOSD->OSD_text_content[9]=(string) fr["cfg_avt_243"];//new
					rOSD->OSD_text_color[9]= (int)fr["cfg_avt_245"];
					rOSD->OSD_text_alpha[9] = (int)fr["cfg_avt_246"];
				}
				if(0x11 == blockId || allFlag)
				{
					rOSD->OSD_text_show[10] = (int)fr["cfg_avt_256"];
					rOSD->OSD_text_positionX[10]= (int)fr["cfg_avt_257"];//new
					rOSD->OSD_text_positionY[10]= (int)fr["cfg_avt_258"];//new
					rOSD->OSD_text_content[10]=(string) fr["cfg_avt_259"];//new
					rOSD->OSD_text_color[10]= (int)fr["cfg_avt_261"];
					rOSD->OSD_text_alpha[10] = (int)fr["cfg_avt_262"];
				}
				if(0x12 == blockId || allFlag)
				{
					rOSD->OSD_text_show[11] = (int)fr["cfg_avt_272"];
					rOSD->OSD_text_positionX[11]= (int)fr["cfg_avt_273"];//new
					rOSD->OSD_text_positionY[11]= (int)fr["cfg_avt_274"];//new
					rOSD->OSD_text_content[11]= (string)fr["cfg_avt_275"];//new
					rOSD->OSD_text_color[11]= (int)fr["cfg_avt_277"];
					rOSD->OSD_text_alpha[11] = (int)fr["cfg_avt_278"];
				}
				if(0x13 == blockId || allFlag)
				{
					rOSD->OSD_text_show[12] = (int)fr["cfg_avt_288"];
					rOSD->OSD_text_positionX[12]= (int)fr["cfg_avt_289"];//new
					rOSD->OSD_text_positionY[12]= (int)fr["cfg_avt_290"];//new
					rOSD->OSD_text_content[12]=(string) fr["cfg_avt_291"];//new
					rOSD->OSD_text_color[12]= (int)fr["cfg_avt_293"];
					rOSD->OSD_text_alpha[12] = (int)fr["cfg_avt_294"];
				}
				if(0x14 == blockId || allFlag)
				{
					rOSD->OSD_text_show[13] = (int)fr["cfg_avt_304"];
					rOSD->OSD_text_positionX[13]= (int)fr["cfg_avt_305"];//new
					rOSD->OSD_text_positionY[13]= (int)fr["cfg_avt_306"];//new
					rOSD->OSD_text_content[13]= (string)fr["cfg_avt_307"];//new
					rOSD->OSD_text_color[13]= (int)fr["cfg_avt_309"];
					rOSD->OSD_text_alpha[13] = (int)fr["cfg_avt_310"];
				}
				if(0x15 == blockId || allFlag)
				{
					rOSD->OSD_text_show[14] = (int)fr["cfg_avt_320"];
					rOSD->OSD_text_positionX[14]= (int)fr["cfg_avt_321"];//new
					rOSD->OSD_text_positionY[14]= (int)fr["cfg_avt_322"];//new
					rOSD->OSD_text_content[14]= (string)fr["cfg_avt_323"];//new
					rOSD->OSD_text_color[14]= (int)fr["cfg_avt_325"];
					rOSD->OSD_text_alpha[14] = (int)fr["cfg_avt_326"];
				}
				if(0x16 == blockId || allFlag)
				{
					rOSD->OSD_text_show[15] = (int)fr["cfg_avt_336"];
					rOSD->OSD_text_positionX[15]= (int)fr["cfg_avt_337"];//new
					rOSD->OSD_text_positionY[15]= (int)fr["cfg_avt_338"];//new
					rOSD->OSD_text_content[15]= (string)fr["cfg_avt_339"];//new
					rOSD->OSD_text_color[15]= (int)fr["cfg_avt_341"];
					rOSD->OSD_text_alpha[15] = (int)fr["cfg_avt_342"];
				}

				if(0x17== blockId || allFlag)
				{
					//viewParam.FovMode[0]=(int)fr["cfg_avt_352"];
					pObj->params.sensorParams[1].fFovRatio = (float)fr["cfg_avt_353"];
					pObj->params.sensorParams[1].nWidth=(int)fr["cfg_avt_354"];
					pObj->params.sensorParams[1].nHeight=(int)fr["cfg_avt_355"];
					float fov_x =(float)fr["cfg_avt_356"];
					fov_x = fov_x * (PI / 180) * 1000;
					pObj->privates.fovX = fov_x;
					pObj->params.sensorParams[1].defaultBoresightPos_x=(int)fr["cfg_avt_357"];
					pObj->params.sensorParams[1].defaultBoresightPos_y=(int)fr["cfg_avt_358"];
				}
#if 0
				if(0x18== blockId || allFlag)
				{
					viewParam.view_field_switch1=(int)fr["cfg_avt_368"];
					viewParam.pos_switch_x1=(int)fr["cfg_avt_369"];
					viewParam.pos_switch_y1=(int)fr["cfg_avt_370"];
					viewParam.view_field_switch2=(int)fr["cfg_avt_371"];
					viewParam.pos_switch_x2=(int)fr["cfg_avt_372"];
					viewParam.pos_switch_y2=(int)fr["cfg_avt_373"];
					viewParam.view_field_switch3=(int)fr["cfg_avt_374"];
					viewParam.pos_switch_x3=(int)fr["cfg_avt_375"];
					viewParam.pos_switch_y3=(int)fr["cfg_avt_376"];
					viewParam.view_field_switch4=(int)fr["cfg_avt_377"];
					viewParam.pos_switch_x4=(int)fr["cfg_avt_378"];
					viewParam.pos_switch_y4=(int)fr["cfg_avt_379"];
					viewParam.view_field_switch5=(int)fr["cfg_avt_380"];
					viewParam.pos_switch_x5=(int)fr["cfg_avt_381"];
					viewParam.pos_switch_y5=(int)fr["cfg_avt_382"];
				}
				if(0x19== blockId || allFlag)
				{
					viewParam.view_field_continue1=(int)fr["cfg_avt_382"];
					viewParam.pos_continue_x1=(int)fr["cfg_avt_383"];
					viewParam.pos_continue_y1=(int)fr["cfg_avt_384"];
					viewParam.view_field_continue2=(int)fr["cfg_avt_385"];
					viewParam.pos_continue_x2=(int)fr["cfg_avt_386"];
					viewParam.pos_continue_y2=(int)fr["cfg_avt_387"];
					viewParam.view_field_continue3=(int)fr["cfg_avt_388"];
					viewParam.pos_continue_x3=(int)fr["cfg_avt_389"];
					viewParam.pos_continue_y3=(int)fr["cfg_avt_390"];
					viewParam.view_field_continue4=(int)fr["cfg_avt_391"];
					viewParam.pos_continue_x4=(int)fr["cfg_avt_392"];
					viewParam.pos_continue_y4=(int)fr["cfg_avt_393"];
					viewParam.view_field_continue5=(int)fr["cfg_avt_394"];
					viewParam.pos_continue_x5=(int)fr["cfg_avt_395"];
					viewParam.pos_continue_y5=(int)fr["cfg_avt_396"];
					viewParam.view_field_continue6=(int)fr["cfg_avt_397"];
				}
				if(0x1a== blockId || allFlag)
				{
					viewParam.pos_continue_x6=(int)fr["cfg_avt_398"];
					viewParam.pos_continue_y6=(int)fr["cfg_avt_399"];
					viewParam.view_field_continue7=(int)fr["cfg_avt_400"];
					viewParam.pos_continue_x7=(int)fr["cfg_avt_401"];
					viewParam.pos_continue_y7=(int)fr["cfg_avt_402"];
					viewParam.view_field_continue8=(int)fr["cfg_avt_403"];
					viewParam.pos_continue_x8=(int)fr["cfg_avt_404"];
					viewParam.pos_continue_y8=(int)fr["cfg_avt_405"];
					viewParam.view_field_continue9=(int)fr["cfg_avt_406"];
					viewParam.pos_continue_x9=(int)fr["cfg_avt_407"];
					viewParam.pos_continue_y9=(int)fr["cfg_avt_408"];
					viewParam.view_field_continue10=(int)fr["cfg_avt_409"];
					viewParam.pos_continue_x10=(int)fr["cfg_avt_410"];
					viewParam.pos_continue_y10=(int)fr["cfg_avt_411"];
					viewParam.view_field_continue11=(int)fr["cfg_avt_412"];
					viewParam.pos_continue_x11=(int)fr["cfg_avt_413"];
				}
				if(0x1b== blockId || allFlag)
				{
					viewParam.pos_continue_y11=(int)fr["cfg_avt_414"];
					viewParam.view_field_continue12=(int)fr["cfg_avt_415"];
					viewParam.pos_continue_x12=(int)fr["cfg_avt_416"];
					viewParam.pos_continue_y12=(int)fr["cfg_avt_417"];
					viewParam.view_field_continue13=(int)fr["cfg_avt_418"];
					viewParam.pos_continue_x13=(int)fr["cfg_avt_419"];
					viewParam.pos_continue_y13=(int)fr["cfg_avt_420"];
				}
#endif
				if(0x1c== blockId || allFlag){
					//uart.uartDevice=(string)fr["cfg_avt_432"];
					uart.baud=(int)fr["cfg_avt_433"];
					uart.data=(int)fr["cfg_avt_434"];
					uart.check=(int)fr["cfg_avt_435"];
					uart.stop=(int)fr["cfg_avt_436"];
					uart.flow=(int)fr["cfg_avt_437"];

				}



				if(0x1d == blockId || allFlag)
				{
					rOSD->OSD_text_show[16] = (int)fr["cfg_avt_448"];
					rOSD->OSD_text_positionX[16]= (int)fr["cfg_avt_449"];//new
					rOSD->OSD_text_positionY[16]= (int)fr["cfg_avt_450"];//new
					rOSD->OSD_text_content[16]=(string) fr["cfg_avt_451"];//new
					rOSD->OSD_text_color[16]= (int)fr["cfg_avt_453"];
					rOSD->OSD_text_alpha[16] = (int)fr["cfg_avt_454"];
				}
				if(0x1e == blockId || allFlag)
				{
					rOSD->OSD_text_show[17] = (int)fr["cfg_avt_464"];
					rOSD->OSD_text_positionX[17]= (int)fr["cfg_avt_465"];//new
					rOSD->OSD_text_positionY[17]= (int)fr["cfg_avt_466"];//new
					rOSD->OSD_text_content[17]=(string) fr["cfg_avt_467"];//new
					rOSD->OSD_text_color[17]= (int)fr["cfg_avt_469"];
					rOSD->OSD_text_alpha[17] = (int)fr["cfg_avt_470"];
				}
				if(0x1f == blockId || allFlag)
				{
					rOSD->OSD_text_show[18] = (int)fr["cfg_avt_480"];
					rOSD->OSD_text_positionX[18]= (int)fr["cfg_avt_481"];//new
					rOSD->OSD_text_positionY[18]= (int)fr["cfg_avt_482"];//new
					rOSD->OSD_text_content[18]=(string) fr["cfg_avt_483"];//new
					rOSD->OSD_text_color[18]= (int)fr["cfg_avt_485"];
					rOSD->OSD_text_alpha[18] = (int)fr["cfg_avt_486"];
				}
				if(0x20 == blockId || allFlag)
				{
					rOSD->OSD_text_show[19] = (int)fr["cfg_avt_496"];
					rOSD->OSD_text_positionX[19]= (int)fr["cfg_avt_497"];//new
					rOSD->OSD_text_positionY[19]= (int)fr["cfg_avt_498"];//new
					rOSD->OSD_text_content[19]=(string) fr["cfg_avt_499"];//new
					rOSD->OSD_text_color[19]= (int)fr["cfg_avt_501"];
					rOSD->OSD_text_alpha[19] = (int)fr["cfg_avt_502"];
				}
				if(0x21 == blockId || allFlag)
				{
					rOSD->OSD_text_show[20] = (int)fr["cfg_avt_512"];
					rOSD->OSD_text_positionX[20]= (int)fr["cfg_avt_513"];//new
					rOSD->OSD_text_positionY[20]= (int)fr["cfg_avt_514"];//new
					rOSD->OSD_text_content[20]= (string)fr["cfg_avt_515"];//new
					rOSD->OSD_text_color[20]= (int)fr["cfg_avt_517"];
					rOSD->OSD_text_alpha[20] = (int)fr["cfg_avt_518"];
				}
				if(0x22 == blockId || allFlag)
				{
					rOSD->OSD_text_show[21] = (int)fr["cfg_avt_528"];
					rOSD->OSD_text_positionX[21]= (int)fr["cfg_avt_529"];//new
					rOSD->OSD_text_positionY[21]= (int)fr["cfg_avt_530"];//new
					rOSD->OSD_text_content[21]=(string) fr["cfg_avt_531"];//new
					rOSD->OSD_text_color[21]= (int)fr["cfg_avt_533"];
					rOSD->OSD_text_alpha[21] = (int)fr["cfg_avt_534"];
				}
				if(0x23 == blockId || allFlag)
				{
					rOSD->OSD_text_show[22] = (int)fr["cfg_avt_544"];
					rOSD->OSD_text_positionX[22]= (int)fr["cfg_avt_545"];//new
					rOSD->OSD_text_positionY[22]= (int)fr["cfg_avt_546"];//new
					rOSD->OSD_text_content[22]= (string)fr["cfg_avt_547"];//new
					rOSD->OSD_text_color[22]= (int)fr["cfg_avt_549"];
					rOSD->OSD_text_alpha[22] = (int)fr["cfg_avt_550"];
				}
				if(0x24 == blockId || allFlag)
				{
					rOSD->OSD_text_show[23] = (int)fr["cfg_avt_560"];
					rOSD->OSD_text_positionX[23]= (int)fr["cfg_avt_561"];//new
					rOSD->OSD_text_positionY[23]= (int)fr["cfg_avt_562"];//new
					rOSD->OSD_text_content[23]= (string)fr["cfg_avt_563"];//new
					rOSD->OSD_text_color[23]= (int)fr["cfg_avt_565"];
					rOSD->OSD_text_alpha[23] = (int)fr["cfg_avt_566"];
				}
				if(0x25 == blockId || allFlag)
				{
					rOSD->OSD_text_show[24] = (int)fr["cfg_avt_576"];
					rOSD->OSD_text_positionX[24]= (int)fr["cfg_avt_577"];//new
					rOSD->OSD_text_positionY[24]= (int)fr["cfg_avt_578"];//new
					rOSD->OSD_text_content[24]=(string) fr["cfg_avt_579"];//new
					rOSD->OSD_text_color[24]= (int)fr["cfg_avt_581"];
					rOSD->OSD_text_alpha[24] = (int)fr["cfg_avt_582"];
				}
				if(0x26 == blockId || allFlag)
				{
					rOSD->OSD_text_show[25] = (int)fr["cfg_avt_592"];
					rOSD->OSD_text_positionX[25]= (int)fr["cfg_avt_593"];//new
					rOSD->OSD_text_positionY[25]= (int)fr["cfg_avt_594"];//new
					rOSD->OSD_text_content[25]=(string) fr["cfg_avt_595"];//new
					rOSD->OSD_text_color[25]= (int)fr["cfg_avt_597"];
					rOSD->OSD_text_alpha[25] = (int)fr["cfg_avt_598"];
				}
				if(0x27 == blockId || allFlag)
				{
					rOSD->OSD_text_show[26] = (int)fr["cfg_avt_608"];
					rOSD->OSD_text_positionX[26]= (int)fr["cfg_avt_609"];//new
					rOSD->OSD_text_positionY[26]= (int)fr["cfg_avt_610"];//new
					rOSD->OSD_text_content[26]=(string) fr["cfg_avt_611"];//new
					rOSD->OSD_text_color[26]= (int)fr["cfg_avt_613"];
					rOSD->OSD_text_alpha[26] = (int)fr["cfg_avt_614"];
				}
				if(0x28 == blockId || allFlag)
				{
					rOSD->OSD_text_show[27] = (int)fr["cfg_avt_624"];
					rOSD->OSD_text_positionX[27]= (int)fr["cfg_avt_625"];//new
					rOSD->OSD_text_positionY[27]= (int)fr["cfg_avt_626"];//new
					rOSD->OSD_text_content[27]=(string) fr["cfg_avt_627"];//new
					rOSD->OSD_text_color[27]= (int)fr["cfg_avt_629"];
					rOSD->OSD_text_alpha[27] = (int)fr["cfg_avt_630"];
				}
				if(0x29 == blockId || allFlag)
				{
					rOSD->OSD_text_show[28] = (int)fr["cfg_avt_640"];
					rOSD->OSD_text_positionX[28]= (int)fr["cfg_avt_641"];//new
					rOSD->OSD_text_positionY[28]= (int)fr["cfg_avt_642"];//new
					rOSD->OSD_text_content[28]= (string)fr["cfg_avt_643"];//new
					rOSD->OSD_text_color[28]= (int)fr["cfg_avt_645"];
					rOSD->OSD_text_alpha[28] = (int)fr["cfg_avt_646"];
				}
				if(0x2a == blockId || allFlag)
				{
					rOSD->OSD_text_show[29] = (int)fr["cfg_avt_656"];
					rOSD->OSD_text_positionX[29]= (int)fr["cfg_avt_657"];//new
					rOSD->OSD_text_positionY[29]= (int)fr["cfg_avt_658"];//new
					rOSD->OSD_text_content[29]= (string)fr["cfg_avt_659"];//new
					rOSD->OSD_text_color[29]= (int)fr["cfg_avt_661"];
					rOSD->OSD_text_alpha[29] = (int)fr["cfg_avt_662"];
				}
				if(0x2b == blockId || allFlag)
				{
					rOSD->OSD_text_show[30] = (int)fr["cfg_avt_672"];
					rOSD->OSD_text_positionX[30]= (int)fr["cfg_avt_673"];//new
					rOSD->OSD_text_positionY[30]= (int)fr["cfg_avt_674"];//new
					rOSD->OSD_text_content[30]=(string) fr["cfg_avt_675"];//new
					rOSD->OSD_text_color[30]= (int)fr["cfg_avt_677"];
					rOSD->OSD_text_alpha[30] = (int)fr["cfg_avt_678"];
				}
				if(0x2c == blockId || allFlag)
				{
					rOSD->OSD_text_show[31] = (int)fr["cfg_avt_688"];
					rOSD->OSD_text_positionX[31]= (int)fr["cfg_avt_689"];//new
					rOSD->OSD_text_positionY[31]= (int)fr["cfg_avt_690"];//new
					rOSD->OSD_text_content[31]=(string) fr["cfg_avt_691"];//new
					rOSD->OSD_text_color[31]= (int)fr["cfg_avt_693"];
					rOSD->OSD_text_alpha[31] = (int)fr["cfg_avt_694"];
				}
				if(0x2d == blockId || allFlag)
				{
					m_ipc->ipc_OSD->ch0_acqRect_width =(int)fr["cfg_avt_704"];
					m_ipc->ipc_OSD->ch1_acqRect_width = (int)fr["cfg_avt_705"];
					m_ipc->ipc_OSD->ch2_acqRect_width = (int)fr["cfg_avt_706"];
					m_ipc->ipc_OSD->ch3_acqRect_width = (int)fr["cfg_avt_707"];
					m_ipc->ipc_OSD->ch4_acqRect_width = (int)fr["cfg_avt_708"];
					m_ipc->ipc_OSD->ch5_acqRect_width = (int)fr["cfg_avt_709"];
					m_ipc->ipc_OSD->ch0_acqRect_height = (int)fr["cfg_avt_710"];
					m_ipc->ipc_OSD->ch1_acqRect_height =(int)fr["cfg_avt_711"];
					m_ipc->ipc_OSD->ch2_acqRect_height = (int)fr["cfg_avt_712"];
					m_ipc->ipc_OSD->ch3_acqRect_height =(int)fr["cfg_avt_713"];
					m_ipc->ipc_OSD->ch4_acqRect_height = (int)fr["cfg_avt_714"];
					m_ipc->ipc_OSD->ch5_acqRect_height	=(int)fr["cfg_avt_715"];
				}
				if(0x2e == blockId || allFlag)
				{
					m_ipc->ipc_OSD->ch0_aim_width = (int)fr["cfg_avt_362"];
					m_ipc->ipc_OSD->ch1_aim_width = (int)fr["cfg_avt_922"];
					m_ipc->ipc_OSD->ch2_aim_width = (int)fr["cfg_avt_1034"];
					m_ipc->ipc_OSD->ch3_aim_width = (int)fr["cfg_avt_1146"];
					m_ipc->ipc_OSD->ch4_aim_width = (int)fr["cfg_avt_1258"];
					//m_ipc->ipc_OSD->ch5_aim_width = (int)fr["cfg_avt_725"];
					m_ipc->ipc_OSD->ch0_aim_height = (int)fr["cfg_avt_363"];
					m_ipc->ipc_OSD->ch1_aim_height = (int)fr["cfg_avt_923"];
					m_ipc->ipc_OSD->ch2_aim_height = (int)fr["cfg_avt_1035"];
					m_ipc->ipc_OSD->ch3_aim_height = (int)fr["cfg_avt_1147"];
					m_ipc->ipc_OSD->ch4_aim_height = (int)fr["cfg_avt_1259"];
					//m_ipc->ipc_OSD->ch5_aim_height = (int)fr["cfg_avt_731"];
				}
				if(0x2f == blockId || allFlag)
				{
					m_ipc->ipc_OSD->CROSS_draw_show[0] =(int)fr["cfg_avt_736"];
					m_ipc->ipc_OSD->OSD_draw_color =(int)fr["cfg_avt_737"];
					m_ipc->ipc_OSD->CROSS_AXIS_WIDTH = (int)fr["cfg_avt_738"];
					m_ipc->ipc_OSD->CROSS_AXIS_HEIGHT = (int)fr["cfg_avt_739"];
					m_ipc->ipc_OSD->Picp_CROSS_AXIS_WIDTH = (int)fr["cfg_avt_740"];
					m_ipc->ipc_OSD->Picp_CROSS_AXIS_HEIGHT = (int)fr["cfg_avt_741"];
				}

				if(0x31 == blockId || allFlag)
				{
					m_ptzSpeed.SpeedLevelPan[0] = (int)fr["cfg_avt_768"];
					m_ptzSpeed.SpeedLevelPan[1] = (int)fr["cfg_avt_769"];
					m_ptzSpeed.SpeedLevelPan[2] = (int)fr["cfg_avt_770"];
					m_ptzSpeed.SpeedLevelPan[3] = (int)fr["cfg_avt_771"];
					m_ptzSpeed.SpeedLevelPan[4] = (int)fr["cfg_avt_772"];
					m_ptzSpeed.SpeedLevelPan[5] = (int)fr["cfg_avt_773"];
					m_ptzSpeed.SpeedLevelPan[6] = (int)fr["cfg_avt_774"];
					m_ptzSpeed.SpeedLevelPan[7] = (int)fr["cfg_avt_775"];
					m_ptzSpeed.SpeedLevelPan[8] = (int)fr["cfg_avt_776"];

					m_ptzSpeed.SpeedLeveTilt[0] = (int)fr["cfg_avt_777"];
					m_ptzSpeed.SpeedLeveTilt[1] = (int)fr["cfg_avt_778"];
					m_ptzSpeed.SpeedLeveTilt[2] = (int)fr["cfg_avt_779"];
					m_ptzSpeed.SpeedLeveTilt[3] = (int)fr["cfg_avt_780"];
					m_ptzSpeed.SpeedLeveTilt[4] = (int)fr["cfg_avt_781"];
					m_ptzSpeed.SpeedLeveTilt[5] = (int)fr["cfg_avt_782"];
					m_ptzSpeed.SpeedLeveTilt[6] = (int)fr["cfg_avt_783"];

				}
				if(0x32 == blockId || allFlag)
				{
					m_ptzSpeed.SpeedLeveTilt[7] = (int)fr["cfg_avt_784"];
					m_ptzSpeed.SpeedLeveTilt[8] = (int)fr["cfg_avt_785"];
				}

			}else{
				printf("Can not find YML. Please put this file into the folder of execute file\n");
				exit (-1);
			}
			m_ipc->IpcConfig();

		}
	}else
		fclose(fp);
	return 0;
}

int CManager::answerRead(int block, int field)
{
	m_ipc->ipc_OSD = m_ipc->getOSDSharedMem();
	m_ipc->ipc_UTC = m_ipc->getUTCSharedMem();

	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;

	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;

	PlatformFilter_Obj* pPidObj;
	string rStr;
	OSD_text* wOSD;
	wOSD = &m_OSD;

	int check = ((block -1) * 16 + field);
	float swp_fovx;
	float value, leve;
	value = cfg_value[check];
	switch(check)
	{
		case 1:
			value = pObj->params.joystickRateDemandParam.fDeadband;
			break;

		case 2:
			value = pObj->params.joystickRateDemandParam.fCutpoint1;
			break;

		case 3:
			value = pObj->params.joystickRateDemandParam.fInputGain_X1;
			break;

		case 4:
			value = pObj->params.joystickRateDemandParam.fInputGain_Y1;
			break;

		case 5:
			value = pObj->params.joystickRateDemandParam.fCutpoint2;
			break;

		case 6:
			value = pObj->params.joystickRateDemandParam.fInputGain_X2;
			break;

		case 7:
			value = pObj->params.joystickRateDemandParam.fInputGain_Y2;
			break;



		case 17:
		case 1441:
		case 1473:
		case 1505:
		case 1537:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			value = pPidObj->params.G;
			//printf("ppid->G=%f\n", pPidObj->params.G);
			break;

		case 18:
		case 1442:
		case 1474:
		case 1506:
		case 1538:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			value = pPidObj->params.P0;//Ki
			break;

		case 19:
		case 1443:
		case 1475:
		case 1507:
		case 1539:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			value = pPidObj->params.P1;//Kd
			break;

		case 20:
		case 1444:
		case 1476:
		case 1508:
		case 1540:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][0];
			value = pPidObj->params.P2;//k
			break;

		case 21:
		case 1445:
		case 1477:
		case 1509:
		case 1541:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			value = pPidObj->params.G;//Kp
			break;

		case 22:
		case 1446:
		case 1478:
		case 1510:
		case 1542:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			value = pPidObj->params.P0;//Ki
			break;

		case 23:
		case 1447:
		case 1479:
		case 1511:
		case 1543:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			value = pPidObj->params.P1;//Kd
			break;

		case 24:
		case 1448:
		case 1480:
		case 1512:
		case 1544:
			pPidObj = (PlatformFilter_Obj*)pObj->privates.filter[0][1];
			value = pPidObj->params.P2;//K
			break;

		case 757:
		case 1461:
		case 1493:
		case 1525:
		case 1557:
			value = pObj->params.bleedX;
			break;

		case 758:
		case 1462:
		case 1494:
		case 1526:
	    case 1558:
			value = pObj->params.bleedY;
			break;

		case 35:
		case 1588:
		case 1620:
		case 1652:
		case 1684:
			value = pObj->params.demandMaxX;
			break;

		case 36:
		case 1589:
		case 1621:
		case 1653:
		case 1685:
			value = pObj->params.demandMaxY;
			break;

		case 37:
		case 1590:
		case 1622:
		case 1654:
		case 1686:
			value = pObj->params.deadbandX;
			break;

		case 38:
		case 1591:
		case 1623:
		case 1655:
		case 1687:
			value = pObj->params.deadbandY;
			break;

		case 39:
			//value = m_ptzSpeed.curve;
			break;

		case 40:
			value = m_ipc->losttime ;
			break;

		case 46:
		//	pObj->params.OutputSwitch=(float) value;
				break;
		case 47:
		//	pObj->params.OutputPort=(float) value;
				break;

		case 48:
			value = m_ipc->ipc_UTC->occlusion_thred;
			printf("--------48-------->m_ipc->ipc_UTC->occlusion_thred=%f",m_ipc->ipc_UTC->occlusion_thred);
			break;
		case 49:
			value = m_ipc->ipc_UTC->retry_acq_thred;
			printf("--------49-------->m_ipc->ipc_UTC->retry_acq_thred=%f\n",m_ipc->ipc_UTC->retry_acq_thred);
			break;
		case 50:
			value = m_ipc->ipc_UTC->up_factor;
			break;
		case 51:
			value = m_ipc->ipc_UTC->res_distance;
			break;
		case 52:
			value = m_ipc->ipc_UTC->res_area;
			break;
		case 53:
			value = m_ipc->ipc_UTC->gapframe;
			break;
		case 54:
			value = m_ipc->ipc_UTC->enhEnable;
			break;
		case 55:
			value = m_ipc->ipc_UTC->cliplimit;
			break;
		case 56:
			value = m_ipc->ipc_UTC->dictEnable;
			break;
		case 57:
			value = m_ipc->ipc_UTC->moveX;
			break;
		case 58:
			value = m_ipc->ipc_UTC->moveY;
			break;
		case 59:
			value = m_ipc->ipc_UTC->moveX2;
			break;
		case 60:
			value = m_ipc->ipc_UTC->moveY2;
			break;
		case 61:
			value = m_ipc->ipc_UTC->segPixelX;
			break;
		case 62:
			value = m_ipc->ipc_UTC->segPixelY;
			break;
		case 63:
			value = m_ipc->ipc_UTC->algOsdRect_Enable;
			break;

		case 64:
			value = m_ipc->ipc_UTC->ScalerLarge;
			printf("--------64-------->m_ipc->ipc_UTC->ScalerLarge=%d\n",m_ipc->ipc_UTC->ScalerLarge);
			break;
		case 65:
			value = m_ipc->ipc_UTC->ScalerMid;
			printf("---------65------->m_ipc->ipc_UTC->ScalerMid=%d\n",m_ipc->ipc_UTC->ScalerMid);
			break;
		case 66:
			value = m_ipc->ipc_UTC->ScalerSmall;
			break;
		case 67:
			value = m_ipc->ipc_UTC->Scatter;
			break;
		case 68:
			value = m_ipc->ipc_UTC->ratio;
			break;
		case 69:
			value = m_ipc->ipc_UTC->FilterEnable;
			break;
		case 70:
			value = m_ipc->ipc_UTC->BigSecEnable;
			break;
		case 71:
			value = m_ipc->ipc_UTC->SalientThred;
			break;
		case 72:
			value = m_ipc->ipc_UTC->ScalerEnable;
			break;
		case 73:
			value = m_ipc->ipc_UTC->DynamicRatioEnable;
			break;
		case 74:
			value = m_ipc->ipc_UTC->acqSize_width;
			break;
		case 75:
			value = m_ipc->ipc_UTC->acqSize_height;
			break;
		case 76:
			value = m_ipc->ipc_UTC->TrkAim43_Enable;
			break;
		case 77:
			value = m_ipc->ipc_UTC->SceneMVEnable;
			break;
		case 78:
			value = m_ipc->ipc_UTC->BackTrackEnable;
			break;
		case 79:
			value = m_ipc->ipc_UTC->bAveTrkPos;
			break;
		case 80:
			value = m_ipc->ipc_UTC->fTau;
			printf("--------80----->m_ipc->ipc_UTC->fTau =%f\n",m_ipc->ipc_UTC->fTau );
			break;
		case 81:
			value = m_ipc->ipc_UTC->buildFrms;
			break;
		case 82:
			value = m_ipc->ipc_UTC->LostFrmThred;
			break;
		case 83:
			value = m_ipc->ipc_UTC->histMvThred;
			break;
		case 84:
			value = m_ipc->ipc_UTC->detectFrms;
			break;
		case 85:
			value = m_ipc->ipc_UTC->stillFrms;
			break;
		case 86:
			value = m_ipc->ipc_UTC->stillThred;
			break;
		case 87:
			value = m_ipc->ipc_UTC->bKalmanFilter;
			break;
		case 88:
			value = m_ipc->ipc_UTC->xMVThred;
			break;
		case 89:
			value = m_ipc->ipc_UTC->yMVThred;
			break;
		case 90:
			value = m_ipc->ipc_UTC->xStillThred;
			break;
		case 91:
			value = m_ipc->ipc_UTC->yStillThred;
			break;
		case 92:
			value = m_ipc->ipc_UTC->slopeThred;
			break;
		case 93:
			value = m_ipc->ipc_UTC->kalmanHistThred;
			break;
		case 94:
			value = m_ipc->ipc_UTC->kalmanCoefQ;
			break;
		case 95:
			value = m_ipc->ipc_UTC->kalmanCoefR;
			break;

		case 96:
			value =wOSD->OSD_text_show[0];
			break;
		case 97:
			value = wOSD->OSD_text_positionX[0];
			break;
		case 98:
			value = wOSD->OSD_text_positionY[0];
			break;
		case 99:
			rStr =wOSD->OSD_text_content[0];
			m_GlobalDate->choose=1;
			break;
		case 101:
			value = wOSD->OSD_text_color[0];
			break;
		case 102:
			value = wOSD->OSD_text_alpha[0];
			break;

		case 112:
			value =wOSD->OSD_text_show[1];
			break;
		case 113:
			value = wOSD->OSD_text_positionX[1];
			break;
		case 114:
			value = wOSD->OSD_text_positionY[1];
			break;
		case 115:
			rStr =wOSD->OSD_text_content[1];
			m_GlobalDate->choose=1;
			break;
		case 117:
			value = wOSD->OSD_text_color[1];
			break;
		case 118:
			value = wOSD->OSD_text_alpha[1];
			break;


		case 128:
			value = wOSD->OSD_text_show[2];
			break;
		case 129:
		   	value = wOSD->OSD_text_positionX[2];
			break;
		case 130:
			value = wOSD->OSD_text_positionY[2];
			break;
		case 131:
			rStr =wOSD->OSD_text_content[2];
			m_GlobalDate->choose=1;
			break;
		case 133:
			value = wOSD->OSD_text_color[2];
			break;
		case 134:
			value = wOSD->OSD_text_alpha[2];
			break;


		case 144:
			value = wOSD->OSD_text_show[3];
			break;
		case 145:
			value = wOSD->OSD_text_positionX[3];
			break;
		case 146:
			value = wOSD->OSD_text_positionY[3];
			break;
		case 147:
			rStr =wOSD->OSD_text_content[3];
			m_GlobalDate->choose=1;
			break;
		case 149:
			value = wOSD->OSD_text_color[3];
			break;
		case 150:
			value = wOSD->OSD_text_alpha[3];
			break;


		case 160:
			value = wOSD->OSD_text_show[4];
			break;
		case 161:
			value = wOSD->OSD_text_positionX[4];
			break;
		case 162:
			value = wOSD->OSD_text_positionY[4];
			break;
		case 163:
			rStr =wOSD->OSD_text_content[4];
			m_GlobalDate->choose=1;
			break;
		case 165:
			value = wOSD->OSD_text_color[4];
			break;
		case 166:
			value = wOSD->OSD_text_alpha[4];
			break;

		case 176:
			value = wOSD->OSD_text_show[5];
			break;
		case 177:
			value = wOSD->OSD_text_positionX[5];
			break;
		case 178:
			value = wOSD->OSD_text_positionY[5];
			break;
		case 179:
			rStr =wOSD->OSD_text_content[5];
			m_GlobalDate->choose=1;
			break;
		case 181:
			value = wOSD->OSD_text_color[5];
			break;
		case 182:
			value = wOSD->OSD_text_alpha[5];
			break;

		case 192:
			value = wOSD->OSD_text_show[6];
			break;
		case 193:
			value = wOSD->OSD_text_positionX[6];
			break;
		case 194:
			value = wOSD->OSD_text_positionY[6];
			break;
		case 195:
			rStr =wOSD->OSD_text_content[6];
			m_GlobalDate->choose=1;
			break;
		case 197:
			value = wOSD->OSD_text_color[6];
			break;
		case 198:
			value = wOSD->OSD_text_alpha[6];
			break;

		case 208:
			value = wOSD->OSD_text_show[7];
			break;
		case 209:
			value = wOSD->OSD_text_positionX[7];
			break;
		case 210:
			value = wOSD->OSD_text_positionY[7];
			break;
		case 211:
			rStr =wOSD->OSD_text_content[7];
			m_GlobalDate->choose=1;
			break;
		case 213:
			value = wOSD->OSD_text_color[7];
			break;
		case 214:
			value = wOSD->OSD_text_alpha[7];
			break;


		case 224:
			value = wOSD->OSD_text_show[8];
			break;
		case 225:
			value = wOSD->OSD_text_positionX[8];
			break;
		case 226:
			value = wOSD->OSD_text_positionY[8];
			break;
		case 227:
			rStr =wOSD->OSD_text_content[8];
			m_GlobalDate->choose=1;
			break;
		case 229:
			value = wOSD->OSD_text_color[8];
			break;
		case 230:
			value = wOSD->OSD_text_alpha[8];
			break;

		case 240:
			value = wOSD->OSD_text_show[9];
			break;
		case 241:
			value = wOSD->OSD_text_positionX[9];
			break;
		case 242:
			value = wOSD->OSD_text_positionY[9];
			break;
		case 243:
			rStr =wOSD->OSD_text_content[9];
			m_GlobalDate->choose=1;
			break;
		case 245:
			value = wOSD->OSD_text_color[9];
			break;
		case 246:
			value = wOSD->OSD_text_alpha[9];
			break;

		case 256:
			value = wOSD->OSD_text_show[10];
			break;
		case 257:
			value = wOSD->OSD_text_positionX[10];
			break;
		case 258:
			value = wOSD->OSD_text_positionY[10];
			break;
		case 259:
			rStr =wOSD->OSD_text_content[10];
			m_GlobalDate->choose=1;
			break;
		case 261:
			value = wOSD->OSD_text_color[10];
			break;
		case 262:
			value = wOSD->OSD_text_alpha[10];
			break;

		case 272:
			value = wOSD->OSD_text_show[11];
			break;
		case 273:
			value = wOSD->OSD_text_positionX[11];
			break;
		case 274:
			value = wOSD->OSD_text_positionY[11];
			break;
		case 275:
			rStr =wOSD->OSD_text_content[11];
			m_GlobalDate->choose=1;
			break;
		case 277:
			value = wOSD->OSD_text_color[11];
			break;
		case 278:
			value = wOSD->OSD_text_alpha[11];
			break;

		case 288:
			value = wOSD->OSD_text_show[12];
			break;
		case 289:
			value = wOSD->OSD_text_positionX[12];
			break;
		case 290:
			value = wOSD->OSD_text_positionY[12];
			break;
		case 291:
			rStr =wOSD->OSD_text_content[12];
			m_GlobalDate->choose=1;
			break;
		case 293:
			value = wOSD->OSD_text_color[12];
			break;
		case 294:
			value = wOSD->OSD_text_alpha[12];
			break;

		case 304:
			value = wOSD->OSD_text_show[13];
			break;
		case 305:
			value = wOSD->OSD_text_positionX[13];
			break;
		case 306:
			value = wOSD->OSD_text_positionY[13];
			break;
		case 307:
			rStr =wOSD->OSD_text_content[13];
			m_GlobalDate->choose=1;
			break;
		case 309:
			value = wOSD->OSD_text_color[13];
			break;
		case 310:
			value = wOSD->OSD_text_alpha[13];
			break;

		case 320:
			value = wOSD->OSD_text_show[14];
			break;
		case 321:
			value = wOSD->OSD_text_positionX[14];
			break;
		case 322:
			value = wOSD->OSD_text_positionY[14];
			break;
		case 323:
			rStr =wOSD->OSD_text_content[14];
			m_GlobalDate->choose=1;
			break;
		case 325:
			value = wOSD->OSD_text_color[14];
			break;
		case 326:
			value = wOSD->OSD_text_alpha[14];
			break;


		case 336:
			value = wOSD->OSD_text_show[15];
			break;
		case 337:
			value = wOSD->OSD_text_positionX[15];
			break;
		case 338:
			value = wOSD->OSD_text_positionY[15];
			break;
		case 339:
			rStr =wOSD->OSD_text_content[15];
			m_GlobalDate->choose=1;
			break;
		case 341:
			value = wOSD->OSD_text_color[15];
			break;
		case 342:
			value = wOSD->OSD_text_alpha[15];
			break;

        case 384:
                 value = m_GlobalDate->switchFovLV[0];
                 break;
         case 944:
                 value =m_GlobalDate->switchFovLV[1];
                 break;
          case 1056:
                 value = m_GlobalDate->switchFovLV[2];
                 break;
          case 1168:
                 value = m_GlobalDate->switchFovLV[3];
                 break;
          case 1280:
                 value = m_GlobalDate->switchFovLV[4];
                 break;
          case 899:
                value = m_GlobalDate->continueFovLv[0];
                break;
          case 1011:
                value = m_GlobalDate->continueFovLv[1];
                break;
          case 1123:
                value = m_GlobalDate->continueFovLv[2];
                break;
          case 1235:
                value = m_GlobalDate->continueFovLv[3];
                break;
          case 1347:
                value = m_GlobalDate->continueFovLv[4];
                break;

          case 352:
                value =m_ipc->ipc_OSD->osdChidIDShow[0];
                break;
          case 354:
                value =m_ipc->ipc_OSD->osdChidNmaeShow[0] ;
                 break;
          case 355:
                value =m_GlobalDate->cameraSwitch[0];
                 break;
          case 361:
                value =m_GlobalDate->AutoBox[0];
                 break;
          case 912:
                 value =m_ipc->ipc_OSD->osdChidIDShow[1];
                 break;
          case 914:
                 value =m_ipc->ipc_OSD->osdChidNmaeShow[1];
                 break;
          case 915:
                 value =m_GlobalDate->cameraSwitch[1];
                 break;
          case 921:
                 value =m_GlobalDate->AutoBox[1];
                 break;

          case 1024:
                 value =m_ipc->ipc_OSD->osdChidIDShow[2];
                 break;
          case 1026:
                 value =m_ipc->ipc_OSD->osdChidNmaeShow[2];
                 break;
          case 1027:
                 value =m_GlobalDate->cameraSwitch[1];
                 break;
          case 1033:
                 value =m_GlobalDate->AutoBox[1];
                 break;

          case 1136:
                 value =m_ipc->ipc_OSD->osdChidIDShow[3];
                 break;
          case 1138:
                 value =m_ipc->ipc_OSD->osdChidNmaeShow[3];
                 break;
          case 1139:
                 value =m_GlobalDate->cameraSwitch[1];
                 break;
          case 1145:
                 value =m_GlobalDate->AutoBox[1];
                 break;

          case 1248:
                 value =m_ipc->ipc_OSD->osdChidIDShow[4];
                 break;
          case 1250:
                 value =m_ipc->ipc_OSD->osdChidNmaeShow[4];
                 break;
          case 1251:
                 value =m_GlobalDate->cameraSwitch[1];
                 break;
          case 1257:
                 value =m_GlobalDate->AutoBox[1];
                 break;

          case 359:
                 value =pThis->m_ipc->ipc_OSD->osdBoxShow[0];
                 break;
          case 919:
                 value =pThis->m_ipc->ipc_OSD->osdBoxShow[1];
                 break;
          case 1031:
                 value =pThis->m_ipc->ipc_OSD->osdBoxShow[2];
                 break;
          case 1143:
                 value =pThis->m_ipc->ipc_OSD->osdBoxShow[3];
                 break;
          case 1255:
         	 	 value =pThis->m_ipc->ipc_OSD->osdBoxShow[4];
         	 break;
          case 360:
                 value =m_ipc->ipc_OSD->CROSS_draw_show[0];
                 break;
          case 920:
                 value =m_ipc->ipc_OSD->CROSS_draw_show[1];
                 break;
          case 1032:
                 value =m_ipc->ipc_OSD->CROSS_draw_show[2];
                 break;
          case 1144:
                 value =m_ipc->ipc_OSD->CROSS_draw_show[3];
                 break;
          case 1256:
                 value =m_ipc->ipc_OSD->CROSS_draw_show[4];
                break;
                /**********************************************************************/
                		case 358:
                                    value =viewParam.radio[0];
                                    break;
                                case 368:
                                    value  =viewParam.level_Fov_fix[0];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 369:
                                    value  =viewParam.vertical_Fov_fix[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 370:
                                    value= viewParam.Boresightpos_fix_x[0];
                                    break;
                                case 371:
                                    value= viewParam.Boresightpos_fix_y[0];
                                    break;

                                case 385:
                                     value  =viewParam.level_Fov_switch1[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 386:
                                    value  =viewParam.vertical_Fov_switch1[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 387:
                                    value= viewParam.Boresightpos_switch_x1[0];
                                    break;
                                case 388:
                                    value= viewParam.Boresightpos_switch_y1[0];
                                    break;
                                case 389:
                                    leve =viewParam.level_Fov_switch2[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 390:
                                    leve =viewParam.vertical_Fov_switch2[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 391:
                                    value=viewParam.Boresightpos_switch_x2[0];
                                    break;
                                case 392:
                                    value=viewParam.Boresightpos_switch_y2[0];
                                    break;
                                case 393:
                                    leve=viewParam.level_Fov_switch3[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 394:
                                    leve=viewParam.vertical_Fov_switch3[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 395:
                                    value=viewParam.Boresightpos_switch_x3[0];
                                    break;
                                case 396:
                                    value=viewParam.Boresightpos_switch_y3[0];
                                    break;
                                case 397:
                                    leve =viewParam.level_Fov_switch4[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 398:
                                    leve =viewParam.vertical_Fov_switch4[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 399:
                                    value=viewParam.Boresightpos_switch_x4[0] ;
                                    break;
                                case 400:
                                    value= viewParam.Boresightpos_switch_y4[0];
                                    break;
                                case 401:
                                    leve=viewParam.level_Fov_switch5[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 402:
                                    leve =viewParam.vertical_Fov_switch5[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 403:
                                    value=viewParam.Boresightpos_switch_x5[0];
                                    break;
                                case 404:
                                    value=viewParam.Boresightpos_switch_y5[0];
                                    break;
                                case 416:
                                    value= viewParam.zoombak1[0];
                                    break;
                                case 417:
                                    leve=viewParam.level_Fov_continue1[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 418:
                                    leve =viewParam.vertical_Fov_continue1[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 419:
                                    value=viewParam. Boresightpos_continue_x1[0] ;
                                    break;
                                case 420:
                                    value= viewParam.Boresightpos_continue_y1[0];
                                    break;
                                case 421:
                                    value= viewParam.zoombak2[0];
                                    break;
                                case 422:
                                    leve =viewParam.level_Fov_continue2[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 423:
                                    leve =viewParam.vertical_Fov_continue2[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 424:
                                    value=viewParam. Boresightpos_continue_x2[0];
                                    break;
                                case 425:
                                    value=viewParam.Boresightpos_continue_y2[0];
                                    break;
                                case 426:
                                    value=viewParam.zoombak3[0];
                                    break;
                                case 427:
                                    leve = viewParam.level_Fov_continue3[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 428:
                                     leve =viewParam.vertical_Fov_continue3[0];
                                    value=  (leve/ 1000) * (180 / PI);
                                    break;
                                case 429:

                                    value= viewParam. Boresightpos_continue_x3[0];
                                    break;
                                case 430:
                                    value= viewParam.Boresightpos_continue_y3[0];
                                    break;
                                case 431:
                                    value= viewParam.zoombak4[0];
                                    break;
                                case 880:
                                    leve =viewParam.level_Fov_continue4[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 881:
                                    leve =viewParam.vertical_Fov_continue4[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 882:
                                    value= viewParam. Boresightpos_continue_x4[0];
                                    break;
                                case 883:
                                    value= viewParam.Boresightpos_continue_y4[0];
                                    break;
                                case 884:
                                    value= viewParam.zoombak5[0];
                                    break;
                                case 885:
                                    leve =viewParam.level_Fov_continue5[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 886:
                                    leve =viewParam.vertical_Fov_continue5[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 887:
                                    value= viewParam. Boresightpos_continue_x5[0];
                                    break;
                                case 888:
                                    value= viewParam.Boresightpos_continue_y5[0];
                                    break;
                                case 889:
                                    value= viewParam.zoombak6[0];
                                    break;
                                case 890:
                                    leve =viewParam.level_Fov_continue6[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 891:
                                    leve =viewParam.vertical_Fov_continue6[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 892:
                                    value= viewParam. Boresightpos_continue_x6[0];
                                    break;
                                case 893:
                                    value= viewParam.Boresightpos_continue_y6[0];
                                    break;
                                case 894:
                                    value= viewParam.zoombak7[0];
                                    break;
                                case 895:
                                    leve =viewParam.level_Fov_continue7[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 896:
                                    leve =viewParam.level_Fov_continue7[0];
                                    value= (leve/ 1000) * (180 / PI);
                                    break;
                                case 897:
                                    value= viewParam. Boresightpos_continue_x7[0];
                                    break;
                                case 898:
                                    value= viewParam.Boresightpos_continue_y7[0];
                                    break;

                                case 918:
                                    value = viewParam.radio[1];
                                    break;
                                case 928:
                                    leve = viewParam.level_Fov_fix[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 929:
                                    leve = viewParam.vertical_Fov_fix[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 930:
                                    value = viewParam.Boresightpos_fix_x[1];
                                    break;
                                case 931:
                                    value = viewParam.Boresightpos_fix_y[1];
                                    break;


                                case 945:
                                    leve = viewParam.level_Fov_switch1[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 946:
                                    leve = viewParam.vertical_Fov_switch1[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 947:
                                    value = viewParam.Boresightpos_switch_x1[1];
                                    break;
                                case 948:
                                    value = viewParam.Boresightpos_switch_y1[1];
                                    break;
                                case 949:
                                    leve = viewParam.level_Fov_switch2[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 950:
                                    leve = viewParam.vertical_Fov_switch2[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 951:
                                    value = viewParam.Boresightpos_switch_x2[1];
                                    break;
                                case 952:
                                    value =viewParam.Boresightpos_switch_y2[1] ;
                                    break;
                                case 953:
                                    leve = viewParam.level_Fov_switch3[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 954:
                                    leve = viewParam.vertical_Fov_switch3[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 955:
                                    value = viewParam.Boresightpos_switch_x3[1];
                                    break;
                                case 956:
                                    value =viewParam.Boresightpos_switch_y3[1];
                                    break;
                                case 957:
                                    leve = viewParam.level_Fov_switch4[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 958:
                                    leve =  viewParam.vertical_Fov_switch4[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 959:
                                    value = viewParam.Boresightpos_switch_x4[1];
                                    break;
                                case 960:
                                    value =viewParam.Boresightpos_switch_y4[1] ;
                                    break;
                                case 961:
                                    leve = viewParam.level_Fov_switch5[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 962:
                                    leve = viewParam.vertical_Fov_switch5[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 963:
                                    value = viewParam.Boresightpos_switch_x5[1];
                                    break;
                                case 964:
                                    value = viewParam.Boresightpos_switch_y5[1];
                                    break;

                                case 976:
                                    value = viewParam.zoombak1[1];
                                    break;
                                case 977:
                                    leve = viewParam.level_Fov_continue1[1];
                                    value =(leve/ 1000) * (180 / PI) ;
                                    break;
                                case 978:
                                    leve = viewParam.vertical_Fov_continue1[1];
                                    value =(leve/ 1000) * (180 / PI) ;
                                    break;
                                case 979:
                                    value = viewParam. Boresightpos_continue_x1[1];
                                    break;
                                case 980:
                                    value = viewParam.Boresightpos_continue_y1[1];
                                    break;

                                case 981:
                                    value = viewParam.zoombak2[1];
                                    break;
                                case 982:
                                    leve = viewParam.level_Fov_continue2[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 983:
                                    leve = viewParam.vertical_Fov_continue2[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 984:
                                    value = viewParam. Boresightpos_continue_x2[1];
                                    break;
                                case 985:
                                    value = viewParam.Boresightpos_continue_y2[1];
                                    break;

                                case 986:
                                    value = viewParam.zoombak3[1];
                                    break;
                                case 987:
                                    leve = viewParam.level_Fov_continue3[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 988:
                                    leve = viewParam.vertical_Fov_continue3[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 989:
                                    value = viewParam. Boresightpos_continue_x3[1];
                                    break;
                                case 990:
                                    value = viewParam.Boresightpos_continue_y3[1];
                                    break;

                                case 991:
                                    value = viewParam.zoombak4[1];
                                    break;
                                case 992:
                                    leve = viewParam.level_Fov_continue4[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 993:
                                    leve = viewParam.vertical_Fov_continue4[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 994:
                                    value = viewParam. Boresightpos_continue_x4[1];
                                    break;
                                case 995:
                                    value = viewParam.Boresightpos_continue_y4[1];
                                    break;

                                case 996:
                                    value = viewParam.zoombak5[1];
                                    break;
                                case 997:
                                    leve = viewParam.level_Fov_continue5[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 998:
                                    leve = viewParam.vertical_Fov_continue5[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 999:
                                    value = viewParam. Boresightpos_continue_x5[1];
                                    break;
                                case 1000:
                                    value = viewParam.Boresightpos_continue_y5[1];
                                    break;


                                case 1001:
                                    value = viewParam.zoombak6[1];
                                    break;
                                case 1002:
                                    leve = viewParam.level_Fov_continue6[1];
                                    value =(leve/ 1000) * (180 / PI) ;
                                    break;
                                case 1003:
                                    leve = viewParam.vertical_Fov_continue6[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 1004:
                                    value = viewParam. Boresightpos_continue_x6[1];
                                    break;
                                case 1005:
                                    value = viewParam.Boresightpos_continue_y6[1];
                                    break;

                                case 1006:
                                    value = viewParam.zoombak7[1];
                                    break;
                                case 1007:
                                    leve = viewParam.level_Fov_continue7[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 1008:
                                    leve = viewParam.vertical_Fov_continue7[1];
                                    value = (leve/ 1000) * (180 / PI);
                                    break;
                                case 1009:
                                    value = viewParam. Boresightpos_continue_x7[1];
                                    break;
                                case 1010:
                                    value = viewParam.Boresightpos_continue_y7[1];
                                    break;
                       /*通道3固定视场*/
                               case 1030:
                                   value =viewParam.radio[2];
                                   break;
                               case 1042:
                                   leve =viewParam.level_Fov_fix[2];
                                   value = (leve/ 1000) * (180 / PI);
                                   break;
                               case 1043:
                                   leve = viewParam.vertical_Fov_continue7[2];
                                   value = (leve/ 1000) * (180 / PI);
                                   break;
                               case 1044:
                                   value =viewParam.Boresightpos_fix_x[2];
                                   break;
                               case 1045:
                                   value =viewParam.Boresightpos_fix_y[2];
                                   break;
                               case 1057:
                                   leve =viewParam.level_Fov_switch1[2];
                                   value= (leve/ 1000) * (180 / PI);
                                   break;
                               case 1058:
                                    leve =viewParam.vertical_Fov_switch1[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1059:
                                   value =viewParam.Boresightpos_switch_x1[2];
                                   break;
                               case 1060:
                                   value =viewParam.Boresightpos_switch_y1[2];
                                   break;
                               case 1061:
                                   leve =viewParam.level_Fov_switch2[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1062:
                                   leve =viewParam.level_Fov_switch2[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1063:
                                   value =viewParam.Boresightpos_switch_x2[2];
                                   break;
                               case 1064:
                                   value =viewParam.Boresightpos_switch_y2[2];
                                   break;
                               case 1065:
                                    leve =viewParam.level_Fov_switch3[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1066:
                                    leve =viewParam.vertical_Fov_switch2[2];
                                    value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1067:
                                   value =viewParam.Boresightpos_switch_x3[2];
                                   break;
                               case 1068:
                                   value =viewParam.Boresightpos_switch_y3[2];
                                   break;
                               case 1069:
                                   leve =viewParam.level_Fov_switch4[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1070:
                                    leve =viewParam.vertical_Fov_switch4[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1071:
                                   value =viewParam.Boresightpos_switch_x4[2];
                                   break;
                               case 1072:
                                   value =viewParam.Boresightpos_switch_y4[2];
                                   break;
                               case 1073:
                                    leve =viewParam.level_Fov_switch5[2];
                                   value = (leve/ 1000) * (180 / PI);
                                   break;
                               case 1074:
                                    leve =viewParam.vertical_Fov_switch5[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1075:
                                   value =viewParam.Boresightpos_switch_x5[2];
                                   break;
                               case 1076:
                                   value =viewParam.Boresightpos_switch_y5[2];
                                   break;

                               case 1088:
                                   value =viewParam.zoombak1[2];
                                   break;
                               case 1089:
                                    leve =viewParam.level_Fov_continue1[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1090:
                                    leve =viewParam.vertical_Fov_continue1[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1091:
                                   value =viewParam.Boresightpos_continue_x1[2];
                                   break;
                               case 1092:
                                   value =viewParam.Boresightpos_continue_y1[2];
                                   break;
                               case 1093:
                                   value =viewParam.zoombak2[2];
                                   break;
                               case 1094:
                                   leve=viewParam.level_Fov_continue2[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1095:
                                    leve =viewParam.vertical_Fov_continue2[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1096:
                                   value =viewParam. Boresightpos_continue_x2[2];
                                   break;
                               case 1097:
                                   value =viewParam.Boresightpos_continue_y2[2];
                                   break;
                               case 1098:
                                   value =viewParam.zoombak3[2];
                                   break;
                               case 1099:
                                    leve =viewParam.level_Fov_continue3[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1100:
                                    leve = viewParam.vertical_Fov_continue3[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1101:
                                   value =viewParam. Boresightpos_continue_x3[2];
                                   break;
                               case 1102:
                                   value =viewParam.Boresightpos_continue_y3[2];
                                   break;
                               case 1103:
                                   value =viewParam.zoombak4[2];
                                   break;
                               case 1104:
                                    leve = viewParam.level_Fov_continue4[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1105:
                                    leve = viewParam.vertical_Fov_continue4[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1106:
                                   value =viewParam. Boresightpos_continue_x4[2];
                                   break;
                               case 1107:
                                   value =viewParam.Boresightpos_continue_y4[2];
                                   break;
                               case 1108:
                                   value =viewParam.zoombak5[2];
                                   break;
                               case 1109:
                                    leve = viewParam.level_Fov_continue5[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1110:
                                    leve =viewParam.vertical_Fov_continue5[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1111:
                                   value =viewParam. Boresightpos_continue_x5[2];
                                   break;
                               case 1112:
                                   value =viewParam.Boresightpos_continue_y5[2];
                                   break;
                               case 1113:
                                   value =viewParam.zoombak6[2];
                                   break;
                               case 1114:
                                    leve = viewParam.level_Fov_continue6[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1115:
                                    leve = viewParam.vertical_Fov_continue6[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;

                               case 1116:
                                   value =viewParam. Boresightpos_continue_x6[2];
                                   break;
                               case 1117:
                                   value =viewParam.Boresightpos_continue_y6[2];
                                   break;
                               case 1118:
                                   value =viewParam.zoombak7[2];
                                   break;
                               case 1119:
                                    leve =viewParam.level_Fov_continue7[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1120:
                                    leve = viewParam.vertical_Fov_continue7[2];
                                   value =(leve/ 1000) * (180 / PI);
                                   break;
                               case 1121:
                                   value =viewParam. Boresightpos_continue_x7[2];
                                   break;
                               case 1122:
                                   value =viewParam.Boresightpos_continue_y7[2];
                                   break;
                   /**********************************************************************/
                           /*通道4固定视场*/
                        case 1142:
                            value = viewParam.radio[3];
                            break;
                        case 1152:
                            leve = viewParam.level_Fov_fix[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1153:
                            leve = viewParam.vertical_Fov_fix[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1154:
                            value = viewParam.Boresightpos_fix_x[3];
                            break;
                        case 1155:
                            value = viewParam.Boresightpos_fix_y[3];
                            break;
                          /*通道4可切换视场*/
                        case 1169:
                            leve = viewParam.level_Fov_switch1[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1170:
                            leve = viewParam.vertical_Fov_switch1[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1171:
                            value = viewParam.Boresightpos_switch_x1[3];
                            break;
                        case 1172:
                            value = viewParam.Boresightpos_switch_y1[3];
                            break;

                        case 1173:
                            leve = viewParam.level_Fov_switch2[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1174:
                            leve = viewParam.vertical_Fov_switch2[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1175:
                            value = viewParam.Boresightpos_switch_x2[3];
                            break;
                        case 1176:
                            value = viewParam.Boresightpos_switch_y2[3];
                            break;

                        case 1177:
                            leve = viewParam.level_Fov_switch3[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1178:
                            leve =  viewParam.vertical_Fov_switch3[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1179:
                            value = viewParam.Boresightpos_switch_x3[3];
                            break;
                        case 1180:
                            value = viewParam.Boresightpos_switch_y3[3];
                            break;

                        case 1181:
                            leve =viewParam.level_Fov_switch4[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1182:
                            leve = viewParam.vertical_Fov_switch4[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1183:
                            value = viewParam.Boresightpos_switch_x4[3];
                            break;
                        case 1184:
                            value = viewParam.Boresightpos_switch_y4[3];
                            break;

                        case 1185:
                            leve = viewParam.level_Fov_switch5[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1186:
                            leve = viewParam.vertical_Fov_switch5[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1187:
                            value = viewParam.Boresightpos_switch_x5[3];
                            break;
                        case 1188:
                            value = viewParam.Boresightpos_switch_y5[3];
                            break;
                         /*通道4连续视场*/
                        case 1200:
                            value = viewParam.zoombak1[3];
                            break;
                        case 1201:
                            leve = viewParam.level_Fov_continue1[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1202:
                            leve = viewParam.vertical_Fov_continue1[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1203:
                            value = viewParam.Boresightpos_continue_x1[3];
                            break;
                        case 1204:
                            value = viewParam.Boresightpos_continue_y1[3];
                            break;

                        case 1205:
                            value = viewParam.zoombak2[3];
                            break;
                        case 1206:
                            leve = viewParam.level_Fov_continue2[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1207:
                            leve = viewParam.vertical_Fov_continue2[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1208:
                            value = viewParam.Boresightpos_continue_x2[3];
                            break;
                        case 1209:
                            value = viewParam.Boresightpos_continue_y2[3];
                            break;

                        case 1210:
                            value = viewParam.zoombak3[3];
                            break;
                        case 1211:
                            leve = viewParam.level_Fov_continue3[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1212:
                            leve = viewParam.vertical_Fov_continue3[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1213:
                            value = viewParam.Boresightpos_continue_x3[3];
                            break;
                        case 1214:
                            value = viewParam.Boresightpos_continue_y3[3];
                            break;

                        case 1215:
                            value = viewParam.zoombak4[3];
                            break;
                        case 1216:
                            leve = viewParam.level_Fov_continue4[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1217:
                            leve = viewParam.vertical_Fov_continue4[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1218:
                            value = viewParam.Boresightpos_continue_x4[3];
                            break;
                        case 1219:
                            value = viewParam.Boresightpos_continue_y4[3];
                            break;

                        case 1220:
                            value = viewParam.zoombak5[3];
                            break;
                        case 1221:
                            leve = viewParam.level_Fov_continue5[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1222:
                            leve = viewParam.vertical_Fov_continue5[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1223:
                            value = viewParam.Boresightpos_continue_x5[3];
                            break;
                        case 1224:
                            value = viewParam.Boresightpos_continue_y5[3];
                            break;

                        case 1225:
                            value = viewParam.zoombak6[3];
                            break;
                        case 1226:
                            leve = viewParam.level_Fov_continue6[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1227:
                            leve = viewParam.vertical_Fov_continue6[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1228:
                            value = viewParam.Boresightpos_continue_x6[3];
                            break;
                        case 1229:
                            value = viewParam.Boresightpos_continue_y6[3];
                            break;

                        case 1230:
                            value = viewParam.zoombak7[3];
                            break;
                        case 1231:
                            leve = viewParam.level_Fov_continue7[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1232:
                            leve = viewParam.vertical_Fov_continue7[3];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1233:
                            value = viewParam.Boresightpos_continue_x7[3];
                            break;
                        case 1234:
                            value = viewParam.Boresightpos_continue_y7[3];
                            break;
                 /*通道５*固定视场*/
                        case 1254:
                            value = viewParam.radio[4];
                            break;
                        case 1264:
                            leve = viewParam.level_Fov_fix[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1265:
                            leve = viewParam.vertical_Fov_fix[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1266:
                            value = viewParam.Boresightpos_fix_x[4];
                            break;
                        case 1267:
                            value = viewParam.Boresightpos_fix_y[4];
                            break;
                      /*通道5可切换视场*/
                        case 1281:
                            leve = viewParam.level_Fov_switch1[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1282:
                            leve = viewParam.vertical_Fov_switch1[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1283:
                            value = viewParam.Boresightpos_switch_x1[4];
                            break;
                        case 1284:
                            value = viewParam.Boresightpos_switch_y1[4];
                            break;

                        case 1285:
                            leve = viewParam.level_Fov_switch2[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1286:
                            leve = viewParam.vertical_Fov_switch2[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1287:
                            value = viewParam.Boresightpos_switch_x2[4];
                            break;
                        case 1288:
                            value = viewParam.Boresightpos_switch_y2[4];
                            break;

                        case 1289:
                            leve = viewParam.level_Fov_switch3[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1290:
                            leve = viewParam.vertical_Fov_switch3[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1291:
                            value = viewParam.Boresightpos_switch_x3[4];
                            break;
                        case 1292:
                            value = viewParam.Boresightpos_switch_y3[4];
                            break;

                        case 1293:
                            leve = viewParam.level_Fov_switch4[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1294:
                            leve = viewParam.vertical_Fov_switch4[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1295:
                            value = viewParam.Boresightpos_switch_x4[4];
                            break;
                        case 1296:
                            value = viewParam.Boresightpos_switch_y4[4];
                            break;

                        case 1297:
                            leve = viewParam.level_Fov_switch5[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1298:
                            leve = viewParam.vertical_Fov_switch5[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1299:
                            value = viewParam.Boresightpos_switch_x5[4];
                            break;
                        case 1300:
                            value = viewParam.Boresightpos_switch_y5[4];
                            break;
                      /*通道5连续视场*/
                        case 1312:
                            value = viewParam.zoombak1[4];
                            break;
                        case 1313:
                            leve = viewParam.level_Fov_continue1[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1314:
                            leve = viewParam.vertical_Fov_continue1[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1315:
                            value = viewParam.Boresightpos_continue_x1[4];
                            break;
                        case 1316:
                            value = viewParam.Boresightpos_continue_y1[4];
                            break;

                        case 1317:
                            value = viewParam.zoombak2[4];
                            break;
                        case 1318:
                            leve = viewParam.level_Fov_continue2[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1319:
                            leve = viewParam.vertical_Fov_continue2[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1320:
                            value = viewParam.Boresightpos_continue_x2[4];
                            break;
                        case 1321:
                            value = viewParam.Boresightpos_continue_y2[4];
                            break;

                        case 1322:
                            value = viewParam.zoombak3[ 4];
                            break;
                        case 1323:
                            leve = viewParam.level_Fov_continue3[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1324:
                            leve = viewParam.vertical_Fov_continue3[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1325:
                            value = viewParam.Boresightpos_continue_x3[4];
                            break;
                        case 1326:
                            value = viewParam.Boresightpos_continue_y3[4];
                            break;

                        case 1327:
                            value = viewParam.zoombak4[4];
                            break;
                        case 1328:
                            leve = viewParam.level_Fov_continue4[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1329:
                            leve = viewParam.vertical_Fov_continue4[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1330:
                            value = viewParam.Boresightpos_continue_x4[4];
                            break;
                        case 1331:
                            value = viewParam.Boresightpos_continue_y4[4];
                            break;

                        case 1332:
                            value = viewParam.zoombak5[4];
                            break;
                        case 1333:
                            leve = viewParam.level_Fov_continue5[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1334:
                            leve = viewParam.vertical_Fov_continue5[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1335:
                            value = viewParam.Boresightpos_continue_x5[4];
                            break;
                        case 1336:
                            value = viewParam.Boresightpos_continue_y5[4];
                            break;

                        case 1337:
                            value = viewParam.zoombak6[4];
                            break;
                        case 1338:
                            leve = viewParam.level_Fov_continue6[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1339:
                            leve = viewParam.vertical_Fov_continue6[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1340:
                            value = viewParam.Boresightpos_continue_x6[4];
                            break;
                        case 1341:
                            value = viewParam.Boresightpos_continue_y6[4];
                            break;

                        case 1342:
                            value = viewParam.zoombak7[4];
                            break;
                        case 1343:
                            leve = viewParam.level_Fov_continue7[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1344:
                            leve = viewParam.vertical_Fov_continue7[4];
                            value = (leve/ 1000) * (180 / PI);
                            break;
                        case 1345:
                            value = viewParam.Boresightpos_continue_x7[4];
                            break;
                        case 1346:
                            value = viewParam.Boresightpos_continue_y7[4];
                            break;

		case 432:
			value = uart.uartDevice;
			break;
		case 433:
			value = uart.baud;
			break;
		case 434:
			value = uart.data;
			break;
		case 435:
			value = uart.check;
			break;
		case 436:
			value = uart.stop;
			break;
		case 437:
			value = uart.flow;
			break;

		case 448:
			value = wOSD->OSD_text_show[16];
			break;
		case 449:
			value = wOSD->OSD_text_positionX[16];
			break;
		case 450:
			wOSD->OSD_text_positionY[16]=(int)value;
			break;
		case 451:
			rStr =wOSD->OSD_text_content[16];
			m_GlobalDate->choose=1;
			break;
		case 453:
			value = wOSD->OSD_text_color[16];
			break;
		case 454:
			value = wOSD->OSD_text_alpha[16];
			break;



		case 464:
			value = wOSD->OSD_text_show[17];
			break;
		case 465:
			value = wOSD->OSD_text_positionX[17];
			break;
		case 466:
			value = wOSD->OSD_text_positionY[17];
			break;
		case 467:
			rStr =wOSD->OSD_text_content[17];
			m_GlobalDate->choose=1;
			break;
		case 469:
			value = wOSD->OSD_text_color[17];
			break;
		case 470:
			value = wOSD->OSD_text_alpha[17];
			break;

		case 480:
			value = wOSD->OSD_text_show[18];
			break;
		case 481:
			value = wOSD->OSD_text_positionX[18];
			break;
		case 482:
			value = wOSD->OSD_text_positionY[18];
			break;
		case 483:
			rStr =wOSD->OSD_text_content[18];
			m_GlobalDate->choose=1;
			break;
		case 485:
			value = wOSD->OSD_text_color[18];
			break;
		case 486:
			value = wOSD->OSD_text_alpha[18];
			break;


		case 496:
			value = wOSD->OSD_text_show[19];
			break;
		case 497:
			value = wOSD->OSD_text_positionX[19];
			break;
		case 498:
			value = wOSD->OSD_text_positionY[19];
			break;
		case 499:
			rStr =wOSD->OSD_text_content[19];
			m_GlobalDate->choose=1;
			break;
		case 501:
			value = wOSD->OSD_text_color[19];
			break;
		case 502:
			value = wOSD->OSD_text_alpha[19];
			break;


		case 512:
			value = wOSD->OSD_text_show[20];
			break;
		case 513:
			value = wOSD->OSD_text_positionX[20];
			break;
		case 514:
			value = wOSD->OSD_text_positionY[20];
			break;
		case 515:
			rStr =wOSD->OSD_text_content[20];
			m_GlobalDate->choose=1;
			break;
		case 517:
			value = wOSD->OSD_text_color[20];
			break;
		case 518:
			value = wOSD->OSD_text_alpha[20];
			break;


		case 528:
			value = wOSD->OSD_text_show[21];
			break;
		case 529:
			value = wOSD->OSD_text_positionX[21];
			break;
		case 530:
			value = wOSD->OSD_text_positionY[21];
			break;
		case 531:
			rStr =wOSD->OSD_text_content[21];
			m_GlobalDate->choose=1;
			break;
		case 533:
			value = wOSD->OSD_text_color[21];
			break;
		case 534:
			value = wOSD->OSD_text_alpha[21];
			break;


		case 544:
			value = wOSD->OSD_text_show[22];
			break;
		case 545:
			value = wOSD->OSD_text_positionX[22];
			break;
		case 546:
			value = wOSD->OSD_text_positionY[22];
			break;
		case 547:
			rStr =wOSD->OSD_text_content[22];
			m_GlobalDate->choose=1;
			break;
		case 549:
			value = wOSD->OSD_text_color[22];
			break;
		case 550:
			value = wOSD->OSD_text_alpha[22];
			break;


		case 560:
			value = wOSD->OSD_text_show[23];
			break;
		case 561:
			value = wOSD->OSD_text_positionX[23];
			break;
		case 562:
			value = wOSD->OSD_text_positionY[23];
			break;
		case 563:
			rStr =wOSD->OSD_text_content[23];
			m_GlobalDate->choose=1;
			break;
		case 565:
			value = wOSD->OSD_text_color[23];
			break;
		case 566:
			value = wOSD->OSD_text_alpha[23];
			break;


		case 576:
			value = wOSD->OSD_text_show[24];
			break;
		case 577:
			value = wOSD->OSD_text_positionX[24];
			break;
		case 578:
			value = wOSD->OSD_text_positionY[24];
			break;
		case 579:
			rStr =wOSD->OSD_text_content[24];
			m_GlobalDate->choose=1;
			break;
		case 581:
			value = wOSD->OSD_text_color[24];
			break;
		case 582:
			value = wOSD->OSD_text_alpha[24];
			break;


		case 592:
			value = wOSD->OSD_text_show[25];
			break;
		case 593:
			value = wOSD->OSD_text_positionX[25];
			break;
		case 594:
			value = wOSD->OSD_text_positionY[25];
			break;
		case 595:
			rStr =wOSD->OSD_text_content[25];
			m_GlobalDate->choose=1;
			break;
		case 597:
			value = wOSD->OSD_text_color[25];
			break;
		case 598:
			value = wOSD->OSD_text_alpha[25];
			break;

		case 608:
			value = wOSD->OSD_text_show[26];
			break;
		case 609:
			value = wOSD->OSD_text_positionX[26];
			break;
		case 610:
			value = wOSD->OSD_text_positionY[26];
			break;
		case 611:
			rStr =wOSD->OSD_text_content[26];
			m_GlobalDate->choose=1;
			break;
		case 613:
			value = wOSD->OSD_text_color[26];
			break;
		case 614:
			value = wOSD->OSD_text_alpha[26];
			break;

		case 624:
			value = wOSD->OSD_text_show[27];
			break;
		case 625:
			value = wOSD->OSD_text_positionX[27];
			break;
		case 626:
			value = wOSD->OSD_text_positionY[27];
			break;
		case 627:
			rStr =wOSD->OSD_text_content[27];
			m_GlobalDate->choose=1;
			break;
		case 629:
			value = wOSD->OSD_text_color[27];
			break;
		case 630:
			value = wOSD->OSD_text_alpha[27];
			break;

		case 640:
			value = wOSD->OSD_text_show[28];
			break;
		case 641:
			value = wOSD->OSD_text_positionX[28];
			break;
		case 642:
			value = wOSD->OSD_text_positionY[28];
			break;
		case 643:
			rStr =wOSD->OSD_text_content[28];
			m_GlobalDate->choose=1;
			break;
		case 645:
			value = wOSD->OSD_text_color[28];
			break;
		case 646:
			value = wOSD->OSD_text_alpha[28];
			break;


		case 656:
			value = wOSD->OSD_text_show[29];
			break;
		case 657:
			value = wOSD->OSD_text_positionX[29];
			break;
		case 658:
			value = wOSD->OSD_text_positionY[29];
			break;
		case 659:
			rStr =wOSD->OSD_text_content[29];
			m_GlobalDate->choose=1;
			break;
		case 661:
			value = wOSD->OSD_text_color[29];
			break;
		case 662:
			value = 	wOSD->OSD_text_alpha[29];
			break;

		case 672:
			value = wOSD->OSD_text_show[30];
			break;
		case 673:
			value = wOSD->OSD_text_positionX[30];
			break;
		case 674:
			value = wOSD->OSD_text_positionY[30];
			break;
		case 675:
			rStr =wOSD->OSD_text_content[30];
			m_GlobalDate->choose=1;
			break;
		case 677:
			value = wOSD->OSD_text_color[30];
			break;
		case 678:
			value = wOSD->OSD_text_alpha[30];
			break;


		case 688:
			value = wOSD->OSD_text_show[31];
			break;
		case 689:
			value = wOSD->OSD_text_positionX[31];
			break;
		case 690:
			value = wOSD->OSD_text_positionY[31];
			break;
		case 691:
			rStr =wOSD->OSD_text_content[31];
			m_GlobalDate->choose=1;
			break;
		case 693:
			value = wOSD->OSD_text_color[31];
			break;
		case 694:
			value = wOSD->OSD_text_alpha[31];
			break;


		case 704:
			value = m_ipc->ipc_OSD->ch0_acqRect_width;
			break;
		case 705:
			value = m_ipc->ipc_OSD->ch1_acqRect_width;
			break;
		case 706:
			value = m_ipc->ipc_OSD->ch2_acqRect_width;
			break;
		case 707:
			value = m_ipc->ipc_OSD->ch3_acqRect_width;
			break;
		case 708:
			value = m_ipc->ipc_OSD->ch4_acqRect_width;
			break;
		case 709:
			value = m_ipc->ipc_OSD->ch5_acqRect_width;
			break;
		case 710:
			value = m_ipc->ipc_OSD->ch0_acqRect_height;
			break;
		case 711:
			value = m_ipc->ipc_OSD->ch1_acqRect_height;
			break;
		case 712:
			value = m_ipc->ipc_OSD->ch2_acqRect_height;
			break;
		case 713:
			value = m_ipc->ipc_OSD->ch3_acqRect_height;
			break;
		case 714:
			value = m_ipc->ipc_OSD->ch4_acqRect_height;
			break;
		case 715:
			value = m_ipc->ipc_OSD->ch5_acqRect_height;
			break;
		case 720:
			value = m_ipc->ipc_OSD->ch0_aim_width;
			break;
		case 721:
			value = m_ipc->ipc_OSD->ch1_aim_width;
			break;
		case 722:
			value = m_ipc->ipc_OSD->ch2_aim_width;
			break;
		case 723:
			value = m_ipc->ipc_OSD->ch3_aim_width;
			break;
		case 724:
			value = m_ipc->ipc_OSD->ch4_aim_width;
			break;
		case 725:
			value = m_ipc->ipc_OSD->ch5_aim_width;
			break;
		case 726:
			value = m_ipc->ipc_OSD->ch0_aim_height;
			break;
		case 727:
			value = m_ipc->ipc_OSD->ch1_aim_height;
			break;
		case 728:
			value = m_ipc->ipc_OSD->ch2_aim_height;
			break;
		case 729:
			value = m_ipc->ipc_OSD->ch3_aim_height;
			break;
		case 730:
			value = m_ipc->ipc_OSD->ch4_aim_height;
			break;
		case 731:
			value = m_ipc->ipc_OSD->ch5_aim_height;
			break;
		case 736:
			value = m_ipc->ipc_OSD->CROSS_draw_show[0];
			break;
		case 737:
			value = m_ipc->ipc_OSD->OSD_draw_color;
			break;
		case 738:
			value = m_ipc->ipc_OSD->CROSS_AXIS_WIDTH;
			break;
		case 739:
			value = m_ipc->ipc_OSD->CROSS_AXIS_HEIGHT;
			break;
		case 740:
			value = m_ipc->ipc_OSD->Picp_CROSS_AXIS_WIDTH;
			break;
		case 741:
			value = m_ipc->ipc_OSD->Picp_CROSS_AXIS_HEIGHT;
			break;

		case 752:
		case 1456:
		case 1488:
		case 1520:
		case 1552:
			value = pObj->params.Kx;

			break;

		case 753:
		case 1457:
		case 1489:
		case 1521:
		case 1553:
			value = pObj->params.Ky;

			break;

		case 754:
		case 1458:
	    case 1490:
		case 1522:
		case 1554:
			value = pObj->params.Error_X;
			break;

		case 755:
		case 1459:
		case 1491:
		case 1523:
		case 1555:
			value = pObj->params.Error_Y;
			break;

		case 756:
		case 1460:
		case 1492:
		case 1524:
		case 1556:
			value = pObj->params.Time;
			break;



		case 768:
		case 1568:
		case 1600:
		case 1632:
		case 1664:
			value = m_ptzSpeed.SpeedLevelPan[0];
			break;

		case 769:
		case 1569:
		case 1601:
		case 1633:
		case 1665:
			value = m_ptzSpeed.SpeedLevelPan[1];
			break;

		case 770:
		case 1570:
		case 1602:
		case 1634:
		case 1666:
			value = m_ptzSpeed.SpeedLevelPan[2];
			break;

		case 771:
		case 1571:
		case 1603:
		case 1635:
		case 1667:
			value = m_ptzSpeed.SpeedLevelPan[3];
			break;

		case 772:
		case 1572:
		case 1604:
		case 1636:
		case 1668:
			value = m_ptzSpeed.SpeedLevelPan[4];
			break;

		case 773:
		case 1573:
		case 1605:
		case 1637:
		case 1669:
			value = m_ptzSpeed.SpeedLevelPan[5];
			break;

		case 774:
		case 1574:
		case 1606:
		case 1638:
		case 1670:
			value = m_ptzSpeed.SpeedLevelPan[6];
			break;

		case 775:
		case 1575:
		case 1607:
		case 1639:
		case 1671:
			value = m_ptzSpeed.SpeedLevelPan[7];
			break;

		case 776:
		case 1576:
		case 1608:
		case 1640:
		case 1672:
			value = m_ptzSpeed.SpeedLevelPan[8];
			break;

		case 777:
		case 1577:
		case 1609:
	    case 1641:
		case 1673:
			value = m_ptzSpeed.SpeedLeveTilt[0];
			break;

		case 778:
		case 1578:
		case 1610:
		case 1642:
		case 1674:
			value = m_ptzSpeed.SpeedLeveTilt[1];
			break;

		case 779:
		case 1579:
		case 1611:
		case 1643:
		case 1675:
			value = m_ptzSpeed.SpeedLeveTilt[2];
			break;

		case 780:
		case 1580:
		case 1612:
		case 1644:
		case 1676:
			value = m_ptzSpeed.SpeedLeveTilt[3];
			break;

		case 781:
		case 1581:
		case 1613:
		case 1645:
		case 1677:
			value = m_ptzSpeed.SpeedLeveTilt[4];
			break;

		case 782:
		case 1582:
		case 1614:
	    case 1646:
		case 1678:
			value = m_ptzSpeed.SpeedLeveTilt[5];
			break;

		case 783:
		case 1583:
		case 1615:
		case 1647:
		case 1679:
			value = m_ptzSpeed.SpeedLeveTilt[6];
			break;

		case 784:
		case 1584:
		case 1616:
		case 1648:
		case 1680:
			value = m_ptzSpeed.SpeedLeveTilt[7];
			break;

		case 785:
		case 1585:
	    case 1617:
		case 1649:
		case 1681:
			value = m_ptzSpeed.SpeedLeveTilt[8];
			break;

		case 786:
		case 1586:
		case 1618:
		case 1650:
		case 1682:
			value = m_ptzSpeed.SpeedLevelPan[9];
			break;

		case 787:
	    case 1587:
		case 1619:
		case 1651:
		case 1683:
			value = m_ptzSpeed.SpeedLeveTilt[9];
			break;

		case 855:
			 value=m_GlobalDate->mtdconfig.trktime / 1000;
				break;
		case 856:
		  value=m_GlobalDate->mtdconfig.warning ;
				break;
		case 857:
			value=m_GlobalDate->mtdconfig.high_low_level ;
				break;

		case 864:
				//detect area
				value=	m_GlobalDate->Mtd_Moitor_X ;
				break;

		case 865:
			value=m_GlobalDate->Mtd_Moitor_Y;
				break;


		default:
			break;
	}

	if(m_GlobalDate->choose){
		if(m_GlobalDate->commode==1){
			memset(m_GlobalDate->ACK_read_content,0,128);
			rStr.copy(m_GlobalDate->ACK_read_content,rStr.length(),0);
		}else if(m_GlobalDate->commode==2){
			memset(m_GlobalDate->ACK_read_content,0,128);
			rStr.copy(m_GlobalDate->ACK_read_content,rStr.length(),0);
		//	printf("%s :%d  read the CONTEXT value = %f \n",__func__,__LINE__,value);
		}
	}else{
		if(m_GlobalDate->commode==1)
			m_GlobalDate->ACK_read.push_back(value);
		else if(m_GlobalDate->commode==2){
			m_GlobalDate->ACK_read.push_back(value);
		}
//		printf("%s :%d  read the param value = %f \n",__func__,__LINE__,value);

	}
	return 0;
}

void CManager::signalFeedBack(int signal, int index, int value, int s_value)
{
	//printf("_________________m_GlobalDate->commode  = %d\n",   m_GlobalDate->commode);
	if(m_GlobalDate->commode ==1)
	{
		m_GlobalDate->feedback = signal;
		m_GlobalDate->mainProStat[index] = value;
		m_GlobalDate->mainProStat[index +1] = s_value;
		OSA_semSignal(&m_GlobalDate->m_semHndl);
	}
	else if(m_GlobalDate->commode == 2)
	{
		m_GlobalDate->feedback = signal;
		m_GlobalDate->mainProStat[index] = value;
		m_GlobalDate->mainProStat[index +1] = s_value;
		OSA_semSignal(&m_GlobalDate->m_semHndl_socket);
	}

}

int CManager::updataProfile()
{
	printf("------->0x34----save------\n");
	string cfgAvtFile;
	int configId_Max = profileNum;
	char  cfg_avt[30] = "cfg_avt_";
	cfgAvtFile = "Profile.yml";

	OSD_text* wOSD;
	wOSD = &m_OSD;

	FILE *fp = fopen(cfgAvtFile.c_str(), "rt+");

	if(fp != NULL){
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len < 10)
			return  -1;
		else{
			FileStorage fr(cfgAvtFile, FileStorage::WRITE);
			if(fr.isOpened()){
				int n =0;
				for(int i=0; i<configId_Max; i++){
									sprintf(cfg_avt, "cfg_avt_%d", i);

									//printf("cfg_value[%d]=%f\n",i,cfg_value[i]);
									n = i / 16;
									if(n >= 7-1 && n <= 22-1){
										if(i - n*16 == 3)
											fr <<cfg_avt << wOSD->OSD_text_content[n -6];
										else
											fr << cfg_avt << cfg_value[i];
									}
									else if(n >= 29 -1 && n <= 44 - 1){
										if(i -n*16 == 3)
											fr << cfg_avt << wOSD->OSD_text_content[n - 28 + 16];
										else
											fr << cfg_avt << cfg_value[i];
									}
									else if(i == 816)
									{
										fr << cfg_avt <<dev[0];
									}
									else if(i == 819)
									{
										fr << cfg_avt <<parity[0];
									}
									else if(i == 1696)
									{
										fr << cfg_avt <<dev[1];
									}
									else if(i == 1699)
									{
									fr << cfg_avt <<parity[1];
									}
									else if(i == 1712)
									{
									fr << cfg_avt <<dev[2];
								     }
									else if(i == 1715)
									{
								  fr << cfg_avt <<parity[2];
									}
									else if(i == 1727)
									{
									fr << cfg_avt <<dev[3];
									}
									else if(i == 1730)
								    {
									fr << cfg_avt <<parity[3];
								     }
									else if(i == 1744)
									{
									fr << cfg_avt <<dev[4];
									}
									else if(i == 1747)
									 {
									fr << cfg_avt <<parity[4];
									 }
									else
									{
										fr<< cfg_avt << cfg_value[i];
									//printf("updata profile_back[%d] --- %f\n",i, fr[cfg_avt]);
									}
				}

			}else
				return -1;
		}
	}else
		fclose(fp);
}

void CManager::MSGAPI_ExtInputCtrl_ZoomSpeed()
{
	if(m_GlobalDate->m_CurrStat.m_zoomSpeedBak != m_GlobalDate->m_CurrStat.m_zoomSpeed)
	{
		m_ptz->m_bChangeZoomSpeed = true;
		m_ptz->m_isetZoom = m_GlobalDate->m_CurrStat.m_zoomSpeed;
		m_ptz->setZoomSpeed();
	}
	m_GlobalDate->m_CurrStat.m_zoomSpeedBak = m_GlobalDate->m_CurrStat.m_zoomSpeed;
	printf("m_ptz->m_isetZoom = %d\n", m_ptz->m_isetZoom);
}

void CManager::MSGAPI_ExtInputCtrl_ZoomLong()
{
	if(m_GlobalDate->m_CurrStat.m_ZoomLongStat)
		m_ptz->m_iSetZoomSpeed = -1;
	else
		m_ptz->m_iSetZoomSpeed = 0;
	if(m_GlobalDate->jos_params.ctrlMode == jos)
	{
		struct timeval tmp;
		tmp.tv_sec = 0;
		tmp.tv_usec = 60000;
		select(0, NULL, NULL, NULL, &tmp);
		refreshPtzParam();
	}
}

void CManager::MSGAPI_ExtInputCtrl_ZoomShort()
{
	if(m_GlobalDate->m_CurrStat.m_ZoomShortStat)
		m_ptz->m_iSetZoomSpeed = 1;
	else
		m_ptz->m_iSetZoomSpeed = 0;
	if(m_GlobalDate->jos_params.ctrlMode == jos)
	{
		struct timeval tmp;
		tmp.tv_sec = 0;
		tmp.tv_usec = 60000;
		select(0, NULL, NULL, NULL, &tmp);
		refreshPtzParam();
	}
}

void CManager::MSGAPI_ExtInputCtrl_IrisDown()
{

	if(m_GlobalDate->m_CurrStat.m_IrisDownStat)
		m_ptz->m_iSetIrisSpeed = -1;
	else
		m_ptz->m_iSetIrisSpeed = 0;
}

void CManager::MSGAPI_ExtInputCtrl_IrisUp()
{

	if(m_GlobalDate->m_CurrStat.m_IrisUpStat)
		m_ptz->m_iSetIrisSpeed = 1;
	else
		m_ptz->m_iSetIrisSpeed = 0;
}

void CManager::MSGAPI_ExtInputCtrlFocusNear()
{
	if(m_GlobalDate->m_CurrStat.m_FoucusNearStat)
		m_ptz->m_iSetFocusNearSpeed = 1;
	else
		m_ptz->m_iSetFocusNearSpeed = 0;
}

void CManager::MSGAPI_ExtInputCtrlFocusFar()
{
	if(m_GlobalDate->m_CurrStat.m_FoucusFarStat)
		m_ptz->m_iSetFocusFarSpeed = -1;
	else
		m_ptz->m_iSetFocusFarSpeed = 0;
}

void CManager::MSGAPI_ExtInputCtrl_Iris(int stat)
{
	if(stat < 0)
		m_ptz->m_iSetIrisSpeed = -1;
	else if(stat  > 0)
		m_ptz->m_iSetIrisSpeed = 1;
	else
		m_ptz->m_iSetIrisSpeed = 0;
}

void CManager::MSGAPI_ExtInputCtrl_Focus(int stat)
{
	if(stat < 0)
		m_ptz->m_iSetFocusFarSpeed = -1;
	else if(stat > 0)
		m_ptz->m_iSetFocusNearSpeed = 1;
	else{
		m_ptz->m_iSetFocusFarSpeed = 0;
		m_ptz->m_iSetFocusNearSpeed = 0;
	}
}

void CManager::AutoExit_SpeedReset(int TrkStatus)
{

	m_pltOutput.fPlatformDemandX = 0.0;
	m_pltOutput.fPlatformDemandY = 0.0;
	m_ipc->trackposx = 0.0;
	m_ipc->trackposy = 0.0;
	m_pltInput.fTargetBoresightErrorX = 0.0;
	m_pltInput.fTargetBoresightErrorY = 0.0;
	memset(&m_pltInput, 0, sizeof(m_pltInput));
	m_pltInput.iTrkAlgState= TrkStatus;
	m_Platform->PlatformCtrl_TrackerInput(m_plt, &m_pltInput);
	m_Platform->PlatformCtrl_TrackerOutput(m_plt, &m_pltOutput);
	m_ptz->m_iSetPanSpeed = 0;
	m_ptz->m_iSetTiltSpeed = 0;
}

void CManager::init_sigaction()
{
	struct sigaction tact;

	tact.sa_handler = detectionFunc;
	tact.sa_flags = 0;
	sigemptyset (&tact.sa_mask);
	sigaction (SIGALRM, &tact, NULL);
}

void CManager::init_time()
{
	struct itimerval value;

	value.it_value.tv_sec = 6;
	value.it_value.tv_usec = 0;
	value.it_interval = value.it_value;
	/* set ITIMER_REAL */
	setitimer (ITIMER_REAL, &value, NULL);
}

void CManager::uninit_time()
{
	struct itimerval value;

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;
	value.it_interval = value.it_value;
	/* set ITIMER_REAL */
	setitimer (ITIMER_REAL, &value, NULL);
}

void detectionFunc (int sign)
{
	sThis->uninit_time();
	sThis->m_GlobalDate->m_osd_triangel.alpha = 0;
	sThis->m_GlobalDate->EXT_Ctrl[MSGID_INPUT_IrisAndFocusAndExit] = 0;
	sThis->m_ipc->IpcIrisAndFocus(&sThis->m_GlobalDate->m_osd_triangel, 0);
}

void CManager::AVTSpeedReset(int TrkStatus, int limit)
{
	if(limit == 0)
		memset(&m_pltInput, 0, sizeof(m_pltInput));
	m_pltInput.iTrkAlgState= !TrkStatus;
	m_Platform->PlatformCtrl_TrackerInput(m_plt, &m_pltInput);
	m_Platform->PlatformCtrl_TrackerOutput(m_plt, &m_pltOutput);
	 if(!TrkStatus == 0){
		 m_ptz->m_iSetPanSpeed = 0;
		 m_ptz->m_iSetTiltSpeed = 0;
		 m_pltOutput.fPlatformDemandX = 0.0;
		 m_pltOutput.fPlatformDemandY = 0.0;
		 m_ipc->trackposx = 0.0;
		 m_ipc->trackposy = 0.0;
		 m_pltInput.fTargetBoresightErrorX = 0.0;
		 m_pltInput.fTargetBoresightErrorY = 0.0;
	 }
}

void CManager::SwitchWarning(unsigned int nPin, unsigned int nValue)
{
	GPIO_set(nPin, nValue);
}

void CManager::curOutputParams()
{
	PLATFORMCTRL_Interface* pcfg;
	pcfg = m_plt;
	PlatformCtrl_Obj *pObj =  (PlatformCtrl_Obj*)pcfg->object;
	struct timeval tmp;
	m_GlobalDate->errorOutPut[0] = (int)m_ipc->trackposx;
	m_GlobalDate->errorOutPut[1] = (int)m_ipc->trackposy;
	signalFeedBack(ACK_avtErrorOutput, 0, 0, 0);
	tmp.tv_sec = 0;
	tmp.tv_usec = 5000;
	select(0, NULL, NULL, NULL, &tmp);

	m_GlobalDate->speedComp[0] = (int)pObj->privates.curRateDemandX;
	m_GlobalDate->speedComp[1] = (int)pObj->privates.curRateDemandY;
	signalFeedBack(ACK_avtSpeedComp, 0, 0, 0);
	tmp.tv_sec = 0;
	tmp.tv_usec = 5000;
	select(0, NULL, NULL, NULL, &tmp);

	m_GlobalDate->speedLv[0] = m_ptz->m_iSetPanSpeed;
	m_GlobalDate->speedLv[1] = m_ptz->m_iSetTiltSpeed;
	signalFeedBack(ACK_avtSpeedLv, 0, 0, 0);
	tmp.tv_sec = 0;
	tmp.tv_usec = 5000;
	select(0, NULL, NULL, NULL, &tmp);
}

