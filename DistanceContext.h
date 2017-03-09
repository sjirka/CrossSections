#pragma once

#include <maya\MPxContext.h>
#include "DistanceManipContainer.h"

class DistanceContext : public MPxContext
{
public:
	DistanceContext();
	virtual ~DistanceContext();

	virtual void	toolOnSetup(MEvent& event);
	virtual void	toolOffCleanup();

	void			completeAction();
	void			deleteAction();
	void			abortAction();
	void			updateManip();
	void			clearManip();

private:
	MStatus setAllPointers(DistanceContext *ctxPtr);

	DistanceManipContainer *m_manipPtr = NULL;
};