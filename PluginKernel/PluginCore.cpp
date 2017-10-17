//
//  pluginCore.cpp
//  VolGUI_I
//
//  Created by Will Pirkle on 3/30/17.
//
//
// --- for OutputDebugString() only; can be removed for MacOS
//#define NOMINMAX		// --- needed for VSTGUI4 min/max defs
//#include <windows.h>

#include "pluginCore.h"
#include "plugindescription.h"

PluginCore::PluginCore()
{
    // --- describe the plugin; call the helper to init the static parts you setup in plugindescription.h
    initPluginDescriptors();

    // --- describe the plugin attributes; set according to your needs
    pluginDescriptor.hasSidechain = false; // or true if you want a sidechain
    // pluginDescriptor.hasCustomGUI = false; // uncomment if you don't want custom GUI
    pluginDescriptor.latencyInSamples = 0;
    pluginDescriptor.tailTimeInMSec = 0.00;
    pluginDescriptor.infiniteTailVST3 = false;

    // --- default I/O combinations
    //     you can add more combinations by modifying this
    pluginDescriptor.numSupportedIOCombinations = 5;
    pluginDescriptor.supportedIOCombinations = new ChannelIOConfig[pluginDescriptor.numSupportedIOCombinations];

	// --- for FX plugins
    pluginDescriptor.supportedIOCombinations[0] = {kCFMono,   kCFMono};
    pluginDescriptor.supportedIOCombinations[1] = {kCFMono,   kCFStereo};
    pluginDescriptor.supportedIOCombinations[2] = {kCFStereo, kCFStereo};

	// --- for SYNTH plugins
	pluginDescriptor.supportedIOCombinations[3] = { kCFNone, kCFMono };
	pluginDescriptor.supportedIOCombinations[4] = { kCFNone, kCFStereo };

    // --- for sidechaining, we support mono and stereo inputs; auxOutputs reserved for future use
    pluginDescriptor.numSupportedAuxIOCombinations = 2;
    pluginDescriptor.supportedAuxIOCombinations = new ChannelIOConfig[pluginDescriptor.numSupportedAuxIOCombinations];
    pluginDescriptor.supportedAuxIOCombinations[0] = {kCFMono,   kCFNone};
    pluginDescriptor.supportedAuxIOCombinations[1] = {kCFStereo, kCFNone};

    // --- VST3 Sample Accurate Automation settings
    apiSpecificInfo.enableVST3SampleAccurateAutomation = false;
    apiSpecificInfo.vst3SampleAccurateGranularity = 1;

    // --- create the parameters
    initPluginParameters();

    // --- create the presets
    initPluginPresets();

	// --- finish inits
	synthEngine = new SynthEngine;

}

bool PluginCore::initPluginParameters()
{
	// ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! -------------------------------------- //
	if (pluginParameterMap.size() > 0)
		return true;

	// **--0xDEA7--**


	// --- Declaration of Plugin Parameter Objects 
	PluginParameter* piParam = nullptr;

	// --- continuous control: EG1 Attack
	piParam = new PluginParameter(controlID::eg1AttackTime_mSec, "EG1 Attack", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 20.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1AttackTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG1 Decay
	piParam = new PluginParameter(controlID::eg1DecayTime_mSec, "EG1 Decay", "mSec", controlVariableType::kDouble, 0.000000, 5000.000000, 200.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1DecayTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG1 Sustain
	piParam = new PluginParameter(controlID::eg1SustainLevel, "EG1 Sustain", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1SustainLevel, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG1 Release
	piParam = new PluginParameter(controlID::eg1ReleaseTime_mSec, "EG1 Release", "mSec", controlVariableType::kDouble, 0.000000, 10000.000000, 1000.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1ReleaseTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Osc1 Wave
	piParam = new PluginParameter(controlID::osc1Wave, "Osc1 Wave", "Saw,Square,Triangle,Sin,WhiteNoise", "Saw");
	piParam->setBoundVariable(&osc1Wave, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Osc1 Level
	piParam = new PluginParameter(controlID::osc1AmpControl_dB, "Osc1 Level", "dB", controlVariableType::kDouble, -60.000000, 12.000000, -6.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc1AmpControl_dB, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Osc1 PW
	piParam = new PluginParameter(controlID::osc1PulseWidthControl_Pct, "Osc1 PW", "%", controlVariableType::kDouble, 2.000000, 98.000000, 50.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc1PulseWidthControl_Pct, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Osc2 Wave
	piParam = new PluginParameter(controlID::osc2Wave, "Osc2 Wave", "Saw,Square,Triangle,Sin,WhiteNoise", "Saw");
	piParam->setBoundVariable(&osc2Wave, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Osc2 Level
	piParam = new PluginParameter(controlID::osc2AmpControl_dB, "Osc2 Level", "dB", controlVariableType::kDouble, -60.000000, 12.000000, -6.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc2AmpControl_dB, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Osc2 PW
	piParam = new PluginParameter(controlID::osc2PulseWidthControl_Pct, "Osc2 PW", "%", controlVariableType::kDouble, 2.000000, 98.000000, 50.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc2PulseWidthControl_Pct, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Osc2 Detune
	piParam = new PluginParameter(controlID::osc2Detune_cents, "Osc2 Detune", "cents", controlVariableType::kDouble, -25.000000, 25.000000, 1.500000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc2Detune_cents, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LFO1 Rate
	piParam = new PluginParameter(controlID::lfo1FreqControl, "LFO1 Rate", "Hz", controlVariableType::kDouble, 0.020000, 20.000000, 0.020000, taper::kVoltOctaveTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&lfo1FreqControl, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LFO1 Level
	piParam = new PluginParameter(controlID::lfo1AmpControl, "LFO1 Level", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&lfo1AmpControl, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: LFO1 Wave
	piParam = new PluginParameter(controlID::lfo1Wave, "LFO1 Wave", "Sin,UpSaw,DownSaw,Square,Triangle,RSH,QRSH,ExpUp,ExpDown,WhiteNoise", "Sin");
	piParam->setBoundVariable(&lfo1Wave, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- discrete control: LFO2 Wave
	piParam = new PluginParameter(controlID::lfo2Wave, "LFO2 Wave", "Sin,UpSaw,DownSaw,Square,Triangle,RSH,QRSH,ExpUp,ExpDown,WhiteNoise", "Sin");
	piParam->setBoundVariable(&lfo2Wave, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: LFO2 Rate
	piParam = new PluginParameter(controlID::lfo2FreqControl, "LFO2 Rate", "Hz", controlVariableType::kDouble, 0.020000, 20.000000, 0.020000, taper::kVoltOctaveTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&lfo2FreqControl, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LFO2 Level
	piParam = new PluginParameter(controlID::lfo2AmpControl, "LFO2 Level", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&lfo2AmpControl, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LP Filter Fc
	piParam = new PluginParameter(controlID::filter1Fc, "LP Filter Fc", "Hz", controlVariableType::kDouble, 20.000000, 20480.000000, 10000.000000, taper::kVoltOctaveTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&filter1Fc, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LP Filter Q
	piParam = new PluginParameter(controlID::filter1Q, "LP Filter Q", "", controlVariableType::kDouble, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&filter1Q, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Attack
	piParam = new PluginParameter(controlID::eg2AttackTime_mSec, "EG2 Attack", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 20.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2AttackTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Decay
	piParam = new PluginParameter(controlID::eg2DecayTime_mSec, "EG2 Decay", "mSec", controlVariableType::kDouble, 0.000000, 5000.000000, 200.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2DecayTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Sustain
	piParam = new PluginParameter(controlID::eg2SustainLevel, "EG2 Sustain", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2SustainLevel, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Release
	piParam = new PluginParameter(controlID::eg2ReleaseTime_mSec, "EG2 Release", "mSec", controlVariableType::kDouble, 0.000000, 10000.000000, 1000.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2ReleaseTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Fc Int
	piParam = new PluginParameter(controlID::eg2FilterFcIntensity, "EG2 Fc Int", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2FilterFcIntensity, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: LP Bass Gain
	piParam = new PluginParameter(controlID::gainComp, "LP Bass Gain", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&gainComp, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: KEYTRK
	piParam = new PluginParameter(controlID::enableKeyTrack, "KEYTRK", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&enableKeyTrack, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: KeyTrk Ratio
	piParam = new PluginParameter(controlID::keytrackRatio, "KeyTrk Ratio", "", controlVariableType::kDouble, 0.500000, 2.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&keytrackRatio, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: VEL-ATT
	piParam = new PluginParameter(controlID::velocityToAttack, "VEL-ATT", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&velocityToAttack, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: NT-DCY
	piParam = new PluginParameter(controlID::noteToDecay, "NT-DCY", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&noteToDecay, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: Mode
	piParam = new PluginParameter(controlID::synthEngineMode, "Mode", "Poly,Mono,Unison", "Poly");
	piParam->setBoundVariable(&synthEngineMode, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: R-T-Z
	piParam = new PluginParameter(controlID::resetToZero, "R-T-Z", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&resetToZero, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: Master Vol
	piParam = new PluginParameter(controlID::masterVolume_dB, "Master Vol", "dB", controlVariableType::kDouble, -60.000000, 12.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&masterVolume_dB, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Tuning Ratio
	piParam = new PluginParameter(controlID::masterTuningRatio, "Tuning Ratio", "", controlVariableType::kDouble, 0.500000, 2.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&masterTuningRatio, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Tuning Offset
	piParam = new PluginParameter(controlID::masterTuningOffset_cents, "Tuning Offset", "cents", controlVariableType::kDouble, -200.000000, 200.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&masterTuningOffset_cents, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: PB Range
	piParam = new PluginParameter(controlID::masterPitchBend, "PB Range", "semis", controlVariableType::kInt, 1.000000, 12.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(false);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&masterPitchBend, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Unison Detune
	piParam = new PluginParameter(controlID::unisonDetune_cents, "Unison Detune", "cents", controlVariableType::kDouble, 0.000000, 25.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&unisonDetune_cents, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Legato
	piParam = new PluginParameter(controlID::legatoMode, "Legato", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&legatoMode, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: GLIDE
	piParam = new PluginParameter(controlID::enablePortamento, "GLIDE", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&enablePortamento, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: Glide Time
	piParam = new PluginParameter(controlID::portamentoTime_mSec, "Glide Time", "mSec", controlVariableType::kDouble, 0.000000, 5000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&portamentoTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: LP Filter
	piParam = new PluginParameter(controlID::LPFilterType, "LP Filter", "LPF4,LPF2", "LPF4");
	piParam->setBoundVariable(&LPFilterType, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: NLP
	piParam = new PluginParameter(controlID::applyNLP, "NLP", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&applyNLP, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: LFO2 Mode
	piParam = new PluginParameter(controlID::lfo2Mode, "LFO2 Mode", "Sync,FreeRun,OneShot", "Sync");
	piParam->setBoundVariable(&lfo2Mode, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- discrete control: LFO1 Mode
	piParam = new PluginParameter(controlID::lfo1Mode, "LFO1 Mode", "Sync,FreeRun,OneShot", "Sync");
	piParam->setBoundVariable(&lfo1Mode, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Osc1 Ratio
	piParam = new PluginParameter(controlID::osc1TuningRatio, "Osc1 Ratio", "", controlVariableType::kDouble, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&osc1TuningRatio, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Inv DCA eg
	piParam = new PluginParameter(controlID::invertDCA_EG, "Inv DCA eg", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&invertDCA_EG, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: Inv Flt Fc
	piParam = new PluginParameter(controlID::invertFilterFc, "Inv Flt Fc", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&invertFilterFc, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: Mod Source 1
	piParam = new PluginParameter(controlID::modSource1, "Mod Source 1", "None,LFO1 Out,LFO1 InvOut,LFO2 Out,LFO2 InvOut,EG1 Out,EG1 BiasOut,EG2 Out,EG2 BiasOut,Osc1 Out,Osc2 Out,SubOsc Out", "None");
	piParam->setBoundVariable(&modSource1, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- discrete control: Mod Dest 1
	piParam = new PluginParameter(controlID::modDest1, "Mod Dest 1", "None,Osc1 Pitch,Osc2 Pitch,All Osc Pitch,Osc1 PW,Osc2 PW,SubOsc PW,All Osc PW,Filter1 fc,Filter1 Q,Filter2 fc,Filter2 Q,DCA Amp,DCA Pan,DelayFX Mix,DelayFX FB,Chorus Depth", "None");
	piParam->setBoundVariable(&modDest1, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Mod 1 Int
	piParam = new PluginParameter(controlID::modRouting1_Intensity, "Mod 1 Int", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&modRouting1_Intensity, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Mod Source 2
	piParam = new PluginParameter(controlID::modSource2, "Mod Source 2", "None,LFO1 Out,LFO1 InvOut,LFO2 Out,LFO2 InvOut,EG1 Out,EG1 BiasOut,EG2 Out,EG2 BiasOut,Osc1 Out,Osc2 Out,SubOsc Out", "None");
	piParam->setBoundVariable(&modSource2, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- discrete control: Mod Dest 2
	piParam = new PluginParameter(controlID::modDest2, "Mod Dest 2", "None,Osc1 Pitch,Osc2 Pitch,All Osc Pitch,Osc1 PW,Osc2 PW,SubOsc PW,All Osc PW,Filter1 fc,Filter1 Q,DCA Amp,DCA Pan,DelayFX Mix,DelayFX FB,Chorus Depth", "None");
	piParam->setBoundVariable(&modDest2, boundVariableType::kInt);
	addPluginParameter(piParam);

	// --- continuous control: Mod 2 Int
	piParam = new PluginParameter(controlID::modRouting2_Intensity, "Mod 2 Int", "", controlVariableType::kDouble, 0.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&modRouting2_Intensity, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: SubOsc Level
	piParam = new PluginParameter(controlID::subOscAmpControl_dB, "SubOsc Level", "dB", controlVariableType::kDouble, -60.000000, 12.000000, -60.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&subOscAmpControl_dB, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Delay Time
	piParam = new PluginParameter(controlID::delayTime_mSec, "Delay Time", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&delayTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Delay Feedback
	piParam = new PluginParameter(controlID::feedback_Pct, "Delay Feedback", "%", controlVariableType::kDouble, 0.000000, 100.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&feedback_Pct, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Delay Ratio
	piParam = new PluginParameter(controlID::delayRatio, "Delay Ratio", "", controlVariableType::kDouble, -0.990000, 0.990000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&delayRatio, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Delay Mix
	piParam = new PluginParameter(controlID::delayMix_Pct, "Delay Mix", "%", controlVariableType::kDouble, 0.000000, 100.000000, 50.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&delayMix_Pct, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: Delay Type
	piParam = new PluginParameter(controlID::delayType, "Delay Type", "normal,cross,pingpong", "normal");
	piParam->setBoundVariable(&delayType, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: DELAY
	piParam = new PluginParameter(controlID::enableDelayFX, "DELAY", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&enableDelayFX, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: Chorus Rate
	piParam = new PluginParameter(controlID::chorusRate_Hz, "Chorus Rate", "Hz", controlVariableType::kDouble, 0.020000, 0.500000, 0.100000, taper::kVoltOctaveTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&chorusRate_Hz, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: Chorus Depth
	piParam = new PluginParameter(controlID::chorusDepth_Pct, "Chorus Depth", "%", controlVariableType::kDouble, 0.000000, 100.000000, 50.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&chorusDepth_Pct, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: CHORUS
	piParam = new PluginParameter(controlID::enableChorusFX, "CHORUS", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&enableChorusFX, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HP Filter
	piParam = new PluginParameter(controlID::HPFilterType, "HP Filter", "HPF4,HPF2", "HPF4");
	piParam->setBoundVariable(&HPFilterType, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: HP Filter Fc
	piParam = new PluginParameter(controlID::filter2Fc, "HP Filter Fc", "Hz", controlVariableType::kDouble, 20.000000, 20480.000000, 50.000000, taper::kVoltOctaveTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&filter2Fc, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: HP Filter Q
	piParam = new PluginParameter(controlID::filter2Q, "HP Filter Q", "", controlVariableType::kDouble, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&filter2Q, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- discrete control: LPF OFF
	piParam = new PluginParameter(controlID::LPFenable, "LPF OFF", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&LPFenable, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HPF OFF
	piParam = new PluginParameter(controlID::HPFenable, "HPF OFF", "SWITCH OFF,SWITCH ON", "SWITCH_OFF");
	piParam->setBoundVariable(&HPFenable, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Delay
	piParam = new PluginParameter(controlID::eg2DelayTime_mSec, "EG2 Delay", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2DelayTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG1 Delay
	piParam = new PluginParameter(controlID::eg1DelayTime_mSec, "EG1 Delay", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1DelayTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG1 Repeat
	piParam = new PluginParameter(controlID::eg1RepeatTime_mSec, "EG1 Repeat", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg1RepeatTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- continuous control: EG2 Repeat
	piParam = new PluginParameter(controlID::eg2RepeatTime_mSec, "EG2 Repeat", "mSec", controlVariableType::kDouble, 0.000000, 2000.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(100.00);
	piParam->setBoundVariable(&eg2RepeatTime_mSec, boundVariableType::kDouble);
	addPluginParameter(piParam);

	// --- BONUS Parameters
	// --- SCALE_GUI_SIZE
	piParam = new PluginParameter(SCALE_GUI_SIZE, "Scale GUI", "tiny,small,medium,normal,large,giant", "normal");
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- Aux Attributes
	AuxParameterAttribute auxAttribute;

	// --- RAFX GUI attributes
	// --- controlID::eg1AttackTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1AttackTime_mSec, auxAttribute);

	// --- controlID::eg1DecayTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1DecayTime_mSec, auxAttribute);

	// --- controlID::eg1SustainLevel
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1SustainLevel, auxAttribute);

	// --- controlID::eg1ReleaseTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1ReleaseTime_mSec, auxAttribute);

	// --- controlID::osc1Wave
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::osc1Wave, auxAttribute);

	// --- controlID::osc1AmpControl_dB
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::osc1AmpControl_dB, auxAttribute);

	// --- controlID::osc1PulseWidthControl_Pct
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::osc1PulseWidthControl_Pct, auxAttribute);

	// --- controlID::osc2Wave
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::osc2Wave, auxAttribute);

	// --- controlID::osc2AmpControl_dB
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::osc2AmpControl_dB, auxAttribute);

	// --- controlID::osc2PulseWidthControl_Pct
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::osc2PulseWidthControl_Pct, auxAttribute);

	// --- controlID::osc2Detune_cents
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::osc2Detune_cents, auxAttribute);

	// --- controlID::lfo1FreqControl
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::lfo1FreqControl, auxAttribute);

	// --- controlID::lfo1AmpControl
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::lfo1AmpControl, auxAttribute);

	// --- controlID::lfo1Wave
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::lfo1Wave, auxAttribute);

	// --- controlID::lfo2Wave
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::lfo2Wave, auxAttribute);

	// --- controlID::lfo2FreqControl
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::lfo2FreqControl, auxAttribute);

	// --- controlID::lfo2AmpControl
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::lfo2AmpControl, auxAttribute);

	// --- controlID::filter1Fc
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(4);
	setParamAuxAttribute(controlID::filter1Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::filter1Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::filter1Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::filter1Fc, auxAttribute);

	// --- controlID::filter1Q
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(4);
	setParamAuxAttribute(controlID::filter1Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::filter1Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::filter1Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(17);
	setParamAuxAttribute(controlID::filter1Q, auxAttribute);

	// --- controlID::eg2AttackTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2AttackTime_mSec, auxAttribute);

	// --- controlID::eg2DecayTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2DecayTime_mSec, auxAttribute);

	// --- controlID::eg2SustainLevel
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2SustainLevel, auxAttribute);

	// --- controlID::eg2ReleaseTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2ReleaseTime_mSec, auxAttribute);

	// --- controlID::eg2FilterFcIntensity
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2FilterFcIntensity, auxAttribute);

	// --- controlID::gainComp
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(4);
	setParamAuxAttribute(controlID::gainComp, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::gainComp, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::gainComp, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(18);
	setParamAuxAttribute(controlID::gainComp, auxAttribute);

	// --- controlID::enableKeyTrack
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::enableKeyTrack, auxAttribute);

	// --- controlID::keytrackRatio
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(4);
	setParamAuxAttribute(controlID::keytrackRatio, auxAttribute);

	// --- controlID::velocityToAttack
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::velocityToAttack, auxAttribute);

	// --- controlID::noteToDecay
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::noteToDecay, auxAttribute);

	// --- controlID::resetToZero
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::resetToZero, auxAttribute);

	// --- controlID::masterVolume_dB
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::masterVolume_dB, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::masterVolume_dB, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::masterVolume_dB, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(7);
	setParamAuxAttribute(controlID::masterVolume_dB, auxAttribute);

	// --- controlID::masterTuningRatio
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::masterTuningRatio, auxAttribute);

	// --- controlID::masterTuningOffset_cents
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::masterTuningOffset_cents, auxAttribute);

	// --- controlID::masterPitchBend
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::masterPitchBend, auxAttribute);

	// --- controlID::unisonDetune_cents
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::unisonDetune_cents, auxAttribute);

	// --- controlID::legatoMode
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::legatoMode, auxAttribute);

	// --- controlID::enablePortamento
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::enablePortamento, auxAttribute);

	// --- controlID::portamentoTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::portamentoTime_mSec, auxAttribute);

	// --- controlID::applyNLP
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::applyNLP, auxAttribute);

	// --- controlID::lfo2Mode
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::lfo2Mode, auxAttribute);

	// --- controlID::lfo1Mode
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::lfo1Mode, auxAttribute);

	// --- controlID::osc1TuningRatio
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::osc1TuningRatio, auxAttribute);

	// --- controlID::invertDCA_EG
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::invertDCA_EG, auxAttribute);

	// --- controlID::invertFilterFc
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::invertFilterFc, auxAttribute);

	// --- controlID::modSource1
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modSource1, auxAttribute);

	// --- controlID::modDest1
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modDest1, auxAttribute);

	// --- controlID::modRouting1_Intensity
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modRouting1_Intensity, auxAttribute);

	// --- controlID::modSource2
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modSource2, auxAttribute);

	// --- controlID::modDest2
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modDest2, auxAttribute);

	// --- controlID::modRouting2_Intensity
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(14);
	setParamAuxAttribute(controlID::modRouting2_Intensity, auxAttribute);

	// --- controlID::subOscAmpControl_dB
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::subOscAmpControl_dB, auxAttribute);

	// --- controlID::delayTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::delayTime_mSec, auxAttribute);

	// --- controlID::feedback_Pct
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::feedback_Pct, auxAttribute);

	// --- controlID::delayRatio
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::delayRatio, auxAttribute);

	// --- controlID::delayMix_Pct
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(2);
	setParamAuxAttribute(controlID::delayMix_Pct, auxAttribute);

	// --- controlID::enableDelayFX
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::enableDelayFX, auxAttribute);

	// --- controlID::chorusRate_Hz
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::chorusRate_Hz, auxAttribute);

	// --- controlID::chorusDepth_Pct
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(12);
	setParamAuxAttribute(controlID::chorusDepth_Pct, auxAttribute);

	// --- controlID::enableChorusFX
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::enableChorusFX, auxAttribute);

	// --- controlID::filter2Fc
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::filter2Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::filter2Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::filter2Fc, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(16);
	setParamAuxAttribute(controlID::filter2Fc, auxAttribute);

	// --- controlID::filter2Q
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(10);
	setParamAuxAttribute(controlID::filter2Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::EnableMIDIControl);
	auxAttribute.setBoolAttribute(true);
	setParamAuxAttribute(controlID::filter2Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlChannel);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::filter2Q, auxAttribute);

	auxAttribute.reset(auxGUIIdentifier::MIDIControlIndex);
	auxAttribute.setUintAttribute(17);
	setParamAuxAttribute(controlID::filter2Q, auxAttribute);

	// --- controlID::LPFenable
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::LPFenable, auxAttribute);

	// --- controlID::HPFenable
	auxAttribute.reset(auxGUIIdentifier::GUI2SSButtonStyle);
	auxAttribute.setUintAttribute(0);
	setParamAuxAttribute(controlID::HPFenable, auxAttribute);

	// --- controlID::eg2DelayTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2DelayTime_mSec, auxAttribute);

	// --- controlID::eg1DelayTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1DelayTime_mSec, auxAttribute);

	// --- controlID::eg1RepeatTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(6);
	setParamAuxAttribute(controlID::eg1RepeatTime_mSec, auxAttribute);

	// --- controlID::eg2RepeatTime_mSec
	auxAttribute.reset(auxGUIIdentifier::GUIKnobGraphic);
	auxAttribute.setUintAttribute(8);
	setParamAuxAttribute(controlID::eg2RepeatTime_mSec, auxAttribute);


	// **--0xEDA5--**

	// --- create the super fast access array
	initPluginParameterArray();

    return true;
}

// --- maybe look at doing this by reading a TXT file that is a resource?
//     write the file from the GUI? Debug control to save file?
bool PluginCore::initPluginPresets()
{
	// **--0xFF7A--**

	// --- Plugin Presets 
	int index = 0;
	PresetInfo* preset = nullptr;

	// --- Preset: Factory Preset
	preset = new PresetInfo(index++, "Factory Preset");
	initPresetParameters(preset->presetParameters);
	setPresetParameter(preset->presetParameters, controlID::eg1AttackTime_mSec, 20.000000);
	setPresetParameter(preset->presetParameters, controlID::eg1DecayTime_mSec, 200.000000);
	setPresetParameter(preset->presetParameters, controlID::eg1SustainLevel, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::eg1ReleaseTime_mSec, 1000.000000);
	setPresetParameter(preset->presetParameters, controlID::osc1Wave, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::osc1AmpControl_dB, -6.000000);
	setPresetParameter(preset->presetParameters, controlID::osc1PulseWidthControl_Pct, 50.000000);
	setPresetParameter(preset->presetParameters, controlID::osc2Wave, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::osc2AmpControl_dB, -6.000000);
	setPresetParameter(preset->presetParameters, controlID::osc2PulseWidthControl_Pct, 50.000000);
	setPresetParameter(preset->presetParameters, controlID::osc2Detune_cents, 1.499998);
	setPresetParameter(preset->presetParameters, controlID::lfo1FreqControl, 0.020000);
	setPresetParameter(preset->presetParameters, controlID::lfo1AmpControl, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::lfo1Wave, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::lfo2Wave, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::lfo2FreqControl, 0.020000);
	setPresetParameter(preset->presetParameters, controlID::lfo2AmpControl, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::filter1Fc, 10000.000000);
	setPresetParameter(preset->presetParameters, controlID::filter1Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2AttackTime_mSec, 20.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2DecayTime_mSec, 200.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2SustainLevel, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::eg2ReleaseTime_mSec, 1000.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2FilterFcIntensity, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::gainComp, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::enableKeyTrack, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::keytrackRatio, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::velocityToAttack, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::noteToDecay, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::synthEngineMode, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::resetToZero, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::masterVolume_dB, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::masterTuningRatio, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::masterTuningOffset_cents, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::masterPitchBend, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::unisonDetune_cents, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::legatoMode, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::enablePortamento, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::portamentoTime_mSec, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::LPFilterType, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::applyNLP, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::lfo2Mode, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::lfo1Mode, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::osc1TuningRatio, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::invertDCA_EG, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::invertFilterFc, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::modSource1, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::modDest1, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::modRouting1_Intensity, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::modSource2, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::modDest2, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::modRouting2_Intensity, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::subOscAmpControl_dB, -60.000000);
	setPresetParameter(preset->presetParameters, controlID::delayTime_mSec, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::feedback_Pct, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::delayRatio, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::delayMix_Pct, 50.000000);
	setPresetParameter(preset->presetParameters, controlID::delayType, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::enableDelayFX, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::chorusRate_Hz, 0.100000);
	setPresetParameter(preset->presetParameters, controlID::chorusDepth_Pct, 50.000000);
	setPresetParameter(preset->presetParameters, controlID::enableChorusFX, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPFilterType, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::filter2Fc, 50.000000);
	setPresetParameter(preset->presetParameters, controlID::filter2Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::LPFenable, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPFenable, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2DelayTime_mSec, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::eg1DelayTime_mSec, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::eg1RepeatTime_mSec, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::eg2RepeatTime_mSec, 0.000000);
	addPreset(preset);


	// **--0xA7FF--**

    return true;
}

bool PluginCore::reset(ResetInfo& resetInfo)
{
    // --- save for audio processing
    audioProcDescriptor.sampleRate = resetInfo.sampleRate;
    audioProcDescriptor.bitDepth = resetInfo.bitDepth;

	synthEngine->reset(resetInfo);

    // --- other reset inits
    return PluginBase::reset(resetInfo);
}

void PluginCore::updateEngine()
{
	if (!synthEngine) return;

	std::shared_ptr<SynthEngineModifiers> synthModifiers = synthEngine->getSynthEngineModifiers();
	if (!synthModifiers) return;

	std::shared_ptr<SynthVoiceModifiers> voiceModifiers = synthEngine->getSynthVoiceModifiers();
	if (!voiceModifiers) return;

	// --- mono/poly operation
	synthModifiers->synthMode = convertEnum(synthEngineMode, synthMode);

	// --- master PB
	synthModifiers->masterPitchBend = masterPitchBend;

	// --- chorus FX (master, on Engine level)
	synthModifiers->chorusFXModifiers->chorusRate_Hz = chorusRate_Hz;
	synthModifiers->chorusFXModifiers->chorusDepth_Pct = chorusDepth_Pct;
	synthModifiers->chorusFXModifiers->enabled = (enableChorusFX == 1);

	// --- delay FX (master, on Engine level)
	synthModifiers->delayFXModifiers->delayTime_mSec = delayTime_mSec;
	synthModifiers->delayFXModifiers->feedback_Pct = feedback_Pct;
	synthModifiers->delayFXModifiers->delayRatio = delayRatio;
	synthModifiers->delayFXModifiers->delayMix_Pct = delayMix_Pct;
	synthModifiers->delayFXModifiers->delayFXMode = convertEnum(delayType, delayFXMode);
	synthModifiers->delayFXModifiers->enabled = (enableDelayFX == 1);

	// --- Portamento
	voiceModifiers->enablePortamento = (enablePortamento == 1);
	voiceModifiers->portamentoTime_mSec = portamentoTime_mSec;

	// --- OSC1:
	voiceModifiers->osc1Modifiers->oscWave = convertEnum(osc1Wave, synthOscWaveform);
	voiceModifiers->osc1Modifiers->oscAmpControl_dB = osc1AmpControl_dB;
	voiceModifiers->osc1Modifiers->pulseWidthControl_Pct = osc1PulseWidthControl_Pct;
	voiceModifiers->osc1Modifiers->masterTuningRatio = masterTuningRatio;
	voiceModifiers->osc1Modifiers->masterTuningOffset_cents = masterTuningOffset_cents;
	voiceModifiers->osc1Modifiers->unisonDetune_cents = unisonDetune_cents;
	voiceModifiers->osc1Modifiers->oscFreqRatio = osc1TuningRatio;

	// --- the sub-oscillator uses:
	//           - same waveshape
	//           - same pulse width
	//           - master tuning stuff
	//           - NOT osc frequency ratio (would confuse user/listener?)
	//           as oscillator 1, it only has a volume control
	voiceModifiers->subOscModifiers->oscWave = convertEnum(osc1Wave, synthOscWaveform);
	voiceModifiers->subOscModifiers->pulseWidthControl_Pct = osc1PulseWidthControl_Pct;
	voiceModifiers->subOscModifiers->masterTuningRatio = masterTuningRatio;
	voiceModifiers->subOscModifiers->masterTuningOffset_cents = masterTuningOffset_cents;
//	voiceModifiers->subOscModifiers->oscFreqRatio = osc1TuningRatio;

	// --- the only control for the subOsc
	voiceModifiers->subOscModifiers->oscAmpControl_dB = subOscAmpControl_dB;


	// --- OSC2:
	voiceModifiers->osc2Modifiers->oscWave = convertEnum(osc2Wave, synthOscWaveform);
	voiceModifiers->osc2Modifiers->oscAmpControl_dB = osc2AmpControl_dB;
	voiceModifiers->osc2Modifiers->pulseWidthControl_Pct = osc2PulseWidthControl_Pct;
	voiceModifiers->osc2Modifiers->masterTuningRatio = masterTuningRatio;
	voiceModifiers->osc2Modifiers->cents = osc2Detune_cents;
	voiceModifiers->osc2Modifiers->masterTuningOffset_cents = masterTuningOffset_cents;
	voiceModifiers->osc2Modifiers->unisonDetune_cents = unisonDetune_cents;

	// --- LFO1
	voiceModifiers->lfo1Modifiers->oscWave = convertEnum(lfo1Wave, LFOWaveform);
	voiceModifiers->lfo1Modifiers->oscAmpControl = lfo1AmpControl;
	voiceModifiers->lfo1Modifiers->oscFreqControl = lfo1FreqControl;
	voiceModifiers->lfo1Modifiers->oscMode = convertEnum(lfo1Mode, LFOMode);

	// --- LFO2
	voiceModifiers->lfo2Modifiers->oscWave = convertEnum(lfo2Wave, LFOWaveform);
	voiceModifiers->lfo2Modifiers->oscAmpControl = lfo2AmpControl;
	voiceModifiers->lfo2Modifiers->oscFreqControl = lfo2FreqControl;
	voiceModifiers->lfo2Modifiers->oscMode = convertEnum(lfo2Mode, LFOMode);
	
	// --- EG1

	voiceModifiers->eg1Modifiers->repeatTime_mSec = eg1RepeatTime_mSec;
	voiceModifiers->eg1Modifiers->delayTime_mSec = eg1DelayTime_mSec;
	voiceModifiers->eg1Modifiers->attackTime_mSec = eg1AttackTime_mSec;
	voiceModifiers->eg1Modifiers->decayTime_mSec = eg1DecayTime_mSec;
	voiceModifiers->eg1Modifiers->sustainLevel = eg1SustainLevel;
	voiceModifiers->eg1Modifiers->releaseTime_mSec = eg1ReleaseTime_mSec;
	
	// --- since EG1 hardwired to output DCA; these scaling values will be applied
	voiceModifiers->eg1Modifiers->velocityToAttackScaling = (velocityToAttack == 1);
	voiceModifiers->eg1Modifiers->noteNumberToDecayScaling = (noteToDecay == 1);

	// --- reset to zero must always be TRUE for NON-MONO modes for voice steal to work properly
	if (synthModifiers->synthMode == synthMode::kMono)
		voiceModifiers->eg1Modifiers->resetToZero = (resetToZero == 1);
	else // is poly or unison mode
		voiceModifiers->eg1Modifiers->resetToZero = true;
	voiceModifiers->eg1Modifiers->legatoMode = (legatoMode == 1);
	
	voiceModifiers->eg2Modifiers->repeatTime_mSec = eg2RepeatTime_mSec;
	voiceModifiers->eg2Modifiers->delayTime_mSec = eg2DelayTime_mSec; 
	voiceModifiers->eg2Modifiers->attackTime_mSec = eg2AttackTime_mSec;
	voiceModifiers->eg2Modifiers->decayTime_mSec = eg2DecayTime_mSec;
	voiceModifiers->eg2Modifiers->sustainLevel = eg2SustainLevel;
	voiceModifiers->eg2Modifiers->releaseTime_mSec = eg2ReleaseTime_mSec;

	// --- LPF
	voiceModifiers->filter1Modifiers->fcControl = filter1Fc;
	voiceModifiers->filter1Modifiers->qControl = filter1Q;
	voiceModifiers->filter1Modifiers->gainCompensation = gainComp;
	voiceModifiers->filter1Modifiers->enableKeyTrack = (enableKeyTrack == 1);
	voiceModifiers->filter1Modifiers->keytrackRatio = keytrackRatio;
	voiceModifiers->filter1Modifiers->applyNLP = (applyNLP == 1);

	// --- HPF
	voiceModifiers->filter2Modifiers->fcControl = filter2Fc;
	voiceModifiers->filter2Modifiers->qControl = filter2Q;
	voiceModifiers->filter2Modifiers->gainCompensation = gainComp;
	voiceModifiers->filter2Modifiers->enableKeyTrack = (enableKeyTrack == 1);
	voiceModifiers->filter2Modifiers->keytrackRatio = keytrackRatio;
	voiceModifiers->filter2Modifiers->applyNLP = (applyNLP == 1);

	// --- LP filter type				voiceModifiers->enablePortamento = (enablePortamento == 1);
	synthModifiers->voiceModifiers->enableLPF = (LPFenable == 0);
	if (compareIntToEnum(LPFilterType, LPFilterTypeEnum::LPF4))
		voiceModifiers->filter1Modifiers->filter = filterType::kLPF4;
	else if (compareIntToEnum(LPFilterType, LPFilterTypeEnum::LPF2))
		voiceModifiers->filter1Modifiers->filter = filterType::kLPF2;

	// --- HP filter type
	synthModifiers->voiceModifiers->enableHPF = (HPFenable == 0);
	if (compareIntToEnum(HPFilterType, HPFilterTypeEnum::HPF4))
		voiceModifiers->filter2Modifiers->filter = filterType::kHPF4;
	else if (compareIntToEnum(HPFilterType, HPFilterTypeEnum::HPF2))
		voiceModifiers->filter2Modifiers->filter = filterType::kHPF2;

	// --- EG2 --> Filter fc Mod Intensity (note channel is EG2 Out)
	voiceModifiers->filter1Modifiers->modulationControls[kVALadderFilterFcMod].modulationIntensity = eg2FilterFcIntensity;
	voiceModifiers->filter1Modifiers->modulationControls[kVALadderFilterFcMod].invertIntensity = (invertFilterFc == 1);

	// --- EG1 --> DCA EG amp mod input (note channel is EG1 out)
	// REMOVED: no real need for this control
	// voiceModifiers->outputDCAModifiers->modulationControls[kDCA_AmpMod].modulationIntensity = eg1DCAIntensity;

	// --- inversion flag for EG1 --> DCA EG amp mod input
	voiceModifiers->outputDCAModifiers->modulationControls[kDCA_AmpMod].invertIntensity = (invertDCA_EG == 1);

	// --- final gain
	voiceModifiers->outputDCAModifiers->gain_dB = masterVolume_dB;

	// --- programmable modulation routings
	//
	//    First, clear out the array and set all source/destinations to none
	for (unsigned int i = 0; i < MAX_MOD_ROUTINGS; i++)
	{
		voiceModifiers->modulationRoutings[i].modSource = modulationSource::kNoneDontCare;
		voiceModifiers->modulationRoutings[i].modDest = modulationDestination::kNoneDontCare;
	}

	uint32_t routingIndex = 0; // change this when you add more programmable routings
	// --- take care of the "All Osc" or "All Filter" etc...
	if (compareIntToEnum(modDest1, modulationDestination::kAll_Osc_Pitch))
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc1_Pitch;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc2_Pitch;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
	}
	else if (compareIntToEnum(modDest1, modulationDestination::kAll_Osc_PW)) 
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc1_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc2_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kSubOsc_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
	}
	else
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource1, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = convertEnum(modDest1, modulationDestination);
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting1_Intensity;
	}

	// --- routing 2
	routingIndex++; // important to increment this

	if (compareIntToEnum(modDest2, modulationDestination::kAll_Osc_Pitch))
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc1_Pitch;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc2_Pitch;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
	}
	else if (compareIntToEnum(modDest2, modulationDestination::kAll_Osc_PW))
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc1_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kOsc2_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
		routingIndex++;

		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = modulationDestination::kSubOsc_PW;
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
	}
	else
	{
		voiceModifiers->modulationRoutings[routingIndex].modSource = convertEnum(modSource2, modulationSource);
		voiceModifiers->modulationRoutings[routingIndex].modDest = convertEnum(modDest2, modulationDestination);
		voiceModifiers->progModulationControls[routingIndex].modulationIntensity = modRouting2_Intensity;
	}

	// --- add more here, don't forget to increment the routingIndex...


	// --- setup update
	UpdateInfo updateInfo;

	// --- do update
	synthEngine->update(updateInfo);
}

bool PluginCore::processAudioFrame(ProcessFrameInfo& processFrameInfo)
{
    // --- fire any MIDI events for this sample interval
    processFrameInfo.midiEventQueue->fireMidiEvents(processFrameInfo.currentFrame);

	// --- do per-frame updates; VST automation and parameter smoothing
	doSampleAccurateParameterUpdates();

 	// --- decode the channelIOConfiguration and process accordingly
	//
	// --- Synth Plugin:
	if (getPluginType() == kSynthPlugin)
	{
		// --- check for output
		if (processFrameInfo.channelIOConfig.outputChannelFormat == kCFNone)
			return false;

		// --- array to hold the outputs
		double synthEngineOutputs[kNumEngineOutputs] = { 0.0 };
		
		RenderInfo synthRenderInfo;
		synthRenderInfo.numOutputChannels = kNumEngineOutputs;
		synthRenderInfo.outputData = &synthEngineOutputs[0];
		
		// --- update engine with current plugin core values
		updateEngine();

		// --- render synth
		synthEngine->render(synthRenderInfo);

		// --- decode and output
		if (processFrameInfo.channelIOConfig.outputChannelFormat == kCFMono)
		{
			// --- silence: change this with your signal processing
			processFrameInfo.outputData[0] = synthEngineOutputs[kEngineLeftOutput];

			return true; /// processed
		}

		// --- Stereo-Out
		else if (processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
		{
			// --- silence: change this with your signal processing
			processFrameInfo.outputData[0] = synthEngineOutputs[kEngineLeftOutput];
			processFrameInfo.outputData[1] = synthEngineOutputs[kEngineRightOutput];

			return true; /// processed
		}
	}

	// --- FX Plugin:
	// --- check for input
	if (processFrameInfo.channelIOConfig.inputChannelFormat == kCFNone)
		return false;

	if (processFrameInfo.channelIOConfig.inputChannelFormat == kCFMono &&
		processFrameInfo.channelIOConfig.outputChannelFormat == kCFMono)
	{
		// --- pass through code: change this with your signal processing
		processFrameInfo.outputData[0] = processFrameInfo.inputData[0];

		return true; /// processed
	}

	// --- Mono-In/Stereo-Out
	else if (processFrameInfo.channelIOConfig.inputChannelFormat == kCFMono &&
		processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
	{
		// --- pass through code: change this with your signal processing
		processFrameInfo.outputData[0] = processFrameInfo.inputData[0];
		processFrameInfo.outputData[1] = processFrameInfo.inputData[0];

		return true; /// processed
	}

	// --- Stereo-In/Stereo-Out
	else if (processFrameInfo.channelIOConfig.inputChannelFormat == kCFStereo &&
		processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
	{
		// --- pass through code: change this with your signal processing
		processFrameInfo.outputData[0] = processFrameInfo.inputData[0];
		processFrameInfo.outputData[1] = processFrameInfo.inputData[1];

		return true; /// processed
	}

    return false; /// NOT processed
}

// --- do pre buffer processing
bool PluginCore::preProcessAudioBuffers(ProcessBufferInfo& processInfo)
{
    // --- sync internal variables to GUI parameters; you can also do this manually if you don't
    //     want to use the auto-variable-binding
    syncInBoundVariables();

	// updateEngine();

    return true;
}

// --- do post buffer processing
bool PluginCore::postProcessAudioBuffers(ProcessBufferInfo& processInfo)
{
	// --- update outbound variables; currently this is meter data only, but could be extended
	//     in the future
	updateOutBoundVariables();

    return true;
}

// --- called by host plugin at top of buffer proc
bool PluginCore::updatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo)
{
    // --- use base class helper
    setPIParamValue(controlID, controlValue);

    // --- do any post-processing
    postUpdatePluginParameter(controlID, controlValue, paramInfo);

    return true; /// handled
}

// --- called by host plugin at top of buffer proc, normalized version
bool PluginCore::updatePluginParameterNormalized(int32_t controlID, double normalizedValue, ParameterUpdateInfo& paramInfo)
{
	// --- use base class helper, returns actual value
	double controlValue = PluginBase::setPIParamValueNormalized(controlID, normalizedValue, paramInfo.applyTaper);

	// --- do any post-processing
	postUpdatePluginParameter(controlID, controlValue, paramInfo);

	return true; /// handled
}

// --- this can be called: 1) after bound variable has been updated or 2) after smoothing occurs
bool PluginCore::postUpdatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo)
{
    // --- now do any post update cooking; be careful with VST Sample Accurate automation
    //     If enabled, then make sure the cooking functions are short and efficient otherwise disable it
    //     for the Parameter involved
    /*switch(controlID)
    {
        case 0:
        {
            return true;    /// handled
        }

        default:
            return false;   /// not handled
    }*/

    return false;
}

// --- this is called when a GUI control is changed, has nothing
//     to do with actual variable or updated variable
//     DO NOT update underlying variables here - this is only for sending GUI updates
//            or letting you know that a parameter was changed; it should not change
//            the state of your plugin.
bool PluginCore::guiParameterChanged(int32_t controlID, double actualValue)
{
	/*
	switch (controlID)
	{
		case controlID::<your control here>
		{

			return true; // handled
		}

		default:
			break;
	}*/

	return false; /// not handled
}

bool PluginCore::setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData)
{
	return true;
}

// --- decode the MIDI event and process; this is sample-accurate so it
//     will be called on each sample interval for each MIDI event that occurred
bool PluginCore::processMIDIEvent(midiEvent& event)
{
	synthEngine->processMIDIEvent(event);

    return true;
}

// --- GUI messaging
bool PluginCore::processMessage(MessageInfo& messageInfo)
{
    // --- decode message
    switch(messageInfo.message)
    {
        // --- add customization appearance here
        case PLUGINGUI_DIDOPEN:
        {
            return false;
        }

        // --- NULL pointers so that we don't accidentally use them
        case PLUGINGUI_WILLCLOSE:
        {
            return false;
        }

        // --- update view; this will only be called if the GUI is actually open
        case PLUGINGUI_TIMERPING:
        {
            return false;
        }

        // --- register the custom view, grab the ICustomView interface
        case PLUGINGUI_REGISTER_CUSTOMVIEW:
        {

            return false;
        }

        case PLUGINGUI_REGISTER_SUBCONTROLLER:
        case PLUGINGUI_QUERY_HASUSERCUSTOM:
        case PLUGINGUI_USER_CUSTOMOPEN:
        case PLUGINGUI_USER_CUSTOMCLOSE:
        case PLUGINGUI_EXTERNAL_SET_NORMVALUE:
        case PLUGINGUI_EXTERNAL_SET_ACTUALVALUE:
        {

            return false;
        }

        default:
            break;
    }

    return false; /// not handled
}


// --- The functions below require NO editing on the user's part; they are
//     here for convenience
// --- setup the descriptor
bool PluginCore::initPluginDescriptors()
{
    pluginDescriptor.pluginName = PluginCore::getPluginName();
    pluginDescriptor.shortPluginName = PluginCore::getShortPluginName();
    pluginDescriptor.vendorName = PluginCore::getVendorName();
    pluginDescriptor.pluginTypeCode = PluginCore::getPluginType();

    // --- AAX
    apiSpecificInfo.aaxManufacturerID = kAAXManufacturerID;
    apiSpecificInfo.aaxProductID = kAAXProductID;
    apiSpecificInfo.aaxBundleID = kAAXBundleID;  /* MacOS only: this MUST match the bundle identifier in your info.plist file */
    apiSpecificInfo.aaxEffectID = "aaxDeveloper.";
    apiSpecificInfo.aaxEffectID.append(PluginCore::getPluginName());

    // --- AU
    apiSpecificInfo.auBundleID = kAUBundleID;   /* MacOS only: this MUST match the bundle identifier in your info.plist file */

    // --- VST3
    apiSpecificInfo.vst3FUID = PluginCore::getVSTFUID(); // OLE string format
    apiSpecificInfo.vst3BundleID = kVST3BundleID;/* MacOS only: this MUST match the bundle identifier in your info.plist file */

    // --- AU and AAX
    apiSpecificInfo.fourCharCode = PluginCore::getFourCharCode();

    return true;
}

// --- static functions required for VST3/AU only
const char* PluginCore::getPluginName(){ return kPluginName; }
const char* PluginCore::getShortPluginName(){ return kShortPluginName; }
const char* PluginCore::getVendorName(){ return kVendorName; }
const char* PluginCore::getVendorURL(){ return kVendorURL; }
const char* PluginCore::getVendorEmail(){ return kVendorEmail; }
const char* PluginCore::getAUCocoaViewFactoryName(){ return AU_COCOA_VIEWFACTORY_STRING; }

// --- get type
pluginType PluginCore::getPluginType(){ return kPluginType; }

// --- FUID and VST2 "magic" string
const char* PluginCore::getVSTFUID(){ return kVSTFUID; }
int32_t PluginCore::getFourCharCode(){ return kFourCharCode; }
