/*
 * CRLARB.h
 *
 *  Created on: Apr 14, 2022
 *      Author: weber
 */

#ifndef SRC_CRLARB_H_
#define SRC_CRLARB_H_
#include "IORegion.h"

class CRL_ARB : public IORegion
{
public:
	CRL_ARB();
	virtual ~CRL_ARB();

	void configure_GEM3_1G_clock();
	void SetUpSLCRDivisors(uint32_t speed);
private:
};

#endif /* SRC_CRLARB_H_ */
