#include "CConnect.hpp"
#include <unistd.h>

#define RECVSIZE (32)
#define RECV_BUF_SIZE (1024)

CConnect::CConnect()
{
	m_connect = -1;
    bConnecting=false;
    _globalDate= getDate();
    //OSA_mutexCreate(&sendMux);
}

CConnect::~CConnect()
{
    bConnecting = false;
    OSA_semSignal(&_globalDate->m_semHndl_socket);
    OSA_semSignal(&_globalDate->m_semHndl_socket_s);
    OSA_thrJoin(&getDataThrID);
    OSA_thrJoin(&sendDataThrID);
    //OSA_mutexDelete(&sendMux);
}

void CConnect::run()
{
    int iret;
    iret = OSA_thrCreate(&getDataThrID,
        getDataThrFun,
        0,
        0,
        (void*)this);

    iret = OSA_thrCreate(&sendDataThrID,
        sendDataThrFun,
        0,
        0,
        (void*)this);
}

int CConnect::recvData()
{
	int sizeRcv;
 	uint8_t dest[1024]={0};
	int read_status = 0;
	int dest_cnt = 0;
	unsigned char  tmpRcvBuff[RECV_BUF_SIZE];
	memset(tmpRcvBuff,0,sizeof(tmpRcvBuff));

	unsigned int uartdata_pos = 0;
	unsigned char frame_head[]={0xEB, 0x53};
	static struct data_buf
	{
		unsigned int len;
		unsigned int pos;
		unsigned char reading;
		unsigned char buf[RECV_BUF_SIZE];
	}swap_data = {0, 0, 0,{0}};
	memset(&swap_data,0,sizeof(swap_data));

	while(bConnecting){
		sizeRcv = NETRecv(tmpRcvBuff,RECVSIZE);
		uartdata_pos = 0;

		if(sizeRcv>0)
		{
#if 0
			printf("%s,%d,recv from socket %d bytes:\n",__FILE__,__LINE__,sizeRcv);
			for(int j=0;j<sizeRcv;j++){
				printf("%02x ",tmpRcvBuff[j]);
			}
			printf("\n");
#endif
			while (uartdata_pos < sizeRcv){
				if((0 == swap_data.reading) || (2 == swap_data.reading)){
					if(frame_head[swap_data.len] == tmpRcvBuff[uartdata_pos]){
						swap_data.buf[swap_data.pos++] =  tmpRcvBuff[uartdata_pos++];
						swap_data.len++;
						swap_data.reading = 2;
						if(swap_data.len == sizeof(frame_head)/sizeof(char))
						swap_data.reading = 1;
					}else{
						uartdata_pos++;
						if(2 == swap_data.reading)
							memset(&swap_data, 0, sizeof(struct data_buf));
					}
				}else if(1 == swap_data.reading){
					swap_data.buf[swap_data.pos++] = tmpRcvBuff[uartdata_pos++];
					swap_data.len++;
					if(swap_data.len>=4){
						if(swap_data.len==((swap_data.buf[2]|(swap_data.buf[3]<<8))+5)){
							for(int i=0;i<swap_data.len;i++)
								_globalDate->rcvBufQue.push_back(swap_data.buf[i]);
							prcRcvFrameBufQue(2);
							memset(&swap_data, 0, sizeof(struct data_buf));
						}
					}
				}
			}
		}
	}
	return 0;
}

int CConnect::sendData()
{
    int retVle,n;

	while(bConnecting){
		OSA_semWait(&_globalDate->m_semHndl_socket,OSA_TIMEOUT_FOREVER);
		if(0 == bConnecting)
			break;
		getSendInfo(_globalDate->feedback, &_globalDate->repSendBuffer);
		retVle = write(m_connect, &_globalDate->repSendBuffer.sendBuff,_globalDate->repSendBuffer.byteSizeSend);
	/*
	 * printf("_globalDate->feedback = %d \n", _globalDate->feedback);
	 */
		if(_globalDate->feedback == ACK_config_Read || _globalDate->feedback == ACK_param_todef)
			OSA_semSignal(&_globalDate->m_semHndl_socket_s);
#if 0
		printf("\n");
		printf("%s,%d, write to socket %d bytes:\n", __FILE__,__LINE__,retVle);

		for(int i = 0; i < retVle; i++)
		{
			printf("%02x ", _globalDate->repSendBuffer.sendBuff[i]);
		}
		printf("-----end-----\n");
#endif
	}
	return 0;
}


int  CConnect::NETRecv(void *rcv_buf,int data_len)
{
		int fs_sel,len;
		fd_set fd_netRead;
		struct timeval timeout;
		FD_ZERO(&fd_netRead);
		FD_SET(m_connect,&fd_netRead);
	    timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		fs_sel = select(m_connect+1,&fd_netRead,NULL,NULL,&timeout);
		if(fs_sel){
			len = read(m_connect,rcv_buf,data_len);
			if((fs_sel==1)&&(len==0))
				bConnecting = false;
			return len;
		}
		else if(-1 == fs_sel){
			printf("ERR: Socket  select  Error!!\r\n");
			return -1;
		}
		else if(0 == fs_sel){
			//printf("Warning: Uart Recv  time  out!!\r\n");
			return 0;
		}
}










