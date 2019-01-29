/*
 * josInterface.h
 *
 *  Created on: 2018年9月25日
 *      Author: jet
 */

#ifndef JOSINTERFACE_H_
#define JOSINTERFACE_H_


class CJosInterface{

public:
	CJosInterface(){};
	virtual ~CJosInterface(){};

	virtual int Create() = 0;
	virtual int  Destroy() = 0;
	virtual int JosToWinX(int x, int sensor) = 0;
	virtual int JosToWinY(int y, int sensor) = 0;


};


#endif /* JOSINTERFACE_H_ */
