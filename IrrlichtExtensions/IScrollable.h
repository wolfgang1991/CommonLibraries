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

};

#endif
