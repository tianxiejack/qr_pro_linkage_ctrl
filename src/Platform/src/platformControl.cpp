#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <osa.h>
#include "platformControl.h"

CStatusManager* CplatFormControl::_stateManager = 0;

CGlobalDate* CplatFormControl::_GlobalDate = 0;

const float EPSINON = 0.000001f;


CplatFormControl::CplatFormControl()
{
	PlatformCtrl_ClassInit();
}

CplatFormControl::~CplatFormControl()
{
	delete _Fiter;
	delete _Kalman;
	delete _DeviceUser;
	delete _Sensor;
	delete _stateManager;
	//delete _Message;
}

void CplatFormControl::PlatformCtrl_ClassInit()
{
	_Sensor = new CSensorComp();
	_Fiter = new CPlatformFilter();
	_Kalman = new CKalman_PTZ();
	_DeviceUser = new CDeviceUser();
	_stateManager = CStatusManager::Instance();
	_Message = CMessage::getInstance();
	_GlobalDate = CGlobalDate::Instance();
}

HPLTCTRL CplatFormControl::PlatformCtrl_Create(PlatformCtrl_CreateParams *pPrm)
{
    PlatformCtrl_Obj *pObj;
    int i;
    if(pPrm == NULL)
        return NULL;
    pObj = (PlatformCtrl_Obj *)malloc(sizeof(PlatformCtrl_Obj));
    if(pObj == NULL)
        return NULL;

    memset(pObj, 0, sizeof(PlatformCtrl_Obj));
    memcpy(&pObj->params, pPrm, sizeof(PlatformCtrl_CreateParams));
    pObj->inter.object = pObj;
    pObj->privates.iTrkAlgStateBak = 0;

    for(i = 0; i < SENSOR_COUNT; i++)
        pObj->privates.hSensor[i] = _Sensor->SensorComp_Create(&pObj->params.sensorParams[i]);

    pObj->inter.output.iSensor = pObj->params.iSensorInit;
    pObj->privates.fovX = pObj->params.fFov[pObj->inter.output.iSensor];
    pObj->privates.fovY = pObj->privates.fovX * pObj->params.sensorParams[pObj->inter.output.iSensor].fFovRatio;
    pObj->privates.width = (float)pObj->params.sensorParams[pObj->inter.output.iSensor].nWidth;
    pObj->privates.height = (float)pObj->params.sensorParams[pObj->inter.output.iSensor].nHeight;

    for(i = 0; i < DevUsr_MAX; i++)
        pObj->privates.hDevUsers[i] = _DeviceUser->DeviceUser_Create(&pObj->params.deviceUsrParam[i], pObj);

    pObj->privates.filter[0][0] = _Fiter->PlatformFilter_Create(&pObj->params.platformFilterParam[0][0]);
    pObj->privates.filter[0][1] = _Fiter->PlatformFilter_Create(&pObj->params.platformFilterParam[0][1]);
    pObj->privates.filter[1][0] = _Fiter->PlatformFilter_Create(&pObj->params.platformFilterParam[1][0]);
    pObj->privates.filter[1][1] = _Fiter->PlatformFilter_Create(&pObj->params.platformFilterParam[1][1]);

    pObj->privates.joystickIntegratorParam[0].P0
        = pObj->params.joystickRateDemandParam.fPlatformIntegratingGain_X;
    pObj->privates.joystickIntegratorParam[0].P1
        = (-1.0f) * pObj->params.joystickRateDemandParam.fPlatformIntegratingDecayGain;
    pObj->privates.joystickIntegrator[0]
        = _Fiter->PlatformFilter_Create(&pObj->privates.joystickIntegratorParam[0]);
    pObj->privates.joystickIntegratorParam[1].P0
        = pObj->params.joystickRateDemandParam.fPlatformIntegratingGain_Y;
    pObj->privates.joystickIntegratorParam[1].P1
        = (-1.0f) * pObj->params.joystickRateDemandParam.fPlatformIntegratingDecayGain;
    pObj->privates.joystickIntegrator[1]
        = _Fiter->PlatformFilter_Create(&pObj->privates.joystickIntegratorParam[1]);

    pObj->privates.hWinFilter = _Kalman->KalmanOpen(6, 3, 0);
    _Kalman->KalmanInitParam(pObj->privates.hWinFilter, 0.0, 0.0, 0, 0.0);

    return &pObj->inter;
}

inline void CplatFormControl::PlatformCtrl_CreateParams_Init(PlatformCtrl_CreateParams *pPrm, configPlatParam_InitParams *m_Prm, View* m_Sensor)
{
	 	int i, chId;
	 	chId =_GlobalDate->chid_camera;
	 	memset(pPrm, 0, sizeof(PlatformCtrl_CreateParams));

	 	for(i=0; i<SENSOR_COUNT; i++){
	 		_Sensor->SensorComp_CreateParams_Init(&pPrm->sensorParams[i], i, m_Sensor);
	 		pPrm->sensorParams[i].iIndex = i;
	 		pPrm->fFov[i] = pPrm->sensorParams[i].fFovMax;
	 	}
	 	pPrm->iSensorInit = 1;
	 	for(i=0; i<DevUsr_MAX; i++){
	 		_DeviceUser->DeviceUser_CreateParams_Init(&pPrm->deviceUsrParam[i]);
	 		pPrm->deviceUsrParam[i].iIndex = i;
	 		pPrm->deviceUsrParam[i].iInputSrcNum = i;
	 		pPrm->deviceUsrParam[i].inputSrcType = DEVUSER_InputSrcType_Virtual;
	 	}
	 	pPrm->deviceUsrParam[DevUsr_AcqWinSizeXInput].gainType = DEVUSER_GainType_Universal;
	 	pPrm->deviceUsrParam[DevUsr_AcqWinSizeYInput].gainType = DEVUSER_GainType_Universal;
	 	pPrm->deviceUsrParam[DevUsr_TrkWinSizeXInput].gainType = DEVUSER_GainType_Universal;
	 	pPrm->deviceUsrParam[DevUsr_TrkWinSizeYInput].gainType = DEVUSER_GainType_Universal;

	 	_Fiter->PlatformFilter_CreateParams_Gettxt(&pPrm->platformFilterParam[0][0],&pPrm->platformFilterParam[0][1],\
	 			&m_Prm->m__cfg_platformFilterParam[chId]);

	 	pPrm->scalarX = m_Prm->scalarX;
	 	pPrm->scalarY = m_Prm->scalarY;

	 	pPrm->demandMaxX = m_Prm->demandMaxX[chId];
	 	pPrm->demandMaxY = m_Prm->demandMaxY[chId];

	 	pPrm->bleedUsed = 1;
	 	pPrm->deadbandX = m_Prm->deadbandX[chId];
	 	pPrm->deadbandY = m_Prm->deadbandY[chId];

	 	pPrm->acqOutputType = m_Prm->acqOutputType;
	 	pPrm->Kx = m_Prm->Kx[chId];
	 	pPrm->Ky = m_Prm->Ky[chId];
	 	pPrm->Error_X = m_Prm->Error_X[chId];
	 	pPrm->Error_Y = m_Prm->Error_Y[chId];
	 	pPrm->Time = m_Prm->Time[chId];
	 	pPrm->bleedX =m_Prm->BleedLimit_X[chId];
	 	pPrm->bleedY =m_Prm->BleedLimit_Y[chId];


	 	pPrm->joystickRateDemandParam.fCutpoint1 = m_Prm->joystickRateDemandParam.fCutpoint1;//0.7f;
	 	pPrm->joystickRateDemandParam.fInputGain_X1 =m_Prm->joystickRateDemandParam.fInputGain_X1;// 0.2f;
	 	pPrm->joystickRateDemandParam.fInputGain_Y1 =m_Prm->joystickRateDemandParam.fInputGain_Y1;// 0.3f;

	 	pPrm->joystickRateDemandParam.fCutpoint2 = m_Prm->joystickRateDemandParam.fCutpoint2;//0.7f;
	 	pPrm->joystickRateDemandParam.fInputGain_X2 =m_Prm->joystickRateDemandParam.fInputGain_X2;// 0.2f;
	 	pPrm->joystickRateDemandParam.fInputGain_Y2 =m_Prm->joystickRateDemandParam.fInputGain_Y2;// 0.3f;

	 	pPrm->joystickRateDemandParam.fDeadband =m_Prm->joystickRateDemandParam.fDeadband;// 0.1f;

	 	pPrm->joystickRateDemandParam.fPlatformAcquisitionModeGain_X = 5000.0f;
	 	pPrm->joystickRateDemandParam.fPlatformAcquisitionModeGain_Y = 5000.0f;

	 	pPrm->bTrkWinFilter = 1;

	 }

int CplatFormControl::PlatformCtrl_sensorCompensation(HPLTCTRL handle)
{
	PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle->object;
	pObj->privates.fovX = _Sensor->ZoomFovCompensation(_GlobalDate->rcv_zoomValue);
	pObj->privates.fovY = pObj->privates.fovX * pObj->params.sensorParams[pObj->inter.output.iSensor].fFovRatio;
	return 0;
}

void CplatFormControl::PlatformCtrl_Delete(HPLTCTRL handle)
{
    int i;
    PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle->object;
    if(pObj == NULL)
        return ;

    _Fiter->PlatformFilter_Delete(pObj->privates.filter[0][0]);
    _Fiter->PlatformFilter_Delete(pObj->privates.filter[0][1]);
    _Fiter->PlatformFilter_Delete(pObj->privates.filter[1][0]);
    _Fiter->PlatformFilter_Delete(pObj->privates.filter[1][1]);

    _Fiter->PlatformFilter_Delete(pObj->privates.joystickIntegrator[0]);
    _Fiter->PlatformFilter_Delete(pObj->privates.joystickIntegrator[1]);

    _Kalman->KalmanClose(pObj->privates.hWinFilter);

    for(i = 0; i < DevUsr_MAX; i++)
    	_DeviceUser->DeviceUser_Delete(pObj->privates.hDevUsers[i]);

    for(i = 0; i < SENSOR_COUNT; i++)
    	_Sensor->SensorComp_Delete(pObj->privates.hSensor[i]);
    free(pObj);
}

static int PlatformCtrl_PlatformCompensation(PlatformCtrl_Obj *pObj)
{
   static float fTmp;

    fTmp = pObj->privates.curRateDemandX;

    if(fTmp > 0 && fTmp > pObj->params.demandMaxX)
        fTmp = pObj->params.demandMaxX;

    if(fTmp < 0 && fTmp < -(pObj->params.demandMaxX))
        fTmp = -pObj->params.demandMaxX;

    if(fabsf(fTmp) < pObj->params.deadbandX)
    {
    	fTmp = 0;
    }

    pObj->inter.output.fPlatformDemandX = fTmp;

    //printf("pObj->inter.output.fPlatformDemandX  = %f \n",  pObj->inter.output.fPlatformDemandX);

    fTmp = pObj->privates.curRateDemandY;

    if(fTmp > 0 && fTmp > pObj->params.demandMaxY)
        fTmp = pObj->params.demandMaxY;

    if(fTmp < 0 && fTmp < -(pObj->params.demandMaxY))
        fTmp = -pObj->params.demandMaxY;

        if(fabsf(fTmp) < pObj->params.deadbandY)
        {
        	fTmp = 0;
        }

    pObj->inter.output.fPlatformDemandY = fTmp;
   // printf("pObj->inter.output.fPlatformDemandY  = %f \n", pObj->inter.output.fPlatformDemandY );

    return 0;
}

int CplatFormControl::PlatformCtrl_AcqRateDemand(PlatformCtrl_Obj *pObj)
{

    float fTmp;
    float fTmp2, K, K2;
    JoystickRateDemandParam *pParam = &pObj->params.joystickRateDemandParam;

    pObj->privates.curRateDemandX = 0.0f;
    pObj->privates.curRateDemandY = 0.0f;

    switch(pObj->params.acqOutputType)
    {
        case AcqOutputType_Zero:
            break;

        case AcqOutputType_JoystickInput:
            pObj->privates.curRateDemandX = pObj->inter.devUsrInput[DevUsr_AcqJoystickXInput];
            pObj->privates.curRateDemandY = pObj->inter.devUsrInput[DevUsr_AcqJoystickYInput];
            break;

        case AcqOutputType_ShapedAndGained:
        case AcqOutputType_ShapedAndGainedAndIntegrated:
        {
            //X Axis
            fTmp = pObj->inter.devUsrInput[DevUsr_AcqJoystickXInput];

            /*param limit*/
            if(pParam->fDeadband > pParam->fCutpoint1)
            	pParam->fDeadband = pParam->fCutpoint1;
            if(pParam->fCutpoint1 > pParam->fCutpoint2)
            	pParam->fCutpoint1 = pParam->fCutpoint2;

            if(pParam->fCutpoint2 > 1)
            	pParam->fCutpoint2 = 1;

            if(pParam->fCutpoint2 < 0)
            	pParam->fCutpoint2 = 0;
            if(pParam->fCutpoint1 < 0)
            	pParam->fCutpoint1 = 0;
            if(pParam->fDeadband < 0)
            	pParam->fDeadband = 0;

		if(fabsf(fTmp) < pParam->fDeadband)
			fTmp = 0.0;


		if(fabsf(fTmp) < pParam->fCutpoint1)
		{
			if(fTmp > 0){
				fTmp -= pParam->fDeadband;
				fTmp *= pParam->fInputGain_X1;
			}
			else if (fTmp < 0){
				fTmp = fabsf(fTmp);
				fTmp -= pParam->fDeadband;
				fTmp *= pParam->fInputGain_X1;	
				fTmp *= -1;
			}
		}
		else if(fabsf(fTmp) >= pParam->fCutpoint1 && fabsf(fTmp) < pParam->fCutpoint2)
		{
			if(fTmp >= 0){
				fTmp = pParam->fInputGain_X1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_X2*( fTmp - pParam->fCutpoint1);
			}else{
				fTmp = fabsf(fTmp);
				fTmp = pParam->fInputGain_X1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_X2*( fTmp - pParam->fCutpoint1);
				fTmp *= -1;
			}
		}
		else
		{
			if(fTmp > 0){
				fTmp2 = pParam->fInputGain_X1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_X2*( fTmp - pParam->fCutpoint1);
				K = 1 - fTmp2;
				K /= (1 - pParam->fCutpoint2);
				fTmp -= pParam->fCutpoint2;
				fTmp *= K ;
				fTmp += fTmp2;
			}
			else if (fTmp < 0){
				fTmp = fabsf(fTmp);
				fTmp2 = pParam->fInputGain_X1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_X2*( fTmp - pParam->fCutpoint1);
				K = 1 - fTmp2;
				K /= (1 - pParam->fCutpoint2);
				fTmp -= pParam->fCutpoint2;
				fTmp *= K ;
				fTmp += fTmp2;
				fTmp *= -1;
			}
		}

            fTmp *= pParam->fPlatformAcquisitionModeGain_X;

            if(pObj->params.acqOutputType == AcqOutputType_ShapedAndGainedAndIntegrated)
            {
                if(pObj->privates.acqOutputTypeBak != pObj->params.acqOutputType)
                	_Fiter->PlatformFilter_Reset(pObj->privates.joystickIntegrator[0]);

            }
            else{
            	_Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][0], 0, fTmp);
            }

            pObj->privates.curRateDemandX = fTmp;

            // Y Axis
            fTmp = pObj->inter.devUsrInput[DevUsr_AcqJoystickYInput];

#if 0

            if(fTmp > EPSINON)
                fTmp += pParam->fDeadband;

            if(fTmp < (-1)*EPSINON)
                fTmp -= pParam->fDeadband;

#else

            if(fabsf(fTmp) < pParam->fDeadband)
                fTmp = 0.0;

#endif

		if(fabsf(fTmp) < pParam->fCutpoint1)
		{
			if(fTmp > 0){
				fTmp -= pParam->fDeadband;
				fTmp *= pParam->fInputGain_Y1;
			}
			else if (fTmp < 0){
				fTmp = fabsf(fTmp);
				fTmp -= pParam->fDeadband;
				fTmp *= pParam->fInputGain_Y1;	
				fTmp *= -1;
			}
		}
		else if(fabsf(fTmp) >= pParam->fCutpoint1 && fabsf(fTmp) < pParam->fCutpoint2)
		{
			if(fTmp >= 0){
				fTmp = pParam->fInputGain_Y1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_Y2*( fTmp - pParam->fCutpoint1);
			}else{
				fTmp = fabsf(fTmp);
				fTmp = pParam->fInputGain_Y1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_Y2*( fTmp - pParam->fCutpoint1);
				fTmp *= -1;
			}
		}
		else
		{
			if(fTmp > 0){
				fTmp2 = pParam->fInputGain_Y1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_Y2*( fTmp - pParam->fCutpoint1);
				K = 1 - fTmp2;
				K /= (1 - pParam->fCutpoint2);
				fTmp -= pParam->fCutpoint2;
				fTmp *= K ;
				fTmp += fTmp2;
			}
			else if (fTmp < 0){
				fTmp = fabsf(fTmp);
				fTmp2 = pParam->fInputGain_Y1* (pParam->fCutpoint1- pParam->fDeadband)
					+ pParam->fInputGain_Y2*( fTmp - pParam->fCutpoint1);
				K = 1 - fTmp2;
				K /= (1 - pParam->fCutpoint2);
				fTmp -= pParam->fCutpoint2;
				fTmp *= K ;
				fTmp += fTmp2;
				fTmp *= -1;
			}
		}

           fTmp *= pParam->fPlatformAcquisitionModeGain_Y;

            if(pObj->params.acqOutputType == AcqOutputType_ShapedAndGainedAndIntegrated)
            {
                if(pObj->privates.acqOutputTypeBak != pObj->params.acqOutputType)
                	_Fiter->PlatformFilter_Reset(pObj->privates.joystickIntegrator[1]);

                //fTmp = PlatformFilter_Get(pObj->privates.joystickIntegrator[1], fTmp);
            }
            else{
            	_Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][1], 0, fTmp);
            }

            pObj->privates.curRateDemandY = fTmp;
        }
            break;

        case AcqOutputType_DeterminedByPosition:
            break;

        case AcqOutputType_ZeroInitFilter:
            break;

        case AcqOutputType_DeterminedByIncomingPlatformData:
            break;

        default:
            break;
    }

    return 0;
}


int CplatFormControl::PlatformCtrl_OutPlatformDemand(PlatformCtrl_Obj *pObj)
{
    PLATFORMCTRL_TrackerInput *pInput = &pObj->inter.trackerInput;
    static float fTmp, fTmpX,  fTmpY;

	if(pInput->iTrkAlgState == PlatStateType_Acquire)
	{
		PlatformCtrl_AcqRateDemand(pObj);

		if(pObj->privates.iTrkAlgStateBak != pInput->iTrkAlgState)
		{
			printf("=====================PlatForm Reset====================\n");
			_Fiter->PlatformFilter_Reset( pObj->privates.filter[pObj->privates.iFilter][0] );
			_Fiter->PlatformFilter_Reset( pObj->privates.filter[pObj->privates.iFilter][1] );
			pObj->privates.curRateDemandX = 0;
			pObj->privates.curRateDemandY = 0;
			fTmp = fTmpX = fTmpY = pInput->fTargetBoresightErrorX = pInput->fTargetBoresightErrorY = 0;
			_stateManager->StatusInit();
		}
	}

    if(pInput->iTrkAlgState == PlatStateType_Trk_Lock)
    {
    	detectionFunc(pObj);

        fTmpX = pInput->fTargetBoresightErrorX;
        fTmpY = pInput->fTargetBoresightErrorY;
       // printf("pInput->fTargetBoresightErrorX   = %f  , pInput->fTargetBoresightErrorY    = %f  \n",    pInput->fTargetBoresightErrorX,    pInput->fTargetBoresightErrorY);
        if(fabs(fTmpX) < 1.01)
        	fTmpX = 0;
        if(fabs(fTmpY) < 1.01)
        	fTmpY = 0;

       // printf("  fTmpX           =              %f   ,                               fTmpY   =         %f    \n", fTmpX, fTmpY);
        fTmp = fTmpX * pObj->privates.fovX / pObj->privates.width;

	if(_stateManager->isTrkPIDInitX()){
		_Fiter->PlatformFilter_init(pObj->privates.filter[pObj->privates.iFilter][0], pObj->privates.curRateDemandX, fTmp);
		_stateManager->trk_State_PIDInitEndX();
	}

        if(pObj->params.bleedUsed == Bleed_BrosightError )
        {
          if(_stateManager->isTimeOut_x() || _stateManager->isThree_CtrlPtz())
        	  pObj->privates.curRateDemandX = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][0], fTmp, 0);
          else
          {
        	  if(_stateManager->isThree_CtrlBox())
        	  {
        		  BleedToPID_X(pObj, fTmpX, fTmp);
        	  }
        	  else if(_stateManager->isThree_TrkSearch())
        	  {
        		  if(_GlobalDate->frame < 6)
        			  _GlobalDate->frame++;
        		  pObj->privates.curRateDemandX = pObj->params.Kx * fTmp;
        		  pObj->privates.curRateDemandX = (fabs(pObj->privates.curRateDemandX) > pObj->params.bleedX) ? \
        				  pObj->params.bleedX*fabs(pObj->privates.curRateDemandX)/pObj->privates.curRateDemandX : pObj->privates.curRateDemandX;
        		  if(_GlobalDate->frame > 4)
        		  {
        			  BleedToPID_X(pObj, fTmpX, fTmp);
        		  }
        	  }
          }

        }
        else
            pObj->privates.curRateDemandX = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][0], fTmp, 0);

        /*************************************/

        fTmp = fTmpY * pObj->privates.fovY / pObj->privates.height;

	if( _stateManager->isTrkPIDInitY()){
		
		_Fiter->PlatformFilter_init(pObj->privates.filter[pObj->privates.iFilter][1],
			pObj->privates.curRateDemandY, fTmp);
		_stateManager->trk_State_PIDInitEndY();
	}

        if(pObj->params.bleedUsed == Bleed_BrosightError )
        {
        	if(_stateManager->isTimeOut_Y() || _stateManager->isThree_CtrlPtz())
        		 pObj->privates.curRateDemandY = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][1], fTmp, 0);
        	else
        	{
        		if(_stateManager->isThree_CtrlBox())
        		{
        			BleedToPID_Y(pObj, fTmpY, fTmp);
        		}
        		else if(_stateManager->isThree_TrkSearch())
        		{
        			pObj->privates.curRateDemandY = pObj->params.Ky * fTmp;
        			pObj->privates.curRateDemandY  = (fabs(pObj->privates.curRateDemandY) > pObj->params.bleedY) ? \
        				pObj->params.bleedY*fabs(pObj->privates.curRateDemandY)/pObj->privates.curRateDemandY : pObj->privates.curRateDemandY;

        			if(_GlobalDate->frame > 4)
        			{
        				BleedToPID_Y(pObj, fTmpY, fTmp);
        			}
        		}
        	}
        }
        else
            pObj->privates.curRateDemandY = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][1], fTmp, 0);

    }

    if(pInput->iTrkAlgState == PlatStateType_Trk_Break_Lock_1)
    {

    }

	if(pInput->iTrkAlgState == PlatStateType_Trk_Break_Lock_2)
	{
		pObj->privates.curRateDemandX = 0;
		pObj->privates.curRateDemandY = 0;
		_Fiter->PlatformFilter_Reset( pObj->privates.filter[pObj->privates.iFilter][0] );
		_Fiter->PlatformFilter_Reset( pObj->privates.filter[pObj->privates.iFilter][1] );
	}

    PlatformCtrl_PlatformCompensation(pObj); /*  ******               ********        **********/

	pObj->privates.lastPlatStat = pInput->iTrkAlgState;

    return 0;
}

int CplatFormControl::PlatformCtrl_BuildDevUsrInput(PlatformCtrl_Obj *pObj)
{
    int i;

    for(i = 0; i < DevUsr_MAX; i++)
        pObj->inter.devUsrInput[i] = _DeviceUser->DeviceUser_Get(pObj->privates.hDevUsers[i]);

    return 0;
}

static int PlatformCtrl_ProccesDevUsrInput(PlatformCtrl_Obj *pObj)
{
    memcpy(pObj->privates.usrInputBak, pObj->inter.devUsrInput, sizeof(pObj->privates.usrInputBak));
    return 0;
}


/* ����ģ������ */
int CplatFormControl::PlatformCtrl_TrackerInput(HPLTCTRL handle, PLATFORMCTRL_TrackerInput *pInput)
{
    int iRet = 0;

    PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle->object;

    if(pObj == NULL || pInput == NULL)
        return -1;

    memcpy(&pObj->inter.trackerInput, pInput, sizeof(PLATFORMCTRL_TrackerInput));

    PlatformCtrl_BuildDevUsrInput(pObj);

    PlatformCtrl_OutPlatformDemand(pObj);

    pObj->inter.output.iTrkAlgState = pInput->iTrkAlgState;
    pObj->inter.output.fBoresightPositionX = pInput->fBoresightPositionX;
    pObj->inter.output.fBoresightPositionY = pInput->fBoresightPositionY;
    pObj->inter.output.fTargetBoresightErrorX = pInput->fTargetBoresightErrorX;
    pObj->inter.output.fTargetBoresightErrorY = pInput->fTargetBoresightErrorY;

    if(pInput->iTrkAlgState == 0 || pInput->iTrkAlgState == 1)
    {
        pObj->inter.output.fWindowPositionX = pInput->fAcqWindowPositionX;
        pObj->inter.output.fWindowPositionY = pInput->fAcqWindowPositionY;
        pObj->inter.output.fWindowSizeX = pInput->fAcqWindowSizeX;
        pObj->inter.output.fWindowSizeY = pInput->fAcqWindowSizeY;

        if(pObj->privates.iTrkAlgStateBak > 1)
        {
        	_Kalman->KalmanInitParam(pObj->privates.hWinFilter, 0.0, 0.0, 0, 0.0);
        }
    }
    else
    {
        pObj->inter.output.fWindowPositionX = pInput->fTargetBoresightErrorX;
        pObj->inter.output.fWindowPositionY = pInput->fTargetBoresightErrorY;
        pObj->inter.output.fWindowSizeX = pInput->fTargetSizeX;
        pObj->inter.output.fWindowSizeY = pInput->fTargetSizeY;

        if(pObj->params.bTrkWinFilter)
        {
            double dbM[3];
            dbM[0] = 0.0f;
            dbM[1] = pInput->fTargetBoresightErrorX;
            dbM[2] = pInput->fTargetBoresightErrorY;
            _Kalman->Kalman(pObj->privates.hWinFilter, dbM, NULL);

            pObj->inter.output.fWindowPositionX = (float)pObj->privates.hWinFilter->state_post[2];
            pObj->inter.output.fWindowPositionY = (float)pObj->privates.hWinFilter->state_post[4];
        }
    }

    PlatformCtrl_ProccesDevUsrInput(pObj);

    pObj->privates.iTrkAlgStateBak = pInput->iTrkAlgState;
    //printf("StateBak = %d\n",  pObj->privates.iTrkAlgStateBak);

    pObj->privates.acqOutputTypeBak = pObj->params.acqOutputType;

    return iRet;
}

int CplatFormControl::PlatformCtrl_TrackerOutput(HPLTCTRL handle, PLATFORMCTRL_Output *pOutput)
{
    int iRet = 0;

    PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle->object;

    if(pObj == NULL || pOutput == NULL)
        return -1;

    memcpy(pOutput, &pObj->inter.output, sizeof(PLATFORMCTRL_Output));

    return iRet;
}

/* �����豸�û��������� */
int CplatFormControl::PlatformCtrl_VirtualInput(HPLTCTRL handle, int iIndex, float fValue)
{
    float fTmp;

    PlatformCtrl_Obj *pObj = (PlatformCtrl_Obj*)handle->object;

    if(pObj == NULL)
        return -1;

    fTmp = fValue;

    _stateManager->AvtInput_Switch();

    if(_stateManager->isCloseExtInput())
    	pObj->inter.virtalInput[iIndex] = 0;
    else
       	pObj->inter.virtalInput[iIndex] = fTmp;
   // printf("pObj->inter.virtalInput[iIndex] = %f\n",   pObj->inter.virtalInput[iIndex]);
  //  printf("\n");
    return 0;
}

void CplatFormControl::BleedToPID_X(PlatformCtrl_Obj *pObj, float fTmpX, float fTmp)
{
	PLATFORMCTRL_TrackerInput *pInput = &pObj->inter.trackerInput;
    float out_x = 0.0;

	if (fTmpX > pObj->params.Error_X || fTmpX < -pObj->params.Error_X)
	{
		out_x = pObj->privates.curRateDemandX = pObj->params.Kx * fTmp;
		pObj->privates.curRateDemandX = (fabs(pObj->privates.curRateDemandX) > pObj->params.bleedX) ? \
		pObj->params.bleedX*fabs(pObj->privates.curRateDemandX)/pObj->privates.curRateDemandX : pObj->privates.curRateDemandX;
	}
	else
	{
		if(_stateManager->ACQ_PTZ_switchInitX == 1)
		{
			_Fiter->PlatformFilter_init(pObj->privates.filter[pObj->privates.iFilter][0],  out_x, fTmp);
			_stateManager->ACQ_PTZ_switchInitX = TRK_PID;
		}
		pObj->privates.curRateDemandX = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][0], fTmp, 0);
	}

}

void CplatFormControl::BleedToPID_Y(PlatformCtrl_Obj *pObj, float fTmpY, float fTmp)
{
    float out_y = 0.0;

	if (fTmpY > pObj->params.Error_Y || fTmpY < -pObj->params.Error_Y)
    {
    		out_y = pObj->privates.curRateDemandY = pObj->params.Ky * fTmp;
    		pObj->privates.curRateDemandY  = (fabs(pObj->privates.curRateDemandY) > pObj->params.bleedY) ? \
    		pObj->params.bleedY*fabs(pObj->privates.curRateDemandY)/pObj->privates.curRateDemandY : pObj->privates.curRateDemandY;
    }
    else
    {
    	if(_stateManager->ACQ_PTZ_switchInitY == 1)
    	{
    		_Fiter->PlatformFilter_init(pObj->privates.filter[pObj->privates.iFilter][1], out_y, fTmp);
    		_stateManager->ACQ_PTZ_switchInitY = TRK_PID;
    	}
    	pObj->privates.curRateDemandY = _Fiter->PlatformFilter_Get(pObj->privates.filter[pObj->privates.iFilter][1], fTmp, 0);
    }
}

void CplatFormControl::detectionFunc(PlatformCtrl_Obj *pObj)
{
	gettimeofday(&_GlobalDate->PID_t, NULL);
	if(_GlobalDate->time_stop < pObj->params.Time)
	{
		_GlobalDate->time_stop = _GlobalDate->PID_t.tv_sec - (_GlobalDate->time_start);
		_stateManager->TimeReset();
	}
	//printf("time_stop  = %ld,      pObj->params.Time = %d\n", _GlobalDate->time_stop, pObj->params.Time);
	if (_GlobalDate->time_stop >=  pObj->params.Time)
		_stateManager->TimeOut();
}







