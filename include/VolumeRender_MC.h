#ifndef VOLUMERENDER_MC
#define VOLUMERENDER_MC

#include "VolumeRender.h"

class VolumeRender_MC : public VolumeRender {
public:
	VolumeRender_MC(CGcontext cg_context, VolumeBuffer *volume);
	~VolumeRender_MC();

	virtual void render();

protected:	
	virtual void loadPrograms();
};

#endif