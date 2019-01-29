#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "sensorComp.h"

#define EPSINON   (0.000001f)
#ifndef min
#define min(a, b)	((a)<(b)?(a):(b))
#endif

typedef struct {
	int nCount;
	int nSize;
	SensorComp_BoresightPos *pos;
}SensorComp_BoresightPosTab;

typedef struct {

	SensorComp_CreateParams params;

	SensorComp_BoresightPosTab *pBoresightPosRunTab;

	SensorComp_BoresightPosTab *pBoresightPosCorrTab;

	SENSORCOMP_MODE curMode;

	SensorComp_BoresightPosTab *pCurTab;

	int iCurPos;

	float fCurFov;

	int iCurX, iCurY;

}SensorComp_Obj;

CGlobalDate* CSensorComp::_GlobalDate = 0;

View viewParam;

CSensorComp::CSensorComp()
{
	_GlobalDate = CGlobalDate::Instance();
}

/************************************************************************/
/*    boresightPosTab                                                   */
/************************************************************************/
static inline SensorComp_BoresightPosTab* boresightPosTab_alloc(int size)
{
	SensorComp_BoresightPosTab *tab = NULL;

	assert(size > 0);

	tab = (SensorComp_BoresightPosTab*)malloc(sizeof(SensorComp_BoresightPosTab));

	assert(tab != NULL);

	memset(tab, 0, sizeof(SensorComp_BoresightPosTab));

	tab->pos = (SensorComp_BoresightPos*)malloc(sizeof(SensorComp_BoresightPos)*size);

	assert(tab->pos != NULL);

	memset(tab->pos, 0, sizeof(SensorComp_BoresightPos)*size);

	tab->nSize = size;

	return tab;
}

static inline void boresightPosTab_free(SensorComp_BoresightPosTab* tab)
{
	assert(tab != NULL);

	if(tab->pos != NULL)
		free(tab->pos);

	free(tab);
}

static inline void boresightPosTab_BuildDef(SensorComp_BoresightPosTab* tab, int x, int y,
									   float fFovMin, float fFovMax)
{
	assert(tab != NULL);
	assert(tab->nSize >= 2);

	tab->nCount = 2;
	tab->pos[0].fFov = fFovMin;
	tab->pos[0].x = x;
	tab->pos[0].y = y;
	tab->pos[1].fFov = fFovMax;
	tab->pos[1].x = x;
	tab->pos[1].y = y;
}

inline void boresightPosTab_Build(SensorComp_BoresightPosTab* tab,
									SensorComp_BoresightPos* posArray,
									int nArraySize)
{
	assert(tab != NULL);
	assert(posArray != NULL);
	assert(tab->nSize >= nArraySize);

	memcpy(tab->pos, posArray, nArraySize*sizeof(SensorComp_BoresightPos));

	tab->nCount = nArraySize;
}

static inline SensorComp_BoresightPosTab* boresightPosTab_copy(SensorComp_BoresightPosTab* dest,
														  SensorComp_BoresightPosTab* src)
{
	assert(dest != NULL);
	assert(src != NULL);

	if(dest->nSize < src->nCount)
	{
		free(dest->pos);		
		dest->pos = (SensorComp_BoresightPos*)malloc(sizeof(SensorComp_BoresightPos)*src->nCount);
		dest->nSize = src->nCount;
		assert(dest->pos != NULL);
	}

	boresightPosTab_Build(dest, src->pos, src->nCount);

	return dest;
}

static inline int boresightPosTab_find(SensorComp_BoresightPosTab* tab,
									float fov)
{
	int i = 0;

	assert(tab != NULL);
	
	for(i=tab->nCount-1; i>=0; i--)
	{
		if(fov >= tab->pos[i].fFov - EPSINON)
			break;
	}

	return i;
}

static inline int boresightPosTab_insert(SensorComp_BoresightPosTab* tab,
									SensorComp_BoresightPos* pBP)
{
	int i = 0;
	int iPos;
	
	assert(tab != NULL);
	
	for(i=tab->nCount-1; i>=0; i--)
	{
		if(pBP->fFov >= tab->pos[i].fFov - EPSINON)
			break;
	}

	if(pBP->fFov <= tab->pos[i].fFov + EPSINON || i == 0)
	{
		tab->pos[i].x = pBP->x;
		tab->pos[i].y = pBP->y;
		return i;
	}

	if(tab->nCount+1 > tab->nSize && tab->nSize < SENSORCOMP_BORESIGHTPOS_TAB_MAX_SIZE)
	{
		SensorComp_BoresightPos* posArray = NULL;
		SensorComp_BoresightPos* posArrayBak;
		int nSize = tab->nSize + 8;
		nSize = min(nSize, SENSORCOMP_BORESIGHTPOS_TAB_MAX_SIZE);

		posArray = (SensorComp_BoresightPos*)malloc(sizeof(SensorComp_BoresightPos)*nSize);
		assert(posArray != NULL);
		memcpy(posArray, tab->pos, tab->nCount*sizeof(SensorComp_BoresightPos));
		posArrayBak = tab->pos;
		tab->pos = posArray;
		free(posArrayBak);
		tab->nSize = nSize;
	}

	iPos = i+1;

	if(iPos < tab->nCount)
		memmove(&tab->pos[iPos+1], &tab->pos[iPos], sizeof(SensorComp_BoresightPos)*(tab->nCount-iPos));

	tab->pos[iPos].fFov = pBP->fFov;
	tab->pos[iPos].x = pBP->x;
	tab->pos[iPos].y = pBP->y;

	tab->nCount ++;

	return iPos;
}

/************************************************************************/
/*    SensorComp                                                        */
/************************************************************************/

static inline void SensorComp_GetBoresightPos_(SensorComp_Obj *pObj, float fFov, int *pX, int *pY)
{
	int x = 0, y = 0;

	float dfov = fFov - pObj->pCurTab->pos[pObj->iCurPos].fFov;

	if((dfov < EPSINON && dfov > -EPSINON) 
		|| pObj->iCurPos == pObj->pCurTab->nCount-1)
	{
		x = pObj->pCurTab->pos[pObj->iCurPos].x;
		y = pObj->pCurTab->pos[pObj->iCurPos].y;
	}
	else
	{
		float dx, dy, unitFov;
		dx = (float)(pObj->pCurTab->pos[pObj->iCurPos+1].x - pObj->pCurTab->pos[pObj->iCurPos].x);
		dy = (float)(pObj->pCurTab->pos[pObj->iCurPos+1].y - pObj->pCurTab->pos[pObj->iCurPos].y);
		unitFov = pObj->pCurTab->pos[pObj->iCurPos+1].fFov - pObj->pCurTab->pos[pObj->iCurPos].fFov;
		assert(dfov > EPSINON);
		assert(unitFov > EPSINON);
		x = pObj->pCurTab->pos[pObj->iCurPos].x;
		y = pObj->pCurTab->pos[pObj->iCurPos].y;
		x += (int)((dx * dfov)/unitFov);
		y += (int)((dy * dfov)/unitFov);
	}
	*pX = x;
	*pY = y;
}

int CSensorComp::SensorComp_GetIndex(HSENSORCOMP hComp)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	if(pObj == NULL)
		return -1;

	return pObj->params.iIndex;
}

HSENSORCOMP CSensorComp::SensorComp_Create(SensorComp_CreateParams *pPrm)
{
	SensorComp_Obj *pObj;

	int tabSize = SENSORCOMP_BORESIGHTPOS_TAB_DEF_SIZE;

	if(pPrm == NULL)
		return NULL;

	pObj = (SensorComp_Obj *)malloc(sizeof(SensorComp_Obj));

	if(pObj == NULL)
		return NULL;

	memset(pObj, 0, sizeof(SensorComp_Obj));

	memcpy(&pObj->params, pPrm, sizeof(SensorComp_CreateParams));

	if(pPrm->initTab != NULL)
		tabSize = pPrm->nInitTabSize;

	assert(tabSize > 0);

	pObj->pBoresightPosRunTab = boresightPosTab_alloc(tabSize);
	if(pObj->pBoresightPosRunTab == NULL)
	{
		SensorComp_Delete(pObj);
		return NULL;
	}

	pObj->pBoresightPosCorrTab = boresightPosTab_alloc(tabSize*2);
	if(pObj->pBoresightPosCorrTab == NULL)
	{
		SensorComp_Delete(pObj);
		return NULL;
	}

	if(pPrm->initTab != NULL)
		boresightPosTab_Build(pObj->pBoresightPosRunTab, pPrm->initTab, tabSize);
	else
		boresightPosTab_BuildDef(pObj->pBoresightPosRunTab, 
			pPrm->defaultBoresightPos_x, 
			pPrm->defaultBoresightPos_y,
			pPrm->fFovMin,
			pPrm->fFovMax);

	pObj->curMode = SENSORCOMP_MODE_RUN;
	pObj->pCurTab = pObj->pBoresightPosRunTab;
	pObj->fCurFov = pObj->pBoresightPosRunTab->pos[0].fFov;
	pObj->iCurPos = 0;
	pObj->iCurX = pObj->pBoresightPosRunTab->pos[0].x;
	pObj->iCurY = pObj->pBoresightPosRunTab->pos[0].y;

	SensorComp_SwitchMode(pObj, pObj->params.initMode, 0);

	return pObj;
}

void CSensorComp::SensorComp_Delete(HSENSORCOMP hComp)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	if(pObj == NULL)
		return ;

	if(pObj->pBoresightPosRunTab != NULL)
		boresightPosTab_free(pObj->pBoresightPosRunTab);
	pObj->pBoresightPosRunTab = NULL;

	if(pObj->pBoresightPosCorrTab != NULL)
		boresightPosTab_free(pObj->pBoresightPosCorrTab);
	pObj->pBoresightPosCorrTab = NULL;

	free(pObj);
}

SENSORCOMP_MODE CSensorComp::SensorComp_GetMode(HSENSORCOMP hComp)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	if(pObj == NULL)
		return error;
	
	return pObj->curMode;
}

int CSensorComp::SensorComp_SwitchMode(HSENSORCOMP hComp, SENSORCOMP_MODE mode, int flag)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	if(pObj == NULL)
		return -1;

	if(pObj->curMode == mode)
		return mode;

	if(mode == SENSORCOMP_MODE_RUN)
	{
		if(flag == 1)
		{
			pObj->pBoresightPosRunTab = 
				boresightPosTab_copy(pObj->pBoresightPosRunTab, pObj->pBoresightPosCorrTab);
		}
		pObj->pCurTab = pObj->pBoresightPosRunTab;
	}

	if(mode == SENSORCOMP_MODE_CORR_CLEAN)
	{
		boresightPosTab_BuildDef(pObj->pBoresightPosCorrTab, 
			pObj->params.defaultBoresightPos_x, 
			pObj->params.defaultBoresightPos_y,
			pObj->params.fFovMin,
			pObj->params.fFovMax);
		pObj->pCurTab = pObj->pBoresightPosCorrTab;
	}

	if(mode == SENSORCOMP_MODE_CORR_NORMAL)
	{
		pObj->pBoresightPosCorrTab = 
			boresightPosTab_copy(pObj->pBoresightPosCorrTab, pObj->pBoresightPosRunTab);
		pObj->pCurTab = pObj->pBoresightPosCorrTab;
	}

	pObj->iCurPos = boresightPosTab_find(pObj->pCurTab, pObj->fCurFov);

	SensorComp_GetBoresightPos_(pObj, pObj->fCurFov, &pObj->iCurX, &pObj->iCurY);

	pObj->curMode = mode;
	
	return pObj->curMode;	
}

int CSensorComp::SensorComp_GetBoresightPos(HSENSORCOMP hComp, float fFov, int *pX, int *pY)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	float dfov;

	if(pObj == NULL || pX == NULL || pY == NULL)
		return -1;
	
	dfov = fFov - pObj->fCurFov;

	if(dfov < EPSINON && dfov > -EPSINON) 
	{
		*pX = pObj->iCurX;
		*pY = pObj->iCurY;

		return 0;
	}

	assert((fFov>=pObj->params.fFovMin));

	pObj->fCurFov = fFov;

	pObj->iCurPos = boresightPosTab_find(pObj->pCurTab, pObj->fCurFov);
	
	SensorComp_GetBoresightPos_(pObj, pObj->fCurFov, &pObj->iCurX, &pObj->iCurY);

	*pX = pObj->iCurX;
	*pY = pObj->iCurY;

	return 0;
}

int CSensorComp::SensorComp_SetBoresightPos(HSENSORCOMP hComp, float fFov, int x, int y)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	SensorComp_BoresightPos bp;
	
	if(pObj == NULL || fFov < pObj->params.fFovMin)
		return -1;

	if(x<0 || x>pObj->params.nWidth)
		return -1;

	if(y<0 || y>pObj->params.nHeight)
		return -1;

	if(pObj->curMode == SENSORCOMP_MODE_RUN)
		return -1;

	bp.fFov = fFov;
	bp.x = x;
	bp.y = y;
	
	boresightPosTab_insert(pObj->pCurTab, &bp);

	return 0;
}

int CSensorComp::SensorComp_GetTab(HSENSORCOMP hComp, SensorComp_BoresightPos **ppTab, int *pSize)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	if(pObj == NULL || ppTab == NULL || pSize == NULL)
		return -1;

	*ppTab = pObj->pCurTab->pos;
	*pSize = pObj->pCurTab->nCount;

	return 0;
}

int CSensorComp::SensorComp_CleanTab(HSENSORCOMP hComp)
{
	SensorComp_Obj *pObj = (SensorComp_Obj*)hComp;
	SensorComp_CreateParams *pPrm;
	if(pObj == NULL)
		return -1;

	pPrm = &pObj->params;

	boresightPosTab_BuildDef(pObj->pCurTab, 
		pPrm->defaultBoresightPos_x, 
		pPrm->defaultBoresightPos_y,
		pPrm->fFovMin,
		pPrm->fFovMax);

	pObj->iCurPos = boresightPosTab_find(pObj->pCurTab, pObj->fCurFov);
	
	SensorComp_GetBoresightPos_(pObj, pObj->fCurFov, &pObj->iCurX, &pObj->iCurY);

	return 0;
}
