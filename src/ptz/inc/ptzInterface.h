/*
 * ptzInterface.h
 *
 *  Created on: 2018年9月28日
 *      Author: jet
 */

#ifndef PTZINTERFACE_H_
#define PTZINTERFACE_H_

class CPtzInterface{

public:
	~CPtzInterface(){};

	virtual int Create() = 0;
	virtual void Destroy() = 0;
	virtual void creatPelco(bool Pelco_D, bool Pelco_P) = 0;

};


#endif /* PTZINTERFACE_H_ */
