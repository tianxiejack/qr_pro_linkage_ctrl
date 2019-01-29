#ifndef _PLATFORM_FILTER_H
#define _PLATFORM_FILTER_H


typedef void * HPLTFILTER;

typedef struct
{
	int iIndex;

	float P0;

	float P1;

	float P2;

	float L1;

	float L2;

	float G;

	float P02;

	float P12;

	float P22;

	float L12;

	float L22;

	float G2;

}PlatformFilter_InitParams;


typedef struct
{
	int iIndex;

	float P0;

	float P1;

	float P2;

	float L1;

	float L2;

	float G;

}PlatformFilter_CreateParams;

typedef struct
{
    float Yc[2];
    float Xc[3];
    float Uc[2];
    float Ycbuf[8];
    float Yc_out;
} PlatformFilter_pri;


typedef struct
{
	PlatformFilter_CreateParams params;

    PlatformFilter_pri privates;

} PlatformFilter_Obj;




class CPlatformFilter{

public:
	CPlatformFilter();
	~CPlatformFilter();


int PlatformFilter_GetIndex(HPLTFILTER hFilter);

HPLTFILTER PlatformFilter_Create(PlatformFilter_CreateParams *pPrm);

void PlatformFilter_Delete(HPLTFILTER hFilter);

void PlatformFilter_Reset(HPLTFILTER hFilter);

float PlatformFilter_Get(HPLTFILTER hFilter, float curValue, float curYc);

void PlatformFilter_CreateParams_Gettxt(
    PlatformFilter_CreateParams *pPrm,PlatformFilter_CreateParams *pPrm2, PlatformFilter_InitParams *m_pPrm);

int PlatformFilter_init(HPLTFILTER hFilter, float out, float curErr);

};


#endif
