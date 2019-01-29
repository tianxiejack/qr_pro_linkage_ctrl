#ifndef _SENSORCOMPENSATE_H
#define _SENSORCOMPENSATE_H

#include <string.h>
#include "globalDate.h"

#define SENSORCOMP_BORESIGHTPOS_TAB_DEF_SIZE		(2)

#define SENSORCOMP_BORESIGHTPOS_TAB_MAX_SIZE		(32)

typedef void * HSENSORCOMP;

typedef enum
{
	error = -1,
	SENSORCOMP_MODE_RUN = 0,
	SENSORCOMP_MODE_CORR_NORMAL,
	SENSORCOMP_MODE_CORR_CLEAN,
} SENSORCOMP_MODE;

typedef struct
{
	float fFov;
	int x;
	int y;
}SensorComp_BoresightPos;

typedef struct
{
	int iIndex;

	int nWidth;

	int nHeight;

	int defaultBoresightPos_x;

	int defaultBoresightPos_y;

	float fFovMin;

	float fFovMax;

	float fFovRatio;

	SensorComp_BoresightPos *initTab;

	int nInitTabSize;

	SENSORCOMP_MODE initMode;

}SensorComp_CreateParams;
const char chid_camera = 5;
typedef struct{

	float radio[chid_camera];
    /*固定视场*/
	int Resolution_level[chid_camera];
	int Resolution_vertical[chid_camera];
	float level_Fov_fix[chid_camera];
	float vertical_Fov_fix[chid_camera];
	float Boresightpos_fix_x[chid_camera];
	float Boresightpos_fix_y[chid_camera];
/*可切换视场*/
	float level_Fov_switch1[chid_camera];
	float vertical_Fov_switch1[chid_camera];
	float Boresightpos_switch_x1[chid_camera];
	float Boresightpos_switch_y1[chid_camera];

	float level_Fov_switch2[chid_camera];
	float vertical_Fov_switch2[chid_camera];
	float Boresightpos_switch_x2[chid_camera];
	float Boresightpos_switch_y2[chid_camera];

	float level_Fov_switch3[chid_camera];
	float vertical_Fov_switch3[chid_camera];
	float Boresightpos_switch_x3[chid_camera];
	float Boresightpos_switch_y3[chid_camera];

	float level_Fov_switch4[chid_camera];
	float vertical_Fov_switch4[chid_camera];
	float Boresightpos_switch_x4[chid_camera];
	float Boresightpos_switch_y4[chid_camera];

	float level_Fov_switch5[chid_camera];
	float vertical_Fov_switch5[chid_camera];
	float Boresightpos_switch_x5[chid_camera];
	float Boresightpos_switch_y5[chid_camera];
/*连续视场*/
	float level_Fov_continue1[chid_camera];
	float vertical_Fov_continue1[chid_camera];
	float Boresightpos_continue_x1[chid_camera];
	float Boresightpos_continue_y1[chid_camera];
	float zoombak1[chid_camera];

	float level_Fov_continue2[chid_camera];
	float vertical_Fov_continue2[chid_camera];
	float Boresightpos_continue_x2[chid_camera];
	float Boresightpos_continue_y2[chid_camera];
	float zoombak2[chid_camera];

	float level_Fov_continue3[chid_camera];
	float vertical_Fov_continue3[chid_camera];
	float Boresightpos_continue_x3[chid_camera];
	float Boresightpos_continue_y3[chid_camera];
	float zoombak3[chid_camera];

	float level_Fov_continue4[chid_camera];
	float vertical_Fov_continue4[chid_camera];
	float Boresightpos_continue_x4[chid_camera];
	float Boresightpos_continue_y4[chid_camera];
	float zoombak4[chid_camera];

	float level_Fov_continue5[chid_camera];
	float vertical_Fov_continue5[chid_camera];
	float Boresightpos_continue_x5[chid_camera];
	float Boresightpos_continue_y5[chid_camera];
	float zoombak5[chid_camera];

	float level_Fov_continue6[chid_camera];
	float vertical_Fov_continue6[chid_camera];
	float Boresightpos_continue_x6[chid_camera];
	float Boresightpos_continue_y6[chid_camera];
	float zoombak6[chid_camera];

	float level_Fov_continue7[chid_camera];
	float vertical_Fov_continue7[chid_camera];
	float Boresightpos_continue_x7[chid_camera];
	float Boresightpos_continue_y7[chid_camera];
	float zoombak7[chid_camera];

	float level_Fov_continue8[chid_camera];
	float vertical_Fov_continue8[chid_camera];
	float Boresightpos_continue_x8[chid_camera];
	float Boresightpos_continue_y8[chid_camera];

	float level_Fov_continue9[chid_camera];
	float vertical_Fov_continue9[chid_camera];
	float Boresightpos_continue_x9[chid_camera];
	float Boresightpos_continue_y9[chid_camera];

	float level_Fov_continue10[chid_camera];
	float vertical_Fov_continue10[chid_camera];
	float Boresightpos_continue_x10[chid_camera];
	float Boresightpos_continue_y10[chid_camera];

	float level_Fov_continue11[chid_camera];
	float vertical_Fov_continue11[chid_camera];
	float Boresightpos_continue_x11[chid_camera];
	float Boresightpos_continue_y11[chid_camera];

	float level_Fov_continue12[chid_camera];
	float vertical_Fov_continue12[chid_camera];
	float Boresightpos_continue_x12[chid_camera];
	float Boresightpos_continue_y12[chid_camera];

	float level_Fov_continue13[chid_camera];
	float vertical_Fov_continue13[chid_camera];
	float Boresightpos_continue_x13[chid_camera];
	float Boresightpos_continue_y13[chid_camera];
}View;


class CSensorComp{
public:
	CSensorComp();
	~CSensorComp(){};

static inline void SensorComp_CreateParams_Init(SensorComp_CreateParams *pPrm, int i, View* Pserson)
{
	memset(pPrm, 0, sizeof(SensorComp_CreateParams));
if(i == 1){
	pPrm->nWidth = 1920; // Pserson->Resolution_level[0];
	pPrm->nHeight = 1080; //Pserson->Resolution_vertical[0];
	pPrm->defaultBoresightPos_x = Pserson->Boresightpos_fix_x[0];
	pPrm->defaultBoresightPos_y = Pserson->Boresightpos_fix_y[0];
	pPrm->fFovMin = 3.0f;
	pPrm->fFovMax = Pserson->level_Fov_fix[0];//20.0f;
	pPrm->fFovRatio = Pserson->radio[0];
	pPrm->initMode = SENSORCOMP_MODE_RUN;
}

}

int SensorComp_GetIndex(HSENSORCOMP hComp);

HSENSORCOMP SensorComp_Create(SensorComp_CreateParams *pPrm);

void SensorComp_Delete(HSENSORCOMP hComp);

SENSORCOMP_MODE SensorComp_GetMode(HSENSORCOMP hComp);

int SensorComp_SwitchMode(HSENSORCOMP hComp, SENSORCOMP_MODE mode, int flag);

int SensorComp_GetBoresightPos(HSENSORCOMP hComp, float fFov, int *pX, int *pY);

int SensorComp_SetBoresightPos(HSENSORCOMP hComp, float fFov, int x, int y);

int SensorComp_GetTab(HSENSORCOMP hComp, SensorComp_BoresightPos **ppTab, int *pSize);

int SensorComp_CleanTab(HSENSORCOMP hComp);

private:
	static CGlobalDate* _GlobalDate;
};


#endif
