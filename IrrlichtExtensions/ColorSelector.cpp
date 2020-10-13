#include <ColorSelector.h>
#include <StringHelpers.h>
#include <font.h>
#include <mathUtils.h>
#include <IInput.h>
#include <Drawer2D.h>

#include <IGUIStaticText.h>
#include <IGUIEditBox.h>
#include <IGUIScrollBar.h>
#include <IGUIButton.h>
#include <IrrlichtDevice.h>
#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IGUIWindow.h>

using namespace std;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

ColorSelector::ColorSelector(ICommonAppContext* context, irr::video::ITexture* alphaBack, irr::video::SColor Color){
	c = context;
	color = Color;
	lastSucc = false;
	selectAlpha = true;
	device = c->getIrrlichtDevice();
	driver = device->getVideoDriver();
	env = device->getGUIEnvironment();

	alpha_back = alphaBack;

	w = driver->getScreenSize().Width; h = driver->getScreenSize().Height;
	int tw = Min(w, 6*c->getRecommendedButtonWidth()), th = Min(h, 8*c->getRecommendedButtonHeight());
	wrect = irr::core::rect<irr::s32>(w/2-tw/2, h/2-th/2, w/2+tw/2, h/2+th/2);
	th -= 40;
	win = env->addWindow(wrect, false, L"");
	win->setDrawTitlebar(false);
	win->getCloseButton()->setVisible(false);
	tr = env->addStaticText(L"R: ", rect<s32>(tw/3, 10, 5*tw/12, 10+th/7), false, false, win, -1);
	tg = env->addStaticText(L"G: ", rect<s32>(tw/3, 10+3*th/14, 5*tw/12, 10+5*th/14), false, false, win, -1);
	tb = env->addStaticText(L"B: ", rect<s32>(tw/3, 10+3*th/7, 5*tw/12, 10+4*th/7), false, false, win, -1);
	ta = env->addStaticText(L"A: ", rect<s32>(tw/3, 10+9*th/14, 5*tw/12, 10+11*th/14), false, false, win, -1);
	tr->setTextAlignment(EGUIA_LOWERRIGHT , EGUIA_CENTER);
	tg->setTextAlignment(EGUIA_LOWERRIGHT , EGUIA_CENTER);
	tb->setTextAlignment(EGUIA_LOWERRIGHT , EGUIA_CENTER);
	ta->setTextAlignment(EGUIA_LOWERRIGHT , EGUIA_CENTER);
	er = env->addEditBox(L"", rect<s32>(5*tw/12, 10, tw/2, 10+th/7), false, win, INT_EDIT);
	eg = env->addEditBox(L"", rect<s32>(5*tw/12, 10+3*th/14, tw/2, 10+5*th/14), false, win, INT_EDIT);
	eb = env->addEditBox(L"", rect<s32>(5*tw/12, 10+3*th/7, tw/2, 10+4*th/7), false, win, INT_EDIT);
	ea = env->addEditBox(L"", rect<s32>(5*tw/12, 10+9*th/14, tw/2, 10+11*th/14), false, win, INT_EDIT);
	sr = env->addScrollBar(true, rect<s32>(tw/2, 10, 11*tw/12, 10+th/7), win, -1);
	sr->setMin(0); sr->setMax(255); sr->setSmallStep(1);
	sg = env->addScrollBar(true, rect<s32>(tw/2, 10+3*th/14, 11*tw/12, 10+5*th/14), win, -1);
	sg->setMin(0); sg->setMax(255); sg->setSmallStep(1);
	sb = env->addScrollBar(true, rect<s32>(tw/2, 10+3*th/7, 11*tw/12, 10+4*th/7), win, -1);
	sb->setMin(0); sb->setMax(255); sb->setSmallStep(1);
	sa = env->addScrollBar(true, rect<s32>(tw/2, 10+9*th/14, 11*tw/12, 10+11*th/14), win, -1);
	sa->setMin(0); sa->setMax(255); sa->setSmallStep(1);
	bok = env->addButton(rect<s32>(3*tw/5, 10+6*th/7, 4*tw/5, 10+th), win, -1, L"Ok");
	bcancel = env->addButton(rect<s32>(tw/5, 10+6*th/7, 2*tw/5, 10+th), win, -1, L"Cancel");
	win->setVisible(false);
	win->setDraggable(false);
	addRecursiveAllowedFocus(win);
}

ColorSelector::~ColorSelector(){
	tr->remove();
	tg->remove();
	tb->remove();
	ta->remove();
	er->remove();
	eg->remove();
	eb->remove();
	ea->remove();
	sr->remove();
	sg->remove();
	sb->remove();
	sa->remove();
	bok->remove();
	bcancel->remove();
	win->remove();
}

void ColorSelector::addRecursiveAllowedFocus(irr::gui::IGUIElement* ele){
	allowedFocus.push_back(ele);
	irr::core::list<IGUIElement*> children = ele->getChildren();
	for(irr::core::list<IGUIElement*>::Iterator it = children.begin(); it != children.end(); ++it){
		addRecursiveAllowedFocus(*it);
	}
}

irr::video::SColor ColorSelector::getColor(){
	return color;
}

void ColorSelector::setColor(irr::video::SColor Color){
	color = Color;
}

void ColorSelector::select(bool SelectAlpha){
	selectAlpha = SelectAlpha;
	win->setVisible(true);
	win->setRelativePosition(wrect);
	if(!selectAlpha){color.setAlpha(255);}
	er->setText(convertToWString(color.getRed()).c_str());
	eg->setText(convertToWString(color.getGreen()).c_str());
	eb->setText(convertToWString(color.getBlue()).c_str());
	ea->setText(convertToWString(color.getAlpha()).c_str());
	sr->setPos(color.getRed());
	sg->setPos(color.getGreen());
	sb->setPos(color.getBlue());
	sa->setPos(color.getAlpha());
	ta->setVisible(selectAlpha);
	ea->setVisible(selectAlpha);
	sa->setVisible(selectAlpha);
	lastSucc = false;
	env->setFocus(win);
}

void ColorSelector::processEvent(irr::SEvent event){
	if(event.EventType == EET_GUI_EVENT){
		if(event.GUIEvent.EventType == EGET_BUTTON_CLICKED){
			IGUIElement* source = event.GUIEvent.Caller;
			if(source == bok){
				lastSucc = true;
				win->setVisible(false);
			}else if(source == bcancel){
				lastSucc = false;
				win->setVisible(false);
			}
		}else if(event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED){
			IGUIElement* source = event.GUIEvent.Caller;
			if(source == sr){
				color.setRed(sr->getPos());
				er->setText(convertToWString(sr->getPos()).c_str());
			}else if(source == sg){
				color.setGreen(sg->getPos());
				eg->setText(convertToWString(sg->getPos()).c_str());
			}else if(source == sb){
				color.setBlue(sb->getPos());
				eb->setText(convertToWString(sb->getPos()).c_str());
			}else if(source == sa){
				color.setAlpha(sa->getPos());
				ea->setText(convertToWString(sa->getPos()).c_str());
			}
		}else if(event.GUIEvent.EventType == EGET_EDITBOX_CHANGED){
			IGUIElement* source = event.GUIEvent.Caller;
			if(source == er){
				int val = convertWStringTo<int>(er->getText());
				color.setRed(val);
				sr->setPos(val);
			}else if(source == eg){
				int val = convertWStringTo<int>(eg->getText());
				color.setGreen(val);
				sg->setPos(val);
			}else if(source == eb){
				int val = convertWStringTo<int>(eb->getText());
				color.setBlue(val);
				sb->setPos(val);
			}else if(source == ea){
				int val = convertWStringTo<int>(ea->getText());
				color.setAlpha(val);
				sa->setPos(val);
			}
		}
	}
}

void ColorSelector::render(){
	if(win->isVisible()){
		rect<s32> curwrect = win->getRelativePosition();
		int tw = curwrect.getWidth(), th = curwrect.getHeight()-40;
		vector2d<s32> l = curwrect.UpperLeftCorner;
		rect<s32> dest = rect<s32>(l.X+tw/12, l.Y+10, l.X+3*tw/12, l.Y+10+4*th/7);
		dest = makeXic(dest,1.0);
		if(alpha_back){
			c->getDrawer()->draw(alpha_back, dest.UpperLeftCorner, dimension2d<int>(dest.getWidth(), dest.getHeight()));
		}
		driver->draw2DRectangle(color, dest);
		//foreground window
		IGUIElement* ele = env->getFocus();
		bool resetFocus = true;
		for(std::list<irr::gui::IGUIElement*>::iterator it = allowedFocus.begin(); it != allowedFocus.end(); ++it){
			if(ele == *it){
				resetFocus = false;
				break;
			}
		}
		if(resetFocus){env->setFocus(win);}
	}
}

bool ColorSelector::isVisible(){
	return win->isVisible();
}

bool ColorSelector::lastSuccess(){
	return lastSucc;
}
