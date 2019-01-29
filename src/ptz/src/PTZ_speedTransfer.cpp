#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "osa.h"
#include "PTZ_speedTransfer.h"
#include <math.h>

const char speedMap = 0;
CGlobalDate* CPTZSpeedTransfer::_GlobalDate = 0;
#define FILL_TAB(TAB, INDEX, MIN, MAX, COUNT)	\
{		\
	minV = MIN;	maxV = MAX;		\
	dt = (maxV-minV)/COUNT;		\
	for(i=0; i<COUNT; i++){		\
		TAB##SpeedTab[INDEX+i] = minV + (i*dt);	\
	}		\
}


CPTZSpeedTransfer::CPTZSpeedTransfer()
{
	_GlobalDate = CGlobalDate::Instance();
	m_bTabInit = FALSE;
	memset(SpeedLevelPan, 0, sizeof(SpeedLevelPan));
	memset(SpeedLeveTilt, 0, sizeof(SpeedLeveTilt));

#if speedMap
	memset(m_PanSpeedTab,0,sizeof(m_PanSpeedTab));
	memset(m_TilSpeedTab,0,sizeof(m_TilSpeedTab));
#endif
}

CPTZSpeedTransfer::~CPTZSpeedTransfer()
{
	#if speedMap
		m_bTabInit = FALSE;
	#endif
}

void CPTZSpeedTransfer::create()
{
	int chid = _GlobalDate->chid_camera;
	memcpy(SpeedLevelPan, &panSpeed[chid], sizeof(panSpeed[chid]));
	memcpy(SpeedLeveTilt, &TiltSpeed[chid], sizeof(TiltSpeed[chid]));
}

void CPTZSpeedTransfer::switchChid()
{
	int chid = _GlobalDate->chid_camera;
	memcpy(SpeedLevelPan, &panSpeed[chid], sizeof(panSpeed[chid]));
	memcpy(SpeedLeveTilt, &TiltSpeed[chid], sizeof(TiltSpeed[chid]));
}

#if speedMap

int CPTZSpeedTransfer::SpeedTabInit()
{

	int i;
	int dt, minV, maxV;

	if(m_bTabInit)
		return 0;

#if 0//���1
	m_PanSpeedTab[0] = 0;
	m_PanSpeedTab[1] = 490;
	m_PanSpeedTab[2] = 490;
	m_PanSpeedTab[3] = 490;
	m_PanSpeedTab[4] = 490;
	m_PanSpeedTab[5] = 490;
	m_PanSpeedTab[6] = 490;
	m_PanSpeedTab[7] = 490;
	m_PanSpeedTab[8] = 490;
	m_PanSpeedTab[9] = 490;
	m_PanSpeedTab[10] = 590;
	m_PanSpeedTab[11] = 690;
	m_PanSpeedTab[12] = 700;
	m_PanSpeedTab[13] = 780;
	m_PanSpeedTab[14] = 890;
	m_PanSpeedTab[15] = 990;
	m_PanSpeedTab[16] = 1000;
	m_PanSpeedTab[17] = 1090;
	m_PanSpeedTab[18] = 1290;
	m_PanSpeedTab[19] = 1390;
	m_PanSpeedTab[20] = 1480;
	m_PanSpeedTab[21] = 1690;
	m_PanSpeedTab[22] = 1790;
	m_PanSpeedTab[23] = 1990;
	m_PanSpeedTab[24] = 2190;
	m_PanSpeedTab[25] = 2390;
	m_PanSpeedTab[26] = 2600;
	m_PanSpeedTab[27] = 2890;
	m_PanSpeedTab[28] = 3190;
	m_PanSpeedTab[29] = 3490;

	m_PanSpeedTab[30] = 3690;
	m_PanSpeedTab[31] = 3690;
	m_PanSpeedTab[32] = 3690;
	m_PanSpeedTab[33] = 5790;
	m_PanSpeedTab[34] = 5790;
	m_PanSpeedTab[35] = 5980;
	m_PanSpeedTab[36] = 6490;
	m_PanSpeedTab[37] = 6490;
	m_PanSpeedTab[38] = 9290;
	m_PanSpeedTab[39] = 9290;

	m_PanSpeedTab[40] = 9590;
	m_PanSpeedTab[41] = 10480;
	m_PanSpeedTab[42] = 11490;
	m_PanSpeedTab[43] = 12590;
	m_PanSpeedTab[44] = 13880;
	m_PanSpeedTab[45] = 15190;
	m_PanSpeedTab[46] = 16690;
	m_PanSpeedTab[47] = 18290;
	m_PanSpeedTab[48] = 19980;
	m_PanSpeedTab[49] = 21980;

	m_PanSpeedTab[50] = 24080;
	m_PanSpeedTab[51] = 26400;
	m_PanSpeedTab[52] = 29000;
	m_PanSpeedTab[53] = 31790;
	m_PanSpeedTab[54] = 34900;
	m_PanSpeedTab[55] = 38170;
	m_PanSpeedTab[56] = 41830;
	m_PanSpeedTab[57] = 45920;
	m_PanSpeedTab[58] = 50400;
	m_PanSpeedTab[59] = 55290;

	m_PanSpeedTab[60] = 60760;
	m_PanSpeedTab[61] = 66540;
	m_PanSpeedTab[62] = 73040;
	m_PanSpeedTab[63] = 80040;

	TiltSpeedTab[0] = 0;
	for(i=1; i<=6; i++){
		TiltSpeedTab[i] = 500;
	}
	TiltSpeedTab[7] = 900;
	TiltSpeedTab[8] = 1300;
	TiltSpeedTab[9] = 1600;
	FULL_TAB(Tilt, 10,   2000,  3700, 5);
	FULL_TAB(Tilt, 15,   3700,  5600, 5);
	FULL_TAB(Tilt, 20,   5600,  7000, 5);
	FULL_TAB(Tilt, 25,   7000,  9400, 5);
	FULL_TAB(Tilt, 30,   9400,  11700, 5);
	FULL_TAB(Tilt, 35,   11700,  14100, 5);
	FULL_TAB(Tilt, 40,   14100,  17000, 5);
	FULL_TAB(Tilt, 45,   17000,  20400, 5);
	FULL_TAB(Tilt, 50,   20400,  25400, 5);
	FULL_TAB(Tilt, 55,   25400,  29400, 5);
	TiltSpeedTab[60] = 29400;
	TiltSpeedTab[61] = 37200;
	TiltSpeedTab[62] = 39800;
	TiltSpeedTab[63] = 44500;

#endif//���1

#if 1
	//FULL_TAB(Pan, 0,  0,  2000, 10);
	FILL_TAB(m_Pan, 0,  0,  8000, 64);
	//FULL_TAB(Tilt, 0,  0,  1000, 10);
	FILL_TAB(m_Til, 0,  0, 4000, 64);
#endif

#if 1 //ԭʼ
	//int i;
	//int dt, minV, maxV;

	m_PanSpeedTab[0] = 39;
#if 1
	m_PanSpeedTab[1] = 78;
	m_PanSpeedTab[2] = 118;
	m_PanSpeedTab[3] = 156;
	m_PanSpeedTab[4] = 196;
	m_PanSpeedTab[5] = 235;
	m_PanSpeedTab[6] = 274;
	m_PanSpeedTab[7] = 314;
	m_PanSpeedTab[8] = 353;
	m_PanSpeedTab[9] = 392;
#else
	m_PanSpeedTab[1] = 5;
	m_PanSpeedTab[2] = 10;
	m_PanSpeedTab[3] = 15;
	m_PanSpeedTab[4] = 20;
	m_PanSpeedTab[5] = 25;
	m_PanSpeedTab[6] = 30;
	m_PanSpeedTab[7] = 35;
	m_PanSpeedTab[8] = 40;
	m_PanSpeedTab[9] = 45;
#endif
	m_PanSpeedTab[10] = 431;
	m_PanSpeedTab[11] = 600;
	m_PanSpeedTab[12] = 627;
	m_PanSpeedTab[13] = 783;
	m_PanSpeedTab[14] = 941;
	m_PanSpeedTab[15] = 1097;
	m_PanSpeedTab[16] = 1253;
	m_PanSpeedTab[17] = 1411;
	m_PanSpeedTab[18] = 1646;
	m_PanSpeedTab[19] = 1882;
	m_PanSpeedTab[20] = 2000;
	m_PanSpeedTab[21] = 2117;
	m_PanSpeedTab[22] = 2235;
	m_PanSpeedTab[23] = 2353;
	m_PanSpeedTab[24] = 2447;
	m_PanSpeedTab[25] = 2541;
	m_PanSpeedTab[26] = 2635;
	m_PanSpeedTab[27] = 2729;
	m_PanSpeedTab[28] = 2823;
	m_PanSpeedTab[29] = 2917;

	m_PanSpeedTab[30] = 3011;
	m_PanSpeedTab[31] = 3105;
	m_PanSpeedTab[32] = 3199;
	m_PanSpeedTab[33] = 3294;
	m_PanSpeedTab[34] = 3412;
	m_PanSpeedTab[35] = 3529;
	m_PanSpeedTab[36] = 3647;
	m_PanSpeedTab[37] = 3764;
	m_PanSpeedTab[38] = 3921;
	m_PanSpeedTab[39] = 4077;

	m_PanSpeedTab[40] = 4235;
	m_PanSpeedTab[41] = 4392;
	m_PanSpeedTab[42] = 4549;
	m_PanSpeedTab[43] = 4705;
	m_PanSpeedTab[44] = 4823;
	m_PanSpeedTab[45] = 4940;
	m_PanSpeedTab[46] = 5058;
	m_PanSpeedTab[47] = 5176;
	m_PanSpeedTab[48] = 5294;
	m_PanSpeedTab[49] = 5411;

	m_PanSpeedTab[50] = 5288;
	m_PanSpeedTab[51] = 5647;
	m_PanSpeedTab[52] = 5882;
	m_PanSpeedTab[53] = 6117;
	m_PanSpeedTab[54] = 6235;
	m_PanSpeedTab[55] = 6352;
	m_PanSpeedTab[56] = 6467;
	m_PanSpeedTab[57] = 6588;
	m_PanSpeedTab[58] = 6823;
	m_PanSpeedTab[59] = 7059;

	m_PanSpeedTab[60] = 7216;
	m_PanSpeedTab[61] = 7373;
	m_PanSpeedTab[62] = 7529;
	m_PanSpeedTab[63] = 8000;

	m_TilSpeedTab[0] = 0;
	for(i=1; i<=6; i++){
		m_TilSpeedTab[i] = 50;
	}
	m_TilSpeedTab[7] = 90;
	m_TilSpeedTab[8] = 130;
	m_TilSpeedTab[9] = 160;
	FILL_TAB(m_Til, 10,   200,  370, 5);
	FILL_TAB(m_Til, 15,   370,  560, 5);
	FILL_TAB(m_Til, 20,   560,  700, 5);
	FILL_TAB(m_Til, 25,   700,  940, 5);
	FILL_TAB(m_Til, 30,   940,  1170, 5);
	FILL_TAB(m_Til, 35,   1170,  1410, 5);
	FILL_TAB(m_Til, 40,   1410,  1700, 5);
	FILL_TAB(m_Til, 45,   1700,  2040, 5);
	FILL_TAB(m_Til, 50,   2040,  2540, 5);
	FILL_TAB(m_Til, 55,   2540,  2940, 5);
	m_TilSpeedTab[60] = 2940;
	m_TilSpeedTab[61] = 3720;
	m_TilSpeedTab[62] = 3980;
	m_TilSpeedTab[63] = 4450;

#endif

	m_bTabInit = TRUE;

	return 0;
}

int CPTZSpeedTransfer::GetPanSpeed(int iIn)
{
	int iOut;
	if(iIn == 0)
		return 0;

	int dir = iIn/(abs(iIn));

	if(!m_bTabInit)
		SpeedTabInit();

	for(iOut = 0; iOut < 64; iOut++)
	{
		if(m_PanSpeedTab[iOut]>abs(iIn))
			break;
	}
	return (iOut)*dir;
}

int CPTZSpeedTransfer::GetTiltSpeed(int iIn)
{
	int iOut;
	if(iIn == 0)
		return 0;

	int dir = iIn/(abs(iIn));

	if(!m_bTabInit)
		SpeedTabInit();

	for(iOut = 0; iOut < 64; iOut++){
		if(m_TilSpeedTab[iOut]>abs(iIn))
			break;
	}
	return (iOut)*dir;
}

#else



int CPTZSpeedTransfer::GetPanSpeed(float fIn)
{
	int level;
	int dir;
	int i;

	dir = fIn < 0 ? -1 : 1;

	level = (int)(fIn*dir + 0.5);
	if(level < SpeedLevelPan[0])
		return 0;

	for (i = 0; i < 8; i++){
			if(level < SpeedLevelPan[i+1])
				break;
	}
	return dir * ( i *8 + (int)(  ( 8.0 * (float) (level -  SpeedLevelPan[i]) )/(float) (SpeedLevelPan[i+1] - SpeedLevelPan[i])  + 0.5)  ) ;

}

int CPTZSpeedTransfer::GetTiltSpeed(float fIn)
{
	int level;
	int dir;
	int i;

	dir = fIn < 0 ? -1 : 1;

	level = (int)(fIn*dir + 0.5);
	if(level < SpeedLeveTilt[0])
		return 0;

	for (i = 0; i < 8; i++){
			if(level < SpeedLeveTilt[i+1])
				break;
	}
	return dir * ( i *8 + (int)(( 8.0 * (float) (level -  SpeedLeveTilt[i]) )/(float) (SpeedLeveTilt[i+1] - SpeedLeveTilt[i])  + 0.5)) ;
}

#endif
