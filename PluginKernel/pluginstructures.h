//
//  pluginstructures.h
//  VolGUI_I
//
//  Created by Will Pirkle on 4/1/17.
//
//

#ifndef _pluginstructures_h
#define _pluginstructures_h

// --- support multichannel operation up to 128 channels
#define MAX_CHANNEL_COUNT 128

#include <string>
#include <sstream>
#include <vector>
class IGUIPluginConnector;
class IGUIWindowFrame;


enum pluginType
{
    kFXPlugin,
    kSynthPlugin
};

enum aaxPlugInCategory
{
    aaxPlugInCategory_None              = 0x00000000,
    aaxPlugInCategory_EQ                = 0x00000001,	///<  Equalization
    aaxPlugInCategory_Dynamics          = 0x00000002,	///<  Compressor, expander, limiter, etc.
    aaxPlugInCategory_PitchShift		= 0x00000004,	///<  Pitch processing
    aaxPlugInCategory_Reverb			= 0x00000008,	///<  Reverberation and room/space simulation
    aaxPlugInCategory_Delay             = 0x00000010,	///<  Delay and echo
    aaxPlugInCategory_Modulation		= 0x00000020,	///<  Phasing, flanging, chorus, etc.
    aaxPlugInCategory_Harmonic          = 0x00000040,	///<  Distortion, saturation, and harmonic enhancement
    aaxPlugInCategory_NoiseReduction    = 0x00000080,	///<  Noise reduction
    aaxPlugInCategory_Dither			= 0x00000100,	///<  Dither, noise shaping, etc.
    aaxPlugInCategory_SoundField		= 0x00000200,  	///<  Pan, auto-pan, upmix and downmix, and surround handling
    aaxPlugInCategory_HWGenerators      = 0x00000400,	///<  Fixed hardware audio sources such as SampleCell
    aaxPlugInCategory_SWGenerators      = 0x00000800,	///<  Virtual instruments, metronomes, and other software audio sources
    aaxPlugInCategory_WrappedPlugin     = 0x00001000,	///<  Plug-ins wrapped by a thrid party wrapper except synth plug-ins which = AAX_PlugInCategory_SWGenerators
    aaxPlugInCategory_Effect			= 0x00002000,	///<  Special effects
};

enum channelFormat
{
    kCFNone,
    kCFMono,
    kCFStereo,
    kCFLCR,
    kCFLCRS,
    kCFQuad,
    kCF5p0,
    kCF5p1,
    kCF6p0,
    kCF6p1,
    kCF7p0Sony,
    kCF7p0DTS,
    kCF7p1Sony,
    kCF7p1DTS,
    kCF7p1Proximity,

    /* the following are NOT directly suppported by AAX/AU */
    kCF8p1,
    kCF9p0,
    kCF9p1,
    kCF10p0,
    kCF10p1,
    kCF10p2,
    kCF11p0,
    kCF11p1,
    kCF12p2,
    kCF13p0,
    kCF13p1,
    kCF22p2,
};

enum auxGUIIdentifier
{
	GUIKnobGraphic,
	GUI2SSButtonStyle,
	EnableMIDIControl,
	MIDIControlChannel,
	MIDIControlIndex
};


struct APISpecificInfo
{
    APISpecificInfo ()
    : aaxManufacturerID(0) /* change this in the plugin core constructor */
    , aaxProductID(0)      /* change this in the plugin core constructor */
    , aaxEffectID("")
    , aaxPluginCategoryCode(aaxPlugInCategory::aaxPlugInCategory_Effect)
    , fourCharCode(0)
    , vst3FUID("")
    , enableVST3SampleAccurateAutomation(0)
    , vst3SampleAccurateGranularity(1)
    , vst3BundleID("")
    , auBundleID("")
    {}

    uint32_t aaxManufacturerID;
    uint32_t aaxProductID;
    std::string aaxEffectID;
    std::string aaxBundleID;    /* MacOS only: this MUST match the bundle identifier in your info.plist file */
    uint32_t aaxPluginCategoryCode;

    // --- common to AU and AAX
    int fourCharCode;

    // --- VST3
    std::string vst3FUID;
    bool enableVST3SampleAccurateAutomation; // false by default
    uint32_t vst3SampleAccurateGranularity; // default is one-sample (perfect)
    std::string vst3BundleID;    /* MacOS only: this MUST match the bundle identifier in your info.plist file */

    // --- AU
    std::string auBundleID;     /* MacOS only: this MUST match the bundle identifier in your info.plist file */

 };

struct VectorJoystickData
{
	VectorJoystickData()
		: vectorA(0.0)
		, vectorB(0.0)
		, vectorC(0.0)
		, vectorD(0.0)
		, vectorACMix(0.0)
		, vectorBDMix(0.0) {}

	VectorJoystickData(double _vectorA, double _vectorB, double _vectorC, double _vectorD, double _vectorACMix, double _vectorBDMix)
		: vectorA(_vectorA)
		, vectorB(_vectorB)
		, vectorC(_vectorC)
		, vectorD(_vectorD)
		, vectorACMix(_vectorACMix)
		, vectorBDMix(_vectorBDMix) {}

	double vectorA;
	double vectorB;
	double vectorC;
	double vectorD;

	double vectorACMix = 0.0;
	double vectorBDMix = 0.0;
};

struct GUIParameter
{
	GUIParameter()
		: controlID(0)
		, actualValue(0.0)
		, useCustomData(0)
		, customData(0) {}

	uint32_t controlID;
	double actualValue;
	bool useCustomData;

	// --- for custom drawing, or other custom data
	void* customData = nullptr;
};

struct PresetParameter
{
    PresetParameter ()
    : controlID(0)
    , actualValue(0.0){}

    PresetParameter (uint32_t _controlID, double _actualValue)
    : controlID(_controlID)
    , actualValue(_actualValue){}

    uint32_t controlID;
    double actualValue;
};

struct PresetInfo
{
    PresetInfo(uint32_t _presetIndex, const char* _name)
    : presetIndex(_presetIndex)
    , presetName(_name) {}

    uint32_t presetIndex; // --- zero indexed
    std::string presetName;

    std::vector<PresetParameter> presetParameters;
};

struct GUIUpdateData
{
    uint32_t guiUpdateCode = -1;

    // --- for control updates
    std::vector<GUIParameter> guiParameters;

    // --- for custom draw updates (graphs, etc...)
    void* customData = nullptr;

    // --- flag
    bool useCustomData = false;
};

enum hostMessage { sendGUIUpdate, sendRAFXStatusWndText };
struct HostMessageInfo
{
	HostMessageInfo()
		: hostMessage(0){}

    uint32_t hostMessage;
	GUIParameter guiParameter;		/* for single param updates */

    // --- for GUI messages
    GUIUpdateData guiUpdateData;	/* for multiple param updates */
	std::string rafxStatusWndText;
};



struct ChannelIOConfig
{
    ChannelIOConfig ()
    : inputChannelFormat(kCFStereo)
    , outputChannelFormat(kCFStereo) {}

    ChannelIOConfig (uint32_t _inputChannelFormat,
                     uint32_t _outputChannelFormat)
    : inputChannelFormat(_inputChannelFormat)
    , outputChannelFormat(_outputChannelFormat){}

    uint32_t inputChannelFormat;
    uint32_t outputChannelFormat;

};

// --- struct for MIDI events
struct midiEvent
{
	midiEvent(uint32_t _midiMessage, uint32_t _midiChannel, uint32_t _midiData1, uint32_t _midiData2, uint32_t _midiSampleOffset)
		: midiMessage(_midiMessage)
		, midiChannel(_midiChannel)
		, midiData1(_midiData1)
		, midiData2(_midiData2)
		, midiSampleOffset(_midiSampleOffset)
	{
		midiPitchBendValue = 0;
		midiNormalizedPitchBendValue = 0.f;
		midiIsDirty = false;
	}

	midiEvent(uint32_t _midiMessage, uint32_t _midiChannel, uint32_t _midiData1, uint32_t _midiData2, uint32_t _midiSampleOffset, double _audioTimeStamp)
		: midiMessage(_midiMessage)
		, midiChannel(_midiChannel)
		, midiData1(_midiData1)
		, midiData2(_midiData2)
		, midiSampleOffset(_midiSampleOffset)
		, audioTimeStamp(_audioTimeStamp)
	{ 
		midiPitchBendValue = 0;
		midiNormalizedPitchBendValue = 0.f;
		midiIsDirty = false;
	}

	midiEvent ()
    : midiMessage(0)
    , midiChannel(0)
    , midiData1(0)
    , midiData2(0)
    , midiSampleOffset(0)
    , midiPitchBendValue(0)
    , midiNormalizedPitchBendValue(0)
    , midiIsDirty(0)
	, audioTimeStamp(0.0){}

    uint32_t    midiMessage;
    uint32_t    midiChannel;
    uint32_t    midiData1;
    uint32_t    midiData2;
    uint32_t    midiSampleOffset;
    int         midiPitchBendValue;
    float       midiNormalizedPitchBendValue;
    bool        midiIsDirty;

	// --- for RAFX2 only
	double		audioTimeStamp;
};

//struct CustomViewData
//{
//    CustomViewData(const UIAttributes& _attributes, const IUIDescription* _description)
//    : attributes(_attributes)
//    , description(_description){}
//
//    const UIAttributes& attributes;
//    const IUIDescription* description;
//};

// enum messageType { preProcessData, postProcessData, updateHostInfo, idleProcessData, midiMessageEx, midiEventList, queryPluginInfo };
//enum messageType { guiMessage };
enum messageType {
	PLUGINGUI_DIDOPEN,						 /* called after successful population of GUI frame, NOT called with GUI_USER_CUSTOMOPEN*/
	PLUGINGUI_WILLCLOSE,					 /* called before window is destroyed, NOT called with GUI_USER_CUSTOM_CLOSE */
	PLUGINGUI_TIMERPING,					/* timer ping for custom views */
	PLUGINGUI_REGISTER_CUSTOMVIEW,			/* register a custom view */
	PLUGINGUI_REGISTER_SUBCONTROLLER,		/* register a subcontroller */
	PLUGINGUI_QUERY_HASUSERCUSTOM,			/* CUSTOM GUI - reply in bHasUserCustomView */
	PLUGINGUI_USER_CUSTOMOPEN,				/* CUSTOM GUI - create your custom GUI, you must supply the code */
	PLUGINGUI_USER_CUSTOMCLOSE,				/* CUSTOM GUI - destroy your custom GUI, you must supply the code */
	PLUGINGUI_USER_CUSTOMSYNC,				/* CUSTOM GUI - re-sync the GUI */
	PLUGINGUI_EXTERNAL_SET_NORMVALUE,		// for VST3??
	PLUGINGUI_EXTERNAL_SET_ACTUALVALUE,
	PLUGINGUI_EXTERNAL_GET_NORMVALUE,		/* currently not used */
	PLUGINGUI_EXTERNAL_GET_ACTUALVALUE,
	PLUGINGUI_PARAMETER_CHANGED,			/* for pluginCore->guiParameterChanged(nControlIndex, fValue); */
	PLUGIN_QUERY_DESCRIPTION,				/* fill in a Rafx2PluginDescriptor for host */
	PLUGIN_QUERY_PARAMETER,					/* fill in a Rafx2PluginParameter for host inMessageData = index of parameter*/
	PLUGIN_QUERY_TRACKPAD_X,
	PLUGIN_QUERY_TRACKPAD_Y
};	

struct MessageInfo
{
    MessageInfo ()
    : message(0)
    , inMessageData(0)
    , outMessageData(0){}

    MessageInfo (uint32_t _message)
    : message(_message)
     , inMessageData(0)
    , outMessageData(0)
    {}

    uint32_t message;
    void* inMessageData;
    void* outMessageData;

	std::string inMessageString;
	std::string outMessageString;
};

struct CreateGUIInfo
{
	CreateGUIInfo()
		: window(0)
		, guiPluginConnector(0)
		, guiWindowFrame(0)
		, width(0.0)
		, height(0.0)
	{ }

	CreateGUIInfo(void* _window, IGUIPluginConnector* _guiPluginConnector, IGUIWindowFrame* _guiWindowFrame)
		: window(_window)
		, guiPluginConnector(_guiPluginConnector)
		, guiWindowFrame(_guiWindowFrame)
		, width(0.0)
		, height(0.0)
	{ }

	void* window;
	IGUIPluginConnector* guiPluginConnector;
	IGUIWindowFrame* guiWindowFrame;

	// --- returned to RAFX host
	double width;
	double height;
};

struct ParameterUpdateInfo
{
	ParameterUpdateInfo()
		: isSmoothing(0)
		, isVSTSampleAccurateUpdate(0)
		, loadingPreset(0)
		, boundVariableUpdate(0)
		, bufferProcUpdate(0)
		, applyTaper(1){}

	ParameterUpdateInfo(bool _isSmoothing, bool _isVSTSampleAccurateUpdate)
		: isSmoothing(_isSmoothing)
		, isVSTSampleAccurateUpdate(_isVSTSampleAccurateUpdate) {
		loadingPreset = false;
		boundVariableUpdate = false;
		bufferProcUpdate = false;
		applyTaper = true;
	}

	bool isSmoothing;               /// param is being (bulk) smoothed
	bool isVSTSampleAccurateUpdate; /// param updated with VST sample accurate automation
	bool loadingPreset;
	bool boundVariableUpdate;
	bool bufferProcUpdate;
	bool applyTaper;
};

enum attributeType { isFloatAttribute, isDoubleAttribute, isIntAttribute, isUintAttribute, isBoolAttribute, isVoidPtrAttribute, isStringAttribute };
union attributeValue
{
	float f;
	double d;
	int n;
	unsigned int u;
	bool b;
	void* vp;
};

struct AuxParameterAttribute
{
	AuxParameterAttribute()
	: attributeID(0) 
	{ memset(&value, 0, sizeof(attributeValue)); }

	AuxParameterAttribute(uint32_t _attributeID)
		: attributeID(_attributeID) { }

	void AuxParameterAttribute::reset(uint32_t _attributeID) { memset(&value, 0, sizeof(attributeValue));  attributeID = _attributeID; }

	void AuxParameterAttribute::setFloatAttribute(float f) { value.f = f; }
	void AuxParameterAttribute::setDoubleAttribute(double d) { value.d = d; }
	void AuxParameterAttribute::setIntAttribute(int n) { value.n = n; }
	void AuxParameterAttribute::setUintAttribute(unsigned int u) { value.u = u; }
	void AuxParameterAttribute::setBoolAttribute(bool b) { value.b = b; }
	void AuxParameterAttribute::setVoidPtrAttribute(void* vp) { value.vp = vp; }

	float AuxParameterAttribute::getFloatAttribute( ) { return value.f; }
	double AuxParameterAttribute::getDoubleAttribute( ) { return value.d; }
	int AuxParameterAttribute::getIntAttribute( ) { return value.n; }
	unsigned int AuxParameterAttribute::getUintAttribute( ) { return  value.u; }
	bool AuxParameterAttribute::getBoolAttribute( ) { return value.b; }
	void* AuxParameterAttribute::getVoidPtrAttribute( ) { return value.vp; }

	attributeValue value;
	uint32_t attributeID;
};

struct ResetInfo
{
    ResetInfo ()
    : sampleRate(44100)
    , bitDepth(16){}

    ResetInfo (double _sampleRate,
               uint32_t _bitDepth)
    : sampleRate(_sampleRate)
    , bitDepth(_bitDepth){}

    double sampleRate;
    uint32_t bitDepth;
};

struct HostInfo
{
    // --- common to all APIs
    unsigned long long uAbsoluteFrameBufferIndex;	// --- the sample index at top of buffer
    double dAbsoluteFrameBufferTime;				// --- the time in seconds of the sample index at top of buffer
    double dBPM;									// --- beats per minute, aka "tempo"
    float fTimeSigNumerator;						// --- time signature numerator
    uint32_t uTimeSigDenomintor;				// --- time signature denominator

    // --- VST3 Specific: note these use same variable names as VST3::struct ProcessContext
    //     see ..\VST3 SDK\pluginterfaces\vst\ivstprocesscontext.h for information on decoding these
    //
    uint32_t state;				// --- a combination of the values from \ref StatesAndFlags; use to decode validity of other VST3 items in this struct
    long long systemTime;			// --- system time in nanoseconds (optional)
    double continousTimeSamples;	// --- project time, without loop (optional)
    double projectTimeMusic;		// --- musical position in quarter notes (1.0 equals 1 quarter note)
    double barPositionMusic;		// --- last bar start position, in quarter notes
    double cycleStartMusic;			// --- cycle start in quarter notes
    double cycleEndMusic;			// --- cycle end in quarter notes
    uint32_t samplesToNextClock;// --- MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest)
    /*
     IF you need SMPTE information, you need to get the information yourself at the start of the process( ) function
     where the above values are filled out. See the variables here in VST3 SDK\pluginterfaces\vst\ivstprocesscontext.h:

     int32 smpteOffsetSubframes;		// --- SMPTE (sync) offset in subframes (1/80 of frame)
     FrameRate frameRate;			// --- frame rate
     */

    // --- AU Specific
    //     see AUBase.h for definitions and information on decoding these
    //
    double dCurrentBeat;						// --- current DAW beat value
    bool bIsPlayingAU;							// --- notorously incorrect in Logic - once set to true, stays stuck there
    bool bTransportStateChanged;				// --- only notifies a change, but not what was changed to...
    uint32_t nDeltaSampleOffsetToNextBeat;	// --- samples to next beat
    double dCurrentMeasureDownBeat;				// --- current downbeat
    bool bIsCycling;							// --- looping
    double dCycleStartBeat;						// --- loop start
    double dCycleEndBeat;						// --- loop end

    // --- AAX Specific
    //     see AAX_ITransport.h for definitions and information on decoding these
    bool bIsPlayingAAX;					// --- flag if playing
    long long nTickPosition;			// --- "Tick" is represented here as 1/960000 of a quarter note
    bool bLooping;						// --- looping flag
    long long nLoopStartTick;			// --- start tick for loop
    long long nLoopEndTick ;			// --- end tick for loop
    /*
     NOTE: there are two optional functions that cause a performance hit in AAX; these are commented outs; 
	 if you decide to use them, you should re-locate them to a non-realtime thread. Use at your own risk!

     int32_t nBars = 0;
     int32_t nBeats = 0;
     int64_t nDisplayTicks = 0;
     int64_t nCustomTickPosition = 0;

     // --- There is a minor performance cost associated with using this API in Pro Tools. It should NOT be used excessively without need
     midiTransport->GetBarBeatPosition(&nBars, &nBeats, &nDisplayTicks, nAbsoluteSampleLocation);

     // --- There is a minor performance cost associated with using this API in Pro Tools. It should NOT be used excessively without need
     midiTransport->GetCustomTickPosition(&nCustomTickPosition, nAbsoluteSampleLocation);

     NOTE: if you need SMPTE or metronome information, you need to get the information yourself at the start of the ProcessAudio( ) function
			  see AAX_ITransport.h for definitions and information on decoding these
     virtual	AAX_Result GetTimeCodeInfo(AAX_EFrameRate* oFrameRate, int32_t* oOffset) const = 0;
     virtual	AAX_Result GetFeetFramesInfo(AAX_EFeetFramesRate* oFeetFramesRate, int64_t* oOffset) const = 0;
     virtual AAX_Result IsMetronomeEnabled(int32_t* isEnabled) const = 0;
     */
};

class IMidiEventQueue;
struct ProcessBufferInfo
{
    ProcessBufferInfo()
    : inputs (0), outputs (0), auxInputs (0), auxOutputs (0)
    , numInputChannels (0), numOutputChannels (0), numAuxInputChannels (0), numAuxOutputChannels (0)
    , numFramesToProcess (0)
    , hostInfo(0)
    , midiEventQueue(0){}

    float** inputs;
    float** outputs;
    float** auxInputs;
    float** auxOutputs; // --- for future use

    // --- prob don't need these either with ChannelIO channelIOConfig; below...
	uint32_t numInputChannels;
	uint32_t numOutputChannels;
	uint32_t numAuxInputChannels;
	uint32_t numAuxOutputChannels;

	uint32_t numFramesToProcess;
    ChannelIOConfig channelIOConfig;
    ChannelIOConfig auxChannelIOConfig;

	// --- should make these const?
    HostInfo* hostInfo;
    IMidiEventQueue* midiEventQueue;

    // --- maybe get rid of this..
    /*void* inputAPISpecificData;
    void* outputAPISpecificData;
    void* auxInputAPISpecificData;
    void* auxOutputAPISpecificData;*/
};

struct ProcessFrameInfo
{
    ProcessFrameInfo()
    : inputData (0), outputData (0), auxInputData (0), auxOutputData (0)
    , numInputChannels (0), numOutputChannels (0), numAuxInputChannels (0), numAuxOutputChannels (0)
    , currentFrame(0)
    , hostInfo(0)
    , midiEventQueue(0) {}

    float* inputData;
    float* outputData;
    float* auxInputData;
    float* auxOutputData; // --- for future use

	uint32_t numInputChannels;
	uint32_t numOutputChannels;
	uint32_t numAuxInputChannels;
	uint32_t numAuxOutputChannels;

    ChannelIOConfig channelIOConfig;
    ChannelIOConfig auxChannelIOConfig;
	uint32_t currentFrame;

    HostInfo* hostInfo;
    IMidiEventQueue* midiEventQueue;

   /* void* inputAPISpecificData;
    void* outputAPISpecificData;
    void* auxInputAPISpecificData;
    void* auxOutputAPISpecificData;*/
};

// --- this is for plugin only
struct AudioProcDescriptor
{
    AudioProcDescriptor ()
    : sampleRate(44100)
    , bitDepth(16){}

    AudioProcDescriptor (double _sampleRate,
                         uint32_t _bitDepth)
    : sampleRate(_sampleRate)
    , bitDepth(_bitDepth){}

    double sampleRate;
    uint32_t bitDepth;
};

struct PluginDescriptor
{
    PluginDescriptor ()
    : pluginName("Long Plugin Name") // max 31 chars
    , shortPluginName("ShortPIName") // max 15 chars
    , vendorName("Plugin Developer")
    , pluginTypeCode(pluginType::kFXPlugin) // FX or synth
    , hasSidechain(0)
    , processFrames(1)                  /* default operation */
    , wantsMIDI(1)                      /* default operation */
    , hasCustomGUI(1)
    , latencyInSamples(0)
    , tailTimeInMSec(0)
    , infiniteTailVST3(0)
    , numSupportedIOCombinations(0)
    , supportedIOCombinations(0)
    , numSupportedAuxIOCombinations(0)
    , supportedAuxIOCombinations(0)
    {}

    // --- string descriptors
    std::string pluginName;
    std::string shortPluginName;
    std::string vendorName;
    uint32_t pluginTypeCode;

    bool hasSidechain;
    bool processFrames; // default is true
    bool wantsMIDI;     // default is true
    bool hasCustomGUI;     // default is true
    uint32_t latencyInSamples;
    double tailTimeInMSec;
    bool infiniteTailVST3;

    uint32_t numSupportedIOCombinations;
    ChannelIOConfig* supportedIOCombinations;

    uint32_t numSupportedAuxIOCombinations;
    ChannelIOConfig* supportedAuxIOCombinations;

    // --- for AU plugins
    uint32_t getDefaultChannelIOConfigForChannelCount(uint32_t channelCount)
    {
        switch(channelCount)
        {
            case 0:
                return kCFNone;
            case 1:
                return kCFMono;
            case 2:
                return kCFStereo;
            case 3:
                return kCFLCR;
            case 4:
                return kCFQuad; // or kCFLCR
            case 5:
                return kCF5p0;
            case 6:
                return kCF5p1; // or kCF6p0
            case 7:
                return kCF6p1; // or kCF7p0Sony kCF7p0DTS
            case 8:
                return kCF7p1DTS; // or kCF7p1Sony or kCF7p1Proximity
            case 9:
                return kCF8p1; // or kCF9p0
            case 10:
                return kCF9p1; // or kCF10p0
            case 11:
                return kCF10p1;
            case 12:
                return kCF11p1; // or kCF10p2
            case 13:
                return kCF13p0; // or kCF12p2
            case 14:
                return kCF13p1;
            case 24:
                return kCF22p2;

            default:
                return 0;
        }
    }

    uint32_t getChannelCountForChannelIOConfig(uint32_t format)
    {
        switch(format)
        {
            case kCFNone:
                return 0;

            case kCFMono:
                return 1;
            case kCFStereo:
                return 2;
            case kCFLCR:
                return 3;

            case kCFQuad:
            case kCFLCRS:
                return 4;

            case kCF5p0:
                return 5;

            case kCF5p1:
            case kCF6p0:
                return 6;

            case kCF6p1:
            case kCF7p0Sony:
            case kCF7p0DTS:
                return 7;

            case kCF7p1Sony:
            case kCF7p1DTS:
            case kCF7p1Proximity:
                return 8;

            case kCF8p1:
            case kCF9p0:
                return 9;

            case kCF9p1:
            case kCF10p0:
                return 10;

            case kCF10p1:
                return 11;

            case kCF10p2:
            case kCF11p1:
                return 12;

            case kCF13p0:
            case kCF12p2:
                return 13;

            case kCF13p1:
                return 14;

            case kCF22p2:
                return 24;

            default:
                return 0;
        }
        return 0;
    }
};

// --- this is for RAFX plugins only to support the RAFX Trackpad/Vector Joystick
struct JSControl
{
	JSControl() {}
	JSControl& operator=(const JSControl& aControl)
	{
		if (this == &aControl)
			return *this;

		trackpadIndex = aControl.trackpadIndex;
		midiControl = aControl.midiControl;
		midiControlCommand = aControl.midiControlCommand;
		midiControlName = aControl.midiControlName;
		midiControlChannel = aControl.midiControlChannel;
		joystickValue = aControl.joystickValue;
		korgVectorJoystickOrientation = aControl.korgVectorJoystickOrientation;
		enableParamSmoothing = aControl.enableParamSmoothing;
		smoothingTimeInMs = aControl.smoothingTimeInMs;
		return *this;
	}

	int32_t trackpadIndex = -1;
	bool midiControl = false;
	uint32_t midiControlCommand = 0;
	uint32_t midiControlName = 0;
	uint32_t midiControlChannel = 0;
	double joystickValue = 0.0;
	bool korgVectorJoystickOrientation = false;
	bool enableParamSmoothing = false;
	double smoothingTimeInMs = 0.0;
};


// --------------------------------------------------------------------------------------------------------------------------- //
// --- INTERFACES
// --------------------------------------------------------------------------------------------------------------------------- //
class ICustomView
{
public:
	// --- specific to this custom view
	virtual void setCustomData(uint32_t index, void* data) { }

	// --- for VSTGUI controls
	virtual float getControlValue() { return 0.f; }
	virtual void setControlValue(float value) { }
	virtual void setVisible(bool visible) { }
	virtual void setPosition(double top, double left, double width = -1, double height = -1) { } // if width/height == -1, use controls current width/height
};

class IGUIWindowFrame
{
public:
	virtual bool setWindowFrameSize(double left = 0, double top = 0, double right = 0, double bottom = 0) = 0;
	virtual bool getWindowFrameSize(double& left, double&  top, double&  right, double&  bottom) = 0;
	virtual void enableGUIDesigner(bool enable) { }
};

class IGUIView
{
public:
	virtual void setGUIWindowFrame(IGUIWindowFrame* frame) = 0;
};

class IGUIPluginConnector
{
public:
	// --- GUI registers ICustomView* for monolithic plugin objects
	virtual bool registerCustomView(std::string customViewName, ICustomView* customViewConnector) = 0;
	virtual bool guiDidOpen() = 0;
	virtual bool guiWillClose() = 0;
	virtual bool guiTimerPing() = 0;

	// --- handle non-bound variables (e.g. tab controls, joystick, trackpad)
	virtual uint32_t getNonBoundVariableCount() { return 0; }
	virtual uint32_t getNextNonBoundVariableTag(int startTag) { return -1; }
	virtual bool checkNonBoundValueChange(int tag, float normalizedValue) { return false; }

	// --- for sending GUI udates only, does not change underlying control variable!
	virtual void checkSendUpdateGUI(int tag, float actualValue, bool loadingPreset, void* data1 = 0, void* data2 = 0) {}

	// --- parameter has changed, derived object handles this in a thread-safe manner
	virtual void parameterChanged(int32_t controlID, double actualValue, double normalizedValue) {}

	// --- get/set parameter, derived object handles this in a thread-safe manner
	virtual double getNormalizedPluginParameter(int32_t controlID) { return 0.0; }
	virtual void setNormalizedPluginParameter(int32_t controlID, double value) { }
	virtual double getActualPluginParameter(int32_t controlID) { return 0.0; }
	virtual void setActualPluginParameter(int32_t controlID, double value) { }
};

// --- host (wrap) plugin shell -> GUI connector
class IPluginGUIConnector
{
public:
	virtual void syncGUIControl(uint32_t controlID) = 0;
};

// --- plugin core -> host (wrap) plugin shell
class IPluginHostConnector
{
public:
	virtual void sendHostMessage(const HostMessageInfo& hostMessageInfo) = 0;
};

// --- MIDI Event Queue
class IMidiEventQueue
{
public:
	// --- Get the event count (extra, does not really need to be used)
	virtual uint32_t getEventCount() = 0;

	// --- Fire off the next
	virtual bool fireMidiEvents(uint32_t uSampleOffset) = 0;
};

// --- Interface for VST3 parameter value update queue (sample accurate automation)
class IParameterUpdateQueue
{
public:
	// --- Get the index number associated with the parameter
	virtual uint32_t getParameterIndex() = 0;

	// --- Get the sample-accurate value of the parameter at the given sample offset. Pass in the last known normalized value.
	//     Returns true if dPreviousValue != dNextValue
	virtual bool getValueAtOffset(long int _sampleOffset, double _previousValue, double& _nextValue) = 0;

	// --- Get the sample-accurate value of the parameter at the next sample offset, determined by an internal counter
	//     Returns true if dNextValue is different than the previous value
	virtual bool getNextValue(double& _nextValue) = 0;
};

// --------------------------------------------------------------------------------------------------------------------------- //
// --- HELPER FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------------- //
inline std::string numberToString(unsigned int number)
{
	return static_cast<std::ostringstream*>(&(std::ostringstream() << number))->str();
}

inline std::string numberToString(int number)
{
	return static_cast<std::ostringstream*>(&(std::ostringstream() << number))->str();
}

inline std::string numberToString(float number)
{
	return static_cast<std::ostringstream*>(&(std::ostringstream() << number))->str();
}

inline std::string numberToString(double number)
{
	return static_cast<std::ostringstream*>(&(std::ostringstream() << number))->str();
}

inline std::string boolToString(bool value)
{
	std::string returnString;
	if (value) returnString.assign("true");
	else returnString.assign("false");
	return returnString;
}


#endif //_pluginstructures_h
