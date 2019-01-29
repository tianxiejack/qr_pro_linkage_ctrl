
#ifndef _UTILS_PORT_H_
#define _UTILS_PORT_H_

#include <osa.h>
#include <osa_que.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif


//#ifdef __cplusplus
//extern "C"{
//#endif

enum{
    PORT_UART	     = 0,
    PORT_NET_UDP	 = 1,
    PORT_NET_SERVER	 = 2,
    PORT_NET_CLIENT  = 3,
    PORT_IPC_PIPE    = 4,
    PORT_I2C_UART	 = 5,
    PORT_GPMC_FPGA   = 6,
    PORT_SPI         = 7,
    PORT_IPC         = 8
};

enum{
    FULL_DUPLEX_MOD = 0,
    HALF_DUPLEX_MOD = 1
};

typedef struct _uart_open_params
{
    char   device[64];
    char   name[64];
    Int32  baudrate;
    Int32  databits;
    char   parity;
    Int32  stopbits;
}
uart_open_params;

#if 0
typedef struct _uart_gpmc_open_params
{
    char   device[64];
    char   name[64];
    Int32  baudrate;
    Int32  databits;
    char   parity;
    Int32  stopbits;
}
uart_gpmc_open_params;

typedef struct _uart_i2c_open_params
{
    char   device[64];
    char   name[64];
    char   dev_i2c_addr;
    Int32  baudrate;
    Int32  databits;
    char   parity;
    Int32  stopbits;
    Int32  duplex;
}
uart_i2c_open_params;

typedef struct _spi_port_params
{
    char device[64];
    char name[64];
    Int32  baudrate;
    Int32  databits;
    char   parity;
    Int32  stopbits;
    Int32  duplex;
}
spi_port_open_params;

typedef struct _ipc_port_open_params
{
    char  device[64];
    char  name[64];
    key_t msgKey;
    int   msgId[2];
    int	qid;
    int	IsQuit;
}
ipc_open_params;
#endif

typedef struct _udp_open_params
{
    char   device[64];
    char   name[64];
    UInt16 port;
    UInt32 interfaceIp;
}
udp_open_params;

typedef struct _udp_trans_context
{
    UInt16 fromPort;
    UInt32 fromIp;
    UInt16 remotePort;
    UInt32 remoteIp;
}
udp_trans_context;

typedef struct _port_handle
{
	void *context;

    Int32   (*open)(struct _port_handle *Hdl, void * openParams);
    Int32   (*close)(struct _port_handle *Hdl);
    Int32   (*send)(struct _port_handle *Hdl, UInt8 *data, UInt32 len);
    Int32   (*recv)(struct _port_handle *Hdl, UInt8 *buffer, UInt32 size);
    Int32   (*ioctl)(struct _port_handle *Hdl, UInt32 cmd, unsigned long arg);

    Int32   portType;
    char	portAddr;
    char    tag[16];
    int	  	fd;
    void *  openParams;
    void *  transContext;
    void *  user;
    Uint32  cntRecvBytes;
    Uint32  cntSndbytes;
    Uint32  cntSnd;
    Uint32  cntRecv;

}
port_handle;


Int32 port_create(IN Int32 portType, OUT port_handle** ppHdl);
Int32 port_destory(IN port_handle* pHdl);
void port_status_print(IN port_handle* pHdl);

//#ifdef __cplusplus
//}
//#endif

#endif
