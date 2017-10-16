
#include "PluginGUI.h"

// --- custom data view example; include more custom views here
#include "WaveFormView.h"

#if MAC
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>

#ifdef AUPLUGIN
// --- for au event granularity/interval; these are recommended defaults
const Float32 AUEVENT_GRANULARITY = 0.0005;
const Float32 AUEVENT_INTERVAL = 0.0005;

// --- this is called when presets change for us to synch GUI
void EventListenerDispatcher(void *inRefCon, void *inObject, const AudioUnitEvent *inEvent, UInt64 inHostTime, Float32 inValue)
{
    PluginGUI *pEditor = (PluginGUI*)inRefCon;
    if(!pEditor)
        return;

    // --- update the GUI control
    if(pEditor)
        pEditor->dispatchAUControlChange(inEvent->mArgument.mParameter.mParameterID, inValue, true);
}
#endif

namespace VSTGUI
{
    void* gBundleRef = 0;
    static int openCount = 0;

	static void CreateVSTGUIBundleRef();
	static void ReleaseVSTGUIBundleRef();

// --- BundleRef Functions for VSTGUI4 only ------------------------------------------------------------------------
void CreateVSTGUIBundleRef()
{
    openCount++;
    if(gBundleRef)
    {
        CFRetain(gBundleRef);
        return;
    }
#if TARGET_OS_IPHONE
    gBundleRef = CFBundleGetMainBundle();
    CFRetain(gBundleRef);
#else
    Dl_info info;
    if(dladdr((const void*)CreateVSTGUIBundleRef, &info))
    {
        if(info.dli_fname)
        {
            std::string name;
            name.assign (info.dli_fname);
            for (int i = 0; i < 3; i++)
            {
                int delPos = name.find_last_of('/');
                if (delPos == -1)
                {
                    fprintf (stdout, "Could not determine bundle location.\n");
                    return; // unexpected
                }
                name.erase (delPos, name.length () - delPos);
            }
            CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)name.c_str(), name.length (), true);
            if (bundleUrl)
            {
                gBundleRef = CFBundleCreate (0, bundleUrl);
                CFRelease (bundleUrl);
            }
        }
    }
#endif
}
void ReleaseVSTGUIBundleRef()
{
    openCount--;
    if(gBundleRef)
        CFRelease (gBundleRef);
    if(openCount == 0)
        gBundleRef = 0;
}

#elif WINDOWS
#pragma warning(disable : 4189)	// --- local variable is initialized but not referenced
#ifdef RAFXPLUGIN
void* hInstance; // VSTGUI hInstance
extern void* moduleHandle;
#endif

#ifdef AAXPLUGIN
extern void* hInstance;
#if WINDOWS_VERSION
#include <windows.h>
#include "vstgui/lib/platform/win32/win32frame.h"
#endif
#endif

#ifdef VSTPLUGIN
void* hInstance; // VSTGUI hInstance
extern void* moduleHandle;
#endif

namespace VSTGUI
{
#endif // MAC ---END  BundleRef Functions for VSTGUI4 only ------------------------------------------------------------------------

PluginGUI::PluginGUI(UTF8StringPtr _xmlFile) :
	IController(),
	IViewAddedRemovedObserver(),
	IMouseObserver(),
	IKeyboardHook(),
	VSTGUIEditorInterface(),
	CBaseObject(),
	IGUIView()
{
#if MAC
	CreateVSTGUIBundleRef();
#elif WINDOWS
#ifndef AAXPLUGIN // VST or RAFX Win
	hInstance = moduleHandle;
#endif
#endif

#ifdef AUPLUGIN
    m_AU = nullptr;
#endif

	// --- create description from XML file
	xmlFile = _xmlFile;
	description = new UIDescription(_xmlFile);
	if (!description->parse())
	{
		description->forget();
		description = nullptr;
	}

    // --- set attributes
    guiPluginConnector = nullptr;
    pluginGUIConnector = nullptr;

    guiDesignerWidth = 1024.0;
    guiDesignerHeight = 768.0;
	guiWidth = 400.0;
	guiHeight = 200.0;

	zoomFactor = 1.0;
	showGUIEditor = false;
	createNewView = true;

    setGUIWindowFrame(nullptr);

    knobAction = kLinearMode; // --- Default is linear

    // --- create timer
    timer = new CVSTGUITimer(dynamic_cast<CBaseObject*>(this));
}

PluginGUI::~PluginGUI()
{
	// --- destroy description
	if (description)
		description->forget();

	// --- kill timer
	if (timer)
		timer->forget();
#if MAC
	ReleaseVSTGUIBundleRef();
#endif
}

void PluginGUI::getSize(float& width, float& height)
{
    CRect rect;
    rect = frame->getViewSize();
	width = (float)rect.getWidth();
	height = (float)rect.getHeight();
}

IPluginGUIConnector* PluginGUI::open(UTF8StringPtr _viewName, void* parent, std::vector<PluginParameter*>* pluginParameterPtr, const PlatformType& platformType, IGUIPluginConnector* _guiPluginConnector, void* data)
{
#ifdef AUPLUGIN
    m_AU = AudioUnit(data); 
#endif

    guiPluginConnector = _guiPluginConnector;
    viewName = _viewName;
    guiWindowFrame = nullptr; 

    if(!pluginParameterPtr)
        return nullptr;

    // --- create our control list of PluginParameter*s
    deleteGUIControlList();
    int size = (int)pluginParameterPtr->size();
    for(int i=0; i<size; i++)
    {
        PluginParameter* ctrl = new PluginParameter(*(*pluginParameterPtr)[i]);
        pluginParameters.push_back(ctrl);
    }

	// --- add the preset file writer (not a plugin parameter, but a GUI parameter - does not need to be stored/refreshed
	PluginParameter* piParam = new PluginParameter(WRITE_PRESET_FILE, "Preset", "SWITCH_OFF,SWITCH_ON", "SWITCH_OFF");
	piParam->setIsDiscreteSwitch(true);
	pluginParameters.push_back(piParam);

    // --- set knob action
    UIAttributes* attributes = description->getCustomAttributes("Settings", true);
    if(attributes)
    {
        int32_t value = 0;
        attributes->getIntegerAttribute("KnobAction", value);
        knobAction = value;
    }

    // --- GUI Designer sizing
    attributes = description->getCustomAttributes("UIEditController", true);
    if(attributes)
    {
        if(attributes->hasAttribute("EditorSize"))
        {
            CRect r;
            if(attributes->getRectAttribute("EditorSize", r))
            {
                guiDesignerWidth = r.getWidth();
                guiDesignerHeight = r.getHeight();
            }
        }
    }

	// --- get the GUI size for scaling purposes (bonus parameter)
	const UIAttributes* viewAttributes = description->getViewAttributes("Editor");
	if (viewAttributes)
	{
		if (viewAttributes->hasAttribute("size"))
		{
			CPoint p;
			if (viewAttributes->getPointAttribute("size", p))
			{
				guiWidth = (float)p.x;
				guiHeight = (float)p.y;
			}
		}
	}

	// --- create empty frame
    const CRect frameSize(0, 0, 0, 0);
    guiEditorFrame = new PluginGUIFrame(frameSize, parent, platformType, (VSTGUIEditorInterface*)this);

    // --- save immediately so getFrame() will work
    frame = guiEditorFrame;

	guiEditorFrame->setTransparency(true);
	guiEditorFrame->registerMouseObserver((IMouseObserver*)this);
    guiEditorFrame->setViewAddedRemovedObserver((IViewAddedRemovedObserver*)this);

#if VSTGUI_LIVE_EDITING
	guiEditorFrame->registerKeyboardHook((IKeyboardHook*)this);
#endif
	guiEditorFrame->enableTooltips(true);

    // --- one time API-specific inits
    preCreateGUI();

    // --- create the views, size the frame
	if (!createGUI(showGUIEditor))
	{
		frame->forget();
		return nullptr;
	}

    // --- begin timer
    if (timer)
    {
        timer->setFireTime((uint32_t)GUI_METER_UPDATE_INTERVAL_MSEC);
        timer->start();
    }

    // --- for custom views
    if(guiPluginConnector)
        guiPluginConnector->guiDidOpen();

    // --- return interface
    pluginGUIConnector = new PluginGUIConnector(this);
	return pluginGUIConnector;
}

/*
 enum CKnobMode
 {
 kCircularMode = 0,
 kRelativCircularMode,
 kLinearMode
 };*/
int32_t PluginGUI::getKnobMode() const
{
    if(knobAction == kHostChoice)
        return kLinearMode; // DEFAULT MODE!
    else if(knobAction == kLinearMode)
        return kLinearMode;
    else if(knobAction == kRelativCircularMode)
        return kRelativCircularMode;
    else if(knobAction == kCircularMode)
        return kCircularMode;

    return kLinearMode;
}

void PluginGUI::close()
{
    // --- stop timer
    if(timer)
        timer->stop();

    // --- for custom views, null interface pointers
    if(guiPluginConnector)
        guiPluginConnector->guiWillClose();

	if (frame)
	{
#if VSTGUI_LIVE_EDITING
		guiEditorFrame->unregisterKeyboardHook((IKeyboardHook*)this);
#endif
		guiEditorFrame->unregisterMouseObserver((IMouseObserver*)this);

#ifdef AUPLUGIN
        if (AUEventListener) verify_noerr (AUListenerDispose(AUEventListener));
        AUEventListener = nullptr;
        m_AU = nullptr;
#endif
	
        deleteControlUpdateReceivers();
        forgetWriteableControls();
        destroyPluginGUIConnector();
        deleteGUIControlList();

        CFrame* oldFrame = frame;
        frame = 0;
        oldFrame->forget();
    }
}

void PluginGUI::destroyPluginGUIConnector()
{
	if (pluginGUIConnector)
	{
		delete pluginGUIConnector;
		pluginGUIConnector = nullptr;
	}
}

void PluginGUI::idle()
{
    if(!showGUIEditor)
    {
        // --- for custom views that need animation
        if(guiPluginConnector)
            guiPluginConnector->guiTimerPing();

        for(std::vector<CControl*>::iterator it = writeableControls.begin(); it != writeableControls.end(); ++it)
        {
            CControl* ctrl = *it;
            if(ctrl)
            {
                double param = 0.0;
                if(guiPluginConnector)
                {
                    param = guiPluginConnector->getNormalizedPluginParameter(ctrl->getTag());
                    ctrl->setValue((float)param);
                    ctrl->invalid();
                }
            }
        }
    }

    // --- update frame - important; this updates all children
    if(frame)
        frame->idle();
}



// --- ONE TIME init of controls, based on the API
void PluginGUI::preCreateGUI()
{
    if(!frame)
        return;

#ifdef AUPLUGIN
    // --- setup event listener
    if(m_AU)
    {
        // --- create the event listener and tell it the name of our Dispatcher function
        //     EventListenerDispatcher
        verify_noerr(AUEventListenerCreate(EventListenerDispatcher, this,
                                           CFRunLoopGetCurrent(), kCFRunLoopDefaultMode,
                                           AUEVENT_GRANULARITY, AUEVENT_INTERVAL,
                                           &AUEventListener));

        // --- add the rest of the params
        int nParams = pluginParameters.size();

        // --- start with first control 0
        AudioUnitEvent auEvent;

        if(nParams > 0)
        {
            PluginParameter* piParam = pluginParameters[0];
            if(piParam)
            {
                  // --- parameter 0
                AudioUnitParameter parameter = {m_AU, piParam->getControlID(), kAudioUnitScope_Global, 0};

                // --- set param & add it
                auEvent.mArgument.mParameter = parameter;
                auEvent.mEventType = kAudioUnitEvent_ParameterValueChange;
                verify_noerr(AUEventListenerAddEventType(AUEventListener, this, &auEvent));

                // --- add the rest of the params
                for(int i=1; i<nParams; i++)
                {
                    PluginParameter* piParam = pluginParameters[i];
                    if(piParam)
                    {
                        auEvent.mArgument.mParameter.mParameterID = piParam->getControlID();
                        verify_noerr(AUEventListenerAddEventType(AUEventListener, this, &auEvent));
                    }
                }
            }
        }
    }
#endif
}

// --- called when the control is created ( see ...TagDidChange() )
//     Note: in the case of ui-view-switch views, this will be called
//           at the time the new view is switched, NOT at startup, unless
//           it is view 0 in the stack
void PluginGUI::syncGUIControl(uint32_t controlID)
{
#ifdef AUPLUGIN
    double param = 0.0;
    if(guiPluginConnector)
    {
        param = guiPluginConnector->getActualParameter(controlID);
        dispatchAUControlChange(controlID, param, false, false);
    }
#endif

#ifdef AAXPLUGIN
    double param = 0.0;
    if(guiPluginConnector)
    {
        param = guiPluginConnector->getNormalizedParameter(controlID);
        updateGUIControlAAX(controlID, -1, (float)param, true); // true: use normalized version
    }
#endif

#ifdef VSTPLUGIN
	double paramNormalizedValue = 0.0;

	if (guiPluginConnector)
	{
		paramNormalizedValue = guiPluginConnector->getNormalizedParameter(controlID);
		updateGUIControlVST(controlID, (float)paramNormalizedValue);
	}
#endif

#ifdef RAFXPLUGIN
	double paramNormalizedValue = 0.0;

	if (guiPluginConnector)
	{
		paramNormalizedValue = guiPluginConnector->getNormalizedPluginParameter(controlID);
		updateGUIControlRAFX(controlID, (float)paramNormalizedValue);
	}
#endif

	// --- built-in bonus parameter for scaling GUI
	if (controlID == SCALE_GUI_SIZE)
	{
		CControlUpdateReceiver* receiver = getControlUpdateReceiver(controlID);
		if (receiver)
		{
			PluginParameter param = receiver->getGuiControl();
			scaleGUISize((uint32_t)param.getControlValue());
		}
	}
}

CMessageResult PluginGUI::notify(CBaseObject* sender, IdStringPtr message)
{
	if(message == CVSTGUITimer::kMsgTimer)
	{
		if(createNewView)
			createGUI(showGUIEditor);

        // --- timer ping for control updates
        if(frame)
          idle();
	}

#if VSTGUI_LIVE_EDITING
	// --- enable/disable Save menu item - tries to find path
	else if(message == CCommandMenuItem::kMsgMenuItemValidate)
	{
		CCommandMenuItem* item = dynamic_cast<CCommandMenuItem*>(sender);
		if (item)
		{
			if (strcmp(item->getCommandCategory(), "File") == 0)
			{
				if (strcmp(item->getCommandName(), "Save") == 0)
				{
					bool enable = false;
					UIAttributes* attributes = description->getCustomAttributes("VST3Editor", true);
					if (attributes)
					{
						const std::string* filePath = attributes->getAttributeValue("Path");
						if (filePath)
						{
                            if(!filePath->empty())
                                enable = true;
						}
					}
					item->setEnabled(enable);
					return kMessageNotified;
				}
			}
		}
	}
	else if (message == CCommandMenuItem::kMsgMenuItemSelected)
	{
		CCommandMenuItem* item = dynamic_cast<CCommandMenuItem*>(sender);
		if (item)
		{
			UTF8StringView cmdCategory = item->getCommandCategory();
			UTF8StringView cmdName = item->getCommandName();
			
			if (cmdCategory == "File")
			{
                if (cmdName == "Open UIDescription Editor")
                {
                    showGUIEditor = true;
                    createNewView = true;
                    return kMessageNotified;
                }
 				else if (cmdName == "Close UIDescription Editor")
				{
					showGUIEditor = false;
					createNewView = true;
					return kMessageNotified;
				}
				else if (cmdName == "Save")
				{
					save(false);
					item->setChecked(false);
					return kMessageNotified;
				}
                else if (cmdName == "Save As")
                {
                    save(true);
                    item->setChecked(false);
                    return kMessageNotified;
                }
                else if (cmdName == "Increase Width")
                {
                    // --- 10% UP
                    guiDesignerWidth += guiDesignerWidth*0.10;
                    showGUIEditor = true;
                    createNewView = true;

                    return kMessageNotified;
                }
                else if (cmdName == "Decrease Width")
                {
                    // --- 10% DOWN
                    guiDesignerWidth -= guiDesignerWidth*0.10;
                    showGUIEditor = true;
                    createNewView = true;

                    return kMessageNotified;
                }
                else if (cmdName == "Increase Height")
                {
                    // --- 10% UP
                    guiDesignerHeight += guiDesignerHeight*0.10;
                    showGUIEditor = true;
                    createNewView = true;

                    return kMessageNotified;
                }
                else if (cmdName == "Decrease Height")
                {
                    // --- 10% DOWN
                    guiDesignerHeight -= guiDesignerHeight*0.10;
                    showGUIEditor = true;
                    createNewView = true;

                    return kMessageNotified;
                }
			}
		}
	}
#endif
	return kMessageNotified;
}

void PluginGUI::save(bool saveAs)
{
    if(!showGUIEditor) return;
    
    int32_t flags = 0;
#if VSTGUI_LIVE_EDITING
    UIEditController* editController = dynamic_cast<UIEditController*> (getViewController (frame->getView (0)));
    if (editController)
    {
        UIAttributes* attributes = editController->getSettings ();
        bool val;
        if (attributes->getBooleanAttribute (UIEditController::kEncodeBitmapsSettingsKey, val) && val == true)
        {
            flags |= UIDescription::kWriteImagesIntoXMLFile;
        }
        if (attributes->getBooleanAttribute (UIEditController::kWriteWindowsRCFileSettingsKey, val) && val == true)
        {
            flags |= UIDescription::kWriteWindowsResourceFile;
        }
    }
#endif

	UIAttributes* attributes = description->getCustomAttributes("VST3Editor", true);
	vstgui_assert(attributes);
    const std::string* filePath = attributes->getAttributeValue("Path");
    if(!filePath) saveAs = true;
    if(filePath && filePath->empty()) saveAs = true;
    
	std::string savePath;

    if (saveAs)
	{
		CNewFileSelector* fileSelector = CNewFileSelector::create(frame, CNewFileSelector::kSelectSaveFile);
		if (fileSelector == 0)
			return;

        fileSelector->setTitle("Save UIDescription File");

        // --- NOTE: setDefaultExtension() causes a crash
		// fileSelector->setDefaultExtension(CFileExtension("VSTGUI UI Description", "uidesc"));
		if (filePath && !filePath->empty())
		{
			fileSelector->setInitialDirectory(filePath->c_str());
		}

        // --- only one option
        fileSelector->setDefaultSaveName("PluginGUI.uidesc");

		if (fileSelector->runModal())
		{
            UTF8StringPtr filePathSelected = fileSelector->getSelectedFile(0);
			if(filePathSelected)
			{
				attributes->setAttribute("Path", filePathSelected);
                attributes = description->getCustomAttributes("UIEditController", true);
                std::string editorSize = getGUIDesignerSize();
                attributes->setAttribute("EditorSize", editorSize.c_str());
				savePath = filePathSelected;
			}
		}
		fileSelector->forget();
	}
	else
	{
        std::string editorSize = getGUIDesignerSize();
        UIAttributes* attributes = description->getCustomAttributes("UIEditController", true);
        attributes->setAttribute("EditorSize", editorSize.c_str());

		if (filePath && !filePath->empty())
			savePath = *filePath;
	}
	if (savePath.empty())
		return;

	// --- save file
	description->save(savePath.c_str(), flags);
}

std::string PluginGUI::getGUIDesignerSize()
{
    std::string editorSize = "0,0,";

    std::stringstream strW;
    strW << guiDesignerWidth;
    editorSize.append((strW.str().c_str()));

    editorSize.append(",");

    std::stringstream strH;
    strH << guiDesignerHeight;
    editorSize.append((strH.str().c_str()));

    return editorSize;
}

bool PluginGUI::createGUI(bool bShowGUIEditor)
{
	createNewView = false;

	if (getFrame())
	{
		getFrame()->removeAll();

#if VSTGUI_LIVE_EDITING
		if (bShowGUIEditor)
		{
			guiEditorFrame->setTransform(CGraphicsTransform());
			nonEditRect = guiEditorFrame->getViewSize();
			description->setController((IController*)this);

            // --- persistent
            UIAttributes* attributes = description->getCustomAttributes("UIEditController", true);
            if(attributes)
            {
                std::string editorSize = getGUIDesignerSize();
                attributes->setAttribute("EditorSize", editorSize.c_str());
            }

            // --- new way, stopped bug with
			UIEditController* editController = new UIEditController(description);
			CView* view = editController->createEditView();
			if (view)
			{
				showGUIEditor = true;
				getFrame()->addView(view);
				int32_t autosizeFlags = view->getAutosizeFlags();
				view->setAutosizeFlags(0);

                // added WP
                getFrame()->setFocusDrawingEnabled(false);

                CRect rect(0,0,0,0);
                rect.right = view->getWidth();
                rect.bottom = view->getHeight();

                getFrame()->setSize(view->getWidth(), view->getHeight());
                rect.right = view->getWidth();
                rect.bottom = view->getHeight();

                view->setAutosizeFlags(autosizeFlags);

				getFrame()->enableTooltips(true);
				CColor focusColor = kBlueCColor;
				editController->getEditorDescription().getColor("focus", focusColor);
				getFrame()->setFocusColor(focusColor);
				getFrame()->setFocusDrawingEnabled(true);
				getFrame()->setFocusWidth(1);

				COptionMenu* fileMenu = editController->getMenuController()->getFileMenu();
				
				if (fileMenu)
				{
					CMenuItem* item = fileMenu->addEntry(new CCommandMenuItem("Save", this, "File", "Save"), 0);
					item->setKey("s", kControl);
					item = fileMenu->addEntry(new CCommandMenuItem("Save As..", this, "File", "Save As"), 1);
					item->setKey("s", kShift | kControl);
					item = fileMenu->addEntry(new CCommandMenuItem("Close Editor", this, "File", "Close UIDescription Editor"));
					item->setKey("e", kControl);
					
                    fileMenu->addSeparator();
                    item = fileMenu->addEntry(new CCommandMenuItem("Increase Width", this, "File", "Increase Width"));
                    item->setKey(">", kShift);
                    item = fileMenu->addEntry(new CCommandMenuItem("Decrease Width", this, "File", "Decrease Width"));
                    item->setKey("<", kShift);
                    item = fileMenu->addEntry(new CCommandMenuItem("Increase Height", this, "File", "Increase Height"));
                    item->setKey(">", kShift | kAlt);
                    item = fileMenu->addEntry(new CCommandMenuItem("Decrease Height", this, "File", "Decrease Height"));
                    item->setKey("<", kShift | kAlt);

				}

                // --- resize it (added WP)
				if (guiWindowFrame)
				{
					guiWindowFrame->setWindowFrameSize(rect.left, rect.top, rect.right, rect.bottom);
					guiWindowFrame->enableGUIDesigner(true);
				}

   				return true;
			}
			editController->forget();
		}
		else
#endif
		{
			showGUIEditor = false;
			CView* view = description->createView(viewName.c_str(), this);
			if (view)
			{
				CCoord width = view->getWidth() * zoomFactor;
				CCoord height = view->getHeight() * zoomFactor;

				getFrame()->setSize(width, height);
				getFrame()->addView(view);
				//getFrame()->setTransform(CGraphicsTransform().scale(zoomFactor, zoomFactor));
				getFrame()->setZoom(zoomFactor);
				getFrame()->invalid();

                CRect rect(0,0,0,0);
                rect.right = width;
                rect.bottom = height;

				// --- resize, in case coming back from editing
				if(guiWindowFrame)
				{
					guiWindowFrame->setWindowFrameSize(rect.left, rect.top, rect.right, rect.bottom);//->resizeView(this, &viewRect) == Steinberg::kResultTrue ? true : false;
					guiWindowFrame->enableGUIDesigner(false);
				}

                getFrame()->setFocusDrawingEnabled(false);
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void PluginGUI::onViewRemoved(CFrame* frame, CView* view)
{
	CControl* control = dynamic_cast<CControl*> (view);
	if (control && control->getTag() != -1)
	{
		// --- check for trackpad and joystick
		CXYPadWP* xyPad = dynamic_cast<CXYPadWP*>(control);
		if (xyPad)
		{
			// --- retrieve the X and Y tags on the CXYPadWP
			int32_t tagX = xyPad->getTagX();
			int32_t tagY = xyPad->getTagY();

			if (tagX != -1)
			{
				CControlUpdateReceiver* receiver = getControlUpdateReceiver(tagX);
				if (receiver)
				{
					receiver->removeControl(control);
					checkRemoveWriteableControl(control);
				}
			}
			if (tagY != -1)
			{
				CControlUpdateReceiver* receiver = getControlUpdateReceiver(tagY);
				if (receiver)
				{
					receiver->removeControl(control);
					checkRemoveWriteableControl(control);
				}
			}
			return;
		}

		CControlUpdateReceiver* receiver = getControlUpdateReceiver(control->getTag());
		if (receiver)
		{
			receiver->removeControl(control);
			checkRemoveWriteableControl(control);
		}
	}
}


void PluginGUI::controlTagWillChange(VSTGUI::CControl* pControl)
{
    if (pControl->getTag() != -1 && pControl->getListener() == this)
    {
        CControlUpdateReceiver* receiver = getControlUpdateReceiver(pControl->getTag());
        if(receiver)
        {
            receiver->removeControl(pControl);
            checkRemoveWriteableControl(pControl);
        }
    }

	return;
}

// --- called when controls are created in UIDescription
void PluginGUI::controlTagDidChange(VSTGUI::CControl* pControl)
{
    if(pControl->getTag() == -1 || pControl->getListener() != this)
        return;
    
	// --- for preset file helper
	if (pControl->getTag() == PRESET_NAME)
	{
		presetNameTextEdit = dynamic_cast<CTextEdit*>(pControl);
		return;
	}

	if(pControl->getTag() == TRACKPAD)
	{
		bool transmitter = pControl->getListener() == this ? true : false;

		// --- check for trackpad and joystick
		CXYPadWP* xyPad = dynamic_cast<CXYPadWP*>(pControl);
		if (xyPad)
		{
			// --- retrieve the X and Y tags on the xyPad CView
 			int32_t tagX = xyPad->getTagX();
			int32_t tagY = xyPad->getTagY();

			if (tagX != -1)
			{
				CControlUpdateReceiver* receiver = getControlUpdateReceiver(tagX);
				PluginParameter* ctrl = getGuiControlWithTag(tagX);
				checkAddWriteableControl(ctrl, pControl);

				if (receiver)
				{
					receiver->addControl(pControl);
					syncGUIControl(tagX);
				}
				else
				{
					CControlUpdateReceiver* receiver = new CControlUpdateReceiver(pControl, ctrl, transmitter);
					controlUpdateReceivers.insert(std::make_pair(tagX, receiver));
					syncGUIControl(tagX);
				}
			}

			if (tagY != -1)
			{
				CControlUpdateReceiver* receiver = getControlUpdateReceiver(tagY);
				PluginParameter* ctrl = getGuiControlWithTag(tagY);
				checkAddWriteableControl(ctrl, pControl);

				if (receiver)
				{
					receiver->addControl(pControl);
					syncGUIControl(tagY);
				}
				else
				{
					CControlUpdateReceiver* receiver = new CControlUpdateReceiver(pControl, ctrl, transmitter);
					controlUpdateReceivers.insert(std::make_pair(tagY, receiver));
					syncGUIControl(tagY);
				}
			}
			return;
		}
    }
    
    if (pControl->getTag() == -1) return;
    bool transmitter = pControl->getListener() == this ? true : false;

    CControlUpdateReceiver* receiver = getControlUpdateReceiver(pControl->getTag());
    PluginParameter* ctrl = getGuiControlWithTag(pControl->getTag());
    checkAddWriteableControl(ctrl, pControl);

    if (receiver)
    {
        receiver->addControl(pControl);
        syncGUIControl(pControl->getTag());
    }
    else
    {
        // --- ctrl may be NULL for tab controls and other non-variable linked items
        CControlUpdateReceiver* receiver = new CControlUpdateReceiver(pControl, ctrl, transmitter);
        controlUpdateReceivers.insert(std::make_pair(pControl->getTag(), receiver));
		syncGUIControl(pControl->getTag());
    }
	return;
}

void PluginGUI::writeToPresetFile(std::string presetName)
{
	if (!guiPluginConnector)
		return;
	
//	std::string name = "preset_";
	std::string name = "";
	name.append(presetName);

	//bool saveAs = false;
	//UIAttributes* attributes = description->getCustomAttributes("PresetFile", true);
	//vstgui_assert(attributes);
	//const std::string* filePath = attributes->getAttributeValue("Path");
	//if (!filePath) saveAs = true;
	//if (filePath && filePath->empty()) saveAs = true;

	std::string savePath;
	std::string saveFile = name;
	saveFile.append(".txt");

	CNewFileSelector* fileSelector = CNewFileSelector::create(frame, CNewFileSelector::kSelectSaveFile);
	if (fileSelector == 0)
		return;

	fileSelector->setTitle("Save Preset File");

	// --- NOTE: setDefaultExtension() causes a crash
	// fileSelector->setDefaultExtension(CFileExtension("VSTGUI UI Description", "uidesc"));

	// --- only one option
	fileSelector->setDefaultSaveName(saveFile.c_str());

	if (fileSelector->runModal())
	{
		UTF8StringPtr filePathSelected = fileSelector->getSelectedFile(0);
		if (filePathSelected)
		{
			savePath = filePathSelected;
		}
	}
	fileSelector->forget();

	if (savePath.empty())
		return;

	CFileStream stream;
	if (stream.open(savePath.c_str(), CFileStream::kWriteMode | CFileStream::kTruncateMode))
	{
		stream.seek(0, SeekableStream::kSeekEnd);
	
		stream << "\tint index = 0;\t/*** declare this once at the top of the presets function, comment out otherwise */\n";
		std::string comment = "\t// --- Plugin Preset: ";
		comment.append(name);
		comment.append("\n");
		stream << comment;

		std::string declaration = "\tPresetInfo* ";
		declaration.append(name);
		declaration.append(" = new PresetInfo(index++, \"");
		declaration.append(presetName);
		declaration.append("\");\n");
		stream << declaration;

		// --- initialize
		std::string presetStr2 = "\tinitPresetParameters(";
		presetStr2.append(name);
		presetStr2.append("->presetParameters);\n");
		stream << presetStr2;

		size_t size = this->pluginParameters.size();
		for (unsigned int i = 0; i<size-1; i++) // size-1 because last parameter is WRITE_PRESET_FILE
		{
			PluginParameter* piParam = pluginParameters[i];
			if (piParam)
			{
				std::string paramName = "\t// --- ";
				paramName.append(piParam->getControlName());
				paramName.append("\n");

				uint32_t id = piParam->getControlID();
				double value = guiPluginConnector->getActualPluginParameter(piParam->getControlID());

				std::string presetStr2 = "\tsetPresetParameter(";
				presetStr2.append(name);
				presetStr2.append("->presetParameters, ");

				std::ostringstream controlID_ss;
				std::ostringstream controlValue_ss;

				controlID_ss << id;
				controlValue_ss << value;
				std::string controlID(controlID_ss.str());
				std::string controlValue(controlValue_ss.str());

				presetStr2.append(controlID);
				presetStr2.append(", ");
				presetStr2.append(controlValue);
				presetStr2.append(");");
				presetStr2.append(paramName); // has \n
				stream << presetStr2;
			}
		}
		// --- add the preset
		if (size > 1)
		{
			std::string presetStr3 = "\taddPreset(";
			presetStr3.append(name);
			presetStr3.append(");\n\n");
			stream << presetStr3;
		}
	}
}

void PluginGUI::scaleGUISize(uint32_t controlValue)
{
	float scalePercent = 100.f;
	switch (controlValue)
	{
		case tinyGUI:
		{
			scalePercent = 65.f;
			break;
		}
		case verySmallGUI:
		{
			scalePercent = 75.f;
			break;
		}
		case smallGUI:
		{
			scalePercent = 85.f;
			break;
		}
		case normalGUI:
		{
			scalePercent = 100.f;
			break;
		}
		case largeGUI:
		{
			scalePercent = 125.f;
			break;
		}
		case veryLargeGUI:
		{
			scalePercent = 150.f;
			break;
		}
	}

	double width = guiWidth;
	double height = guiHeight;
	zoomFactor = scalePercent / 100.f;
	width *= zoomFactor;
	height *= zoomFactor;

	getFrame()->setSize(width, height);
	//getFrame()->setTransform(CGraphicsTransform().scale(zoomFactor, zoomFactor));
	getFrame()->setZoom(zoomFactor);
	getFrame()->invalid();

	CRect rect(0, 0, 0, 0);
	rect.right = width;
	rect.bottom = height;

	// --- resize
	if (guiWindowFrame)
	{
		guiWindowFrame->setWindowFrameSize(rect.left, rect.top, rect.right, rect.bottom);
	}
}

void PluginGUI::valueChanged(VSTGUI::CControl* pControl)
{
 	// --- handle changes not bound to plugin variables (tab controls, joystick)
    if(guiPluginConnector && guiPluginConnector->checkNonBoundValueChange(pControl->getTag(), pControl->getValueNormalized()))
        return;

    float actualValue = 0.f;

	// --- check default controls
	if (pControl->getTag() == SCALE_GUI_SIZE)
	{
		// --- scale the GUI
		scaleGUISize((uint32_t)pControl->getValue());

		// --- update variable for persistence
		CControlUpdateReceiver* receiver = getControlUpdateReceiver(pControl->getTag());
		float normalizedValue = pControl->getValueNormalized();
		if (receiver)
		{
			// --- this CONTROL update uses the control normalized value, no taper
			receiver->updateControlsWithNormalizedValue(pControl->getValueNormalized(), pControl);
			actualValue = receiver->getActualValueWithNormalizedValue(pControl->getValueNormalized());
			setPluginParameterFromGUIControl(pControl, pControl->getTag(), actualValue, normalizedValue);
		}
		return;
	}

	// --- save preset code for easy pasting into non-RackAFX projects
	if (pControl->getTag() == WRITE_PRESET_FILE)
	{
		std::string preset;

		// --- an easier option is to leave off the edit control for this, and just have them enter the name
		//     in the file-save dialog!
		//if (presetNameTextEdit)
		//{
		//	const UTF8String presetName = presetNameTextEdit->getText();
		//	preset.assign(presetName);
		//}

		// --- save it
		writeToPresetFile(preset);
		return;
	}
	
    // --- check for trackpad and joystick
    CXYPadWP* xyPad = dynamic_cast<CXYPadWP*>(pControl);
    if(xyPad)
    {
        float x = 0.f;
        float y = 0.f;
        xyPad->calculateXY(pControl->getValue(), x, y);
 
        CControlUpdateReceiver* receiver = getControlUpdateReceiver(xyPad->getTagX());
        if(receiver)
        {
            receiver->updateControlsWithNormalizedValue(x, pControl);
            actualValue = receiver->getActualValueWithNormalizedValue(x);
        }

        setPluginParameterFromGUIControl(pControl, xyPad->getTagX(), actualValue, x);

        receiver = getControlUpdateReceiver(xyPad->getTagY());
        if(receiver)
        {
            receiver->updateControlsWithNormalizedValue(y, pControl);
            actualValue = receiver->getActualValueWithNormalizedValue(y);
        }

        setPluginParameterFromGUIControl(pControl, xyPad->getTagY(), actualValue, y);
        return;
    }

    CControlUpdateReceiver* receiver = getControlUpdateReceiver(pControl->getTag());
    float normalizedValue = pControl->getValueNormalized();
    if(receiver)
    {
        // --- this CONTROL update uses the control normalized value, no taper
        receiver->updateControlsWithNormalizedValue(pControl->getValueNormalized(), pControl);
        
        // --- get the actual value, with tapers applied
        actualValue = receiver->getActualValueWithNormalizedValue(pControl->getValueNormalized());
        
        // --- get the normalized value, with tapers applied
        normalizedValue = receiver->getNormalizedValueWithActualValue(actualValue);
    }

    setPluginParameterFromGUIControl(pControl, pControl->getTag(), actualValue, normalizedValue);
}

#ifdef AUPLUGIN
void PluginGUI::dispatchAUControlChange(int tag, float actualPluginValue, int message, bool fromEventListener)//, bool checkUpdateGUI)
{
    CControlUpdateReceiver* receiver = getControlUpdateReceiver(tag);
    if(receiver)
        receiver->updateControlsWithActualValue(actualPluginValue);
}

void PluginGUI::setAUEventFromGUIControl(CControl* control, int tag, float actualValue)//, bool checkUpdateGUI)
{
    // --- notification only for custom GUI updates
    if(guiPluginConnector)
        guiPluginConnector->parameterChanged(tag, actualValue, actualValue);

    CControlUpdateReceiver* receiver = getControlUpdateReceiver(tag);
    if(!receiver) return;
    
    if(getGuiControlWithTag(tag))
    {
        AudioUnitParameter param = {m_AU, static_cast<AudioUnitParameterID>(tag), kAudioUnitScope_Global, 0};
        AUParameterSet(AUEventListener, control, &param, actualValue, 0);
    }
}
#endif

#ifdef AAXPLUGIN

void PluginGUI::setAAXParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue)//, bool checkUpdateGUI)
{
    // --- notification for custom GUI updates & safe parameter set()
    if(guiPluginConnector)
    {
        guiPluginConnector->parameterChanged(tag, actualValue, normalizedValue);
        guiPluginConnector->setNormalizedParameter(tag, normalizedValue);
    }
}

void PluginGUI::updateGUIControlAAX(int tag, float actualPluginValue, float normalizedValue, bool useNormalized)
{
    CControlUpdateReceiver* receiver = getControlUpdateReceiver(tag);
    if(receiver)
    {
        // --- internal calls to this need the normalized values (thread safety)
        if(useNormalized)
            receiver->updateControlsWithNormalizedValue(normalizedValue);
        else
            receiver->updateControlsWithActualValue(actualPluginValue); // external updates use actual value
    }
}

#endif

#ifdef VSTPLUGIN
void PluginGUI::setVSTParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue)
{
    // --- notification parameter sync and for custom GUI updates
    if(guiPluginConnector)
        guiPluginConnector->parameterChanged(tag, actualValue, normalizedValue);
}

void PluginGUI::updateGUIControlVST(int tag, float normalizedValue)
{
    CControlUpdateReceiver* receiver = getControlUpdateReceiver(tag);
    if(receiver)
    {
        receiver->updateControlsWithNormalizedValue(normalizedValue);
    }
}

#endif

#ifdef RAFXPLUGIN
void PluginGUI::setRAFXParameterFromGUIControl(CControl* control, int tag, float actualValue, float normalizedValue)//, bool checkUpdateGUI = true);
{
	// --- notification parameter sync and for custom GUI updates
	if (guiPluginConnector)
		guiPluginConnector->parameterChanged(tag, actualValue, normalizedValue);
}

void PluginGUI::updateGUIControlRAFX(int tag, float normalizedValue)
{
	CControlUpdateReceiver* receiver = getControlUpdateReceiver(tag);
	if (receiver)
	{
		receiver->updateControlsWithNormalizedValue(normalizedValue);
	}
}
#endif

// --- for AAX only
int PluginGUI::getControlID_WithMouseCoords(const CPoint& where)
{
    for (CControlUpdateReceiverMap::const_iterator it = controlUpdateReceivers.begin(), end = controlUpdateReceivers.end(); it != end; ++it)
    {
        // it->second is receiver
        CControlUpdateReceiver* receiver = it->second;
        if(receiver)
        {
            int controlID = receiver->getControlID_WithMouseCoords(where);
            if(controlID != -1)
                return controlID;
            // else keep looking
        }
    }

    return -1;
}
    
CControl* PluginGUI::getControl_WithMouseCoords(const CPoint& where)
{
    for (CControlUpdateReceiverMap::const_iterator it = controlUpdateReceivers.begin(), end = controlUpdateReceivers.end(); it != end; ++it)
    {
        // it->second is receiver
        CControlUpdateReceiver* receiver = it->second;
        if(receiver)
        {
            CControl* control = receiver->getControl_WithMouseCoords(where);
            if(control)
                return control;
            // else keep looking
        }
    }
    
    return nullptr;
}

    
// --- createSubController: currently not used, maybe do a simple example for book
//-----------------------------------------------------------------------------
IController* PluginGUI::createSubController(UTF8StringPtr name, const IUIDescription* description)
{
	return nullptr;
}

//-----------------------------------------------------------------------------
CView* PluginGUI::createView(const UIAttributes& attributes, const IUIDescription* description)
{
	const std::string* customViewName = attributes.getAttributeValue(IUIDescription::kCustomViewName);
	if (!customViewName) return nullptr;
	const std::string viewname(customViewName->c_str());
	if (viewname.empty()) return nullptr;

    // --- USER CUSTOM VIEWS
    //
    // --- decode the custom view string
    if(viewname == "CustomWaveView")
    {
        // --- our wave view testing object
        const std::string* sizeString = attributes.getAttributeValue("size");
        const std::string* originString = attributes.getAttributeValue("origin");
        const std::string* tagString = attributes.getAttributeValue("control-tag");
		if (!tagString) return nullptr;

        // --- create the rect
        CPoint origin;
        CPoint size;
        parseSize(*sizeString, size);
        parseSize(*originString, origin);
        
        const CRect rect(origin, size);
        
		int32_t tag = -1;
		IControlListener* listener = nullptr;
			
		// --- get listener
		listener = description->getControlListener(tagString->c_str());

		// --- get tag
		tag = description->getTagForName(tagString->c_str());
        
        // --- create our custom view
        CWaveFormView* waveView = new CWaveFormView(rect, listener, tag);
        
        // --- if the view has the ICustomView interface, we register it with the plugin for updates
        if(hasICustomView(waveView))
        {
            if (guiPluginConnector)
                guiPluginConnector->registerCustomView(viewname, (ICustomView*)waveView);
        }
        
        return waveView;
    }
    
    
	// --- BUILT-IN CUSTOM VIEWS
    int nTP = (int)viewname.find("TrackPad_");
    if(nTP >= 0)
    {
        const std::string* sizeString = attributes.getAttributeValue("size");
        const std::string* originString = attributes.getAttributeValue("origin");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        
        // --- rect
        CPoint origin;
        CPoint size;
        parseSize(*sizeString, size);
        parseSize(*originString, origin);
        
        const CRect rect(origin, size);
 
        // --- decoding code
        int x = (int)viewname.find("_X");
        int y = (int)viewname.find("_Y");
        int len = (int)viewname.length();
        
        if(x < 0 || y < 0 || len < 0)
            return nullptr;
        
        if(x < y && y < len)
        {
            std::string strX = viewname.substr(x + 2, y - 2 - x);
            std::string strY = viewname.substr(y + 2, len - 2 - y);
            int32_t _tagX = atoi(strX.c_str());
            int32_t _tagY = atoi(strY.c_str());
            
            CXYPadWP* p = new CXYPadWP(rect);
            if(!p) return nullptr;
            
            // --- save tags
            p->setTagX(_tagX);
            p->setTagY(_tagY);
            
            return p;
        }
    }

    if (viewname == "CustomKickButton" ||
		viewname == "CustomKickButtonDU" ||
		viewname == "CustomKickButtonU" ||
		viewname == "CustomKickButtonD")
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
		const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* bitmapString = attributes.getAttributeValue("bitmap");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
		const std::string* offsetString = attributes.getAttributeValue("background-offset");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        if(!bitmapString) return nullptr;
        if(!tagString) return nullptr;
        if(!offsetString) return nullptr;

		// --- rect
		CPoint origin;
		CPoint size;
		parseSize(*sizeString, size);
		parseSize(*originString, origin);

		const CRect rect(origin, size);

		// --- tag
		int32_t tag = description->getTagForName(tagString->c_str());

		// --- listener "hears" the control
		const char* controlTagName = tagString->c_str();
		IControlListener* listener = description->getControlListener(controlTagName);

		// --- bitmap
		std::string BMString = *bitmapString;
		BMString += ".png";
		UTF8StringPtr bmp = BMString.c_str();
		CResourceDescription bmpRes(bmp);
		CBitmap* pBMP = new CBitmap(bmpRes);

		// --- offset
		CPoint offset;
		parseSize(*offsetString, offset);

		CKickButtonWP* p = new CKickButtonWP(rect, listener, tag, pBMP, offset);
		if (p)
		{
			if (viewname == "CustomKickButtonDU")
				p->setMouseMode(mouseDirUpAndDown);
			else if (viewname == "CustomKickButtonD")
				p->setMouseMode(mouseDirDown);
			else if (viewname == "CustomKickButtonU")
				p->setMouseMode(mouseDirUp);
			else
				p->setMouseMode(mouseDirUp); // old
		}
		if (pBMP) pBMP->forget();

		return p;
	}

	if (viewname == "CustomTextButton" ||
		viewname == "CustomTextButtonDU" ||
		viewname == "CustomTextButtonU" ||
		viewname == "CustomTextButtonD")
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
		const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
		const std::string* titleString = attributes.getAttributeValue("title");
		if (!sizeString) return nullptr;
		if (!originString) return nullptr;
		if (!tagString) return nullptr;
		if (!titleString) return nullptr;

		// --- rect
		CPoint origin;
		CPoint size;
		parseSize(*sizeString, size);
		parseSize(*originString, origin);

		const CRect rect(origin, size);

		// --- tag
		int32_t tag = description->getTagForName(tagString->c_str());

		// --- listener "hears" the control
		const char* controlTagName = tagString->c_str();
		IControlListener* listener = description->getControlListener(controlTagName);

		CTextButtonWP* p = new CTextButtonWP(rect, listener, tag, titleString->c_str(), CTextButton::Style::kKickStyle);
		if (p)
		{
			if (viewname == "CustomTextButtonDU")
				p->setMouseMode(mouseDirUpAndDown);
			else if (viewname == "CustomTextButtonD")
				p->setMouseMode(mouseDirDown);
			else if (viewname == "CustomTextButtonU")
				p->setMouseMode(mouseDirUp);
			else
				p->setMouseMode(mouseDirUp); // old
		}
		return p;
	}


	// --- special knobs
	if (viewname == "KnobSwitchView" || viewname == "UniversalAPIKnob")
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
        const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* offsetString = attributes.getAttributeValue("background-offset");
		const std::string* bitmapString = attributes.getAttributeValue("bitmap");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
		const std::string* heightOneImageString = attributes.getAttributeValue("height-of-one-image");
		const std::string* subPixmapsString = attributes.getAttributeValue("sub-pixmaps");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        if(!offsetString) return nullptr;
        if(!bitmapString) return nullptr;
        if(!tagString) return nullptr;
        if(!heightOneImageString) return nullptr;
        if(!subPixmapsString) return nullptr;

        bool isSwitchControl = viewname == "KnobSwitchView" ? true : false;
        bool isUniversalAPIControl = viewname == "UniversalAPIKnob" ? true : false;

		// --- rect
		CPoint origin;
		CPoint size;
		parseSize(*sizeString, size);
		parseSize(*originString, origin);

		const CRect rect(origin, size);

		// --- listener "hears" the control
		const char* controlTagName = tagString->c_str();
		IControlListener* listener = description->getControlListener(controlTagName);

		// --- tag
		int32_t tag = description->getTagForName(tagString->c_str());

		// --- subPixmaps
		int32_t subPixmaps = strtol(subPixmapsString->c_str(), 0, 10);

		// --- height of one image
		CCoord heightOfOneImage = strtod(heightOneImageString->c_str(), 0);

		// --- bitmap
		std::string BMString = *bitmapString;
		BMString += ".png";
		UTF8StringPtr bmp = BMString.c_str();
		CResourceDescription bmpRes(bmp);
		CBitmap* pBMP = new CBitmap(bmpRes);

		// --- offset
		CPoint offset;
		parseSize(*offsetString, offset);
		const CPoint offsetPoint(offset);

		PluginParameter* piParam = getGuiControlWithTag(tag);
		if (!piParam)
		{
			if (pBMP) pBMP->forget();
			return nullptr;
		}

		// --- the knobswitch; more well behaved than the VSTGUI4 object IMO
		CKnobWP* p = new CKnobWP(rect, listener, tag, subPixmaps, heightOfOneImage, pBMP, offsetPoint, isSwitchControl);
		if(isSwitchControl)
            p->setSwitchMax((float)piParam->getMaxValue());
		
#ifdef AAXPLUGIN
        if(isUniversalAPIControl)
            p->setAAXKnob(true);
#endif
        if (pBMP) pBMP->forget();
		return p;
	}

    // --- special sliders
    if (viewname == "SliderSwitchView" || viewname == "UniversalAPISlider")
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
        const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* offsetString = attributes.getAttributeValue("handle-offset");
		const std::string* bitmapString = attributes.getAttributeValue("bitmap");
		const std::string* handleBitmapString = attributes.getAttributeValue("handle-bitmap");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
		const std::string* styleString = attributes.getAttributeValue("orientation");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        if(!offsetString) return nullptr;
        if(!bitmapString) return nullptr;
        if(!handleBitmapString) return nullptr;
        if(!tagString) return nullptr;
        if(!styleString) return nullptr;
        
        bool isSwitchControl = viewname == "SliderSwitchView" ? true : false;
        bool isUniversalAPIControl = viewname == "UniversalAPISlider" ? true : false;
        
		// --- rect
		CPoint origin;
		CPoint size;
		parseSize(*sizeString, size);
		parseSize(*originString, origin);

		const CRect rect(origin, size);

		// --- listener
		const char* controlTagName = tagString->c_str();
		IControlListener* listener = description->getControlListener(controlTagName);

		// --- tag
		int32_t tag = description->getTagForName(tagString->c_str());

		// --- bitmap
		std::string BMString = *bitmapString;
		BMString += ".png";
		UTF8StringPtr bmp = BMString.c_str();
		CResourceDescription bmpRes(bmp);
		CBitmap* pBMP_back = new CBitmap(bmpRes);

		std::string BMStringH = *handleBitmapString;
		BMStringH += ".png";
		UTF8StringPtr bmpH = BMStringH.c_str();
		CResourceDescription bmpResH(bmpH);
		CBitmap* pBMP_hand = new CBitmap(bmpResH);

		// --- offset
		CPoint offset;
		parseSize(*offsetString, offset);
		const CPoint offsetPoint(offset);

		PluginParameter* piParam = getGuiControlWithTag(tag);
		if (!piParam) return nullptr;

		// --- the knobswitch
		if (strcmp(styleString->c_str(), "vertical") == 0)
		{
			CVerticalSliderWP* p = new CVerticalSliderWP(rect, listener, tag, 0, 1, pBMP_hand, pBMP_back, offsetPoint);
			if(isSwitchControl)
            {
                p->setSwitchSlider(true);
                p->setSwitchMax((float)piParam->getMaxValue());
            }
            
#ifdef AAXPLUGIN
            if(isUniversalAPIControl)
                p->setAAXSlider(true);
#endif
			if (pBMP_back) pBMP_back->forget();
			if (pBMP_hand) pBMP_hand->forget();
			return p;
		}
		else
		{
			CHorizontalSliderWP* p = new CHorizontalSliderWP(rect, listener, tag, 0, 1, pBMP_hand, pBMP_back, offsetPoint);
            if(isSwitchControl)
            {
                p->setSwitchSlider(true);
                p->setSwitchMax((float)piParam->getMaxValue());
            }
            
#ifdef AAXPLUGIN
            if(isUniversalAPIControl)
                p->setAAXSlider(true);
#endif
			if (pBMP_back) pBMP_back->forget();
			if (pBMP_hand) pBMP_hand->forget();
			return p;
		}
		return nullptr;
	}

    // --- meters
	std::string customView(viewname);
	std::string analogMeter("AnalogMeterView");
	std::string invAnalogMeter("InvertedAnalogMeterView");
	int nAnalogMeter = (int)customView.find(analogMeter);
	int nInvertedAnalogMeter = (int)customView.find(invAnalogMeter);

	if (nAnalogMeter >= 0)
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
		const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* ONbitmapString = attributes.getAttributeValue("bitmap");
		const std::string* OFFbitmapString = attributes.getAttributeValue("off-bitmap");
		const std::string* numLEDString = attributes.getAttributeValue("num-led");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        if(!ONbitmapString) return nullptr;
        if(!OFFbitmapString) return nullptr;
        if(!numLEDString) return nullptr;
        if(!tagString) return nullptr;


        CPoint origin;
        CPoint size;
        parseSize(*sizeString, size);
        parseSize(*originString, origin);

        const CRect rect(origin, size);

        std::string onBMString = *ONbitmapString;
        onBMString += ".png";
        UTF8StringPtr onbmp = onBMString.c_str();
        CResourceDescription bmpRes(onbmp);
        CBitmap* onBMP = new CBitmap(bmpRes);

        std::string offBMString = *OFFbitmapString;
        offBMString += ".png";
        UTF8StringPtr offbmp = offBMString.c_str();
        CResourceDescription bmpRes2(offbmp);
        CBitmap* offBMP = new CBitmap(bmpRes2);

        int32_t nbLed = strtol(numLEDString->c_str(), 0, 10);

        CVuMeterWP* p = nullptr;

        // --- decode our stashed variables
        // decode hieght one image and zero db frame
        int nX = (int)customView.find("_H");
        int nY = (int)customView.find("_Z");
        int len = (int)customView.length();
        std::string sH = customView.substr(nX + 2, nY - 2 - nX);
        std::string sZ = customView.substr(nY + 2, len - 2 - nY);

        p->setHtOneImage(atof(sH.c_str()));
        p->setImageCount(atof(numLEDString->c_str()));
        p->setZero_dB_Frame(atof(sZ.c_str()));

        if (nInvertedAnalogMeter >= 0)
            p = new CVuMeterWP(rect, onBMP, offBMP, nbLed, true, true); // inverted, analog
        else
            p = new CVuMeterWP(rect, onBMP, offBMP, nbLed, false, true); // inverted, analog

        if (onBMP) onBMP->forget();
        if (offBMP) offBMP->forget();

        // --- connect meters/variables
        int32_t tag = description->getTagForName(tagString->c_str());

        PluginParameter* piParam = getGuiControlWithTag(tag);
        if (!piParam) return nullptr;

        // --- set detector
        float fSampleRate = 1.f / (GUI_METER_UPDATE_INTERVAL_MSEC*0.001f);
        p->initDetector(fSampleRate, (float)piParam->getMeterAttack_ms(),
            (float)piParam->getMeterRelease_ms(), true,
            piParam->getDetectorMode(),
            piParam->getLogMeter());

        return p;
	}

	if (viewname == "InvertedMeterView" || viewname == "MeterView")
	{
		const std::string* sizeString = attributes.getAttributeValue("size");
		const std::string* originString = attributes.getAttributeValue("origin");
		const std::string* ONbitmapString = attributes.getAttributeValue("bitmap");
		const std::string* OFFbitmapString = attributes.getAttributeValue("off-bitmap");
		const std::string* numLEDString = attributes.getAttributeValue("num-led");
		const std::string* tagString = attributes.getAttributeValue("control-tag");
        if(!sizeString) return nullptr;
        if(!originString) return nullptr;
        if(!ONbitmapString) return nullptr;
        if(!OFFbitmapString) return nullptr;
        if(!numLEDString) return nullptr;
        if(!tagString) return nullptr;

		if (sizeString && originString && ONbitmapString && OFFbitmapString && numLEDString)
		{
			CPoint origin;
			CPoint size;
			parseSize(*sizeString, size);
			parseSize(*originString, origin);

			const CRect rect(origin, size);

			std::string onBMString = *ONbitmapString;
			onBMString += ".png";
			UTF8StringPtr onbmp = onBMString.c_str();
			CResourceDescription bmpRes(onbmp);
			CBitmap* onBMP = new CBitmap(bmpRes);

			std::string offBMString = *OFFbitmapString;
			offBMString += ".png";
			UTF8StringPtr offbmp = offBMString.c_str();
			CResourceDescription bmpRes2(offbmp);
			CBitmap* offBMP = new CBitmap(bmpRes2);

			int32_t nbLed = strtol(numLEDString->c_str(), 0, 10);

			bool bInverted = false;

			if (viewname == "InvertedMeterView")
				bInverted = true;

			CVuMeterWP* p = new CVuMeterWP(rect, onBMP, offBMP, nbLed, bInverted, false); // inverted, analog

			if (onBMP) onBMP->forget();
			if (offBMP) offBMP->forget();

			// --- connect meters/variables
			int32_t tag = description->getTagForName(tagString->c_str());

			PluginParameter* piParam = getGuiControlWithTag(tag);
			if (!piParam) return nullptr;

			// --- set detector
			float fSampleRate = 1.f / (GUI_METER_UPDATE_INTERVAL_MSEC*0.001f);
			p->initDetector(fSampleRate, (float)piParam->getMeterAttack_ms(),
				(float)piParam->getMeterRelease_ms(), true,
				piParam->getDetectorMode(),
				piParam->getLogMeter());

			return p;
		}
	}

	return nullptr;
}

// --- find a receiver
CControlUpdateReceiver* PluginGUI::getControlUpdateReceiver(int32_t tag) const
{
    if (tag != -1)
    {
        CControlUpdateReceiverMap::const_iterator it = controlUpdateReceivers.find (tag);
        if (it != controlUpdateReceivers.end ())
        {
            return it->second;
        }
    }
    return 0;
}

CMouseEventResult PluginGUI::onMouseDown(CFrame* frame, const CPoint& where, const CButtonState& buttons)
{
	CMouseEventResult result = kMouseEventNotHandled;

	// --- added kShift because control + left click behaves as right-click for MacOS
	//     to support the ancient one-mouse paradigm; in ProTools, control + move is for Fine Adjustment
	if (buttons.isRightButton() && buttons & kShift)
	{
#if VSTGUI_LIVE_EDITING
		COptionMenu* controllerMenu = 0;// (delegate && editingEnabled == false) ? delegate->createContextMenu(where, this) : 0;
		if (showGUIEditor == false)
		{
			if (controllerMenu == 0)
				controllerMenu = new COptionMenu();
			else
				controllerMenu->addSeparator();

			CMenuItem* item = controllerMenu->addEntry(new CCommandMenuItem("Open UIDescription Editor", this, "File", "Open UIDescription Editor"));
			item->setKey("e", kControl);
		}

		if (controllerMenu)
		{
			controllerMenu->setStyle(kPopupStyle | kMultipleCheckStyle);
			controllerMenu->popup(frame, where);
			result = kMouseEventHandled;
		}

		if (controllerMenu)
			controllerMenu->forget();

		return result;
#endif
	}

	// --- try other overrides
	if (!showGUIEditor)
	{
		int controlID = getControlID_WithMouseCoords(where);
		if (sendAAXMouseDown(controlID, buttons) == kMouseEventHandled)
			return kMouseEventHandled;

		CControl* control = getControl_WithMouseCoords(where);
		if (sendAUMouseDown(control, buttons) == kMouseEventHandled)
			return kMouseEventHandled;
	}
	return result;
}


CMouseEventResult PluginGUI::onMouseMoved(CFrame* frame, const CPoint& where, const CButtonState& buttons)
{
	CMouseEventResult result = kMouseEventNotHandled;
	if (!showGUIEditor)
	{
		int controlID = getControlID_WithMouseCoords(where);
		if (sendAAXMouseMoved(controlID, buttons, where) == kMouseEventHandled)
			return kMouseEventHandled;

		CControl* control = getControl_WithMouseCoords(where);
		if (sendAUMouseDown(control, buttons) == kMouseEventHandled)
			return kMouseEventHandled;
	}
	return result;
}
    
/*  For AAX/ProTools:
    This handles several ProTools-specific mouse events;
    * ctrl-alt-cmd: enable automation
    * alt:          set control to default value (VSTGUI uses ctrl and this overrides it)
*/
CMouseEventResult PluginGUI::sendAAXMouseDown(int controlID, const CButtonState& buttons)
{
#ifdef AAXPLUGIN
    VSTGUI::CMouseEventResult result = VSTGUI::kMouseEventNotHandled;
    const AAX_CVSTGUIButtonState aax_buttons(buttons, aaxViewContainer);
    
    if (aaxViewContainer && controlID >= 0)
    {
        std::stringstream str;
        str << controlID + 1;

        if (aaxViewContainer->HandleParameterMouseDown(str.str().c_str(), aax_buttons.AsAAX() ) == AAX_SUCCESS)
        {
            result = kMouseEventHandled;
        }
    }
    return result;
    
#endif
    return kMouseEventNotHandled;
}
    
CMouseEventResult PluginGUI::sendAAXMouseMoved(int controlID, const CButtonState& buttons, const CPoint& where)
{
#ifdef AAXPLUGIN
    VSTGUI::CMouseEventResult result = VSTGUI::kMouseEventNotHandled;
    const AAX_CVSTGUIButtonState aax_buttons(buttons, aaxViewContainer);
    
    if (aaxViewContainer && controlID >= 0)
    {
        std::stringstream str;
        str << controlID + 1;
        
        if (aaxViewContainer->HandleParameterMouseDrag(str.str().c_str(), aax_buttons.AsAAX() ) == AAX_SUCCESS)
        {
            result = kMouseEventHandled;
        }
    }
 
    return result;
    
#endif
    return kMouseEventNotHandled;
}
    
CMouseEventResult PluginGUI::sendAUMouseDown(CControl* control, const CButtonState& buttons)
{
	if (!control) return kMouseEventNotHandled;

#ifdef AUPLUGIN
	VSTGUI::CMouseEventResult result = VSTGUI::kMouseEventNotHandled;

	if (buttons.isLeftButton() && (buttons & kAlt))
	{
		CButtonState newState(kLButton | kControl);
		control->checkDefaultValue(newState);
		result = kMouseEventHandled;
	}
	else if (buttons.isLeftButton() && (buttons & kControl))
	{
		result = kMouseEventNotImplemented;
	}
	return result;

#endif
	return kMouseEventNotHandled;
}


/* not needed for AU as SHIFT + DRAG is also AU-correct */
CMouseEventResult PluginGUI::sendAUMouseMoved(CControl* control, const CButtonState& buttons, const CPoint& where)
{
	return kMouseEventNotImplemented;
}
int32_t PluginGUI::onKeyDown(const VstKeyCode& code, CFrame* frame)
{
	return -1;
}

//-----------------------------------------------------------------------------
int32_t PluginGUI::onKeyUp(const VstKeyCode& code, CFrame* frame)
{
	return -1;
}


PluginGUIFrame::PluginGUIFrame(const CRect &inSize, void* inSystemWindow, const PlatformType& platformType, VSTGUIEditorInterface* pEditor)
: CFrame(inSize, pEditor)
{
    CFrame::open(inSystemWindow, platformType);	
}

PluginGUIFrame::~PluginGUIFrame(void)
{
}

#ifdef AAXPLUGIN
// --- fix for PTSW-186182 (ProTools bug)
#if WINDOWS_VERSION
unsigned char translateVirtualKeyWin (WPARAM winVKey)
{
    switch (winVKey)
    {
        case VKEY_BACK: return VK_BACK;
        case VKEY_TAB: return VK_TAB;
        case VKEY_CLEAR: return VK_CLEAR;
        case VKEY_RETURN: return VK_RETURN;
        case VKEY_PAUSE: return VK_PAUSE;
        case VKEY_ESCAPE: return VK_ESCAPE;
        case VKEY_SPACE: return VK_SPACE;
        case VKEY_END: return VK_END;
        case VKEY_HOME: return VK_HOME;
        case VKEY_LEFT: return VK_LEFT;
        case VKEY_RIGHT: return VK_RIGHT;
        case VKEY_UP: return VK_UP;
        case VKEY_DOWN: return VK_DOWN;
        case VKEY_PAGEUP: return VK_PRIOR;
        case VKEY_PAGEDOWN: return VK_NEXT;
        case VKEY_SELECT: return VK_SELECT;
        case VKEY_PRINT: return VK_PRINT;
        case VKEY_SNAPSHOT: return VK_SNAPSHOT;
        case VKEY_INSERT: return VK_INSERT;
        case VKEY_DELETE: return VK_DELETE;
        case VKEY_HELP: return VK_HELP;
        case VKEY_NUMPAD0: return VK_NUMPAD0;
        case VKEY_NUMPAD1: return VK_NUMPAD1;
        case VKEY_NUMPAD2: return VK_NUMPAD2;
        case VKEY_NUMPAD3: return VK_NUMPAD3;
        case VKEY_NUMPAD4: return VK_NUMPAD4;
        case VKEY_NUMPAD5: return VK_NUMPAD5;
        case VKEY_NUMPAD6: return VK_NUMPAD6;
        case VKEY_NUMPAD7: return VK_NUMPAD7;
        case VKEY_NUMPAD8: return VK_NUMPAD8;
        case VKEY_NUMPAD9: return VK_NUMPAD9;
        case VKEY_MULTIPLY: return VK_MULTIPLY;
        case VKEY_ADD: return VK_ADD;
        case VKEY_SEPARATOR: return VK_SEPARATOR;
        case VKEY_SUBTRACT: return VK_SUBTRACT;
        case VKEY_DECIMAL: return VK_DECIMAL;
        case VKEY_DIVIDE: return VK_DIVIDE;
        case VKEY_F1: return VK_F1;
        case VKEY_F2: return VK_F2;
        case VKEY_F3: return VK_F3;
        case VKEY_F4: return VK_F4;
        case VKEY_F5: return VK_F5;
        case VKEY_F6: return VK_F6;
        case VKEY_F7: return VK_F7;
        case VKEY_F8: return VK_F8;
        case VKEY_F9: return VK_F9;
        case VKEY_F10: return VK_F10;
        case VKEY_F11: return VK_F11;
        case VKEY_F12: return VK_F12;
        case VKEY_NUMLOCK: return VK_NUMLOCK;
        case VKEY_SCROLL: return VK_SCROLL;
        case VKEY_SHIFT: return VK_SHIFT;
        case VKEY_CONTROL: return VK_CONTROL;
        case VKEY_ALT: return VK_MENU;
        case VKEY_EQUALS: return VKEY_EQUALS;
    }
    return 0;
}
#endif
#endif

// --- fix for PTSW-186182
bool PluginGUIFrame::platformOnKeyDown(VstKeyCode& keyCode)
{
#ifdef AAXPLUGIN
#if WINDOWS_VERSION
    CBaseObjectGuard bog (this);
    int result = onKeyDown (keyCode);
    
    if (result != 1)
    {
        Win32Frame* win32frame = reinterpret_cast<Win32Frame*>(this->getPlatformFrame());
        HWND parentH = win32frame->getParentPlatformWindow();
        
        if (parentH != 0)
        {
            UINT   message = WM_KEYDOWN;
            WPARAM wParam  = translateVirtualKeyWin(keyCode.virt);
            LPARAM lParam = (LPARAM)nullptr;
            result = (PostMessage (parentH, message, wParam, lParam) != 0);
        }
    }
    
    return 0 != result;
#else
    return CFrame::platformOnKeyDown(keyCode);
#endif
#endif
    return CFrame::platformOnKeyDown(keyCode);
}
    

}

