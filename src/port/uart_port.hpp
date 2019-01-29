/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 ChamRun Tech Incorporated - http://www.casevision.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
*  
*
*  \brief .
*
*/

#ifndef _UART_PORT_H_
#define _UART_PORT_H_

#include "port.hpp"

typedef struct _uart_port_handle
{
	port_handle Hdl;
	uart_open_params openParams;
} uart_port_handle;

Int32 uart_port_create(port_handle **ppHdl);
Int32 uart_port_destroy(port_handle *pHdl);


#endif
