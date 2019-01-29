#ifndef __CPORTINTERFACE_H__
#define __CPORTINTERFACE_H__

#include "CMessage.hpp"
#include "globalDate.h"
#include "stateCtrl.h"

class CPortInterface
{
public:
    virtual ~CPortInterface(){};
    virtual int create(){return 0;};
    virtual int closePort(){return 0;};
    virtual void run()=0;
    virtual int recvData(){return 0;};
    virtual int sendData(){return 0;};

    virtual void EnableSelfTest(){};
    virtual void EnableswitchVideoChannel(){};
    virtual void selectVideoChannel(){};
    virtual void EnableTrk(){};
    virtual void SwitchSensor(){};
    virtual void EnableParamBackToDefault(){};
    virtual void AxisMove(){};
    virtual void EnableTrkSearch(){};
    virtual void Enablealgosdrect(){};
    virtual void ZoomSpeedCtrl(){};
    virtual int ZoomShortCtrl(){return 0;};
    virtual int ZoomLongCtrl(){return 0;};
    virtual void IrisDown(){};
    virtual void IrisUp(){};
    virtual void FocusDown(){};
    virtual void FocusUp(){};
    virtual void AvtAxisCtrl(){};
    virtual void EnableOsdbuffer(){};
    virtual void EnablewordType(){};
    virtual void EnablewordSize(){};
    virtual void Config_Write_Save(){};
    virtual void Config_Read(){};
    virtual void EnableSavePro(){};
    virtual void AXISCtrl(){};
    virtual void Preset_Mtd(){};
    virtual void MtdAreaBox(){};
    virtual void OSDSwitch(){};
    virtual void callPreset(){};
    virtual void camera_setPan(){};
    virtual void camera_setTilt(){};
    virtual void camera_setZoom(){};
    virtual void camera_queryPan(){};
    virtual void camera_queryTilt(){};
    virtual void camera_queryZoom(){};
    virtual void registfunc(){};
    virtual CMessage *getpM(){return 0;};
    virtual CGlobalDate* getDate()=0;
    virtual CStatusManager* getStatus()=0;
};

#endif
