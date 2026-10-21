
/** 
 * @file LLPanelEmergence.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelemergence.h"
// linden library includes
#include "llradiogroup.h"
#include "llbutton.h"
#include "llcolorswatch.h"
#include "lluictrlfactory.h"

#include "llcombobox.h"
#include "llslider.h"
#include "lltexturectrl.h"

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h"

#include "lltabcontainer.h"

#include "llinventorymodel.h"
#include "llfilepicker.h"
#include "llstartup.h"

#include "lltexteditor.h"

#include "llagent.h"

#include "lldirpicker.h"


#include "llweb.h" // [$PLOTR$/]
#include "lggbeamcolormapfloater.h"
#include "lggbeammapfloater.h"
#include "lggbeammaps.h"
//#include "lggbeamscolors.h"
#include "llsliderctrl.h"
#include "mfdkeywordfloater.h"
#include "lgghunspell_wrapper.h"
#include "llspinctrl.h"
#include "lggautocorrectfloater.h"

#include "llprimitive.h"
#include "a_emergenceviewerlink.h"

////////begin drop utility/////////////
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class JCInvDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//
// altered to support a callback so i can slap it in things and it just return the item to a func of my choice
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~




JCInvDropTarget::JCInvDropTarget(const std::string& name, const LLRect& rect,
						  void (*callback)(LLViewerInventoryItem*)) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mDownCallback(callback)
{
}

JCInvDropTarget::~JCInvDropTarget()
{
}

void JCInvDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "JCInvDropTarget::doDrop()" << llendl;
}

BOOL JCInvDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	if(getParent())
	{
		handled = TRUE;
		// check the type
		//switch(cargo_type)
		//{
		//case DAD_ANIMATION:
		//{
			LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
			if(gInventory.getItem(inv_item->getUUID()))
			{
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					//printchat("accepted");
					mDownCallback(inv_item);
				}
			}
			else
			{
				*accept = ACCEPT_NO;
			}
		//	break;
		//}
		//default:
		//	*accept = ACCEPT_NO;
		//	break;
		//}
	}
	return handled;
}
////////end drop utility///////////////

LLPanelEmergence* LLPanelEmergence::sInstance;

JCInvDropTarget * LLPanelEmergence::mObjectDropTarget;
JCInvDropTarget * LLPanelEmergence::mBuildObjectDropTarget;

LLPanelEmergence::LLPanelEmergence()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_emergence.xml");
	if(sInstance)delete sInstance;
	sInstance = this;
}

LLPanelEmergence::~LLPanelEmergence()
{
	sInstance = NULL;
	delete mObjectDropTarget;
	delete mBuildObjectDropTarget;
	mObjectDropTarget = NULL;
	mBuildObjectDropTarget = NULL;
}

void LLPanelEmergence::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}

void LLPanelEmergence::onClickHelp(void* data)
{
	std::string* xml_alert = (std::string*)data;
	LLNotifications::instance().add(*xml_alert);
}

BOOL LLPanelEmergence::postBuild()
{
	refresh();

	LLComboBox* tagcombo = getChild<LLComboBox>("TagCombo");
	tagcombo->setCommitCallback(onTagComboBoxCommit);
	for(LLSD::map_iterator itr = EmergenceViewerLink::emergence_tags.beginMap(); itr != EmergenceViewerLink::emergence_tags.endMap(); ++itr) 
	{
		tagcombo->add(itr->first,itr->second, ADD_BOTTOM, TRUE);
	}
	tagcombo->setSimple(gSavedSettings.getString("EmergenceTagColor"));

	getChild<LLComboBox>("material")->setSimple(gSavedSettings.getString("EmergenceBuildPrefs_Material"));
	getChild<LLComboBox>("combobox shininess")->setSimple(gSavedSettings.getString("EmergenceBuildPrefs_Shiny"));
	

	LLSliderCtrl* mShapeScaleSlider = getChild<LLSliderCtrl>("EmergenceBeamShapeScale",TRUE,FALSE);
	mShapeScaleSlider->setCommitCallback(&LLPanelEmergence::beamUpdateCall);
	mShapeScaleSlider->setCallbackUserData(this);

	LLSliderCtrl* mBeamsPerSecondSlider = getChild<LLSliderCtrl>("EmergenceMaxBeamsPerSecond",TRUE,FALSE);
	mBeamsPerSecondSlider->setCommitCallback(&LLPanelEmergence::beamUpdateCall);
	mBeamsPerSecondSlider->setCallbackUserData(this);

	getChild<LLComboBox>("material")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("combobox shininess")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("EmergenceBeamShape_combo")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("BeamColor_combo")->setCommitCallback(onComboBoxCommit);
	getChild<LLTextureCtrl>("texture control")->setDefaultImageAssetID(LLUUID("89556747-24cb-43ed-920b-47caed15465f"));
	getChild<LLTextureCtrl>("texture control")->setCommitCallback(onTexturePickerCommit);

	getChild<LLComboBox>("EmergenceSpellBase")->setCommitCallback(onSpellBaseComboBoxCommit);

	getChild<LLCheckBoxCtrl>("ColorTagsClient_toggle")->setCommitCallback(onTagsBoxCommit);

		
	//childSetCommitCallback("material",onComboBoxCommit);
	//childSetCommitCallback("combobox shininess",onComboBoxCommit);


	getChild<LLButton>("keyword_allert")->setClickedCallback(onKeywordAllertButton,this);
	getChild<LLButton>("ac_button")->setClickedCallback(onAutoCorrectButton,this);
	
	
	getChild<LLButton>("EmSpell_EditCustom")->setClickedCallback(onSpellEditCustom, this);
	getChild<LLButton>("EmSpell_GetMore")->setClickedCallback(onSpellGetMore, this);
	getChild<LLButton>("EmSpell_Add")->setClickedCallback(onSpellAdd, this);
	getChild<LLButton>("EmSpell_Remove")->setClickedCallback(onSpellRemove, this);
	

	getChild<LLButton>("BeamColor_new")->setClickedCallback(onCustomBeamColor, this);
	getChild<LLButton>("BeamColor_refresh")->setClickedCallback(onRefresh,this);
	getChild<LLButton>("BeamColor_delete")->setClickedCallback(onBeamColorDelete,this);
			
	getChild<LLButton>("custom_beam_btn")->setClickedCallback(onCustomBeam, this);
	getChild<LLButton>("refresh_beams")->setClickedCallback(onRefresh,this);
	getChild<LLButton>("delete_beam")->setClickedCallback(onBeamDelete,this);

	getChild<LLButton>("revert_production_voice_btn")->setClickedCallback(onClickVoiceRevertProd, this);
	getChild<LLButton>("revert_debug_voice_btn")->setClickedCallback(onClickVoiceRevertDebug, this);

	getChild<LLButton>("EmergenceBreastReset")->setClickedCallback(onClickBoobReset, this);
	

	childSetCommitCallback("production_voice_field", onCommitApplyControl);//onCommitVoiceProductionServerName);
	childSetCommitCallback("debug_voice_field", onCommitApplyControl);//onCommitVoiceDebugServerName);

	childSetCommitCallback("EmergenceCmdLinePos", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineGround", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineHeight", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineTeleportHome", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineRezPlatform", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineMapTo", onCommitApplyControl);	
	childSetCommitCallback("EmergenceCmdLineCalc", onCommitApplyControl);

	childSetCommitCallback("EmergenceCmdLineDrawDistance", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdTeleportToCam", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineKeyToName", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineOfferTp", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineTP2", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineAO", onCommitApplyControl);
	childSetCommitCallback("EmergenceCmdLineClearChat", onCommitApplyControl);

	childSetCommitCallback("X Modifier", onCommitAvatarModifier);
	childSetCommitCallback("Y Modifier", onCommitAvatarModifier);
	childSetCommitCallback("Z Modifier", onCommitAvatarModifier);
	if(gAgent.getID() != LLUUID::null)
	{
		getChild<LLSpinCtrl>("X Modifier")->set(gSavedPerAccountSettings.getF32("EmergenceAvatarXModifier"));
		getChild<LLSpinCtrl>("Y Modifier")->set(gSavedPerAccountSettings.getF32("EmergenceAvatarYModifier"));
		getChild<LLSpinCtrl>("Z Modifier")->set(gSavedPerAccountSettings.getF32("EmergenceAvatarZModifier"));
	}else
	{
		getChild<LLSpinCtrl>("X Modifier")->setEnabled(FALSE);
		getChild<LLSpinCtrl>("Y Modifier")->setEnabled(FALSE);
		getChild<LLSpinCtrl>("Z Modifier")->setEnabled(FALSE);
	}


	childSetValue("EmergenceUseOTR", LLSD((S32)gSavedSettings.getU32("EmergenceUseOTR"))); // [$PLOTR$]
	getChild<LLButton>("otr_help_btn")->setClickedCallback(onClickOtrHelp, this);      // [/$PLOTR$]
	const bool Wear_Dbl_Click = gSavedSettings.getBOOL("EmergenceDoubleClickWearInventoryObjects");
	getChild<LLCheckBoxCtrl>("add-inv-toggle")->setEnabled(Wear_Dbl_Click);

	initHelpBtn("EmergenceHelp_TeleportLogin",	"EmergenceHelp_TeleportLogin");
	initHelpBtn("EmergenceHelp_Voice",			"EmergenceHelp_Voice");
	initHelpBtn("EmergenceHelp_Shields",			"EmergenceHelp_Shields");
	initHelpBtn("EmergenceHelp_IM",				"EmergenceHelp_IM");
	initHelpBtn("EmergenceHelp_Chat",				"EmergenceHelp_Chat");
	initHelpBtn("EmergenceHelp_Misc",				"EmergenceHelp_Misc");
	initHelpBtn("EmergenceHelp_CmdLine",			"EmergenceHelp_CmdLine");
	initHelpBtn("EmergenceHelp_Avatar",			"EmergenceHelp_Avatar");
	initHelpBtn("EmergenceHelp_Build",			"EmergenceHelp_Build");
	initHelpBtn("EmergenceHelp_IRC",				"EmergenceHelp_IRC");
	initHelpBtn("EmergenceHelp_UtilityStream",	"EmergenceHelp_UtilityStream");
	initHelpBtn("EmergenceHelp_Inventory",		"EmergenceHelp_Inventory");
	initHelpBtn("EmergenceHelp_Effects",			"EmergenceHelp_Effects");
	initHelpBtn("EmergenceHelp_SpellCheck",		"EmergenceHelp_SpellCheck");

	LLView *target_view = getChild<LLView>("im_give_drop_target_rect");
	if(target_view)
	{
		if (mObjectDropTarget)//shouldn't happen
		{
			delete mObjectDropTarget;
		}
		mObjectDropTarget = new JCInvDropTarget("drop target", target_view->getRect(), IMAutoResponseItemDrop);//, mAvatarID);
		addChild(mObjectDropTarget);
	}
	target_view = getChild<LLView>("build_item_add_disp_rect");
	if(target_view)
	{
		if (mBuildObjectDropTarget) delete mBuildObjectDropTarget;
		mBuildObjectDropTarget = new JCInvDropTarget("build drop target", target_view->getRect(),BuildAutoResponseItemDrop);
		addChild(mBuildObjectDropTarget);
	}

	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		LLUUID itemid = (LLUUID)gSavedPerAccountSettings.getString("EmergenceInstantMessageResponseItemData");
		LLViewerInventoryItem* item = gInventory.getItem(itemid);
		if(item)
		{
			childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
		}else if(itemid.isNull())
		{
			childSetValue("im_give_disp_rect_txt","Currently not set");
		}else
		{
			childSetValue("im_give_disp_rect_txt","Currently set to a item not on this account");
		}
		itemid = (LLUUID)gSavedSettings.getString("EmergenceBuildPrefs_Item");
		item = gInventory.getItem(itemid);
		if(item)
		{
			childSetValue("build_item_add_disp_rect_txt","Currently set to: "+item->getName());
		}else if(itemid.isNull())
		{
			childSetValue("build_item_add_disp_rect_txt","Currently not set");
		}else
		{
			childSetValue("build_item_add_disp_rect_txt","Currently set to a item not on this account");
		}
	}else
	{
		childSetValue("im_give_disp_rect_txt","Not logged in");
		childSetValue("build_item_add_disp_rect_txt","Not logged in");
	}

	LLWString auto_response = utf8str_to_wstring( gSavedPerAccountSettings.getString("EmergenceInstantMessageResponse") );
	LLWStringUtil::replaceChar(auto_response, '^', '\n');
	LLWStringUtil::replaceChar(auto_response, '%', ' ');
	childSetText("im_response", wstring_to_utf8str(auto_response));
	childSetValue("EmergenceInstantMessageResponseFriends", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageResponseFriends"));
	childSetValue("EmergenceInstantMessageResponseMuted", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageResponseMuted"));
	childSetValue("EmergenceInstantMessageResponseAnyone", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageResponseAnyone"));
	childSetValue("EmergenceInstantMessageShowResponded", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageShowResponded"));
	//Kadah - Force IMShowOnTyping to resolve a bug were if AnnounceIncoming was on, autoreply would not be sent
	const bool AnnounceIncoming = gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageAnnounceIncoming");
	childSetValue("EmergenceInstantMessageShowOnTyping", AnnounceIncoming || gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageShowOnTyping"));
	getChild<LLCheckBoxCtrl>("EmergenceInstantMessageShowOnTyping")->setEnabled(!AnnounceIncoming);
	childSetValue("EmergenceInstantMessageResponseRepeat", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageResponseRepeat" ));
	childSetValue("EmergenceInstantMessageResponseItem", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageResponseItem"));
	childSetValue("EmergenceInstantMessageAnnounceIncoming", AnnounceIncoming);
	getChild<LLCheckBoxCtrl>("EmergenceInstantMessageAnnounceIncoming")->setCommitCallback(onEmergenceInstantMessageAnnounceIncoming);
	getChild<LLCheckBoxCtrl>("wear-inv-toggle")->setCommitCallback(onWearInvToggle);
	childSetValue("EmergenceInstantMessageAnnounceStealFocus", gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageAnnounceStealFocus"));
	childSetValue("EmergenceShadowsON", gSavedSettings.getBOOL("EmergenceShadowsToggle"));

	childSetAction("set_mirror", onClickSetMirror, this);
	childSetCommitCallback("mirror_location", onCommitApplyControl);

	childSetAction("set_includeHDD", onClickSetHDDInclude, this);
	childSetCommitCallback("include_location", onCommitApplyControl);

	//EmergenceLSLExternalEditor
	childSetAction("set_xed", onClickSetXed, this);
	childSetCommitCallback("xed_location", onCommitApplyControl);
	childSetValue("xed_location", gSavedSettings.getString("EmergenceLSLExternalEditor"));

	getChild<LLCheckBoxCtrl>("telerequest_toggle")->setCommitCallback(onConditionalPreferencesChanged);
	getChild<LLCheckBoxCtrl>("mldct_toggle")->setCommitCallback(onConditionalPreferencesChanged);
	
	getChild<LLColorSwatchCtrl>("friend_tag_color_swatch")->set(gSavedSettings.getColor4("EmergenceFriendNameColor"));

	refresh();
	return TRUE;
}

void LLPanelEmergence::refresh()
{
	
	LLComboBox* comboBox = getChild<LLComboBox>("EmergenceBeamShape_combo");

	if(comboBox != NULL) 
	{
		comboBox->removeall();
		comboBox->add("===OFF===");
		std::vector<std::string> names = gLggBeamMaps.getFileNames();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("EmergenceBeamShape"));
	}

	comboBox = getChild<LLComboBox>("BeamColor_combo");
	if(comboBox != NULL) 
	{
		comboBox->removeall();
		comboBox->add("===OFF===");
		std::vector<std::string> names = gLggBeamMaps.getColorsFileNames();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("EmergenceBeamColorFile"));
	}
	comboBox = getChild<LLComboBox>("EmergenceSpellBase");
	if(comboBox != NULL) 
	{
		comboBox->removeall();
		std::vector<std::string> names = glggHunSpell->getDicts();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("EmergenceSpellBase"));
	}
	comboBox = getChild<LLComboBox>("EmSpell_Avail");
	if(comboBox != NULL) 
	{
		comboBox->removeall();

		comboBox->add("");
		std::vector<std::string> names = glggHunSpell->getAvailDicts();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(std::string(""));
	}
	comboBox = getChild<LLComboBox>("EmSpell_Installed");
	if(comboBox != NULL) 
	{
		comboBox->removeall();

		comboBox->add("");
		std::vector<std::string> names = glggHunSpell->getInstalledDicts();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(std::string(""));
	}

	//epic hax (TODO: make this less hax)
	/*onConditionalPreferencesChanged(getChild<LLCheckBoxCtrl>("telerequest_toggle"), NULL);*/

	//mSkin = gSavedSettings.getString("SkinCurrent");
	//getChild<LLRadioGroup>("skin_selection")->setValue(mSkin);
}

void LLPanelEmergence::IMAutoResponseItemDrop(LLViewerInventoryItem* item)
{
	gSavedPerAccountSettings.setString("EmergenceInstantMessageResponseItemData", item->getUUID().asString());
	sInstance->childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
}
void LLPanelEmergence::BuildAutoResponseItemDrop(LLViewerInventoryItem* item)
{
	gSavedSettings.setString("EmergenceBuildPrefs_Item", item->getUUID().asString());
	sInstance->childSetValue("build_item_add_disp_rect_txt","Currently set to: "+item->getName());
}

void LLPanelEmergence::apply()
{
	LLTextEditor* im = sInstance->getChild<LLTextEditor>("im_response");
	LLWString im_response;
	if (im) im_response = im->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(im_response, 4);
	LLWStringUtil::replaceChar(im_response, '\n', '^');
	LLWStringUtil::replaceChar(im_response, ' ', '%');
	glggHunSpell->setNewHighlightSetting(gSavedSettings.getBOOL("EmergenceSpellDisplay"));
	gSavedPerAccountSettings.setString("EmergenceInstantMessageResponse", std::string(wstring_to_utf8str(im_response)));

	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseMuted", childGetValue("EmergenceInstantMessageResponseMuted").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseFriends", childGetValue("EmergenceInstantMessageResponseFriends").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseMuted", childGetValue("EmergenceInstantMessageResponseMuted").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseAnyone", childGetValue("EmergenceInstantMessageResponseAnyone").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageShowResponded", childGetValue("EmergenceInstantMessageShowResponded").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageShowOnTyping", childGetValue("EmergenceInstantMessageShowOnTyping").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseRepeat", childGetValue("EmergenceInstantMessageResponseRepeat").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageResponseItem", childGetValue("EmergenceInstantMessageResponseItem").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageAnnounceIncoming", childGetValue("EmergenceInstantMessageAnnounceIncoming").asBoolean());
	gSavedPerAccountSettings.setBOOL("EmergenceInstantMessageAnnounceStealFocus", childGetValue("EmergenceInstantMessageAnnounceStealFocus").asBoolean());
	if(((gSavedSettings.getU32("RenderQualityPerformance")>=3) && gSavedSettings.getBOOL("WindLightUseAtmosShaders") && gSavedSettings.getBOOL("VertexShaderEnable")) && childGetValue("EmergenceShadowsON").asBoolean())
	{
		gSavedSettings.setBOOL("RenderUseFBO", childGetValue("EmergenceShadowsON").asBoolean());
		gSavedSettings.setBOOL("RenderDeferred", childGetValue("EmergenceShadowsON").asBoolean());
	}
	else if(!childGetValue("EmergenceShadowsON").asBoolean())
	{
		if(gSavedSettings.getBOOL("RenderDeferred"))
		{
			gSavedSettings.setBOOL("RenderDeferred", childGetValue("EmergenceShadowsON").asBoolean());
			gSavedSettings.setBOOL("RenderUseFBO", childGetValue("EmergenceShadowsON").asBoolean());
		}
	}
	else if(((gSavedSettings.getU32("RenderQualityPerformance")<3) && !gSavedSettings.getBOOL("WindLightUseAtmosShaders") && !gSavedSettings.getBOOL("VertexShaderEnable")) && childGetValue("EmergenceShadowsON").asBoolean())
	{
		childSetValue("EmergenceShadowsON",false);
		LLNotifications::instance().add("NoShadows");
		llwarns<<"Attempt to enable shadow rendering while graphics settings not on ultra!"<<llendl;
	}
	gSavedSettings.setBOOL("EmergenceShadowsToggle", childGetValue("EmergenceShadowsON").asBoolean());
	gSavedSettings.setU32("EmergenceUseOTR", (U32)childGetValue("EmergenceUseOTR").asReal());
	gLggBeamMaps.forceUpdate();
}

void LLPanelEmergence::cancel()
{
}
void LLPanelEmergence::onClickVoiceRevertProd(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	gSavedSettings.setString("vivoxProductionServerName", "bhr.vivox.com");
	self->getChild<LLLineEditor>("production_voice_field")->setValue("bhr.vivox.com");
}

void LLPanelEmergence::onClickBoobReset(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	LLControlVariable *var;

	var = self->findControl("EmergenceBoobMass");
	self->getChild<LLSliderCtrl>("EmergenceBoobMass")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("EmergenceBoobHardness");
	self->getChild<LLSliderCtrl>("EmergenceBoobHardness")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("EmergenceBoobVelMax");
	self->getChild<LLSliderCtrl>("EmergenceBoobVelMax")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("EmergenceBoobFriction");
	self->getChild<LLSliderCtrl>("EmergenceBoobFriction")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("EmergenceBoobVelMin");
	self->getChild<LLSliderCtrl>("EmergenceBoobVelMin")->setValue(var->getDefault());
	var->resetToDefault();
}

void LLPanelEmergence::onCustomBeam(void* data)
{
	//LLPanelEmergence* self =(LLPanelEmergence*)data;
	LggBeamMap::show(true, data);

}
void LLPanelEmergence::onKeywordAllertButton(void * data)
{
	MfdKeywordFloaterStart::show(true,data);
}
void LLPanelEmergence::onAutoCorrectButton(void * data)
{
	lggAutoCorrectFloaterStart::show(TRUE,data);
}
void LLPanelEmergence::onCustomBeamColor(void* data)
{
	LggBeamColorMap::show(true,data);
}
void LLPanelEmergence::onSpellAdd(void* data)
{
	LLPanelEmergence* panel = (LLPanelEmergence*)data;
	if(panel)
	{
		glggHunSpell->addButton(panel->childGetValue("EmSpell_Avail").asString());
	}
	panel->refresh();
}
void LLPanelEmergence::onSpellRemove(void* data)
{
	LLPanelEmergence* panel = (LLPanelEmergence*)data;
	if(panel)
	{
		glggHunSpell->removeButton(panel->childGetValue("EmSpell_Installed").asString());
	}
	panel->refresh();
}
void LLPanelEmergence::onSpellGetMore(void* data)
{
	glggHunSpell->getMoreButton(data);
}
void LLPanelEmergence::onSpellEditCustom(void* data)
{
	glggHunSpell->editCustomButton();
}

void LLPanelEmergence::beamUpdateCall(LLUICtrl* crtl, void* userdata)
{
	gLggBeamMaps.forceUpdate();
}
void LLPanelEmergence::onTagComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		gSavedSettings.setString("EmergenceTagColor",box->getSimple());
		LLPrimitive::tagstring = EmergenceViewerLink::emergence_tags[box->getSimple()].asString();
		gAgent.sendAgentSetAppearance();
	}	
}
void LLPanelEmergence::onComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{

	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		gSavedSettings.setString(box->getControlName(),box->getValue().asString());
	}	
}
void LLPanelEmergence::onTagsBoxCommit(LLUICtrl* ctrl, void* userdata)
{
	EmergenceViewerLink::getInstance()->downloadClientTags();
}
void LLPanelEmergence::onSpellBaseComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{

	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		glggHunSpell->newDictSelection(box->getValue().asString());
		//LLPanelEmergence* panel = (LLPanelEmergence*)userdata;//box->getParent();
		if(sInstance)sInstance->refresh();

	}
	//LLPanelEmergence* panel = (LLPanelEmergence*)userdata;
	//if(panel)panel->refresh();
}
void LLPanelEmergence::onTexturePickerCommit(LLUICtrl* ctrl, void* userdata)
{
	LLTextureCtrl*	image_ctrl = (LLTextureCtrl*)ctrl;
	if(image_ctrl)
	{
		gSavedSettings.setString("EmergenceBuildPrefs_Texture", image_ctrl->getImageAssetID().asString());
	}
}

void LLPanelEmergence::onClickOtrHelp(void* data)           // [$PLOTR$]
{
    LLWeb::loadURL("http://www.cypherpunks.ca/otr/");
}                                                         // [/$PLOTR$]

void LLPanelEmergence::onRefresh(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	self->refresh();
	
	
}
void LLPanelEmergence::onClickVoiceRevertDebug(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	gSavedSettings.setString("vivoxDebugServerName", "bhd.vivox.com");
	self->getChild<LLLineEditor>("debug_voice_field")->setValue("bhd.vivox.com");

 
 
}
void LLPanelEmergence::onBeamDelete(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	
	LLComboBox* comboBox = self->getChild<LLComboBox>("EmergenceBeamShape_combo");

	if(comboBox != NULL) 
	{
		std::string filename = comboBox->getValue().asString()+".xml";
		std::string path_name1(gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS , "beams", filename));
		std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "beams", filename));
		
		if(gDirUtilp->fileExists(path_name1))
		{
			LLFile::remove(path_name1);
			gSavedSettings.setString("EmergenceBeamShape","===OFF===");
		}
		if(gDirUtilp->fileExists(path_name2))
		{
			LLFile::remove(path_name2);
			gSavedSettings.setString("EmergenceBeamShape","===OFF===");
		}
	}
	self->refresh();
}
void LLPanelEmergence::onBeamColorDelete(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;

	LLComboBox* comboBox = self->getChild<LLComboBox>("BeamColor_combo");

	if(comboBox != NULL) 
	{
		std::string filename = comboBox->getValue().asString()+".xml";
		std::string path_name1(gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS , "beamsColors", filename));
		std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "beamsColors", filename));

		if(gDirUtilp->fileExists(path_name1))
		{
			LLFile::remove(path_name1);
			gSavedSettings.setString("EmergenceBeamColorFile","===OFF===");
		}
		if(gDirUtilp->fileExists(path_name2))
		{
			LLFile::remove(path_name2);
			gSavedSettings.setString("EmergenceBeamColorFile","===OFF===");
		}
	}
	self->refresh();
}



//workaround for lineeditor dumbness in regards to control_name
void LLPanelEmergence::onCommitApplyControl(LLUICtrl* caller, void* user_data)
{
	LLLineEditor* line = (LLLineEditor*)caller;
	if(line)
	{
		LLControlVariable *var = line->findControl(line->getControlName());
		if(var)var->setValue(line->getValue());
	}
}

void LLPanelEmergence::onCommitAvatarModifier(LLUICtrl* ctrl, void* userdata)
{
	gSavedPerAccountSettings.setF32("EmergenceAvatarXModifier", sInstance->getChild<LLSpinCtrl>("X Modifier")->get());
	gSavedPerAccountSettings.setF32("EmergenceAvatarYModifier", sInstance->getChild<LLSpinCtrl>("Y Modifier")->get());
	gSavedPerAccountSettings.setF32("EmergenceAvatarZModifier", sInstance->getChild<LLSpinCtrl>("Z Modifier")->get());
	gAgent.sendAgentSetAppearance();
}

void LLPanelEmergence::onClickSetMirror(void* user_data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)user_data;

	std::string cur_name(gSavedSettings.getString("EmergenceInvMirrorLocation"));
	std::string proposed_name(cur_name);
	
	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
	{
		self->childSetText("mirror_location", dir_name);
		gSavedSettings.setString("EmergenceInvMirrorLocation", dir_name);
	}
}

void LLPanelEmergence::onClickSetHDDInclude(void* user_data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)user_data;

	std::string cur_name(gSavedSettings.getString("EmergenceHDDIncludeLocation"));
	std::string proposed_name(cur_name);
	
	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
	{
		self->childSetText("include_location", dir_name);
		gSavedSettings.setString("EmergenceHDDIncludeLocation", dir_name);
	}
}
void LLPanelEmergence::onClickSetXed(void* user_data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)user_data;

	std::string cur_name(gSavedSettings.getString("EmergenceLSLExternalEditor"));
	
	LLFilePicker& picker = LLFilePicker::instance();
	if (! picker.getOpenFile(LLFilePicker::FFLOAD_APP) )
	{
		return; //Canceled!
	}
	std::string file_name = picker.getFirstFile();
	if (!file_name.empty() && file_name != cur_name)
	{
		self->childSetText("xed_location", file_name);
		gSavedSettings.setString("EmergenceLSLExternalEditor", file_name);
		
	} else {
		//self->childSetText("xed_location", " ");
	  gSavedSettings.setString("EmergenceLSLExternalEditor", " ");
	}
}

//Kadah - Force IMShowOnTyping to resolve a bug were if AnnounceIncoming was on, autoreply would not be sent
void LLPanelEmergence::onEmergenceInstantMessageAnnounceIncoming(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEmergence* self = (LLPanelEmergence*)ctrl->getParent();
	const bool AnnounceIncoming = self->childGetValue("EmergenceInstantMessageAnnounceIncoming").asBoolean();
	LLCheckBoxCtrl* ShowOnTyping = self->getChild<LLCheckBoxCtrl>("EmergenceInstantMessageShowOnTyping");
	ShowOnTyping->setEnabled(!AnnounceIncoming);
	if (AnnounceIncoming)
	{
		ShowOnTyping->setValue(true);
	} else {
		ShowOnTyping->setValue(gSavedPerAccountSettings.getBOOL("EmergenceInstantMessageShowOnTyping")); //restore to last value
	}
}

void LLPanelEmergence::onWearInvToggle(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEmergence* self = (LLPanelEmergence*)ctrl->getParent();
	const bool Wear_Dbl_Click = gSavedSettings.getBOOL("EmergenceDoubleClickWearInventoryObjects");
	self->getChild<LLCheckBoxCtrl>("add-inv-toggle")->setEnabled(Wear_Dbl_Click);
}

void LLPanelEmergence::onConditionalPreferencesChanged(LLUICtrl* ctrl, void* userdata)
{
	/*LLPanelEmergence* self = (LLPanelEmergence*)ctrl->getParent();
	if(!self)return;
	LLCheckBoxCtrl* teleport = self->getChild<LLCheckBoxCtrl>("telerequest_toggle");
	LLCheckBoxCtrl* movelock = self->getChild<LLCheckBoxCtrl>("mldct_toggle");
	if(!(teleport && movelock))return;
	//bool teep = (bool)teleport->getValue().asBoolean();
	bool moov = (bool)movelock->getValue().asBoolean();
	if(moov)
	{
		teleport->setEnabled(true);
	}
	else
	{
		teleport->setValue(LLSD(true));
		gSavedSettings.setBOOL("EmergenceRequestLocalTeleports", true);
		teleport->setEnabled(false);
	}*/
}

/*
//static 
void LLPanelEmergence::onClickSilver(void* data)
{
	LLPanelEmergence* self = (LLPanelEmergence*)data;
	gSavedSettings.setString("SkinCurrent", "silver");
	self->getChild<LLRadioGroup>("skin_selection")->setValue("silver");
}*/
