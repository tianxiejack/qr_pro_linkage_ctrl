
#include "port.hpp"
#include "uart_port.hpp"
#include "udp_port.hpp"


Int32 port_create(IN Int32 portType, OUT port_handle** ppHdl)
{

	if(portType == PORT_UART)
	{
		return uart_port_create(ppHdl);
	}

	if(portType == PORT_NET_UDP)
	{
		return udp_port_create(ppHdl);
	}

	return  OSA_EFAIL;
}

Int32 port_destory(IN port_handle* pHdl)
{
	if(pHdl != NULL && pHdl->portType == PORT_UART){
		return uart_port_destroy(pHdl);
	}

	if(pHdl != NULL && pHdl->portType == PORT_NET_UDP){
		return udp_port_destroy(pHdl);
	}
    
	return  OSA_EFAIL;
}

static char PortName[][16] = {"UART" , "NET UDP", "NET SERVER", "NET CLIENT", "I2C UART"};
void port_status_print(IN port_handle* pHdl)
{
	uart_open_params* uartOpenParams = (uart_open_params*)pHdl->openParams;
	udp_open_params* udpOpenParams = (udp_open_params*)pHdl->openParams;
	
	OSA_printf("\t------------- port status--------------\n");
	OSA_printf("\tPORT TYPE \t|      DEV \t| FD \t| recv cnt \t| snd cnt \t| recv bytes \t| snd bytes \n");
	OSA_printf("\t %9s ", PortName[pHdl->portType]);
	
	if(pHdl->portType == PORT_UART)
		OSA_printf("\t %9s ", uartOpenParams->device);
	else if(pHdl->portType == PORT_NET_UDP)
		OSA_printf("\t %d ", udpOpenParams->port);
	else
		OSA_printf("\t    null ");

	OSA_printf("\t %d ", pHdl->fd);
	OSA_printf("\t  %9d", pHdl->cntRecv);
	OSA_printf("\t  %9d", pHdl->cntSnd);
	OSA_printf("\t  %9d", pHdl->cntRecvBytes);
	OSA_printf("\t  %9d", pHdl->cntSndbytes);
	
	OSA_printf("\n");
}
