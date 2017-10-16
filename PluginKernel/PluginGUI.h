#pragma once
#ifndef __PluginGUI__
#define __PluginGUI__

// --- custom VSTGUI4 derived classes
/*#include "XYPadWP.h"
#include "VuMeterWP.h"
#include "SliderWP.h"
#include "KnobWP.h"
#include "KickButtonWP.h"*/
#include "CustomControls.h"

// --- VSTGUI
#include "vstgui/vstgui.h"
#include "vstgui/uidescription/uiviewswitchcontainer.h"
#include "vstgui/vstgui_uidescription.h"
#include "vstgui/lib/crect.h"

#if VSTGUI_LIVE_EDITING
#include "vstgui/uidescription/editing/uieditcontroller.h"
#include "vstgui/uidescription/editing/uieditmenucontroller.h"
#endif

#ifdef AUPLUGIN
#import <CoreFoundation/CoreFoundation.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#endif

#ifdef AAXPLUGIN
#include "AAX_IEffectParameters.h"
#include "AAXtoVSTGUIButtonState.h"
#endif

// --- plugin stuff
#include "pluginParameter.h"
//#include "plugininterfaces.h"

#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <map>

const uint32_t kHostChoice = 3;

#ifdef AUPLUGIN
typedef struct
{
    void* pWindow;
    float width;
    float height;
    AudioUnit au;
    IGUIWindowFrame* pGUIFrame;
    IGUIView* pGUIView;
} VIEW_STRUCT;
#endif

// --- listener for each GUI Control ID
class CControlUpdateReceiver
{
public:
    CControlUpdateReceiver(CControl* control, PluginParameter* pluginParameterPtr, bool _isControlListener)
    {
       hasRefGuiControl = pluginParameterPtr ? true : false;

        if(hasRefGuiControl)
            refGuiControl = *pluginParameterPtr;

        addControl(control);
        isControlListener = _isControlListener;
    }

    ~CControlUpdateReceiver()
    {
		for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* ctrl = *it;
            ctrl->forget();
        }
    }

    bool hasControl(CControl* control)
    {
        return std::find(guiControls.begin (), guiControls.end (), control) != guiControls.end ();
    }

    void addControl(CControl* control)
    {
        if(hasControl(control))
            return;

        control->remember();
        guiControls.push_back(control);
        
        // --- set default value (rather than from XML, which is a pain because it must be normalized)
        if(hasRefGuiControl)
        {
            float normalizedValue = (float)refGuiControl.getDefaultValueNormalized();
            control->setDefaultValue(normalizedValue);
        }
    }

    void removeControl(CControl* control)
    {
        for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl == control)
            {
                ctrl->forget();
                guiControls.erase(it);
                return;
            }
        }
    }
    
    bool controlInRxGroupIsEditing()
    {
        for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl && ctrl->isEditing())
                return true;
        }
        return false;
    }
    
    float getActualValueWithNormalizedValue(float normalizedValue)
    {
        return (float)refGuiControl.getControlValueWithNormalizedValue(normalizedValue);
    }

    float getNormalizedValueWithActualValue(float actualValue)
    {
		return (float)refGuiControl.getNormalizedControlValueWithActualValue(actualValue);
    }

    void updateControlsWithControl(CControl* control)
    {
        updateControlsWithActualValue(getActualValueWithNormalizedValue(control->getValueNormalized()), control);
    }

    void updateControlsWithNormalizedValue(float normalizedValue, CControl* control = nullptr)
    {
        updateControlsWithActualValue(getActualValueWithNormalizedValue(normalizedValue), control);
    }

    void initControl(CControl* control)
    {
		float actualValue = (float)refGuiControl.getControlValue();
        updateControlsWithActualValue(actualValue, control);
    }

    void updateControlsWithActualValue(float actualValue, CControl* control = nullptr)
    {
        // --- eliminiate glitching from AAX parameter loop
        if(!control && controlInRxGroupIsEditing())
            return;

        // --- store on our reference control
        refGuiControl.setControlValue(actualValue);

        // --- synchoronize all controls that share this controlID
        for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* guiCtrl = *it;
            if(guiCtrl && control != guiCtrl) // nned the check for XY pads
            {
                // --- need to take care of multiple control types, slight differences in setup
                CTextLabel* label = dynamic_cast<CTextLabel*>(guiCtrl);
                COptionMenu* oMenu = dynamic_cast<COptionMenu*>(guiCtrl);
                CXYPadWP* xyPad = dynamic_cast<CXYPadWP*>(guiCtrl);
                CVuMeter* meter = dynamic_cast<CVuMeter*>(guiCtrl);

                if(meter) /// ignore meters; this is handled in idle() for consistency
                {
                    continue;
                }
                else if(label)
                {
                    label->setText(refGuiControl.getControlValueAsString().c_str());
                    label->invalid();
                }
                else if(oMenu)
                {
                    // --- current rafx GUI Designer does not set max to anything
					guiCtrl->setMin((float)refGuiControl.getGUIMin());
					guiCtrl->setMax((float)refGuiControl.getGUIMin());

                    // --- load for first time, NOT dynamic loading here
                    oMenu->removeAllEntry();

                    for (uint32_t i = 0; i < refGuiControl.getStringCount(); i++)
                    {
                        oMenu->addEntry(refGuiControl.getStringByIndex(i).c_str());
                    }

					oMenu->setValue((float)refGuiControl.getControlValue());
                }
                else if(xyPad)
                {
                    float x = 0.f;
                    float y = 0.f;
                    xyPad->calculateXY(xyPad->getValue(), x, y);
 
                    // --- retrieve the X and Y tags on the CXYPadWP
                    int32_t tagX = xyPad->getTagX();
                    int32_t tagY = xyPad->getTagY();
  
                    if(tagX == refGuiControl.getControlID())
						x = (float)refGuiControl.getControlValueNormalized();
                    if(tagY == refGuiControl.getControlID())
						y = (float)refGuiControl.getControlValueNormalized();

                    if(tagX >= 0 && tagY >= 0 && !guiCtrl->isEditing())
                        xyPad->setValue(xyPad->calculateValue(x, y));
                }
                else if(!guiCtrl->isEditing())
					guiCtrl->setValueNormalized((float)refGuiControl.getControlValueNormalized());

                guiCtrl->invalid();
            }
        }
    }

    PluginParameter getGuiControl(){return refGuiControl;}
    
    // --- for AAX only
    inline bool controlAndContainerVisible(CControl* ctrl)
    {
        if(!ctrl) return false;
        
        bool stickyVisible = ctrl->isVisible();
        
        if(!stickyVisible)
            return false;
        
        // --- check parents
        CView* parent = ctrl->getParentView();
        while(parent)
        {
            stickyVisible = parent->isVisible();
            if(!stickyVisible)
                return false;
            
            parent = parent->getParentView();
        }
        return stickyVisible;
    }

    int getControlID_WithMouseCoords(const CPoint& where)
    {
        CPoint mousePoint = where;
        
        for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl && controlAndContainerVisible(ctrl))
            {
                CPoint point = ctrl->frameToLocal(mousePoint);
                CRect rect = ctrl->getViewSize();
                if(rect.pointInside(point))
                {
                    int tag = ctrl->getTag();
                    if(isReservedTag(tag))
                        return -1;
                    else
                        return tag;
                }
            }
        }
        return -1;
    }
    
    CControl* getControl_WithMouseCoords(const CPoint& where)
    {
        CPoint mousePoint = where;
        
        for(std::vector<CControl*>::iterator it = guiControls.begin(); it != guiControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl && controlAndContainerVisible(ctrl))
            {
                CPoint point = ctrl->frameToLocal(mousePoint);
                CRect rect = ctrl->getViewSize();
                if(rect.pointInside(point))
                    return ctrl;
            }
        }
        return nullptr;
    }
    
protected:
    PluginParameter refGuiControl; // -- single parameter with this control tag
    std::vector<CControl*> guiControls;
    bool hasRefGuiControl;
    bool isControlListener;
};

namespace VSTGUI {

class PluginGUIConnector;
class PluginGUIFrame;

// --- IController inherits from IControlListener
class PluginGUI : public IController, 
				  public IViewAddedRemovedObserver, 
				  public IMouseObserver, 
				  public IKeyboardHook, 
				  public VSTGUIEditorInterface, 
				  public CBaseObject, 
				  public IGUIView
{

public:
    PluginGUI(UTF8StringPtr _xmlFile);
	virtual ~PluginGUI();

	// --- create the GUI
    IPluginGUIConnector* open(UTF8StringPtr _viewName,
							  void* parent, 
							  std::vector<PluginParameter*>* pluginParameterPtr, 
							  const PlatformType& platformType = kDefaultNative, 
							  IGUIPluginConnector* _guiPluginConnector = nullptr, 
							  void* data = nullptr);

	// --- destroy the GUI
    void close();

	// --- syntc GUI control to current plugin value, thread-safe
    void syncGUIControl(uint32_t controlID);

	// --- manipualate GUI size (see advanced GUI chapter in FX book)
    void getSize(float& width, float& height);
	void scaleGUISize(uint32_t controlValue);

	// --- for preset saving helper, non-RackAFX, which writes presets for you
	void writeToPresetFile(std::string presetName);
	CTextEdit* presetNameTextEdit = nullptr;

protected:
	// --- update meters, do animations, etc...
    virtual void idle();

	// --- the actual view object creation functions
	//
	// --- do any API specific stuff first (currently AU only)
	void preCreateGUI();

	// --- create the GUI, or the GUI Designer if user is launching it
	bool createGUI(bool bShowGUIEditor);


	uint32_t numUIControls;

    void deleteGUIControlList()
    {
        for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it != pluginParameters.end(); ++it) {
            delete *it;
        }
        pluginParameters.clear();
    }

    PluginParameter* getGuiControlWithTag(int tag)
    {
        for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it != pluginParameters.end(); ++it) {
            PluginParameter* ctrl = *it;
            if(ctrl->getControlID() == tag)
                return ctrl;
        }
        return nullptr;
    }

    PluginGUIConnector* pluginGUIConnector;
    IGUIPluginConnector* guiPluginConnector;

	UIDescription* description;
	std::string viewName;
	std::string xmlFile;

	double zoomFactor;
	CVSTGUITimer* timer;

	CPoint minSize;
	CPoint maxSize;
	CRect nonEditRect;

	// --- flags for showing view
	bool showGUIEditor;
	bool createNewView;

    // --- our functions
	void save(bool saveAs = false);

	double guiDesignerWidth;
	double guiDesignerHeight;
	float guiWidth;
	float guiHeight;

    IGUIWindowFrame* guiWindowFrame;
    PluginGUIFrame* guiEditorFrame;
    std::string getGUIDesignerSize();

    uint32_t knobAction;
    virtual int32_t getKnobMode() const;

    // --- for AAX only
    int getControlID_WithMouseCoords(const CPoint& where);
    CControl* getControl_WithMouseCoords(const CPoint& where);

	// --- AAX Specific mouse/key stuff
    CMouseEventResult sendAAXMouseDown(int controlID, const CButtonState& buttons);
    CMouseEventResult sendAAXMouseMoved(int controlID, const CButtonState& buttons, const CPoint& where);

	// --- AU Specific mouse/key stuff
	CMouseEventResult sendAUMouseDown(CControl* control, const CButtonState& buttons);
	CMouseEventResult sendAUMouseMoved(CControl* control, const CButtonState& buttons, const CPoint& where);


#ifdef AAXPLUGIN
    void setAAXParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue);//, bool checkUpdateGUI = true);
    void updateGUIControlAAX(int tag, float actualPluginValue, float normalizedValue = 0.f, bool useNormalized = false);
    void setAAXViewContainer(AAX_IViewContainer* _aaxViewContainer){ aaxViewContainer = _aaxViewContainer;}
#endif

#ifdef AUPLUGIN
    AudioUnit m_AU;
    AUEventListenerRef AUEventListener;
    void setAU(AudioUnit inAU){m_AU = inAU;}

    void dispatchAUControlChange(int tag, float actualPluginValue, int message = -1, bool fromEventListener = false);//, bool checkUpdateGUI = false);
    void setAUEventFromGUIControl(CControl* control, int tag, float normalizedValue);//, bool checkUpdateGUI = true);
#endif

#ifdef VSTPLUGIN
	void setVSTParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue);//, bool checkUpdateGUI = true);
	void updateGUIControlVST(int tag, float normalizedValue);
#endif

#ifdef RAFXPLUGIN
	void setRAFXParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue);//, bool checkUpdateGUI = true);
	void updateGUIControlRAFX(int tag, float normalizedValue);
#endif

void setPluginParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue)
{
#ifdef AUPLUGIN
    setAUEventFromGUIControl(control, tag, actualValue);
#endif

#ifdef AAXPLUGIN
    setAAXParameterFromGUIControl(control, tag, actualValue, normalizedValue);
#endif

#ifdef VSTPLUGIN
	setVSTParameterFromGUIControl(control, tag, actualValue, normalizedValue);
#endif

#ifdef RAFXPLUGIN
	setRAFXParameterFromGUIControl(control, tag, actualValue, normalizedValue);
#endif
}

public:
	// --- this must happen AFTER the open( ) function creates the frame
	virtual void setGUIWindowFrame(IGUIWindowFrame* frame)
	{
		// --- we keep for resiging here
		guiWindowFrame = frame;
	}

	// --- notifier
	CMessageResult notify(CBaseObject* sender, IdStringPtr message) VSTGUI_OVERRIDE_VMETHOD;

	// --- IControlListener
	virtual void valueChanged(VSTGUI::CControl* pControl);

	virtual int32_t controlModifierClicked(VSTGUI::CControl* pControl, VSTGUI::CButtonState button) { return 0; }	///< return 1 if you want the control to not handle it, otherwise 0
	virtual void controlBeginEdit(VSTGUI::CControl* pControl) {}
	virtual void controlEndEdit(VSTGUI::CControl* pControl) {}
	virtual void controlTagWillChange(VSTGUI::CControl* pControl);
	virtual void controlTagDidChange(VSTGUI::CControl* pControl);
#if DEBUG
	virtual char controlModifierClicked(VSTGUI::CControl* pControl, long button) { return 0; }
#endif

    // --- IController
    CView* createView(const UIAttributes& attributes, const IUIDescription* description) VSTGUI_OVERRIDE_VMETHOD;
    IController* createSubController(UTF8StringPtr name, const IUIDescription* description) VSTGUI_OVERRIDE_VMETHOD;

    CControlUpdateReceiver* getControlUpdateReceiver(int32_t tag) const;

	// --- IViewAddedRemovedObserver
	void onViewAdded(CFrame* frame, CView* view) VSTGUI_OVERRIDE_VMETHOD {}
	void onViewRemoved(CFrame* frame, CView* view) VSTGUI_OVERRIDE_VMETHOD;

	//---  IMouseObserver
	void onMouseEntered(CView* view, CFrame* frame) VSTGUI_OVERRIDE_VMETHOD {}
	void onMouseExited(CView* view, CFrame* frame) VSTGUI_OVERRIDE_VMETHOD {}
    CMouseEventResult onMouseMoved(CFrame* frame, const CPoint& where, const CButtonState& buttons) VSTGUI_OVERRIDE_VMETHOD;
    CMouseEventResult onMouseDown(CFrame* frame, const CPoint& where, const CButtonState& buttons) VSTGUI_OVERRIDE_VMETHOD;

	// --- IKeyboardHook
	int32_t onKeyDown(const VstKeyCode& code, CFrame* frame) VSTGUI_OVERRIDE_VMETHOD;
	int32_t onKeyUp(const VstKeyCode& code, CFrame* frame) VSTGUI_OVERRIDE_VMETHOD;

    // --- helpers
    inline static bool parseSize (const std::string& str, CPoint& point)
    {
        size_t sep = str.find (',', 0);
        if (sep != std::string::npos)
        {
            point.x = strtol (str.c_str (), 0, 10);
            point.y = strtol (str.c_str () + sep+1, 0, 10);
            return true;
        }
        return false;
    }

    // --- for Meters only right now, but could be used to mark any control as writeable
    bool hasWriteableControl(CControl* control)
    {
        return std::find (writeableControls.begin (), writeableControls.end (), control) != writeableControls.end ();
    }

    void checkAddWriteableControl(PluginParameter* piParam, CControl* control)
    {
       if(!piParam)
            return;
        if(!control)
            return;
        if(!piParam->getIsWritable())
            return;
        if(!hasWriteableControl(control))
        {
            writeableControls.push_back(control);
            control->remember();
        }
    }

    void checkRemoveWriteableControl(CControl* control)
    {
        if(!hasWriteableControl(control)) return;

        for(std::vector<CControl*>::iterator it = writeableControls.begin(); it != writeableControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl == control)
            {
                ctrl->forget();
				writeableControls.erase(it);
				return;
            }
        }
    }

	void deleteControlUpdateReceivers()
	{
		for (CControlUpdateReceiverMap::const_iterator it = controlUpdateReceivers.begin(), end = controlUpdateReceivers.end(); it != end; ++it)
		{
			delete it->second;
		}
		controlUpdateReceivers.clear();
	}

	void forgetWriteableControls()
	{
		for (std::vector<CControl*>::iterator it = writeableControls.begin(); it != writeableControls.end(); ++it)
		{
			CControl* ctrl = *it;
			ctrl->forget();
		}
        writeableControls.clear();
	}

	void destroyPluginGUIConnector();

    bool hasICustomView(CView* view)
    {
        ICustomView* customView = dynamic_cast<ICustomView*>(view);
        if(customView)
            return true;
        return false;
    }

private:
    typedef std::map<int32_t, CControlUpdateReceiver*> CControlUpdateReceiverMap;
    CControlUpdateReceiverMap controlUpdateReceivers;
    std::vector<CControl*> writeableControls;
    std::vector<PluginParameter*> pluginParameters;
#ifdef AAXPLUGIN
    AAX_IViewContainer* aaxViewContainer = nullptr;
#endif
};

// --- IPluginGUIConnector object
class PluginGUIConnector : public IPluginGUIConnector
{
public:
    PluginGUIConnector(PluginGUI* _vstGUIEditor){vstGUIEditor = _vstGUIEditor;}
    virtual ~PluginGUIConnector(){}

    virtual void syncGUIControl(uint32_t controlID)
    {
        if(vstGUIEditor) vstGUIEditor->syncGUIControl(controlID);
    }
    protected:

    PluginGUI* vstGUIEditor;
};

// --- custom Frame object, mainly needed for a ProTools KeyDown fix
class PluginGUIFrame : public CFrame
{
public:
    PluginGUIFrame(const CRect &inSize, void* inSystemWindow, const PlatformType& platformType, VSTGUIEditorInterface* pEditor);
    ~PluginGUIFrame(void);
    
    // --- for protools SPR
    virtual bool platformOnKeyDown(VstKeyCode& keyCode);
};



}

#endif