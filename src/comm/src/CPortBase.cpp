#include "CPortBase.hpp"

CGlobalDate* CPortBase::_globalDate = 0;

CStatusManager* CPortBase:: _StateManager = 0;

CPortBase::CPortBase()
{
	pM = getpM();
	_globalDate = getDate();
	_StateManager = getStatus();
}

CPortBase::~CPortBase()
{

}

CMessage * CPortBase::getpM()
{
    pM =  CMessage::getInstance();
    return pM;
}

CGlobalDate* CPortBase::getDate()
{
	_globalDate = CGlobalDate::Instance();
	return _globalDate;
}

CStatusManager* CPortBase::getStatus()
{
	_StateManager = CStatusManager::Instance();
	return _StateManager;
}

void CPortBase::EnableSelfTest()
{
    printf("send %s msg\n",__FUNCTION__);
}

void CPortBase::EnableswitchVideoChannel()
{
    _globalDate->Host_Ctrl[chooseVideoChannel] =  _globalDate->rcvBufQue.at(5);
    pM->MSGDRIV_send(MSGID_IPC_chooseVideoChannel, 0);
}

void CPortBase:: selectVideoChannel()
{
    char  selectChannel = _globalDate->rcvBufQue.at(5);
	_globalDate->selectch.idx = 0;
	for(int i = 0; i < 5;i++)
	{
		_globalDate->selectch.validCh[i] = false;
	}
	for(int i = 0; i < 5;i++)
	{
		if((selectChannel>>i) & 0x1 == 1 )
			_globalDate->selectch.validCh[i] = true;
	}
}

void CPortBase::EnableTrk()
{
	if(_globalDate->mtdMode == 0)
		pM->MSGDRIV_send(MSGID_EXT_INPUT_TRACKCTRL, 0);
}

void CPortBase::SwitchSensor()
{
    pM->MSGDRIV_send(MSGID_EXT_INPUT_SwitchSensor, 0);
}

void CPortBase::AIMPOS_X()
{
	_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_X]=0;

    switch(_globalDate->rcvBufQue.at(5)){
		case 0x1:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_X]=1;
			break;
		case 0x2:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_X]=2;
			break;
		default:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_X]=0;
	}

    pM->MSGDRIV_send(MSGID_EXT_INPUT_AIMPOSXCTRL, 0);
}

void CPortBase::AIMPOS_Y()
{
	_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_Y]=0;
    switch(_globalDate->rcvBufQue.at(6)){
		case 0x1:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_Y]=1;
			break;
		case 0x2:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_Y]=2;
			break;
		default:
			_globalDate->EXT_Ctrl[Cmd_Mesg_AIMPOS_Y]=0;
    }

    pM->MSGDRIV_send(MSGID_EXT_INPUT_AIMPOSYCTRL, 0);
}

void CPortBase::EnableParamBackToDefault()
{
	int id = _globalDate->rcvBufQue.at(5);
	if(0x0 == id)
		id = 255;
	_globalDate->defConfigBuffer.push_back(id);
    pM->MSGDRIV_send(MSGID_EXT_INPUT_PRMBACK, 0);
}

void CPortBase::AxisMove()
{
	_globalDate->Host_Ctrl[moveAxisX]=0;
	_globalDate->Host_Ctrl[moveAxisY]=0;
    switch(_globalDate->rcvBufQue.at(5)){
		case 0x1:
			_globalDate->Host_Ctrl[moveAxisX]=1;
			break;
		case 0x2:
			_globalDate->Host_Ctrl[moveAxisX]=2;
			break;
		default:
			_globalDate->Host_Ctrl[moveAxisX]=0;
     }
    switch(_globalDate->rcvBufQue.at(6)){
		case 0x1:
			_globalDate->Host_Ctrl[moveAxisY]=1;
			break;
		case 0x2:
			_globalDate->Host_Ctrl[moveAxisY]=2;
			break;
		default:
			_globalDate->Host_Ctrl[moveAxisY]=0;
	}

    pM->MSGDRIV_send(MSGID_IPC_AxisMove, 0);
}

void CPortBase::EnableTrkSearch()
{
	int tarkSearkSwitch,trakSearchx,trackSearchy;
	trakSearchx=trackSearchy = 0;
	tarkSearkSwitch = _globalDate->rcvBufQue.at(5);
	trakSearchx	=  (_globalDate->rcvBufQue.at(6)|_globalDate->rcvBufQue.at(7)<<8);
	trackSearchy	=  (_globalDate->rcvBufQue.at(8)|_globalDate->rcvBufQue.at(9)<<8);

	if(tarkSearkSwitch == 1)
		tarkSearkSwitch = 3;
	else if(tarkSearkSwitch == 2)
		tarkSearkSwitch = 4;
	_globalDate->EXT_Ctrl[Cmd_Mesg_TrkSearch] =  tarkSearkSwitch;
	_globalDate->EXT_Ctrl[Cmd_Mesg_AXISX] 	 =  trakSearchx;
	_globalDate->EXT_Ctrl[Cmd_Mesg_AXISY] 	 =  trackSearchy;

    pM->MSGDRIV_send(MSGID_EXT_INPUT_TRACKSEARCHCTRL, 0);
}

void CPortBase::Enablealgosdrect()
{
	int tmp = _globalDate->rcvBufQue.at(5);
	_globalDate->EXT_Ctrl[Cmd_IPC_Algosdrect] = tmp;
    pM->MSGDRIV_send(MSGID_IPC_Algosdrect, 0);
}

void CPortBase::ZoomSpeedCtrl()
{
	_globalDate->Host_Ctrl[zoomSpeed] = _globalDate->rcvBufQue.at(5);
    pM->MSGDRIV_send(MSGID_EXT_INPUT_zoomSpeed, 0);
}

int CPortBase::ZoomShortCtrl()
{
	if(_globalDate->rcvBufQue.at(5) == 0x02)
		return 0;
	if(_globalDate->rcvBufQue.at(5) == 0x01)
		_globalDate->EXT_Ctrl.at(Cmd_Mesg_ZoomShort) = 1;
	else
	    _globalDate->EXT_Ctrl.at(Cmd_Mesg_ZoomShort) = 0;

    pM->MSGDRIV_send(MSGID_EXT_INPUT_OPTICZOOMSHORTCTRL, 0);
    return 0;
}

int CPortBase::ZoomLongCtrl()
{
	if(_globalDate->rcvBufQue.at(5) == 0x01)
		return 0;
	if(_globalDate->rcvBufQue.at(5) == 0x02)
		_globalDate->EXT_Ctrl.at(Cmd_Mesg_ZoomLong) = 1;
	else
	    _globalDate->EXT_Ctrl.at(Cmd_Mesg_ZoomLong) = 0;

    pM->MSGDRIV_send(MSGID_EXT_INPUT_OPTICZOOMLONGCTRL, 0);
    return 0;
}

void CPortBase::IrisDown()
{
	if(_globalDate->rcvBufQue.at(5) == 0x01)
		_globalDate->EXT_Ctrl[Cmd_Mesg_IrisDown] = 1;
	else
		_globalDate->EXT_Ctrl[Cmd_Mesg_IrisDown] = 0;
    pM->MSGDRIV_send(MSGID_EXT_INPUT_IRISDOWNCTRL, 0);
}

void CPortBase::IrisUp()
{
	if(_globalDate->rcvBufQue.at(5) == 0x02)
		_globalDate->EXT_Ctrl[Cmd_Mesg_IrisUp] = 1;
	else
		_globalDate->EXT_Ctrl[Cmd_Mesg_IrisUp] = 0;
    pM->MSGDRIV_send(MSGID_EXT_INPUT_IRISUPCTRL, 0);
}

void CPortBase::FocusDown()
{
	if(_globalDate->rcvBufQue.at(5) == 0x01)
		_globalDate->EXT_Ctrl[Cmd_Mesg_FocusNear] = 1;
	else
		_globalDate->EXT_Ctrl[Cmd_Mesg_FocusNear] = 0;

    pM->MSGDRIV_send(MSGID_EXT_INPUT_FOCUSNEARCTRL, 0);
}

void CPortBase::FocusUp()
{
	if(_globalDate->rcvBufQue.at(5) == 0x02)
		_globalDate->EXT_Ctrl[Cmd_Mesg_FocusFar] = 1;
	else
		_globalDate->EXT_Ctrl[Cmd_Mesg_FocusFar] = 0;
    pM->MSGDRIV_send(MSGID_EXT_INPUT_FOCUSFARCHCTRL, 0);
}

void  CPortBase::AvtAxisCtrl()
{
	int AcqStat = _globalDate->rcvBufQue.at(5);
	if(AcqStat == 1)
		_StateManager->three_State_CtrlBox();
	else if(AcqStat == 2)
		_StateManager->three_State_CtrlPtz();
	pM->MSGDRIV_send(MSGID_EXT_INPUT_AxisCtrl, 0);
}

void CPortBase::EnableOsdbuffer()
{
	unsigned char contentBuff[128]={0};
	_globalDate->recvOsdbuf.osdID = _globalDate->rcvBufQue.at(5);
	_globalDate->recvOsdbuf.ctrl =  _globalDate->rcvBufQue.at(6);
	_globalDate->recvOsdbuf.posx = _globalDate->rcvBufQue.at(7)|(_globalDate->rcvBufQue.at(8)<<8);
	_globalDate->recvOsdbuf.posy = _globalDate->rcvBufQue.at(9)|(_globalDate->rcvBufQue.at(10)<<8);
	_globalDate->recvOsdbuf.color = _globalDate->rcvBufQue.at(11);
	_globalDate->recvOsdbuf.alpha = _globalDate->rcvBufQue.at(12);

	int length= (_globalDate->rcvBufQue.at(2)|_globalDate->rcvBufQue.at(3)<<8)+5;
	for(int i=0;i<length-14;i++){
		contentBuff[i]= _globalDate->rcvBufQue.at(13+i);
	}
	memcpy((void *)_globalDate->recvOsdbuf.buf,(void *)contentBuff,sizeof(contentBuff));
    pM->MSGDRIV_send(MSGID_IPC_word, 0);
}

void CPortBase::EnablewordType()
{
	int  fontStyle = _globalDate->rcvBufQue.at(5);
	_globalDate->Host_Ctrl[wordType]= fontStyle;
    pM->MSGDRIV_send(MSGID_IPC_WORDFONT, 0);
}

void CPortBase::EnablewordSize()
{
    int  fontSize = _globalDate->rcvBufQue.at(6);
    _globalDate->Host_Ctrl[wordSize]= fontSize;
    pM->MSGDRIV_send(MSGID_IPC_WORDSIZE, 0);
}

void CPortBase::Config_Write_Save()
{
	uint8_t  tempbuf[4];
	memset(&_globalDate->avtSetting,0,sizeof(_globalDate->avtSetting));
	_globalDate->avtSetting.cmdBlock = _globalDate->rcvBufQue.at(5) ;
	_globalDate->avtSetting.cmdFiled = _globalDate->rcvBufQue.at(6) ;
	for(int m=7;m<11;m++)
		tempbuf[m-7] = _globalDate->rcvBufQue.at(m);
	memcpy(&_globalDate->avtSetting.confitData,tempbuf,sizeof(float));

	_globalDate->Host_Ctrl[config_Wblock]	=	_globalDate->avtSetting.cmdBlock;
	_globalDate->Host_Ctrl[config_Wfield]	=	_globalDate->avtSetting.cmdFiled;
	_globalDate->Host_Ctrl[config_Wvalue]	=	_globalDate->avtSetting.confitData;
    pM->MSGDRIV_send(MSGID_EXT_INPUT_configWrite, 0);
}

void CPortBase::Config_Read()
{
	Read_config_buffer tmp;
	systemSetting currtSetting;
	memset(&currtSetting,0,sizeof(currtSetting));
	currtSetting.cmdBlock=_globalDate->rcvBufQue.at(5);
	currtSetting.cmdFiled=_globalDate->rcvBufQue.at(6);

	tmp.block = currtSetting.cmdBlock;
	tmp.filed = currtSetting.cmdFiled;
	_globalDate->readConfigBuffer.push_back(tmp);

    pM->MSGDRIV_send(MSGID_EXT_INPUT_config_Read, 0);
	//printf("tmp.block = %d, tmp.filed = %d\n", tmp.block, tmp.filed);
}

void CPortBase::EnableSavePro()
{
    printf("send %s msg\n",__FUNCTION__);
    pM->MSGDRIV_send(MSGID_EXT_INPUT_configWrite_Save, 0);
}

void CPortBase::AXISCtrl()
{
    	int type= _globalDate->rcvBufQue.at(5);
    	int Axis = _globalDate->rcvBufQue.at(6);
    	short value =  (_globalDate->rcvBufQue.at(7)|_globalDate->rcvBufQue.at(8)<<8);
	switch(type){
		case 0x01:
			printf("button \n");
			break;

		case 0x02:
			if(Axis)
				_globalDate->EXT_Ctrl.at(Cmd_Mesg_AXISY) = value;
			else if(Axis == 0)
				_globalDate->EXT_Ctrl.at(Cmd_Mesg_AXISX) = value;
			break;

		default:
			break;
	}
	pM->MSGDRIV_send(MSGID_EXT_INPUT_PLATCTRL, 0);
}

void CPortBase::Preset_Mtd()
{
	pM->MSGDRIV_send(MSGID_EXT_INPUT_MtdPreset, 0);
}

void CPortBase::MtdAreaBox()
{
	_globalDate->MtdSetAreaBox = _globalDate->rcvBufQue.at(5);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_MtdAreaBox, 0);
}

void CPortBase::OSDSwitch()
{
	_globalDate->osdUserSwitch = _globalDate->rcvBufQue.at(5);
	pM->MSGDRIV_send(MSGID_IPC_UserOSD, 0);
}

void CPortBase::callPreset()
{
	pM->MSGDRIV_send(MSGID_EXT_INPUT_CallPreset, 0);
}

void CPortBase::camera_setPan()
{
	int chId = _globalDate->chid_camera;
	_globalDate->m_camera_pos[chId].setpan =  (_globalDate->rcvBufQue.at(5) |_globalDate->rcvBufQue.at(6)<<8);
	printf("_globalDate->m_camera_pos[%d].setpan = %d \n", chId, _globalDate->m_camera_pos[chId].setpan);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_setPan, 0);
}

void CPortBase::camera_setTilt()
{
	int chId = _globalDate->chid_camera;
	_globalDate->m_camera_pos[chId].settilt = (_globalDate->rcvBufQue.at(5) |_globalDate->rcvBufQue.at(6)<<8);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_setTilt, 0);
}

void CPortBase::camera_setZoom()
{
	int chId = _globalDate->chid_camera;
	_globalDate->m_camera_pos[chId].setzoom =  (_globalDate->rcvBufQue.at(5) |_globalDate->rcvBufQue.at(6)<<8);
	printf("_globalDate->m_camera_pos[%d].setzoom = %d \n", chId, _globalDate->m_camera_pos[chId].setzoom);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_setZoom, 0);
}

void CPortBase::camera_queryPan()
{
	int value = _globalDate->rcvBufQue.at(5);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_queryPan, 0);
}

void CPortBase::camera_queryTilt()
{
	int value = _globalDate->rcvBufQue.at(5);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_queryTilt, 0);
}

void CPortBase::camera_queryZoom()
{
	int value = _globalDate->rcvBufQue.at(5);
	pM->MSGDRIV_send(MSGID_EXT_INPUT_queryZoom, 0);
}

int CPortBase::prcRcvFrameBufQue(int method)
{
    int ret =  -1;
    int cmdLength= (_globalDate->rcvBufQue.at(2)|_globalDate->rcvBufQue.at(3)<<8)+5;

    if(cmdLength<6){
        printf("Warning::Invaild frame\r\n");
        _globalDate->rcvBufQue.erase(_globalDate->rcvBufQue.begin(),_globalDate->rcvBufQue.begin()+cmdLength);
        return ret;
    }
    unsigned char checkSum = check_sum(cmdLength);

    if(checkSum== _globalDate->rcvBufQue.at(cmdLength-1))
    {	
    	_globalDate->commode = method;
        switch(_globalDate->rcvBufQue.at(4))
        {
            case 0x01:
                //startSelfCheak();
                break;
            case 0x02:
                EnableswitchVideoChannel();
                break;
            case 0x03:
                selectVideoChannel();
                break;
            case 0x04:
                EnableTrk();
                break;
            case 0x05:
                SwitchSensor();
                break;
            case 0x06:
                break;
            case 0x08:
                AIMPOS_X();
                AIMPOS_Y();
                break;
            case 0x09:
                EnableParamBackToDefault();
                break;
            case 0x0a:
                AxisMove();
                break;
            case 0x0b:
                EnableTrkSearch();
                break;
            case 0x0d:
                Enablealgosdrect();
                break;
            case 0x11:                             
                ZoomSpeedCtrl();
                break;
            case 0x12:
                ZoomShortCtrl();
                ZoomLongCtrl();
                break;
            case 0x13:
                IrisDown();
                IrisUp();
                break;
            case 0x14:
                FocusDown();
                FocusUp();
                break;
            case 0x15:
            	AXISCtrl();
                break;
            case 0x16:
                //wait to finish
                break;
            case 0x17:
                //wait to finish
                break;
            case 0x18:
                AvtAxisCtrl();
                break;
            case 0x20:
                EnableOsdbuffer();
                break;
            case 0x21:
                EnablewordType();
                EnablewordSize();
                break;
            case 0x30:
                Config_Write_Save();
                break;
            case 0x31:
                Config_Read();
                break;
            case 0x32:
                //provided by a single server
                break;
            case 0x33:
                //provided by a single server
                break;
            case 0x34:
                EnableSavePro();
                break;
            case 0x35:
                //provided by a single server
                break;
            case 0x36:

                break;
            case 0x41:
            	Preset_Mtd();
            	break;
            case 0x47:
            	MtdAreaBox();
            	break;
            case 0x48:
            	OSDSwitch();
            	break;
            case 0x49:
            	callPreset();
            	break;
            case 0x57:
            	camera_setPan();
            	break;
            case 0x58:
            	camera_setTilt();
            	break;
            case 0x59:
            	camera_setZoom();
            	break;
            case 0x60:
            	camera_queryPan();
            	break;
            case 0x61:
            	camera_queryTilt();
            	break;
            case 0x62:
            	camera_queryZoom();
            	break;
            default:
                printf("INFO: Unknow  Control Command, please check !\r\n ");
                ret =0;
                break;
        }
    }
    else
        printf("%s,%d, checksum error:,cal is %02x,recv is %02x\n",__FILE__,__LINE__,checkSum,_globalDate->rcvBufQue.at(cmdLength-1));
    _globalDate->rcvBufQue.erase(_globalDate->rcvBufQue.begin(),_globalDate->rcvBufQue.begin()+cmdLength);
    return 1;
}

int  CPortBase::getSendInfo(int  respondId, sendInfo * psendBuf)
{

	switch(respondId){
		case ACK_selfTest:
			//startCheckAnswer(psendBuf);
			break;
		case ACK_wordColor:
			break;
		case   ACK_wordType:
			break;
		case  ACK_wordSize:
			break;
		case  NAK_wordColor:
			break;
		case   NAK_wordType:
			break;
		case  NAK_wordSize:
			break;
		case   ACK_mainVideoStatus:
			mainVedioChannel(psendBuf);
			break;
		case   ACK_Channel_bindingStatus:
			bindVedioChannel(psendBuf);
			break;
		case  ACK_avtStatus:
			AVTStatus(psendBuf);
			break;
		case   ACK_avtErrorOutput:
			trackErrOutput(psendBuf);
			break;
		case 	ACK_avtSpeedComp:
		    trackSpeedComp(psendBuf);
			break;
		case ACK_avtSpeedLv:
			trackSpeedLv(psendBuf);
			break;
		case  ACK_mmtStatus:
			mutilTargetNoticeStatus(psendBuf);
			break;
		case   ACK_mmtSelectStatus:
			multilTargetNumSelectStatus(psendBuf);
			break;
		case   ACK_EnhStatus:
			imageEnhanceStatus(psendBuf);
			break;
		case   ACK_MtdStatus:
			moveTargetDetectedStat(psendBuf);
			break;
		case   ACK_TrkSearchStatus:
			trackSearchStat(psendBuf);
			break;
		case   ACK_posMoveStatus:
			trackFinetuningStat(psendBuf);
			break;
		case   ACK_moveAxisStatus:
			confirmAxisStat(psendBuf);
			break;
		case   ACK_ElectronicZoomStatus:
			ElectronicZoomStat(psendBuf);
			break;
		case  ACK_picpStatus:
			pictureInPictureStat(psendBuf);
			break;
		case  ACK_VideoChannelStatus:
			vedioTransChannelStat(psendBuf);
			break;
		case  ACK_frameCtrlStatus:
			break;
		case  ACK_compression_quality:
			break;
		case ACK_config_Write:
			settingCmdRespon(psendBuf);
			break;
		case ACK_config_Read:
			readConfigSetting(psendBuf);
			break;
		case ACK_jos_Kboard:
			extExtraInputResponse(psendBuf);
			break;
		case ACK_upgradefw:
			printf("%s,%d, upgradefw response!!!\n",__FILE__,__LINE__);
			upgradefwStat(psendBuf);
			break;
		case ACK_param_todef:
			paramtodef(psendBuf);
			break;
		case ACK_pan:
			ack_pan(psendBuf);
			break;
		case ACK_Tilt:
			ack_Tilt(psendBuf);
			break;
		case ACK_zoom:
			ack_zoom(psendBuf);
			break;
	}
	return 0;
}

void  CPortBase:: mainVedioChannel(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  mainVedioChannel[msg_length+5];
	mainVedioChannel[4] = 0x04;
	mainVedioChannel[5]=_globalDate->avt_status.SensorStat;
	package_frame(msg_length, mainVedioChannel);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,mainVedioChannel,msg_length+5);
}

void  CPortBase:: bindVedioChannel(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  restartCheckAnswer[msg_length+5];
	restartCheckAnswer[4] = 0x05;
	restartCheckAnswer[5]=0x07;
	package_frame(msg_length, restartCheckAnswer);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,restartCheckAnswer,msg_length+5);
}

void  CPortBase::AVTStatus(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  retrackStatus[msg_length+5];
	retrackStatus[4] = 0x05;
	retrackStatus[5]=_globalDate->avtStatus;
	package_frame(msg_length, retrackStatus);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,retrackStatus,msg_length+5);
}

void  CPortBase:: trackErrOutput(sendInfo * spBuf)
{
	u_int8_t sumCheck;
	spBuf->sendBuff[0]=0xEB;
	spBuf->sendBuff[1]=0x53;
	spBuf->sendBuff[2]=0x05;
	spBuf->sendBuff[3]=0x00;
	spBuf->sendBuff[4]=0x06;
	spBuf->sendBuff[5]=_globalDate->errorOutPut[0]&0xff;
	spBuf->sendBuff[6]=(_globalDate->errorOutPut[0]>>8)&0xff;
	spBuf->sendBuff[7]=_globalDate->errorOutPut[1]&0xff;
	spBuf->sendBuff[8]=(_globalDate->errorOutPut[1]>>8)&0xff;
	sumCheck=sendCheck_sum(8,spBuf->sendBuff+1);
	spBuf->sendBuff[9]=(sumCheck&0xff);
	spBuf->byteSizeSend=10;
}

void CPortBase::trackSpeedComp(sendInfo * spBuf)
{
	u_int8_t sumCheck;
	spBuf->sendBuff[0]=0xEB;
	spBuf->sendBuff[1]=0x53;
	spBuf->sendBuff[2]=0x05;
	spBuf->sendBuff[3]=0x00;
	spBuf->sendBuff[4]=0x07;
	spBuf->sendBuff[5]=_globalDate->speedComp[0]&0xff;
	spBuf->sendBuff[6]=(_globalDate->speedComp[0]>>8)&0xff;
	spBuf->sendBuff[7]=_globalDate->speedComp[1]&0xff;
	spBuf->sendBuff[8]=(_globalDate->speedComp[1]>>8)&0xff;
	sumCheck=sendCheck_sum(8,spBuf->sendBuff+1);
	spBuf->sendBuff[9]=(sumCheck&0xff);
	spBuf->byteSizeSend=10;

}
void CPortBase::trackSpeedLv(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  retrackSpeedLv[msg_length+5];
	retrackSpeedLv[4] = 0x08;
	retrackSpeedLv[5]= _globalDate->speedLv[0];
	retrackSpeedLv[6]= _globalDate->speedLv[1];
	package_frame(msg_length, retrackSpeedLv);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,retrackSpeedLv,msg_length+5);
}

void  CPortBase:: mutilTargetNoticeStatus(sendInfo * spBuf)
{
	int msg_length = 2;
	channelTable    mutilTargetSta;
	u_int8_t  remutilTargetNoticeStatus[msg_length+5];
	mutilTargetSta.channel0=_globalDate->avt_status.MtdState[0];
	mutilTargetSta.channel1=_globalDate->avt_status.MtdState[1];
	mutilTargetSta.channel2=_globalDate->avt_status.MtdState[2];
	mutilTargetSta.channel3=_globalDate->avt_status.MtdState[3];
	mutilTargetSta.channel4=_globalDate->avt_status.MtdState[4];
	mutilTargetSta.channel5=_globalDate->avt_status.MtdState[5];
	remutilTargetNoticeStatus[4] = 0x09;
	remutilTargetNoticeStatus[5]=*(u_int8_t*)(&mutilTargetSta);
	package_frame(msg_length, remutilTargetNoticeStatus);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,remutilTargetNoticeStatus,msg_length+5);
}

void CPortBase::multilTargetNumSelectStatus(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  tmpbuf[msg_length+5];
	tmpbuf[4] = 0x0a;
	tmpbuf[5]=(u_int8_t) (_globalDate->mainProStat[ACK_mmtSelect_value]&0xff);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void  CPortBase::imageEnhanceStatus(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  tmpbuf[msg_length+5];
	channelTable    enhanceChannelSta;
	enhanceChannelSta.channel0=_globalDate->avt_status.ImgEnhStat[0];
	enhanceChannelSta.channel1=_globalDate->avt_status.ImgEnhStat[1];
	enhanceChannelSta.channel2=_globalDate->avt_status.ImgEnhStat[2];
	enhanceChannelSta.channel3=_globalDate->avt_status.ImgEnhStat[3];
	enhanceChannelSta.channel4=_globalDate->avt_status.ImgEnhStat[4];
	enhanceChannelSta.channel5=_globalDate->avt_status.ImgEnhStat[5];
	tmpbuf[4] = 0x0b;
	tmpbuf[5]=*(u_int8_t*)(&enhanceChannelSta);;
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::trackFinetuningStat(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  retrackFinetuningStat[msg_length+5];
	retrackFinetuningStat[4] = 0x0c;
	retrackFinetuningStat[5] = (u_int8_t) (_globalDate->mainProStat[ACK_posMove_value]&0xff);
	retrackFinetuningStat[6] = (u_int8_t) (_globalDate->mainProStat[ACK_posMove_value+1]&0xff);
	package_frame(msg_length, retrackFinetuningStat);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,retrackFinetuningStat,msg_length+5);
}

void CPortBase::confirmAxisStat(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  tmpbuf[msg_length+5];
	tmpbuf[4] = 0x0d;
	tmpbuf[5]=(u_int8_t) (_globalDate->mainProStat[ACK_moveAxis_value]&0xff);
	tmpbuf[6]=(u_int8_t) (_globalDate->mainProStat[ACK_moveAxis_value+1]&0xff);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::ElectronicZoomStat(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  tmpbuf[msg_length+5];
	tmpbuf[4] = 0x0e;
	tmpbuf[5]=(u_int8_t) (_globalDate->mainProStat[ACK_ElectronicZoom_value]&0xff);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::trackSearchStat(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  retrackSearchStat[msg_length+5];
	retrackSearchStat[4] = 0x0f;
	unsigned int sectrkstat = _globalDate->avt_status.AvtTrkStat;
	if(sectrkstat == 4)
		sectrkstat = 1;
	else
		sectrkstat = 2;
	retrackSearchStat[5] = sectrkstat;
	package_frame(msg_length, retrackSearchStat);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,retrackSearchStat,msg_length+5);
}

void CPortBase::moveTargetDetectedStat(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  tmpbuf[msg_length+5];
	channelTable    currtmoveTargetDet;
	currtmoveTargetDet.channel0=_globalDate->avt_status.MtdState[0];
	currtmoveTargetDet.channel1=_globalDate->avt_status.MtdState[1];
	currtmoveTargetDet.channel2=_globalDate->avt_status.MtdState[2];
	currtmoveTargetDet.channel3=_globalDate->avt_status.MtdState[3];
	currtmoveTargetDet.channel4=_globalDate->avt_status.MtdState[4];
	currtmoveTargetDet.channel5=_globalDate->avt_status.MtdState[5];
	tmpbuf[4] = 0x10;
	tmpbuf[5]=*(u_int8_t*)(&currtmoveTargetDet);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::pictureInPictureStat(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  tmpbuf[msg_length+5];
	channelTable    currtPiPChanneStat;
	currtPiPChanneStat.channel0=_globalDate->avt_status.MtdState[0];
	currtPiPChanneStat.channel1=_globalDate->avt_status.MtdState[1];
	currtPiPChanneStat.channel2=_globalDate->avt_status.MtdState[2];
	currtPiPChanneStat.channel3=_globalDate->avt_status.MtdState[3];
	currtPiPChanneStat.channel4=_globalDate->avt_status.MtdState[4];
	currtPiPChanneStat.channel5=_globalDate->avt_status.MtdState[5];
	tmpbuf[4] = 0x11;
	tmpbuf[5] = _globalDate->avt_status.PicpSensorStat;
	tmpbuf[6] = *(u_int8_t*)(&currtPiPChanneStat);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void  CPortBase::vedioTransChannelStat(sendInfo * spBuf)
{
	int msg_length = 2;
	u_int8_t  tmpbuf[msg_length+5];
	channelTable    currtVedioTransChnel;
	currtVedioTransChnel.channel0=_globalDate->avt_status.ImgVideoTrans[0];
	currtVedioTransChnel.channel1=_globalDate->avt_status.ImgVideoTrans[1];
	currtVedioTransChnel.channel2=_globalDate->avt_status.ImgVideoTrans[2];
	currtVedioTransChnel.channel3=_globalDate->avt_status.ImgVideoTrans[3];
	currtVedioTransChnel.channel4=_globalDate->avt_status.ImgVideoTrans[4];
	currtVedioTransChnel.channel5=_globalDate->avt_status.ImgVideoTrans[5];
	tmpbuf[4] = 0x20;
	tmpbuf[5] = *(u_int8_t*)(&currtVedioTransChnel);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::frameFrequenceStat(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  tmpbuf[msg_length+5];
	int  frameFrequenceStat[2]={0};
	tmpbuf[4] = 0x21;
	tmpbuf[5] = frameFrequenceStat[0];
	tmpbuf[6] = frameFrequenceStat[1];
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::vedioCompressStat(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  tmpbuf[msg_length+5];
	int  vedioCompressStat[2]={0};
	tmpbuf[4] = 0x22;
	tmpbuf[5] = vedioCompressStat[0];
	tmpbuf[6] = vedioCompressStat[1];
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::settingCmdRespon(sendInfo * spBuf)
{
	int msg_length = 7;
	u_int8_t  tmpbuf[msg_length+5];
	tmpbuf[4] = 0x30;
	tmpbuf[5] = (u_int8_t) (_globalDate->mainProStat[ACK_config_Wblock]&0xff);
	tmpbuf[6] = (u_int8_t) (_globalDate->mainProStat[ACK_config_Wfield]&0xff);
	memcpy(tmpbuf+7,&(_globalDate->Host_Ctrl[config_Wvalue]),4);
	package_frame(msg_length, tmpbuf);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,tmpbuf,msg_length+5);
}

void CPortBase::readConfigSetting(sendInfo * spBuf)
{
	u_int8_t sumCheck;
	u_int8_t  readCfgSetting[8];
	float tmp ;
	int msglen = 0x07;
	spBuf->byteSizeSend = 0x0c;

	int len ;
	if(_globalDate->choose){
		len = strlen(_globalDate->ACK_read_content);
		msglen = msglen -4 + len;
		spBuf->byteSizeSend = msglen + 5;
	}

	spBuf->sendBuff[0] = 0xEB;
	spBuf->sendBuff[1] = 0x53;
	spBuf->sendBuff[2]= msglen&0xff;
	spBuf->sendBuff[3]= (msglen>>8)&0xff;
	spBuf->sendBuff[4]= 0x31;
	spBuf->sendBuff[5]= (u_int8_t) (_globalDate->mainProStat[ACK_config_Rblock]&0xff);
	spBuf->sendBuff[6]= (u_int8_t) (_globalDate->mainProStat[ACK_config_Rfield]&0xff);

	if(_globalDate->choose){
		memcpy((void*)&spBuf->sendBuff[7],(void*)_globalDate->ACK_read_content,len);
		sumCheck=sendCheck_sum(msglen+3, spBuf->sendBuff+1);
		spBuf->sendBuff[msglen+4]=(sumCheck&0xff);
		_globalDate->choose = 0;
	}else{
		tmp = _globalDate->ACK_read[0];
	//	printf("%s: %d      block = %d , field = %d ,value = %f \n",__func__,__LINE__,spBuf->sendBuff[5],spBuf->sendBuff[6],tmp);
		memcpy(spBuf->sendBuff+7,&tmp,4);
		sumCheck=sendCheck_sum(msglen+3, spBuf->sendBuff+1);
		spBuf->sendBuff[msglen+4]=(sumCheck&0xff);
		_globalDate->ACK_read.erase(_globalDate->ACK_read.begin());
	}

}

void CPortBase::extExtraInputResponse(sendInfo * spBuf)
{

}


void  CPortBase:: upgradefwStat(sendInfo * spBuf)
{
	u_int8_t sumCheck;
	u_int8_t trackStatus[3];
	spBuf->sendBuff[0]=0xEB;
	spBuf->sendBuff[1]=0x53;
	spBuf->sendBuff[2]=0x03;
	spBuf->sendBuff[3]=0x00;
	spBuf->sendBuff[4]=0x35;
	spBuf->sendBuff[5]=_globalDate->respupgradefw_stat;
	spBuf->sendBuff[6]=_globalDate->respupgradefw_perc;
	sumCheck=sendCheck_sum(6,spBuf->sendBuff+1);
	spBuf->sendBuff[7]=(sumCheck&0xff);
	spBuf->byteSizeSend=0x08;
}
void  CPortBase:: paramtodef(sendInfo * spBuf)
{
	u_int8_t sumCheck;
	spBuf->sendBuff[0]=0xEB;
	spBuf->sendBuff[1]=0x53;
	spBuf->sendBuff[2]=0x02;
	spBuf->sendBuff[3]=0x00;
	spBuf->sendBuff[4]=0x0d;
	spBuf->sendBuff[5]= (u_int8_t) (_globalDate->mainProStat[ACK_config_Rblock]&0xff);
	sumCheck=sendCheck_sum(5,spBuf->sendBuff+1);
	spBuf->sendBuff[6]=(sumCheck&0xff);
	spBuf->byteSizeSend=0x07;
}

void CPortBase::ack_pan(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  curPan[msg_length+5];
	curPan[4] = 0x60;
	curPan[5]= _globalDate->Mtd_Moitor_X & 0xff;
	curPan[6] = (_globalDate->Mtd_Moitor_X>>8)&0xff;
	package_frame(msg_length, curPan);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,curPan,msg_length+5);
}

void CPortBase::ack_Tilt(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  curTilt[msg_length+5];
	curTilt[4] = 0x61;
	curTilt[5]= _globalDate->Mtd_Moitor_Y & 0xff;
	curTilt[6] = (_globalDate->Mtd_Moitor_Y>>8)&0xff;
	package_frame(msg_length, curTilt);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,curTilt,msg_length+5);
}

void CPortBase::ack_zoom(sendInfo * spBuf)
{
	int msg_length = 3;
	u_int8_t  curZoom[msg_length+5];
	curZoom[4] = 0x62;
	curZoom[5]= _globalDate->rcv_zoomValue& 0xff;
	curZoom[6] = (_globalDate->rcv_zoomValue>>8)&0xff;
	package_frame(msg_length, curZoom);
	spBuf->byteSizeSend=msg_length+5;
	memcpy(spBuf->sendBuff,curZoom,msg_length+5);
}

u_int8_t CPortBase:: sendCheck_sum(uint len, u_int8_t *tmpbuf)
{
	u_int8_t  ckeSum=0;
	for(int n=0;n<len;n++)
		ckeSum ^= tmpbuf[n];
	return ckeSum;
}

u_int8_t CPortBase:: package_frame(uint len, u_int8_t *tmpbuf)
{
	tmpbuf[0] = 0xEB;
	tmpbuf[1] = 0x53;
	tmpbuf[2] = len&0xff;
	tmpbuf[3] = (len>>8)&0xff;
	tmpbuf[len+4]= sendCheck_sum(len+3,tmpbuf+1);
	return 0;
}

unsigned char CPortBase::check_sum(int len_t)
{
    unsigned char cksum = 0;
    for(int n=1; n<len_t-1; n++)
    {
        cksum ^= _globalDate->rcvBufQue.at(n);
    }
    return  cksum;
}

FILE *fp = NULL;
FILE *fp2 = NULL;
int current_len = 0;
int current_len2 = 0;
int CPortBase::upgradefw(unsigned char *swap_data_buf, unsigned int swap_data_len)
{
	int status;
	int write_len;
	int file_len;

	int recv_len = swap_data_len-13;
	unsigned char buf[8] = {0xEB,0x53,0x03,0x00,0x35,0x00,0x00,0x00};

	unsigned char cksum = 0;
	for(int n=1;n<swap_data_len-1;n++)
	{
		cksum ^= swap_data_buf[n];
	}
	if(cksum !=swap_data_buf[swap_data_len-1])
	{
		printf("checksum error,cal is %02x, recv is %02x\n",cksum,swap_data_buf[swap_data_len-1]);
		return -1;
	}
	memcpy(&file_len,swap_data_buf+5,4);
	if(NULL==fp)
	{
		if(NULL ==(fp = fopen("dss_pkt.tar.gz","w")))
		{
			perror("fopen\r\n");
			return -1;
		}
		else
		{
			printf("openfile dss_pkt.tar.gz success\n");
		}
	}

	write_len = fwrite(swap_data_buf+12,1,recv_len,fp);
	fflush(fp);
	if(write_len < recv_len)
	{
		printf("Write upgrade tgz file error!\r\n");
		return -1;
	}
	current_len += recv_len;
	if(current_len == file_len)
	{
		current_len = 0;
		fclose(fp);
		fp = NULL;

		if(fw_update_runtar() == 0)
			_globalDate->respupgradefw_stat= 0x01;
		else
			_globalDate->respupgradefw_stat = 0x02;

		_globalDate->respupgradefw_perc = 100;

		status = update_startsh();
		printf("update start.sh return %d\n",status);

		status = update_fpga();
		printf("update load_fpga.ko return %d\n",status);

		system("sync");
		
		_globalDate->feedback=ACK_upgradefw;
		OSA_semSignal(&_globalDate->m_semHndl);
	}
	else
	{
		_globalDate->respupgradefw_stat= 0x00;
		_globalDate->respupgradefw_perc = current_len*100/file_len;
		_globalDate->feedback=ACK_upgradefw;
		OSA_semSignal(&_globalDate->m_semHndl);
	}

}
int CPortBase::fw_update_runtar(void)
{
	printf("tar zxvf dss_pkt.tar.gz...\r\n"); 
	int iRtn=-1;
	char cmdBuf[128];
	sprintf(cmdBuf, "tar -zxvf %s -C ~/", "dss_pkt.tar.gz");
	iRtn = system(cmdBuf);
	return iRtn;
}

int CPortBase::update_startsh(void)
{
	printf("update start.sh...\r\n"); 
	int iRtn=-1;
	char cmdBuf[128];	
	sprintf(cmdBuf, "mv ~/dss_pkt/start.sh ~/");
	iRtn = system(cmdBuf);
	return iRtn;
}

int CPortBase::update_fpga(void)
{
	printf("update load_fpga.ko...\r\n"); 
	int iRtn=-1;
	char cmdBuf[128];	
	sprintf(cmdBuf, "mv ~/dss_pkt/load_fpga.ko ~/dss_bin/");
	iRtn = system(cmdBuf);
	return iRtn;
}

void CPortBase::seconds_sleep(unsigned seconds)
{
    struct timeval tv;

    int err;
    do{
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        err = select(0, NULL, NULL, NULL, &tv);
    }while(err<0 && errno==EINTR);
}

