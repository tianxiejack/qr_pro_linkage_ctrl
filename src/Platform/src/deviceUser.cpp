#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "deviceUser.h"
#include "platformControl.h"

#define EPSINON   (0.000001f)

typedef struct
{

    DeviceUser_CreateParams params;

    void *pUser;

    float curValue;

} DeviceUser_Obj;


float CDeviceUser::PlatforCtrl_GetDevInputValue(void* handle, eDEVUSER_InputSrcType type, int iNumber)
{
    float fRet = 0.0f;
    PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle;

    if(pObj == NULL)
        return -1;
    //printf("platformControl=====> type = %d\n", type);
    switch(type)
    {
        case DEVUSER_InputSrcType_User:
            fRet = pObj->inter.localInput[iNumber];
            break;

        case DEVUSER_InputSrcType_Analog:
            fRet = pObj->inter.analogInput[iNumber];
            break;

        case DEVUSER_InputSrcType_Virtual:
            fRet = pObj->inter.virtalInput[iNumber];
            break;

        default:
            fRet = pObj->inter.localInput[iNumber];
            break;
    }

    return fRet;
}

int CDeviceUser::DeviceUser_GetIndex(HDEVUSR hDevUsr)
{
    DeviceUser_Obj *pObj = (DeviceUser_Obj*)hDevUsr;

    if(pObj == NULL)
        return -1;

    return pObj->params.iIndex;
}

HDEVUSR CDeviceUser::DeviceUser_Create(DeviceUser_CreateParams *pPrm, void *pUser)
{
    DeviceUser_Obj *pObj;

    if(pPrm == NULL)
        return NULL;

    pObj = (DeviceUser_Obj *)malloc(sizeof(DeviceUser_Obj));

    if(pObj == NULL)
        return NULL;

    memset(pObj, 0, sizeof(DeviceUser_Obj));

    memcpy(&pObj->params, pPrm, sizeof(DeviceUser_CreateParams));

    pObj->pUser = pUser;

    return pObj;
}

int CDeviceUser::DeviceUser_SetParam(HDEVUSR hDevUsr, DeviceUser_CreateParams *pPrm)
{
    DeviceUser_Obj *pObj = (DeviceUser_Obj*)hDevUsr;

    if(pObj == NULL)
        return -1;

    memcpy(&pObj->params, pPrm, sizeof(DeviceUser_CreateParams));

    return 0;
}

void CDeviceUser::DeviceUser_Delete(HDEVUSR hDevUsr)
{
    DeviceUser_Obj *pObj = (DeviceUser_Obj*)hDevUsr;

    if(pObj == NULL)
        return ;

    free(pObj);
}

void CDeviceUser::DeviceUser_Reset(HDEVUSR hDevUsr)
{
    DeviceUser_Obj *pObj = (DeviceUser_Obj*)hDevUsr;

    if(pObj == NULL)
        return ;

    pObj->curValue = 0.0f;
}

float CDeviceUser::DeviceUser_Get(HDEVUSR hDevUsr)
{
    float ret = 0.0;
    DeviceUser_Obj *pObj = (DeviceUser_Obj*)hDevUsr;

    if(pObj == NULL)
        return ret;

    ret = PlatforCtrl_GetDevInputValue(pObj->pUser, pObj->params.inputSrcType, pObj->params.iInputSrcNum);

    if(pObj->params.inputMethod == DEVUSER_InputMethod_Incremental)
    {
        if(pObj->params.gainType == DEVUSER_GainType_Incremental){
            pObj->curValue += ret * pObj->params.fGain;
        }
        else{
            pObj->curValue += ret;
        }
    }
    else
    {
        pObj->curValue = ret;
    }

    ret = pObj->curValue;

    if(pObj->params.gainType == DEVUSER_GainType_Universal)
    {
        ret *= pObj->params.fGain;
    }

    return ret;
}
