#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "platformFilter.h"
#include "platformparam.h"

#define EPSINON   (0.000001f)

CPlatformFilter::CPlatformFilter()
{

}

CPlatformFilter::~CPlatformFilter()
{

}


int CPlatformFilter::PlatformFilter_GetIndex(HPLTFILTER hFilter)
{
    PlatformFilter_Obj *pObj = (PlatformFilter_Obj*)hFilter;

    if(pObj == NULL)
        return -1;

    return pObj->params.iIndex;
}

HPLTFILTER CPlatformFilter::PlatformFilter_Create(PlatformFilter_CreateParams *pPrm)
{
    PlatformFilter_Obj *pObj;

    if(pPrm == NULL)
        return NULL;

    pObj = (PlatformFilter_Obj *)malloc(sizeof(PlatformFilter_Obj));

    if(pObj == NULL)
        return NULL;

    memset(pObj, 0, sizeof(PlatformFilter_Obj));

    memcpy(&pObj->params, pPrm, sizeof(PlatformFilter_CreateParams));

    return pObj;
}

void CPlatformFilter::PlatformFilter_Delete(HPLTFILTER hFilter)
{
    PlatformFilter_Obj *pObj = (PlatformFilter_Obj*)hFilter;

    if(pObj == NULL)
        return ;

    free(pObj);
}

void CPlatformFilter::PlatformFilter_Reset(HPLTFILTER hFilter)
{
    PlatformFilter_Obj *pObj = (PlatformFilter_Obj*)hFilter;
    if(pObj == NULL)
        return ;

    memset(&pObj->privates, 0, sizeof(pObj->privates));

}

float CPlatformFilter::PlatformFilter_Get(HPLTFILTER hFilter, float curXc, float curYc)
{
	/* G = Kp;   P0 = Ki; P1 = Kd; P2 = K*/
    float ret = 0.0;
    float Ki = 0.0;
    PlatformFilter_Obj *pObj = (PlatformFilter_Obj*)hFilter;
	static int numx =0,numy = 0;
    if(pObj == NULL)
        return ret;

    Ki = pObj->params.P0;

    pObj->privates.Ycbuf[7] = pObj->privates.Ycbuf[6];
    pObj->privates.Ycbuf[6] = pObj->privates.Ycbuf[5];
    pObj->privates.Ycbuf[5] = pObj->privates.Ycbuf[4];
    pObj->privates.Ycbuf[4] = pObj->privates.Ycbuf[3];
    pObj->privates.Ycbuf[3] = pObj->privates.Ycbuf[2];
    pObj->privates.Ycbuf[2] = pObj->privates.Ycbuf[1];
    pObj->privates.Ycbuf[1] = pObj->privates.Ycbuf[0];
    pObj->privates.Ycbuf[0] = curYc;



    pObj->privates.Xc[2] = pObj->privates.Xc[1];
    pObj->privates.Xc[1] = pObj->privates.Xc[0];
    pObj->privates.Xc[0] = curXc;

	if(curYc)
	{
		pObj->privates.Yc_out = (pObj->privates.Ycbuf[0] + pObj->privates.Ycbuf[1] + pObj->privates.Ycbuf[2] + pObj->privates.Ycbuf[3] + \
    			pObj->privates.Ycbuf[4] + pObj->privates.Ycbuf[5] + pObj->privates.Ycbuf[6] + pObj->privates.Ycbuf[7]) / 8;

		pObj->privates.Yc[0] = pObj->privates.Yc[1] = pObj->privates.Yc_out;
	}
	else
		pObj->privates.Yc[1] = pObj->privates.Yc[0];


	pObj->privates.Uc[1] += pObj->privates.Xc[0];

    pObj->privates.Uc[0] =  pObj->params.G * pObj->privates.Xc[0]
                                              + Ki *  pObj->privates.Uc[1]
                                              + pObj->params.P1 * (pObj->privates.Xc[0] - pObj->privates.Xc[1]);

     ret = pObj->params.P2 * pObj->privates.Yc[1] + (1-pObj->params.P2) * pObj->privates.Uc[0];

     pObj->privates.Yc[0] = ret;

    return ret;
}

void CPlatformFilter::PlatformFilter_CreateParams_Gettxt(PlatformFilter_CreateParams *pPrm,PlatformFilter_CreateParams *pPrm2, PlatformFilter_InitParams *m_pPrm)
{
	pPrm->P0 = m_pPrm->P0;
	pPrm->P1 = m_pPrm->P1;
	pPrm->P2 = m_pPrm->P2;
	pPrm->L1 = m_pPrm->L1;
	pPrm->L2 = m_pPrm->L2;
	pPrm->G  = m_pPrm->G;

	pPrm2->P0 = m_pPrm->P02;
	pPrm2->P1 = m_pPrm->P12;
	pPrm2->P2 = m_pPrm->P22;
	pPrm2->L1 = m_pPrm->L12;
	pPrm2->L2 = m_pPrm->L22;
	pPrm2->G  = m_pPrm->G2;
}

int CPlatformFilter::PlatformFilter_init(HPLTFILTER hFilter, float out, float curErr)
{
	int ret = -1;
	PlatformFilter_Obj *pObj = (PlatformFilter_Obj*)hFilter;

	if(pObj == NULL)
		return ret;

	pObj->privates.Xc[2] = pObj->privates.Xc[1] = pObj->privates.Xc[0] = curErr;

	pObj->privates.Yc[0] = pObj->privates.Yc[1] = out;
	pObj->privates.Uc[1] = 0; //curErr;

	printf("****************INIT*******************\n");
	
	return 0;	
}
