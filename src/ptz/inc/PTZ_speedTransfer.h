#ifndef _PTZ_SPEED_TRANSFER_H
#define _PTZ_SPEED_TRANSFER_H

#include <iostream>
#include "globalDate.h"

using namespace std;

	typedef struct{
		int x0;
		int x1;
		int x2;
		int x3;
		int x4;
		int x5;
		int x6;
		int x7;
		int x8;
		int x9;
	}PanSpeedTab;

	typedef struct{
		int y0;
		int y1;
		int y2;
		int y3;
		int y4;
		int y5;
		int y6;
		int y7;
		int y8;
		int y9;
	}TiltSpeedTab;

class CPTZSpeedTransfer 
{
public:
	CPTZSpeedTransfer();
	virtual ~CPTZSpeedTransfer();
	void create();
	static CGlobalDate* _GlobalDate;
	int m_PanSpeedTab[64];
	int m_TilSpeedTab[64];
	bool m_bTabInit;

	 PanSpeedTab panSpeed[5];
	 TiltSpeedTab TiltSpeed[5];

		int GetPanSpeed(float fIn);
		int GetTiltSpeed(float fIn);
		int SpeedTabInit();
		int PTZSpeed_creat();
		void switchChid();

		int SpeedLevelPan[10];
		int SpeedLeveTilt[10];

};
#endif
