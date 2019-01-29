#ifndef _IPCPROC_H_
#define _IPCPROC_H_

#include "osa_thr.h"
#include "Ipcctl.h"
#include   "globalDate.h"
#include <string>
#define n_size 32
#pragma once
using namespace std;

typedef struct{
int OSD_text_show[n_size];
int OSD_text_color[n_size];
int OSD_text_alpha[n_size];
int OSD_text_positionX[n_size];
int OSD_text_positionY[n_size];
string OSD_text_content[n_size];
}OSD_text;

class  CIPCProc{

public:
		    CIPCProc();
            ~CIPCProc();
            int Create();
            int Destroy();
			
			int IpcWordFont(char font);
			int IpcWordSize(char size);
		    int  ipcTrackCtrl(volatile unsigned char AvtTrkStat);
		    int ipcSceneTrkCtrl(volatile unsigned char senceTrkStat);
		    int  ipcMutilTargetDetecCtrl(volatile unsigned char ImgMmtStat);//1:open 0:close
		    int ipcMutilTargetSelectCtrl(volatile unsigned char ImgMmtSelect);
		    int IpcMmtLockCtrl(int mmt_Select);
		    int  ipcImageEnhanceCtrl(volatile unsigned char ImgEnhStat); //1open 0close
		    int ipcMoveTatgetDetecCtrl(volatile unsigned char ImgMtdStat);
		    int ipcSecTrkCtrl(selectTrack *m_selcTrak);
		    int ipcAcqBoxCtrl(AcqBoxPos_ipc* m_acqPos);
		    int IpcSensorSwitch(volatile unsigned char ImgSenchannel);
		    int IpcpinpCtrl(volatile unsigned char ImgPipStat);
		    int IpcAcqDoorCtrl(AcqBoxSize *BoxSize);
		    int IpcIrisAndFocus(osd_triangle* osd_tri, char sign);
		    int IpcFuncMenu(char sign);
		    int IPCBoresightPosCtrl(unsigned int x,unsigned int y);
		    int IpcElectronicZoom(int zoom);
		    int IpcConfig();
		    int IpcConfigOSD();
		    int IpcConfigOSDTEXT();
		    int IpcConfigUTC();
		    int IPCConfigCamera();
		    int IPCMtdSwitch(volatile unsigned char ImgMtdStat, volatile unsigned char mtdMode);
		    int IPCMtdSelectCtrl(volatile unsigned char MtdSelect);
		    int IpcSwitchTarget();
		    int IPCChannel_binding(int channel);
		    int IPCAxisMove(int x, int y);
		    int IPCpicp(int status, int pipChannel);
		    int IPCAlgosdrect(int tmp);
		    int IPCchooseVideoChannel(int channel);
		    int IPCframeCtrl(int fps, int channel);
		    int IPCcompression_quality(int quality, int channel);
		    int IPCLKOSD();
		    int IPCResolution(CMD_IPCRESOLUTION resolu);
		    int IPCOsdWord(volatile osdbuffer_t &osd);
			int IpcSendLosttime();
			int IPCSendMtdFrame();
			int IPCSendPos(int pan, int tilt, int zoom);
			int refreshPtzParam(int pan, int tilt, int zoom);
			int IPCSendBallParam();


		    IMGSTATUS  *getAvtStatSharedMem();
		    OSDSTATUS *getOSDSharedMem();
		    UTCTRKSTATUS *getUTCSharedMem();
		    LKOSDSTATUS* getLKOSDShareMem();
		    osdtext_t *getOSDTEXTSharedMem();
			  	unsigned int trackstatus;
			  	float trackposx;
			  	float trackposy;
				int losttime;
			  	CMD_TRK fr_img_cmd_trk;
			  	CMD_SECTRK fr_img_cmd_sectrk;
			  	CMD_ENH fr_img_cmd_enh;
			  	CMD_MTD fr_img_cmd_mtd;
			  	CMD_MMT fr_img_cmd_mmt;
			  	CMD_MMTSELECT fr_img_cmd_mmtsel;
			  	CMD_TRKDOOR fr_img_cmd_trkdoor;
			  	CMD_MOUSEPTZ cmd_mouseptz;
			  	CMD_Mtd_Frame Mtd_Frame;
			  	CMD_Mtd_Frame* pMtd;


			    IMGSTATUS *ipc_status;
			    OSDSTATUS *ipc_OSD;
			    UTCTRKSTATUS *ipc_UTC;
			    LKOSDSTATUS 	*ipc_LKOSD;
			    osdtext_t *ipc_OSDTEXT;

#if 0
		      static Void  *IPCShamThrdFxn(Void * prm){
		    	  CIPCProc *pThis = (CIPCProc*)prm;
		      	   pThis->getShamdataInThrd();
		      };
#endif
		      static Void   *IPC_childdataRcvn(Void * prm){
		    	  CIPCProc *pThis = (CIPCProc*)prm;
		           pThis->getIPCMsgProc();
		          };
private:
		      static CIPCProc* pThis;
		      	CMessage* _Message;
		      	static CGlobalDate* _GlobalDate;
			  	bool exitThreadIPCRcv;
			  	bool exitGetShamDatainThrd;
			     OSA_ThrHndl thrHandlPCRcv;
			     OSA_ThrHndl thrHandlPCSham;
			  	pthread_mutex_t mutex;
				SENDST test;

			  	SENDST fr_img_test;
			  	void getIPCMsgProc();
			  	void getShamdataInThrd();
				void ipc_Config_Write_Save(CMD_SETCONFIG config);
};
#endif
