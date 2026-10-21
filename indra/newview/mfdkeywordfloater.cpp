/* Copyright (c) 2010
 *
 * Modular Systems. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "mfdkeywordfloater.h"

#include "llagentdata.h"
#include "llcommandhandler.h"
#include "llfloater.h"
#include "llsdutil.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llfilepicker.h"
#include "llpanel.h"
#include "lliconctrl.h"
#include "llbutton.h"
#include "llcolorswatch.h"

#include "llsdserialize.h"
#include "llpanelemergence.h"
#include "llsliderctrl.h"
#include "llfocusmgr.h"
#include <boost/regex.hpp>
#include "llstartup.h"


class mfdKeywordFloater;

const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
const F32 CONTEXT_FADE_TIME = 0.08f;

class mfdKeywordFloater : public LLFloater, public LLFloaterSingleton<mfdKeywordFloater>
{
public:
	mfdKeywordFloater(const LLSD& seed);
	virtual ~mfdKeywordFloater();
	

	BOOL postBuild(void);
	void draw();
	void update();
	
	void setData(void* data);
	
	// UI Handlers
	static void onClickSave(void* data);
	static void onClickCancel(void* data);

protected:
	F32 mContextConeOpacity;
	LLPanelEmergence * empanel;

};

void mfdKeywordFloater::draw()
{
	
	//Try draw rectangle attach beam
	LLRect swatch_rect;
	LLButton* createButton = empanel->getChild<LLButton>("keyword_allert");	
	createButton->localRectToOtherView(createButton->getLocalRect(), &swatch_rect, this);
	LLRect local_rect = getLocalRect();
	if (gFocusMgr.childHasKeyboardFocus(this) && empanel->isInVisibleChain() && mContextConeOpacity > 0.001f)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLGLEnable(GL_CULL_FACE);
		gGL.begin(LLRender::QUADS);
		{
			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mTop);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mTop);
			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mRight, local_rect.mTop);
			gGL.vertex2i(local_rect.mLeft, local_rect.mTop);

			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
			gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mBottom);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mTop);

			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
			gGL.vertex2i(local_rect.mRight, local_rect.mTop);
			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mTop);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mBottom);

			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
			gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
			gGL.color4f(0.0f, 0.0f, 0.0f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mBottom);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mBottom);
		}
		gGL.end();
	}

	mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLCriticalDamp::getInterpolant(CONTEXT_FADE_TIME));


	//Draw Base Stuff
	LLFloater::draw();
}

mfdKeywordFloater::~mfdKeywordFloater()
{
}
mfdKeywordFloater::mfdKeywordFloater(const LLSD& seed):mContextConeOpacity(0.0f)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_alertwindow.xml");
	
	if (getRect().mLeft == 0 
		&& getRect().mBottom == 0)
	{
		center();
	}

}

BOOL mfdKeywordFloater::postBuild(void)
{
	setCanMinimize(false);

	childSetAction("EmergenceKeywords_cancel",onClickCancel,this);
	if(LLStartUp::getStartupState() >= STATE_INVENTORY_SEND)
	{
		childSetAction("EmergenceKeywords_save",onClickSave,this);\

		childSetValue("EmergenceKeywords_Alert",gSavedPerAccountSettings.getBOOL("EmergenceKeywordOn"));
		childSetValue("EmergenceKeywords_Entries",gSavedPerAccountSettings.getString("EmergenceKeywords"));
		childSetValue("EmergenceKeywords_IM",gSavedPerAccountSettings.getBOOL("EmergenceKeywordInIM"));
		childSetValue("EmergenceKeywords_GroupChat",gSavedPerAccountSettings.getBOOL("EmergenceKeywordInGroup"));
		childSetValue("EmergenceKeywords_LocalChat",gSavedPerAccountSettings.getBOOL("EmergenceKeywordInChat"));
		childSetValue("EmergenceKeywords_IRC",gSavedPerAccountSettings.getBOOL("EmergenceKeywordInIRC"));
		childSetValue("EmergenceKeywords_Highlight",gSavedPerAccountSettings.getBOOL("EmergenceKeywordChangeColor"));
		//childSetValue("EmergenceKeywords_Color",gSavedPerAccountSettings.getLLSD("EmergenceKeywordColor"));
		childSetValue("EmergenceKeywords_PlaySound",gSavedPerAccountSettings.getBOOL("EmergenceKeywordPlaySound"));
		childSetValue("EmergenceKeywords_SoundUUID",gSavedPerAccountSettings.getString("EmergenceKeywordSound"));

		LLColorSwatchCtrl* colorctrl = getChild<LLColorSwatchCtrl>("EmergenceKeywords_Color");
		colorctrl->set(LLColor4(gSavedPerAccountSettings.getColor4("EmergenceKeywordColor")),TRUE);
	}else
	{
		childSetEnabled("EmergenceKeywords_save",FALSE);
		childSetValue("EmergenceKeywords_Entries","You must login to change these settings");

	}

	return true;
}
void mfdKeywordFloater::setData(void* data)
{
	empanel = (LLPanelEmergence*)data;
	if(empanel)
	{
		gFloaterView->getParentFloater(empanel)->addDependentFloater(this);
	}
}
void mfdKeywordFloater::update()
{
	
}
void mfdKeywordFloater::onClickSave(void* data)
{
	
	mfdKeywordFloater* self = (mfdKeywordFloater*)data;

	gSavedPerAccountSettings.setBOOL("EmergenceKeywordOn", self->childGetValue("EmergenceKeywords_Alert").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordInIM", self->childGetValue("EmergenceKeywords_IM").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordInGroup", self->childGetValue("EmergenceKeywords_GroupChat").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordInChat", self->childGetValue("EmergenceKeywords_LocalChat").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordInIRC", self->childGetValue("EmergenceKeywords_IRC").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordChangeColor", self->childGetValue("EmergenceKeywords_Highlight").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceKeywordPlaySound", self->childGetValue("EmergenceKeywords_PlaySound").asBoolean());

	gSavedPerAccountSettings.setString("EmergenceKeywords", self->childGetValue("EmergenceKeywords_Entries").asString());
	gSavedPerAccountSettings.setString("EmergenceKeywordSound", self->childGetValue("EmergenceKeywords_SoundUUID").asString());

	
	gSavedPerAccountSettings.setColor4("EmergenceKeywordColor", 
		 ((LLColorSwatchCtrl*)(self->getChild<LLColorSwatchCtrl>("EmergenceKeywords_Color")))->get()
		 );



	self->close();

}

void mfdKeywordFloater::onClickCancel(void* data)
{
	mfdKeywordFloater* self = (mfdKeywordFloater*)data;	
	self->close();	
}
void MfdKeywordFloaterStart::show(BOOL showin, void * data)
{
	if(showin)
	{
		
		mfdKeywordFloater* floater = mfdKeywordFloater::showInstance();
		floater->setData(data);

	}
}
bool containsKeyWord(std::string source)
{
	std::string s = gSavedPerAccountSettings.getString("EmergenceKeywords");
	LLStringUtil::toLower(s);
	LLStringUtil::toLower(source);
	boost::regex re(",");
	boost::sregex_token_iterator i(s.begin(), s.end(), re, -1);
	boost::sregex_token_iterator j;

	while(i != j)
	{
		if(source.find( *i++) != std::string::npos)
		{
			if(gSavedPerAccountSettings.getBOOL("EmergenceKeywordPlaySound"))
				LLUI::sAudioCallback(LLUUID(gSavedPerAccountSettings.getString("EmergenceKeywordSound")));

			return true;
		}
	}
	return false;

}

BOOL MfdKeywordFloaterStart::hasKeyword(std::string msg,int source)
{
	if(!gSavedPerAccountSettings.getBOOL("EmergenceKeywordOn"))return FALSE;

// 	if((source ==3) && (gSavedPerAccountSettings.getBOOL("EmergenceKeywordInGroup")))
// 		return containsKeyWord(msg);
// 	if((source ==4) && (gSavedPerAccountSettings.getBOOL("EmergenceKeywordInIRC")))
// 		return containsKeyWord(msg);
	if((source == 1) && (gSavedPerAccountSettings.getBOOL("EmergenceKeywordInChat")))
		return containsKeyWord(msg);
	if((source == 2) && (gSavedPerAccountSettings.getBOOL("EmergenceKeywordInIM")))
		return containsKeyWord(msg);
	return FALSE;
	
}


