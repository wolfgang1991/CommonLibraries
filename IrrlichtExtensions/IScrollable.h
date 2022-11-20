#ifndef IScrollable_H_INCLUDED
#define IScrollable_H_INCLUDED

#include "ScrollBar.h"

//! Interface for GUI Elements which can be scrolled with a scroll bar
class IScrollable{

	protected:
	
	ScrollBar* scrollbar;

	public:
	
	IScrollable():scrollbar(NULL){}
	
	virtual ~IScrollable(){
		unlinkScrollBar();
	}
	
	virtual void linkToScrollBar(ScrollBar* scrollbar){
		if(this->scrollbar!=scrollbar){//avoid infinite link recursions
			this->scrollbar = scrollbar;
			scrollbar->linkToScrollable(this);
		}
	}
	
	virtual void unlinkScrollBar(){
		if(this->scrollbar!=NULL){
			ScrollBar* tmp = scrollbar;
			scrollbar = NULL;
			tmp->unlinkScrollable();
		}
	}
	
	virtual void OnScrollBarChanged(ScrollBar* scrollbar) = 0;
	
	//! used for scrolling with wheel: stepsize depends usually on the content (larger content => smaller stepsize); in [0-1]
	virtual irr::f32 getScrollStepSize() const = 0;
	
	//! returns true if the content is scrollable not only by flag but also because of the used space
	virtual bool isTrulyScrollable() const = 0;

};

#endif
