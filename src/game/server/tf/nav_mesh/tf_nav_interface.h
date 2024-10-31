/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_NAV_INTERFACE_H
#define TF_NAV_INTERFACE_H
#ifdef _WIN32
#pragma once
#endif


class CPointNavInterface : public CPointEntity
{
public:
	CPointNavInterface() {}
	virtual ~CPointNavInterface() {}
	
	DECLARE_CLASS(CPointNavInterface, CPointEntity);
	DECLARE_DATADESC();
	
	void RecomputeBlockers(inputdata_t& inputdata);
};


#endif
