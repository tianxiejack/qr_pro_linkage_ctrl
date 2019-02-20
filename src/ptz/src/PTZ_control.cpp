#include "PTZ_control.h"

#ifndef min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

#define DATAIN_TSK_PRI              (2)
#define DATAIN_TSK_STACK_SIZE       (0)
unsigned char trackBuf[10];
CPTZControl* CPTZControl::pThis = 0;

CPTZControl::CPTZControl():m_port(NULL), exitQuery_X(false), exitQuery_Y(false)
{

	m_bStopZoom = false;
	m_circle = 0;
	uiCurRecvLen = 0;
	m_nWait = 0;
	m_tResponse = PELCO_RESPONSE_Null;

	m_iCurPanSpeed = m_iCurTiltSpeed = m_iCurZoomSpeed = m_iCurIrisSpeed = m_iCurFocusNearSpeed = m_iCurFocusFarSpeed = m_iSetPreset = 0;
	m_iSetPanSpeed = m_iSetTiltSpeed = m_iSetZoomSpeed = m_iSetIrisSpeed =m_iSetFocusNearSpeed =  m_iSetFocusFarSpeed = m_iSetPreset =  0;
	m_bChangePanSpeed = m_bChangeTiltSpeed = m_bChangeZoomSpeed = m_bChangeIrisSpeed = \
	m_bChangeFocusNearSpeed = m_bChangeFocusFarSpeed = m_bChangePreset = false;
	m_iPanPos = m_iTiltPos = m_iZoomPos = m_iIrisPos= m_iFocusPos = 0;
	m_isetZoom = 0;

	m_changeZoom = false;

	m_bQuryZoomPos = false;
	m_fZoomLimit = 32.0;

	m_cmd1Code = 0;
	m_cmd2Code = 0;
	m_data1Code = 0;
	m_data2Code = 0;
	pThis = this;
	_GlobalDate = _GlobalDate->Instance();
	memset(&sendBuffer1, 0, sizeof(sendBuffer1));
	memset(&recvBuffer, 0, sizeof(recvBuffer));
	memset(&sendBuffer, 0, sizeof(sendBuffer));
}

CPTZControl::~CPTZControl()
{
	Destroy();
}

int CPTZControl::Create()
{
	int chid = _GlobalDate->chid_camera;
	m_byAddr = _GlobalDate->m_uart_params[chid].balladdress;
	m_pReq = (LPPELCO_D_REQPKT)sendBuffer;
	m_pReqMove = (LPPELCO_D_REQPKT)sendBuffer1;
	m_pResp = (LPPELCO_D_RESPPKT)recvBuffer;

	creatPelco(true, false);

	ptzUart_Creat();

    OSA_mutexCreate(&m_mutex);
    OSA_semCreate(&m_sem, 1, 0);

    exitDataInThread = false;
    //OSA_printf("%s: thrHandleDataIn create call... \n", __func__);
    OSA_thrCreate(&thrHandleDataIn, port_dataInFxn,  DATAIN_TSK_PRI, DATAIN_TSK_STACK_SIZE, this);

    exitThreadMove = false;
    OSA_thrCreate(&thrHandleMove,MoveThrdFxn, DATAIN_TSK_PRI, DATAIN_TSK_STACK_SIZE, this);

    //OSA_printf("%s: finish %d ", __func__, iRet);
    DXTimerCreat();

	return 0;
}

void CPTZControl::DXTimerCreat()
{
	breakQuery_X = dtimer.createTimer();
	dtimer.registerTimer(breakQuery_X, Tcallback_QueryX, &breakQuery_X);

	breakQuery_Y = dtimer.createTimer();
	dtimer.registerTimer(breakQuery_Y, Tcallback_QueryY, &breakQuery_Y);
}


void CPTZControl::Tcallback_QueryX(void* p)
{
    int tid = *(int *)p;
	if(pThis->breakQuery_X == tid)
	{
		pThis->exitQuery_X = true;
		pThis->dtimer.stopTimer(pThis->breakQuery_X);
	}
}

void CPTZControl::Tcallback_QueryY(void* p)
{
    int tid = *(int *)p;
	if(pThis->breakQuery_Y == tid)
	{
		pThis->exitQuery_Y = true;
		pThis->dtimer.stopTimer(pThis->breakQuery_Y);
	}
}

void CPTZControl::Destroy()
{
	if(m_port != NULL){
		OSA_mutexLock(&m_mutex);
		exitDataInThread = true;
		do{
			OSA_waitMsecs(5);
		}while(exitDataInThread);
		OSA_thrDelete(&thrHandleDataIn);

		exitThreadMove = true;
		do{
			OSA_waitMsecs(5);
		}while(exitThreadMove);
		OSA_thrDelete(&thrHandleMove);

		port_destory(m_port);
		m_port = NULL;

		OSA_mutexUnlock(&m_mutex);
		OSA_mutexDelete(&m_mutex);
		OSA_semSignal(&m_sem);
		OSA_semDelete(&m_sem);
	}
}

void CPTZControl::ptzUart_Creat()
{
	int iRet = OSA_SOK;
	int chid = _GlobalDate->chid_camera;
	iRet = port_create(PORT_UART, &m_port);
    OSA_assert( iRet == OSA_SOK );
    OSA_assert( m_port != NULL );
    //OSA_printf("%s: port open [%p]call... ", __func__, m_port->open);
    OSA_assert(m_port->open != NULL);
    iRet = m_port->open( m_port, &_GlobalDate->m_uart_params[chid] );
    OSA_assert( iRet == OSA_SOK );
}

void CPTZControl::dataInThrd()
{
    Int32 result;
    fd_set rd_set;
    Uint8 *buffer = NULL;
    Int32 rlen;
    Int32 i;
    struct timeval timeout;

    memset(&timeout, 0, sizeof(timeout));

    buffer = (Uint8*)malloc(1024);

    OSA_assert(buffer != NULL);

    OSA_printf(" %d: %s   : run  !!!\n",
               OSA_getCurTimeInMsec(), __func__);

    while(!exitDataInThread && m_port != NULL && m_port->fd != -1)
    {
        FD_ZERO(&rd_set);
        FD_SET(m_port->fd, &rd_set);
        timeout.tv_sec   = 0;
        timeout.tv_usec = 200000;
        result = select(m_port->fd+1, &rd_set, NULL, NULL, &timeout);
        if(result == -1 || exitDataInThread )
            break;

        if(result == 0)
        {
            //OSA_waitMsecs(1);
        	//OSA_printf("%s: timeout.", __func__);
            continue;
        }

        if(FD_ISSET(m_port->fd, &rd_set))
        {
            rlen =  m_port->recv(m_port, buffer, 1024);
            OSA_assert(rlen > 0);
            for(i=0; i<rlen; i++)
            	RecvByte(buffer[i]);
            /*OSA_printf("%s: [%02x %02x %02x %02x %02x %02x %02x]\n",__func__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);*/
        }
    }
    free(buffer);

    OSA_printf(" %d: %s   : exit !!!\n",
               OSA_getCurTimeInMsec(), __func__);
    exitDataInThread = false;
}

void CPTZControl::RecvByte(unsigned char byRecv)
{
	int sync_pan, sync_Tilt, sync_zoom;
	if(uiCurRecvLen == 0){

		if(byRecv == 0xFF){
			if(m_tResponse == PELCO_RESPONSE_Null)
				return ;
			recvBuffer[uiCurRecvLen++] = byRecv;
			if(m_tResponse == PELCO_RESPONSE_Query)
				m_nWait = 18;
			else if(m_tResponse == PELCO_RESPONSE_Extended)
				m_nWait = 7;
			else
				m_nWait = 4;
		}
	}else{
		recvBuffer[uiCurRecvLen++] = byRecv;
		if(uiCurRecvLen == m_nWait){
			if(recvBuffer[3] != 0x59 && recvBuffer[3] != 0x5B
				&& recvBuffer[3] != 0x5D && recvBuffer[3] != 0x63){
#if 0
				OSA_printf("PTZ Recv > %d %02X %02X %02X %02X %02X %02X %02X\n", uiCurRecvLen,
					recvBuffer[0], recvBuffer[1], recvBuffer[2], recvBuffer[3],
					recvBuffer[4], recvBuffer[5], recvBuffer[6]);
#endif
			}
			switch(recvBuffer[3])
			{
			case 0x59:
				m_iPanPos = recvBuffer[4];
				m_iPanPos <<= 8;
				m_iPanPos += recvBuffer[5];
			//	fprintf(stdout, "INFO: m_iPanPos is %d\n",m_iPanPos);
				if(posx_bak == m_iPanPos)
				{
					exitQuery_X = true;
					posx_bak =0;
				}
				if(_GlobalDate->Mtd_Moitor == 1)
				{
					_GlobalDate->Mtd_Moitor_X = m_iPanPos;
					m_iPanPos = 0;
				}
				sync_pan = 1;
				break;
			case 0x5B:
				m_iTiltPos = recvBuffer[4];
				m_iTiltPos <<= 8;
				m_iTiltPos += recvBuffer[5];
				if(posy_bak == m_iTiltPos)
				{
					exitQuery_Y = true;
					posy_bak = 0;
				}
				if(_GlobalDate->Mtd_Moitor == 1)
				{
					_GlobalDate->Mtd_Moitor_Y  = m_iTiltPos;
					m_iTiltPos = _GlobalDate->Mtd_Moitor = 0;
				}
				sync_Tilt = 1;
			//	printf("INFO: m_iTiltPos is %d\n",m_iTiltPos);
				break;
			case 0x5D:
				m_iZoomPos = recvBuffer[4];
				m_iZoomPos <<= 8;
				m_iZoomPos += recvBuffer[5];
				_GlobalDate->rcv_zoomValue = (unsigned short)m_iZoomPos;
				sync_zoom = 1;
				//printf("INFO: zoompos is %d\n",m_iZoomPos);
				break;
			case 0x63:
				m_iMagnification = recvBuffer[4];
				m_iMagnification <<= 8;
				m_iMagnification += recvBuffer[5];
				printf("INFO: Magnification is %d\n",m_iMagnification);
				break;
			}

			if(m_tResponse != PELCO_RESPONSE_Null)
				OSA_semSignal(&m_sem);
			uiCurRecvLen = 0;
			m_nWait = 0;
			if(sync_pan &&  sync_Tilt && sync_zoom)
				_GlobalDate->Sync_Query = 0;
			if(sync_pan &&  sync_Tilt)
				_GlobalDate->sync_pos= 0;
			if(sync_zoom)
				_GlobalDate->sync_fovComp = 0;
			sync_pan = sync_Tilt = sync_zoom = 0;
		}
	}
}

int CPTZControl::SendCmd(LPPELCO_D_REQPKT pCmd, PELCO_RESPONSE_t tResp /* = PELCO_RESPONSE_General */)
{
	int iRet = OSA_SOK;
	unsigned char *buffer = (unsigned char *)pCmd;

	OSA_mutexLock(&m_mutex);
	m_tResponse = tResp;
	if(m_port == NULL || m_port->send(m_port, (unsigned char *)pCmd, sizeof(PELCO_D_REQPKT)) != sizeof(PELCO_D_REQPKT))
	{
		OSA_printf("%s: send error! \n", __func__);
		OSA_mutexUnlock(&m_mutex);
		return OSA_EFAIL;
	}
	if(tResp != PELCO_RESPONSE_Null)
		iRet = OSA_semWait(&m_sem, 200);
	
	if( iRet != OSA_SOK ){
		OSA_printf(">>>>>>>>>>>>>>>>>>>>>>>>PTZ send msg time out!\n");
		iRet = -1;
	}
	OSA_mutexUnlock(&m_mutex);
	return iRet;
}

int CPTZControl::setZoomSpeed()
{
	int iRet = 0;
	if(m_bChangeZoomSpeed){
		p_D->MakeSetZoomSpeed(m_pReqMove, m_isetZoom , m_byAddr);
	iRet = SendCmd(m_pReqMove, PELCO_RESPONSE_Extended);
	m_bChangeZoomSpeed = false;
	printf("zoom speed set success\n");
	}
	return iRet;
}

bool mask = true;

int CPTZControl::MoveSync()
{
	int iRet = 0;
	//OSA_mutexLock(&m_mutex);

	if(m_changeZoom || m_iSetZoomSpeed)
	{
		int Pan  = abs((int)(m_iCurPanSpeed));
		int Tilt   = abs((int)m_iCurTiltSpeed);

		m_cmd1Code=0;
		m_cmd2Code=0;

		if(m_iCurFocusNearSpeed > 0)
			m_cmd2Code |= 0x80;
		else if(m_iCurFocusFarSpeed < 0)
			m_cmd1Code |= 0x01;

		if(m_iCurZoomSpeed > 0)
			m_cmd2Code |= 0x20;
		else if(m_iCurZoomSpeed <0)
			m_cmd2Code |= 0x40;
		
		if(m_iCurIrisSpeed > 0)
			m_cmd1Code |= 0x02;
		else if(m_iCurIrisSpeed <0)
			m_cmd1Code |= 0x04;

		if(m_iCurPanSpeed>0)
			m_cmd2Code |= 0x02;
		else if(m_iCurPanSpeed<0)
			m_cmd2Code |= 0x04;
		
		if(m_iCurTiltSpeed > 0)
			m_cmd2Code |= 0x10;
		else if(m_iCurTiltSpeed < 0)
			m_cmd2Code |= 0x08;

		m_data1Code = min(Pan, 63);
		m_data2Code = min(Tilt, 63);

		p_D->PktFormat(m_pReqMove, m_cmd1Code, m_cmd2Code, m_data1Code, m_data2Code, m_byAddr);
		iRet = SendCmd(m_pReqMove, PELCO_RESPONSE_Null);
#if 0
		switch(_GlobalDate->outputMode)
		{
		case 0:
			p_D->PktFormat(m_pReqMove, m_cmd1Code, m_cmd2Code, m_data1Code, m_data2Code, m_byAddr);
			iRet = SendCmd(m_pReqMove, PELCO_RESPONSE_Null);
			break;

		case 1:
		case 2:
			trackErrOutput();
			iRet = SendtrackErr();
			break;
		}
#endif
		//OSA_mutexUnlock(&m_mutex);
		//static unsigned int t1 = 0;
		//printf("during time :%d \n",OSA_getCurTimeInMsec() - t1);
		//t1 = OSA_getCurTimeInMsec();

		m_changeZoom = false;
	}
#if 0
	if(mask)
	{
		p_D->PktFormat(m_pReqMove, 0, 0, 0, 0, m_byAddr);
		SendCmd(m_pReqMove, PELCO_RESPONSE_Null);
	}
	mask = false;
#endif
	return iRet;
}

int CPTZControl::Move()
{

	int iRet = 0;
	if(m_iCurPanSpeed == m_iSetPanSpeed
		&& m_iCurTiltSpeed == m_iSetTiltSpeed
		&& m_iCurZoomSpeed == m_iSetZoomSpeed
		&& m_iCurIrisSpeed == m_iSetIrisSpeed
		&& m_iCurFocusNearSpeed == m_iSetFocusNearSpeed
		&&m_iCurFocusFarSpeed == m_iSetFocusFarSpeed)
		return 0;

	m_changeZoom = 1;
	

	m_iCurPanSpeed = m_iSetPanSpeed;
	m_iCurTiltSpeed = m_iSetTiltSpeed;
	m_iCurZoomSpeed = m_iSetZoomSpeed;
	m_iCurIrisSpeed = m_iSetIrisSpeed;
	m_iCurFocusFarSpeed = m_iSetFocusFarSpeed;
	m_iCurFocusNearSpeed = m_iSetFocusNearSpeed;

	return 0;
}

int CPTZControl::circle()
{
	Preset(PTZ_PRESET_GOTO, m_circle);
	m_circle = (m_circle+1)%5;
	return m_circle;
}

int CPTZControl::Preset(int nCtrlType, int iPresetNum)
{
	p_D->MakePresetCtrl(m_pReq, nCtrlType, iPresetNum, m_byAddr);
	return SendCmd(m_pReq, PELCO_RESPONSE_General);
}

int CPTZControl::Pattern(int nCtrlType, int iPatternNum)
{
	p_D->MakePatternCtrl(m_pReq, nCtrlType, iPatternNum, m_byAddr);
	OSA_printf("Pattern type = %d\n", nCtrlType);
	return SendCmd(m_pReq, PELCO_RESPONSE_General);
}

int CPTZControl::Query(int iQueryType)
{
	p_D->MakeExtCommand(m_pReq, iQueryType, 0, 0, m_byAddr);
	return SendCmd(m_pReq, PELCO_RESPONSE_Extended);
}

int CPTZControl::Dummy()
{
	p_D->MakeDummy(m_pReq, m_byAddr);
	return SendCmd(m_pReq, PELCO_RESPONSE_Null);
}

int CPTZControl::ptzMove(INT32 iDirection, UINT8 bySpeed)
{
	p_D->MakeMove(m_pReq,iDirection,bySpeed,1,m_byAddr);
	return SendCmd(m_pReq, PELCO_RESPONSE_Null);
}

int CPTZControl::ptzStop()
{
	p_D->PktFormat(m_pReqMove, 0, 0, 0, 0, m_byAddr);
	SendCmd(m_pReqMove, PELCO_RESPONSE_Null);
}

void CPTZControl::ptzSetPos(Uint16 posx, Uint16 posy)
{
	timeout = 0;
	posx_bak = posx;
	posy_bak = posy;
	p_D->MakeSetPanPos(m_pReq,posx,m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Null);
	printf("set pan  \n");
	struct timeval tmp;
	dtimer.startTimer(breakQuery_X, 3000);
	for(; exitQuery_X == false;)
	{
		tmp.tv_sec = 0;
		tmp.tv_usec = 30*1000;
		select(0, NULL, NULL, NULL, &tmp);
		QueryPanPos();
	}
	exitQuery_X = false;
	pThis->dtimer.stopTimer(pThis->breakQuery_X);

	p_D->MakeSetTilPos(m_pReq,posy,m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Null);
	printf("set Tilt \n");
	dtimer.startTimer(breakQuery_Y, 3000);
	for(; exitQuery_Y == false;)
	{
		tmp.tv_sec = 0;
		tmp.tv_usec = 30*1000;
		select(0, NULL, NULL, NULL, &tmp);
		QueryTiltPos();
	}
	exitQuery_Y = false;
	pThis->dtimer.stopTimer(pThis->breakQuery_Y);
}
int CPTZControl::getPanSpeed(Uint16 deltax)
{
	if(deltax < 100)
		return 0;
	if( deltax <200)
		return 12;
	else if( deltax <500)
		return 25;
	else if( deltax <1500)
		return 35;
	else if( deltax <3000)
		return 40;
	else if( deltax <5000)
		return 55;
	else if( deltax <10000)
		return 55;
	else
		return 60;
}


int CPTZControl::getTilSpeed(Uint16 deltay)
{
	if(deltay < 100)
		return 0;
	if( deltay <200)
		return 12;
	else if( deltay <500)
		return 30;
	else if( deltay <1500)
		return 40;
	else if( deltay <3000)
		return 40;
	else if( deltay <5000)
		return 45;
	else if( deltay <9000)
		return 55;
	else
		return 60;
}


void CPTZControl::ptzLinkage_SpeedLoop(Uint16 Inputx, Uint16 Inputy)
{
	m_cmd1Code=0;
	m_cmd2Code=0;
	int stop = 0;

	int deltaPan = m_iPanPos - Inputx;
	int deltax1,deltax2 ;

	//pan
	if( abs(deltaPan) >= 18000)
	{
		if(deltaPan < 0)
		{
			deltax1= abs(deltaPan);
			deltax2 = m_iPanPos + 36000 - Inputx;
			deltaPan = (deltax1 < deltax2)?deltax1:deltax2;
			m_cmd2Code |= 0x04;
		}
		else
		{
			deltax1= abs(deltaPan);
			deltax2 = Inputx + 36000 - m_iPanPos;
			deltaPan = (deltax1 < deltax2)?deltax1:deltax2;
			m_cmd2Code |= 0x02;
		}
	}
	else
	{
		if(deltaPan < 0)
			m_cmd2Code |= 0x02;
		else
			m_cmd2Code |= 0x04;
	}

	stop = getPanSpeed(abs(deltaPan));

	m_data1Code = min(stop, 63);

	int deltaTil   = m_iTiltPos - Inputy;

	if( abs(deltaTil) > 20000 )
	{
		if( deltaTil > 0 )
			m_cmd2Code |= 0x10;
		else
			m_cmd2Code |= 0x08;
	}
	else
	{
		if( m_iTiltPos > 30000)
		{
			if( deltaTil > 0 )
				m_cmd2Code |= 0x10;
			else
				m_cmd2Code |= 0x08;
		}
		else
		{
			if( deltaTil > 0 )
				m_cmd2Code |= 0x08;
			else
				m_cmd2Code |= 0x10;
		}
	}

	if( abs(deltaTil) > 20000)
	{
		deltaTil = m_iTiltPos + Inputy - 32768 ;
	}

	stop = getTilSpeed(abs(deltaTil));
	m_data2Code = min(stop, 63);

	p_D->PktFormat(m_pReqMove, m_cmd1Code, m_cmd2Code, m_data1Code, m_data2Code, m_byAddr);

	SendCmd(m_pReqMove, PELCO_RESPONSE_Null);

}

void CPTZControl::ptzSetSpeed(Uint16 Inputx, Uint16 Inputy)
{
	m_cmd1Code=0;
	m_cmd2Code=0;
	int stop = 0;

	struct timeval tmp;
	QueryPanPos();
	tmp.tv_sec = 0;
	tmp.tv_usec = 30*1000;
	select(0, NULL, NULL, NULL, &tmp);

	int deltaPan = m_iPanPos - Inputx;
	int deltax1,deltax2 ;
	QueryTiltPos();
	tmp.tv_sec = 0;
	tmp.tv_usec = 30 * 1000;
	select(0, NULL, NULL, NULL, &tmp);
	//pan
	if( abs(deltaPan) >= 18000)
	{
		if(deltaPan < 0)
		{
			deltax1= abs(deltaPan);
			deltax2 = m_iPanPos + 36000 - Inputx;
			deltaPan = (deltax1 < deltax2)?deltax1:deltax2;
			m_cmd2Code |= 0x04;
		}
		else
		{
			deltax1= abs(deltaPan);
			deltax2 = Inputx + 36000 - m_iPanPos;
			deltaPan = (deltax1 < deltax2)?deltax1:deltax2;
			m_cmd2Code |= 0x02;
		}
	}
	else
	{
		if(deltaPan < 0)
			m_cmd2Code |= 0x02;
		else
			m_cmd2Code |= 0x04;
	}

	stop = getPanSpeed(abs(deltaPan));

	m_data1Code = min(stop, 63);

	int deltaTil   = m_iTiltPos - Inputy;

	if( abs(deltaTil) > 20000 )
	{
		if( deltaTil > 0 )
			m_cmd2Code |= 0x10;
		else
			m_cmd2Code |= 0x08;
	}
	else
	{
		if( m_iTiltPos > 30000)
		{
			if( deltaTil > 0 )
				m_cmd2Code |= 0x10;
			else
				m_cmd2Code |= 0x08;
		}
		else
		{
			if( deltaTil > 0 )
				m_cmd2Code |= 0x08;
			else
				m_cmd2Code |= 0x10;
		}
	}

	if( abs(deltaTil) > 20000)
	{
		deltaTil = m_iTiltPos + Inputy - 32768 ;
	}

	stop = getTilSpeed(abs(deltaTil));
	m_data2Code = min(stop, 63);

	p_D->PktFormat(m_pReqMove, m_cmd1Code, m_cmd2Code, m_data1Code, m_data2Code, m_byAddr);

	SendCmd(m_pReqMove, PELCO_RESPONSE_Null);

//pan
	//printf("m_iPanPos   =  %d,      m_iTiltPos   =    %d,    deltaPan  =   %d,    deltaTil   =  %d   \n", m_iPanPos, m_iTiltPos, deltaPan, deltaTil);
	if(m_iPanPos >= 18000 && Inputx >= 18000)
	{
		if(deltaPan > 100)
			ptzSetSpeed( Inputx, Inputy);
		else if(deltaPan < -100)
			ptzSetSpeed( Inputx, Inputy);
	}
	else if(m_iPanPos < 18000 && Inputx < 18000)
	{
		if( deltaPan > 100)
			ptzSetSpeed( Inputx, Inputy);
		else if(deltaPan < -100)
			ptzSetSpeed( Inputx, Inputy);
	}
	else if(m_iPanPos >= 18000 && Inputx < 18000)
	{
		if( deltaPan < 35850 && deltaPan > 100)
			ptzSetSpeed( Inputx, Inputy);
	}
	else if(m_iPanPos < 18000 && Inputx >= 18000)
	{
		if( deltaPan > -35850 && deltaPan < -100)
			ptzSetSpeed( Inputx, Inputy);
	}
//Tilt
	if(m_iTiltPos <= 32768 && Inputy <= 32768)
	{
		if(m_iTiltPos == 32768)
		{
			 if(deltaTil < 32668)
				 ptzSetSpeed( Inputx, Inputy);
		}
		else
		{
			if(deltaTil < -100)
				ptzSetSpeed( Inputx, Inputy);
		}


		if(Inputy == 0)
		{
			if(deltaTil < -32668)
				ptzSetSpeed( Inputx, Inputy);
		}
		else
		{
			if(deltaTil > 100)
				ptzSetSpeed( Inputx, Inputy);
		}
	}
	else if(m_iTiltPos > 32768 && Inputy > 32768)
	{
		if(abs(deltaTil) > 100)
			ptzSetSpeed( Inputx, Inputy);
	}
	else if(m_iTiltPos <= 32768 && Inputy > 32768)
	{
		if(m_iTiltPos == 0)
		{
			if(deltaTil < -32869)
				ptzSetSpeed( Inputx, Inputy);
		}
		else if(m_iTiltPos == 32768)
		{
			if(deltaTil < -100)
				ptzSetSpeed( Inputx, Inputy);
		}
		else
		{
		if(deltaTil > -32669)
			ptzSetSpeed( Inputx, Inputy);
		}
	}
	else if(m_iTiltPos > 32768 && Inputy <= 32768)
	{
		if(Inputy == 0)
		{
			if(deltaTil > 32869)
				ptzSetSpeed( Inputx, Inputy);
		}
		else if(Inputy == 32768)
		{
			if(deltaTil > 100)
				ptzSetSpeed( Inputx, Inputy);
		}
		else
		{
			if(deltaTil < 32669)
				ptzSetSpeed( Inputx, Inputy);
		}
	}

	return;
}

void CPTZControl::setZoomPos(Uint16 value)
{
	p_D->MakeSetZoomPos(m_pReq,value,m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Null);
	printf("set Zoom \n");
}

void CPTZControl::CtrlZoomPos(unsigned int zoom)
{
	QueryZoom();
	if(zoom < 2849)
		zoom = 2849;
	else if(zoom > 65535)
		zoom = 65535;

	int delta = zoom - m_iZoomPos;
		if(abs(delta) <= 1000)
		{
			m_iSetZoomSpeed = 0;
			return;
		}
		if(delta < -1000)
			m_iSetZoomSpeed = -1;
		else if (delta > 1000)
			m_iSetZoomSpeed = 1;
		if(abs(delta) > 1000)
			CtrlZoomPos(zoom);

}

void CPTZControl::QueryPos()
{
	QueryPanPos();
	struct timeval tmp;
	tmp.tv_sec = 0;
	tmp.tv_usec = 30*1000;
	select(0, NULL, NULL, NULL, &tmp);
	QueryTiltPos();
	tmp.tv_sec = 0;
	tmp.tv_usec = 30*1000;
	select(0, NULL, NULL, NULL, &tmp);
}

void CPTZControl::QueryZoom()
{
	p_D->QueryZoomPos(m_pReq, m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Extended);
	struct timeval tmp;
	tmp.tv_sec = 0;
	tmp.tv_usec = 30*1000;
	select(0, NULL, NULL, NULL, &tmp);
}

void CPTZControl::QueryPanPos()
{
	p_D->QueryPanPos(m_pReq, m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Extended);
}

void CPTZControl::QueryTiltPos()
{
	p_D->QueryTiltPos(m_pReq, m_byAddr);
	SendCmd(m_pReq, PELCO_RESPONSE_Extended);
}

u_int8_t CPTZControl:: sendCheck_sum(uint len, u_int8_t *tmpbuf)
{
	u_int8_t  ckeSum=0;
	for(int n=0;n<len;n++)
		ckeSum ^= tmpbuf[n];
	return ckeSum;
}

u_int8_t CPTZControl:: package_frame(uint len, u_int8_t *tmpbuf)
{
	tmpbuf[0] = 0xEB;
	tmpbuf[1] = 0x53;
	tmpbuf[2] = len&0xff;
	tmpbuf[3] = (len>>8)&0xff;
	tmpbuf[len+4]= sendCheck_sum(len+3,tmpbuf+1);
	return 0;
}

void  CPTZControl::trackErrOutput()
{
	int msg_length = 5;
	u_int8_t  retrackErrOutput[msg_length+5];
	retrackErrOutput[4] = 0x08;
	retrackErrOutput[5]= _GlobalDate->errorOutPut[0];
	retrackErrOutput[5] = retrackErrOutput[5]&0xff;

	retrackErrOutput[6]= _GlobalDate->errorOutPut[0];
	retrackErrOutput[6] = (retrackErrOutput[6]>>8)&0xff;

	retrackErrOutput[7]= _GlobalDate->errorOutPut[1];
	retrackErrOutput[7] = retrackErrOutput[7]&0xff;

	retrackErrOutput[8]= _GlobalDate->errorOutPut[1];
	retrackErrOutput[8] = (retrackErrOutput[8]>>8)&0xff;

	package_frame(msg_length, retrackErrOutput);
	memcpy(trackBuf,retrackErrOutput,msg_length+5);
}

int CPTZControl::SendtrackErr()
{
	int iRet = OSA_SOK;

	OSA_mutexLock(&m_mutex);
	if(m_port == NULL || m_port->send(m_port, (unsigned char *)trackBuf, sizeof(trackBuf)) != sizeof(trackBuf))
	{
		OSA_printf("%s: send trackErr error! \n", __func__);
		OSA_mutexUnlock(&m_mutex);
		return OSA_EFAIL;
	}
	else
	{
		OSA_mutexUnlock(&m_mutex);
		return iRet;
	}
}

void CPTZControl::creatPelco(bool Pelco_D, bool Pelco_P)
{
	char type_D, type_P;
	if(Pelco_D)
		type_D = pelco_D;
	else
		type_D = 0;
	if(Pelco_P)
		type_P = pelco_P;
	else
		type_P = 0;
	p_D = IPelcoFactory::createIpelco(type_D);
	p_P = IPelcoFactory::createIpelco(type_P);
}


