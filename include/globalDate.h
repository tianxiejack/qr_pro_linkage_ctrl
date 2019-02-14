/*
 * globalDate.h
 *
 *  Created on: 2018年9月26日
 *      Author: jet
 */

#ifndef GLOBALDATE_H_
#define GLOBALDATE_H_

#include <iostream>
#include <vector>
#include <time.h>
#include "app_status.h"
#include "CMessage.hpp"
#include "osa_sem.h"
#include "Ipcctl.h"

using namespace std;

const char MAXSENDSIZE = 136;

typedef struct
{
	bool validCh[5];
	char idx;
}selectCh_t;

typedef struct {
    int byteSizeSend;
    unsigned char sendBuff[MAXSENDSIZE];
}sendInfo;

typedef  struct  {
	u_int8_t   cmdBlock;
	u_int8_t   cmdFiled;
	float    confitData;
}systemSetting;

typedef struct{
     unsigned char SecAcqStat;// eSecTrkMode
     int ImgPixelX; //SecTrk X
     int ImgPixelY; //SecTrk  Y
}selectTrack;

typedef struct{
	volatile unsigned char AvtMoveX;// eTrkRefine (axis or aim) x
	volatile unsigned char AvtMoveY;// eTrkRefine (axis or aim) y
}POSMOVE;

typedef struct{
	volatile unsigned int BoresightPos_x;
	volatile unsigned int BoresightPos_y;
}iBoresightPos;

typedef struct{
	volatile unsigned char AcqStat;
	volatile unsigned int BoresightPos_x;
	volatile unsigned int BoresightPos_y;
}AcqBoxPos_ipc;

typedef struct{
	volatile int AcqBoxW[eSen_Max];
	volatile int AcqBoxH[eSen_Max];
}AcqBoxSize;

typedef struct{
	volatile int dir;
	volatile int alpha;
}osd_triangle;


typedef struct {
		int  m_TrkStat;
		int  m_SenSwStat;
		int m_ZoomLongStat;
		int m_ZoomShortStat;
		int m_zoomSpeed;
		int m_zoomSpeedBak;
		int m_TrkBoxSizeStat;
		int m_SecTrkStat;
		int m_AcqBoxStat;
		int m_AcqStat_ipc;
		int m_IrisUpStat;
		int m_IrisDownStat;
		int m_FoucusFarStat;
		int m_FoucusNearStat;
		int m_PresetStat;
		int m_ImgEnhanceStat;
		int m_MmtStat;
		int m_MmtSelectNum;
		volatile unsigned char m_AimPosXStat;
		volatile unsigned char m_AimPosYStat;
		int m_AxisXStat;
		int m_AxisYStat;
}CurrParaStat,*PCurrParaStat;

typedef struct{
	int block;
	int filed;
}Read_config_buffer;

typedef enum {
	Exit = 0,
	iris,
	Focus
}IrisAndFocusAndExit;

typedef struct{
	uint channel0:1;
	uint channel1:1;
	uint channel2:1;
	uint channel3:1;
	uint channel4:1;
	uint channel5:1;
	uint reserve5:2;
}channelTable;

typedef enum{
	DefaultOff = 0,
	PosLoop ,
	SpeedLoop
}MtdMode;

typedef enum{
	Default = 0,
	Next,
	Prev,
	Selext
}MtdSelect;

typedef enum{
	Auto = 0,
	Manual
}MtdExitMode;

enum{recv_AutoMtdDate = 0, shield_AutoMtdDate};

typedef struct{
	volatile int mptzx;
	volatile int mptzy;
}MOUSEPTZ;

typedef struct{
	volatile unsigned int preset;
	volatile unsigned int warning;
	volatile unsigned int high_low_level;
	volatile unsigned int trktime;  /*  ms  */
}MtdConfig;

typedef struct{
	unsigned int panPos;
	unsigned int tilPos;
	unsigned int zoom;
}LinkageParam;

typedef enum{
	platFormCapture = 0,
	boxCapture,
	ManualMtdCapture
}Capture_Mode;

typedef struct _uart_params
{
    char   device[64];
    char   name[64];
    Int32  baudrate;
    Int32  databits;
    char   parity[2];
    Int32  stopbits;
    Int32 balladdress;
}G_uart_open_params;

typedef struct camera_Pos
{
	unsigned short setpan;
	unsigned short settilt;
	unsigned short setzoom;
}camera_PosParams;

class CGlobalDate
{
public:
	~CGlobalDate(){};

static CGlobalDate* Instance();
static const char chid = 5;
char avtStatus;
short errorOutPut[2];
short speedComp[2];
signed char speedLv[2];
char  ACK_read_content[128];
vector <float> ACK_read;
vector <int> EXT_Ctrl;
vector <float> Host_Ctrl;
vector<unsigned char>  rcvBufQue;
static vector<Read_config_buffer>  readConfigBuffer;
static vector<int>  defConfigBuffer;
static selectCh_t selectch;

static osdbuffer_t recvOsdbuf;
sendInfo repSendBuffer;
CurrParaStat pParam;
selectTrack m_selectPara;
AcqBoxPos_ipc m_acqBox_Pos;
POSMOVE m_avtMove;
CurrParaStat  m_CurrStat;
systemSetting avtSetting;
osd_triangle m_osd_triangel;
IMGSTATUS  avt_status;
MOUSEPTZ ipc_mouseptz;
MtdConfig mtdconfig;
LinkageParam linkagePos;
ctrlParams jos_params;
G_uart_open_params m_uart_params[5];
camera_PosParams m_camera_pos[5];

OSA_SemHndl  m_semHndl;
OSA_SemHndl  m_semHndl_s;
OSA_SemHndl m_semHndl_socket;
OSA_SemHndl m_semHndl_socket_s;
OSA_SemHndl m_semHndl_automtd;
int joystick_flag;
int commode;
int feedback;
int mainProStat[ACK_value_max];
int choose;
int IrisAndFocus_Ret;
int respupgradefw_stat;
int respupgradefw_perc;
int respimpconfig_stat;
int respimpconfig_perc;
int respexpconfig_stat;
int respexpconfig_len;
unsigned char respexpconfig_buf[1024+256];
struct timeval PID_t;
int time_start;
long int time_stop;
 int frame;
 int target;
char ImgMtdStat;
char mtdMode;
char Mtd_Moitor; /*Recording detection area; 1: record; 0: don't record*/
char Mtd_ExitMode;
char Mtd_ipc_target; /*receive ipc detect message; 1: find target; 0: no target*/
char Mtd_Limit;  /* Block IPC messages if you enter the trace */
char Mtd_Manual_Limit;  /*检测到目标后才能响应进跟踪指令*/
unsigned short Mtd_Moitor_X;   	/*Detect regional center coordinates*/
unsigned short Mtd_Moitor_Y;
bool MtdAutoLoop;
int ThreeMode_bak;
int Sync_Query;
int sync_fovComp;
int sync_pos;
unsigned short rcv_zoomValue;
int jos_ctrl;
int host_ctrl;
int workMode;
int outputMode;
int chid_camera;
unsigned char MtdSetAreaBox;
unsigned char MtdWait;
unsigned char ScenceTrkWait;

unsigned char cameraSwitch[chid];
unsigned char cameraResolution[chid];
unsigned char AutoBox[chid];
unsigned char osdUserSwitch;
unsigned char fovmode[chid];
unsigned char switchFovLV[chid];
unsigned char continueFovLv[chid];

private:
CGlobalDate();
static CGlobalDate* _Instance;

};


#endif /* GLOBALDATE_H_ */
