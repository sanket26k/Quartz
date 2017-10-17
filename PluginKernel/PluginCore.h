#ifndef __pluginCore_h__
#define __pluginCore_h__

#include "PluginBase.h"
#include "SynthEngine.h"

// **--0x7F1F--**

// --- Plugin Variables controlID Enumeration 

enum controlID {
	eg1AttackTime_mSec = 33,
	eg1DecayTime_mSec = 34,
	eg1SustainLevel = 35,
	eg1ReleaseTime_mSec = 36,
	osc1Wave = 0,
	osc1AmpControl_dB = 1,
	osc1PulseWidthControl_Pct = 2,
	osc2Wave = 10,
	osc2AmpControl_dB = 11,
	osc2PulseWidthControl_Pct = 12,
	osc2Detune_cents = 13,
	lfo1FreqControl = 112,
	lfo1AmpControl = 113,
	lfo1Wave = 111,
	lfo2Wave = 121,
	lfo2FreqControl = 122,
	lfo2AmpControl = 123,
	filter1Fc = 5,
	filter1Q = 6,
	eg2AttackTime_mSec = 103,
	eg2DecayTime_mSec = 104,
	eg2SustainLevel = 105,
	eg2ReleaseTime_mSec = 106,
	eg2FilterFcIntensity = 107,
	gainComp = 7,
	enableKeyTrack = 45,
	keytrackRatio = 15,
	velocityToAttack = 47,
	noteToDecay = 3072,
	synthEngineMode = 41,
	resetToZero = 3074,
	masterVolume_dB = 9,
	masterTuningRatio = 19,
	masterTuningOffset_cents = 29,
	masterPitchBend = 39,
	unisonDetune_cents = 109,
	legatoMode = 3076,
	enablePortamento = 3078,
	portamentoTime_mSec = 119,
	LPFilterType = 42,
	applyNLP = 3080,
	lfo2Mode = 120,
	lfo1Mode = 110,
	osc1TuningRatio = 3,
	invertDCA_EG = 46,
	invertFilterFc = 48,
	modSource1 = 115,
	modDest1 = 116,
	modRouting1_Intensity = 117,
	modSource2 = 125,
	modDest2 = 126,
	modRouting2_Intensity = 127,
	subOscAmpControl_dB = 4,
	delayTime_mSec = 8,
	feedback_Pct = 18,
	delayRatio = 28,
	delayMix_Pct = 38,
	delayType = 43,
	enableDelayFX = 3075,
	chorusRate_Hz = 129,
	chorusDepth_Pct = 139,
	enableChorusFX = 3077,
	HPFilterType = 44,
	filter2Fc = 25,
	filter2Q = 26,
	LPFenable = 3081,
	HPFenable = 3083,
	eg2DelayTime_mSec = 102,
	eg1DelayTime_mSec = 32,
	eg1RepeatTime_mSec = 30,
	eg2RepeatTime_mSec = 100
};

	// **--0x0F1F--**

class PluginCore : public PluginBase
{
public:
    PluginCore();
    virtual ~PluginCore()
	{
		if (synthEngine)
			delete synthEngine;
		synthEngine = nullptr;
	}

	// --- PluginBase Overrides ---
	//
	// --- reset: called when plugin is loaded, a new audio file is playing or sample rate changes
	virtual bool reset(ResetInfo& resetInfo);

	// --- Process audio by frames (default)
	virtual bool processAudioFrame(ProcessFrameInfo& processFrameInfo);

	// --- uncomment and override this for buffer processing; see base class implementation for
	//     help on breaking up buffers and getting info from processBufferInfo
	//virtual bool processAudioBuffers(ProcessBufferInfo& processBufferInfo);

	// --- preProcess: sync GUI parameters here; override if you don't want to use automatic variable-binding
	virtual bool preProcessAudioBuffers(ProcessBufferInfo& processInfo);

	// --- postProcess: do post processing here
	virtual bool postProcessAudioBuffers(ProcessBufferInfo& processInfo);

	// --- updatePluginParameter: called by host plugin at top of buffer proccess; this sets parameters
	//     for current buffer process cycle
	virtual bool updatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo);
	virtual bool updatePluginParameterNormalized(int32_t controlID, double normalizedValue, ParameterUpdateInfo& paramInfo);

	// --- postUpdatePluginParameter: this can be called: 1) after bound variable has been updated or 2) after smoothing occurs
	//     do any variable cooking here
	virtual bool postUpdatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo);

	// --- guiParameterChanged: this is ony called when the user makes a GUI control change
	//     it is for checking variables or updating the GUI in repsonse to user controls
	//     it is NOT for altering underlying parameter values; that only happens in updatePluginParameter
	virtual bool guiParameterChanged(int32_t controlID, double actualValue);

	// --- processMessage: messaging system; currently used for custom/special GUI operations
	virtual bool processMessage(MessageInfo& messageInfo);

	// --- processMIDIEvent: MIDI event processing
	virtual bool processMIDIEvent(midiEvent& event);

	virtual bool setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData);

	// --- NON VIRTUAL OVERRIDES ---
	//
	// --- initPluginParameters: create and load parameters
	bool initPluginParameters();

	// --- initPluginPresets: create presets
	bool initPluginPresets();

	// --- user data
	SynthEngine* synthEngine = nullptr;
	void updateEngine();

	// --- end user variables/functions

private:
	//  **--0x07FD--**

	// --- Continuous Plugin Variables 
	double eg1AttackTime_mSec = 0.0;
	double eg1DecayTime_mSec = 0.0;
	double eg1SustainLevel = 0.0;
	double eg1ReleaseTime_mSec = 0.0;
	double osc1AmpControl_dB = 0.0;
	double osc1PulseWidthControl_Pct = 0.0;
	double osc2AmpControl_dB = 0.0;
	double osc2PulseWidthControl_Pct = 0.0;
	double osc2Detune_cents = 0.0;
	double lfo1FreqControl = 0.0;
	double lfo1AmpControl = 0.0;
	double lfo2FreqControl = 0.0;
	double lfo2AmpControl = 0.0;
	double filter1Fc = 0.0;
	double filter1Q = 0.0;
	double eg2AttackTime_mSec = 0.0;
	double eg2DecayTime_mSec = 0.0;
	double eg2SustainLevel = 0.0;
	double eg2ReleaseTime_mSec = 0.0;
	double eg2FilterFcIntensity = 0.0;
	double gainComp = 0.0;
	double keytrackRatio = 0.0;
	double masterVolume_dB = 0.0;
	double masterTuningRatio = 0.0;
	double masterTuningOffset_cents = 0.0;
	int masterPitchBend = 0;
	double unisonDetune_cents = 0.0;
	double portamentoTime_mSec = 0.0;
	double osc1TuningRatio = 0.0;
	double modRouting1_Intensity = 0.0;
	double modRouting2_Intensity = 0.0;
	double subOscAmpControl_dB = 0.0;
	double delayTime_mSec = 0.0;
	double feedback_Pct = 0.0;
	double delayRatio = 0.0;
	double delayMix_Pct = 0.0;
	double chorusRate_Hz = 0.0;
	double chorusDepth_Pct = 0.0;
	double filter2Fc = 0.0;
	double filter2Q = 0.0;
	double eg2DelayTime_mSec = 0.0;
	double eg1DelayTime_mSec = 0.0;
	double eg1RepeatTime_mSec = 0.0;
	double eg2RepeatTime_mSec = 0.0;

	// --- Discrete Plugin Variables 
	int osc1Wave = 0;
	enum class osc1WaveEnum { Saw,Square,Triangle,Sin,WhiteNoise };	// to compare: if(compareEnum(osc1WaveEnum::Saw, osc1Wave)) etc... 

	int osc2Wave = 0;
	enum class osc2WaveEnum { Saw,Square,Triangle,Sin,WhiteNoise };	// to compare: if(compareEnum(osc2WaveEnum::Saw, osc2Wave)) etc... 

	int lfo1Wave = 0;
	enum class lfo1WaveEnum { Sin,UpSaw,DownSaw,Square,Triangle,RSH,QRSH,ExpUp,ExpDown,WhiteNoise };	// to compare: if(compareEnum(lfo1WaveEnum::Sin, lfo1Wave)) etc... 

	int lfo2Wave = 0;
	enum class lfo2WaveEnum { Sin,UpSaw,DownSaw,Square,Triangle,RSH,QRSH,ExpUp,ExpDown,WhiteNoise };	// to compare: if(compareEnum(lfo2WaveEnum::Sin, lfo2Wave)) etc... 

	int enableKeyTrack = 0;
	enum class enableKeyTrackEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(enableKeyTrackEnum::SWITCH_OFF, enableKeyTrack)) etc... 

	int velocityToAttack = 0;
	enum class velocityToAttackEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(velocityToAttackEnum::SWITCH_OFF, velocityToAttack)) etc... 

	int noteToDecay = 0;
	enum class noteToDecayEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(noteToDecayEnum::SWITCH_OFF, noteToDecay)) etc... 

	int synthEngineMode = 0;
	enum class synthEngineModeEnum { Poly,Mono,Unison };	// to compare: if(compareEnum(synthEngineModeEnum::Poly, synthEngineMode)) etc... 

	int resetToZero = 0;
	enum class resetToZeroEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(resetToZeroEnum::SWITCH_OFF, resetToZero)) etc... 

	int legatoMode = 0;
	enum class legatoModeEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(legatoModeEnum::SWITCH_OFF, legatoMode)) etc... 

	int enablePortamento = 0;
	enum class enablePortamentoEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(enablePortamentoEnum::SWITCH_OFF, enablePortamento)) etc... 

	int LPFilterType = 0;
	enum class LPFilterTypeEnum { LPF4,LPF2 };	// to compare: if(compareEnum(LPFilterTypeEnum::LPF4, LPFilterType)) etc... 

	int applyNLP = 0;
	enum class applyNLPEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(applyNLPEnum::SWITCH_OFF, applyNLP)) etc... 

	int lfo2Mode = 0;
	enum class lfo2ModeEnum { Sync,FreeRun,OneShot };	// to compare: if(compareEnum(lfo2ModeEnum::Sync, lfo2Mode)) etc... 

	int lfo1Mode = 0;
	enum class lfo1ModeEnum { Sync,FreeRun,OneShot };	// to compare: if(compareEnum(lfo1ModeEnum::Sync, lfo1Mode)) etc... 

	int invertDCA_EG = 0;
	enum class invertDCA_EGEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(invertDCA_EGEnum::SWITCH_OFF, invertDCA_EG)) etc... 

	int invertFilterFc = 0;
	enum class invertFilterFcEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(invertFilterFcEnum::SWITCH_OFF, invertFilterFc)) etc... 

	int modSource1 = 0;
	enum class modSource1Enum { None,LFO1_Out,LFO1_InvOut,LFO2_Out,LFO2_InvOut,EG1_Out,EG1_BiasOut,EG2_Out,EG2_BiasOut,Osc1_Out,Osc2_Out,SubOsc_Out };	// to compare: if(compareEnum(modSource1Enum::None, modSource1)) etc... 

	int modDest1 = 0;
	enum class modDest1Enum { None,Osc1_Pitch,Osc2_Pitch,All_Osc_Pitch,Osc1_PW,Osc2_PW,SubOsc_PW,All_Osc_PW,Filter1_fc,Filter1_Q,Filter2_fc,Filter2_Q,DCA_Amp,DCA_Pan,DelayFX_Mix,DelayFX_FB,Chorus_Depth };	// to compare: if(compareEnum(modDest1Enum::None, modDest1)) etc... 

	int modSource2 = 0;
	enum class modSource2Enum { None,LFO1_Out,LFO1_InvOut,LFO2_Out,LFO2_InvOut,EG1_Out,EG1_BiasOut,EG2_Out,EG2_BiasOut,Osc1_Out,Osc2_Out,SubOsc_Out };	// to compare: if(compareEnum(modSource2Enum::None, modSource2)) etc... 

	int modDest2 = 0;
	enum class modDest2Enum { None,Osc1_Pitch,Osc2_Pitch,All_Osc_Pitch,Osc1_PW,Osc2_PW,SubOsc_PW,All_Osc_PW,Filter1_fc,Filter1_Q,DCA_Amp,DCA_Pan,DelayFX_Mix,DelayFX_FB,Chorus_Depth };	// to compare: if(compareEnum(modDest2Enum::None, modDest2)) etc... 

	int delayType = 0;
	enum class delayTypeEnum { normal,cross,pingpong };	// to compare: if(compareEnum(delayTypeEnum::normal, delayType)) etc... 

	int enableDelayFX = 0;
	enum class enableDelayFXEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(enableDelayFXEnum::SWITCH_OFF, enableDelayFX)) etc... 

	int enableChorusFX = 0;
	enum class enableChorusFXEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(enableChorusFXEnum::SWITCH_OFF, enableChorusFX)) etc... 

	int HPFilterType = 0;
	enum class HPFilterTypeEnum { HPF4,HPF2 };	// to compare: if(compareEnum(HPFilterTypeEnum::HPF4, HPFilterType)) etc... 

	int LPFenable = 0;
	enum class LPFenableEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(LPFenableEnum::SWITCH_OFF, LPFenable)) etc... 

	int HPFenable = 0;
	enum class HPFenableEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnum(HPFenableEnum::SWITCH_OFF, HPFenable)) etc... 

	// **--0x1A7F--**
    // --- end member variables

public:
    // --- description functions: you do NOT need to alter these
    static const char* getPluginName();
    static const char* getShortPluginName();
    static const char* getVendorName();
    static const char* getVendorURL();
    static const char* getVendorEmail();
    static const char* getAUCocoaViewFactoryName();
    static pluginType getPluginType();
    static const char* getVSTFUID();
    static int32_t getFourCharCode();
    bool initPluginDescriptors();

};




#endif /* defined(__pluginCore_h__) */
