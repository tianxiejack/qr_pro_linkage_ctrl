/*
 * manager.h
 *
 *  Created on: 2018年9月29日
 *      Author: jet
 */

#ifndef MANAGER_H_
#define MANAGER_H_

#include <stdio.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <vector>
#include <time.h>
#include <pthread.h>
#include "osa_sem.h"
#include "CMessage.hpp"
#include "globalDate.h"
#include "stateCtrl.h"
#include "PTZ_control.h"
#include "platformControl.h"
#include "ipcProc.h"
#include "joyStick.h"
#include "CPortInterface.hpp"
#include "PortFactory.hpp"
#include "CPortBase.hpp"
#include "DxTimer.hpp"
#include "gpio_rdwr.h"

using namespace cv;
using namespace std;

class CManager{
public:

	CManager();
	~CManager();

	CIPCProc* m_ipc;
	static CGlobalDate* m_GlobalDate;
	void signalFeedBack(int signal, int index, int value, int s_value);
	void uninit_time();
	void AVTSpeedReset(int TrkStatus, int limit);

private:
	static CManager* pThis;
	static CStatusManager* m_StateManager;
	CPlatformInterface* m_Platform;
	CPTZControl* m_ptz;
	CMessage* m_Message;
	MSGDRIV_Handle _Handle;
	CJosInterface* m_Jos;

private:
	//float* cfg_value;
	vector <float> cfg_value;
	vector <float> DetectArea_value;
	HPLTCTRL  m_plt;
	PLATFORMCTRL_TrackerInput m_pltInput;
	PLATFORMCTRL_Output m_pltOutput;
   	PlatformCtrl_CreateParams m_cfgPlatParam;
   	configPlatParam_InitParams  m_InitParam;

   	Uart uart;
   	CPTZSpeedTransfer  m_ptzSpeed;
	CMD_IPCRESOLUTION resolu;
	OSD_text m_OSD;
	DxTimer dtimer;
	int swtarget_id;

private:
	void creat();
	void Message_Creat();
	void Sem_Creat();
	void Sem_Init();
	void MSGAPI_Init();
	void Message_destroy();
	void classObject_Creat();
	void IPC_Creat();
	void Platform_Creat();
	void Comm_Creat();
	void GPIO_Create(unsigned int num, unsigned int mode);
	void sensorChIdCreate();
	void DxTimer_Create();
	static void Tcallback_Linkage(void *p);
	void destroy();
	void Sem_destroy();
	int GetIpcAddress(int type);
	int configAvtFromFile();
	int readMtdDetectArea();
	int setMtdDetectArea();
	void Enableresolution(CMD_IPCRESOLUTION resolu);
	void Ipc_resolution();
	void modifierAVTProfile(int block, int field, float value,char *inBuf);
	int updataProfile();
	int paramBackToDefault(int blockId);
	void MSGAPI_ExtInputCtrl_ZoomSpeed();
	int answerRead(int block, int field);
	void init_sigaction();
	void init_time();


private:
	static void MSGAPI_ExtInpuCtrl_AXIS(long p);
	static void MSGAPI_ExtInpuCtrl_AxisCtrl(long p);
	static void MSGAPI_ExtInpuCtrl_TRACKCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_IrisAndFocusAndExit(long p);
	static void MSGAPI_ExtInpuCtrl_TRACKSEARCHCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_SwitchSensor(long p);
	static void MSGAPI_ExtInpuCtrl_PRMBACK(long p);
	static void MSGAPI_ExtInpuCtrl_zoomSpeed(long p);
	static void MSGAPI_ExtInpuCtrl_ZOOMSHORTCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_ZOOMLONGCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_IRISDOWNCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_IRISUPCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_FOCUSNEARCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_FOCUSFARCHCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_configWrite(long p);
	static void MSGAPI_ExtInpuCtrl_config_Read(long p);
	static void MSGAPI_ExtInpuCtrl_configWrite_Save(long p);
	static void MSGAPI_ExtInpuCtrl_MTDCTRL(long p);
	static void MSGAPI_ExtInpuCtrl_MTDMode(long p);
	static void MSGAPI_EXT_INPUT_MtdPreset(long p);
	static void MSGAPI_EXT_INPUT_CallPreset(long p);
	static void MSGAPI_EXT_INPUT_setPan(long p);
	static void MSGAPI_EXT_INPUT_setTilt(long p);
	static void MSGAPI_EXT_INPUT_setZoom(long p);
	static void MSGAPI_EXT_INPUT_queryPan(long p);
	static void MSGAPI_EXT_INPUT_queryTilt(long p);
	static void MSGAPI_EXT_INPUT_queryZoom(long p);
	static void MSGAPI_EXT_INPUT_setPreset(long p);
	static void MSGAPI_EXT_INPUT_goToPreset(long p);
	static void MSGAPI_ExtInpuCtrl_WORKMODEWITCH(long p);

	static void test_left(long p);
	static void test_right(long p);
	static void test_stop(long p);

	static void MSGAPI_IPC_INPUT_TRACKCTRL(long p);
	static void MSGAPI_IPC_ChooseVideoChannel(long p);
	static void MSGAPI_IPC_Resolution(long p);
	static void MSGAPI_IPC_AxisMove(long p);
	static void MSGAPI_IPC_Algosdrect(long p);
	static void MSGAPI_IPC_word(long p);
	static void MSGAPI_IPC_WORDFONT(long p);
	static void MSGAPI_IPC_WORDSIZE(long p);
	static void MSGAPI_IPC_AutoMtd(long p);
	static void MSGAPI_IPC_Mtdoutput(long p);
	static void MSGAPI_IPC_Mtdpolar(long p);
	static void MSGAPI_IPC_ballparam(long p);
	static void MSGAPI_IPC_UserOSDSWITCH(long p);
	static void MSGAPI_IPC_INPUT_reset_swtarget_timer(long p);

	static void MSGAPI_IPC_QueryPos(long p);
	static void MSGAPI_IPC_INPUT_PosAndZoom(long p);
	static void MSGAPI_IPC_INPUT_VideoNameAndPos(long p);
	static void MSGAPI_IPC_INPUT_CTRLPARAMS(long p);
	static void MSGAPI_IPC_INPUT_defaultWorkMode(long p);
	static void MSGAPI_IPC_INPUT_MtdParams(long p);
	static void MSGAPI_IPC_INPUT_fontSize(long p);

	void usd_API_AXIS();
	void usd_API_Track();
	void usd_API_StatusSwitch();
	void usd_API_IrisAndFocusAndExit();
	void usd_API_TrkSearch();
	void usd_API_ChooseVideoChannel();
	void usd_API_SwitchSensor();
	void usd_API_Resolution();
	void usd_API_AIMPOSXCTRL();
	void usd_API_AIMPOSYCTRL();
	void usd_API_PRMBACK();
	void usd_API_AxisMove();
	void usd_API_Algosdrect();
	void usd_API_word();
	void usd_API_WORDFONT();
	void usd_API_WORDSIZE();
	void usd_API_speedchid();
	void usd_API_MTDMode();
	void usd_API_queryPos();
	void usd_API_switchCameraFovAndLV(char chid);
	void usd_API_fovCompensation();
	void usd_API_IPC_TRACKCTRL();
	void usd_API_WORKMODEWITCH();
	void usd_API_ctrlParams();

	void reset_status_AutoMtd();
	void IPC_videoName_Pos();
	void IPC_fontSize();
	void IPC_ballbaud();
	void preset_Mtd();
	void switch_speedchid();
	void switch_PIDchid();
	void switch_CameraChid();
	void Observer_Chid();
	void Observer_Action(int block, int field, float value);
	void refreshPtzParam();



	void ExtInputCtrl_AcqBoxPos();
	void ExtInputCtrl_AXIS();
	void ExtInputCtrl_SendSeaTrkPos(int sensor);
	void MSGAPI_ExtInputCtrl_ZoomLong();
	void MSGAPI_ExtInputCtrl_ZoomShort();
	void MSGAPI_ExtInputCtrl_IrisDown();
	void MSGAPI_ExtInputCtrl_IrisUp();
	void MSGAPI_ExtInputCtrlFocusNear();
	void MSGAPI_ExtInputCtrlFocusFar();
	void MSGAPI_ExtInputCtrl_Iris(int stat);
	void MSGAPI_ExtInputCtrl_Focus(int stat);

	void AutoExit_SpeedReset(int TrkStatus);
	void SwitchWarning(unsigned int nPin, unsigned int nValue);
	void curOutputParams();

	void thrCreate_LinkageSpeedLoop();
	void thrDelete_LinkageSpeedLoop();

	bool exitSpeedLoop_Linkage;
	OSA_ThrHndl Linkage_SpeedLoop;
	int AutoTimer = true;
	static void* linkage_SpeedLoop(void* prm)
	{
		CManager* sThis = (CManager*) prm;

		printf("  [Mtd] speedLoop is start \n");
		for(;sThis->exitSpeedLoop_Linkage == false;)
		{
			OSA_semWait(&m_GlobalDate->m_semHndl_automtd, OSA_TIMEOUT_FOREVER);
			if(sThis->AutoTimer)
			{
				sThis->dtimer.startTimer(sThis->swtarget_id, m_GlobalDate->mtdconfig.trktime);
				sThis->AutoTimer = false;
				printf("[%s] %d recv speedLoop, open Timer [Time = %d ]\n", __func__, __LINE__, m_GlobalDate->mtdconfig.trktime);
			}
			if(m_GlobalDate->MtdAutoLoop)
			{
				int pan = m_GlobalDate->linkagePos.panPos;
				int Tilt = m_GlobalDate->linkagePos.tilPos;
				int zoom = m_GlobalDate->linkagePos.zoom;
				sThis->m_ptz->ptzSetSpeed(pan, Tilt);
				sThis->m_ptz->setZoomPos(zoom);
			}
		}

	}


};
void detectionFunc (int sign);


#endif /* MANAGER_H_ */
