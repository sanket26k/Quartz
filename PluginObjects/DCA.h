#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"

// --- LIMITS (always at top) 
//
// --- modulation limits
const double kMinModGain_dB = -60.0;
const double kMaxModGain_dB =   0.0;

// --- absolute limits, note there is no max limit here (volume control can go to 11 if needed)
const double kMinAbsoluteGain_dB = -96.0;
const double kDCA_Amp_ModRange = 1.0;			// --> unipolar, 100%
const double kDCA_Pan_ModRange = 1.0;			// --> unipolar, 100%

// --- outputs[] indexes for this component
enum {
	kDCALeftOutput,
	kDCARightOutput,
	kNumDCAOutputs
};

// --- modulator indexes for this component
enum {	
	kDCA_AmpMod,
	kDCA_MaxDownAmpMod,
	kDCA_PanMod,
	kNumDCAModulators
};

/**
	\struct DCAModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the DCA component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param gain_dB:					gain in dB control, may also be connected to a shared (across voices) Master Volume control
	\param mute:					mute on/off
	\param modControls:				intensity and range controls for each modulator object
*/
struct DCAModifiers
{
	DCAModifiers() {}

	// --- modifiers
	double gain_dB = 0.0;
	bool mute = false;

	// --- modulator controls
	ModulatorControl modulationControls[kNumDCAModulators];
};

/**
	\class DCA
	\ingroup SynthClasses
	\brief Encapsulates a digitally controlled amplifier

	I/O: supports the following channel combinations:
	- mono -> mono
	- mono -> stereo
	- stereo -> stereo

	Outputs: contains 2 outputs
	- Left Channel
	- Right Channel

	Control I/F:
	Use DCAModifiers structure

	\param gain_dB:					gain in dB control, may also be connected to a shared (across voices) Master Volume control
	\param mute:					mute on/off
	\param modControls:				intensity and range controls for each modulator object

	Modulator indexes:
	- kDCA_AmpMod:		[ 0, +1] gain modulation, usually from an EG 
	- kDCA_MaxDownAmpMod:		[-1, +1] gain modulation -- modulation is from the max gain downwards (tremolo or AM effect)
	- kDCA_PanMod:				[-1, +1] pan modulation --- uses constant power curves simulated with sin/cos quadrants

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class DCA : public ISynthAudioProcessor
{
public:
	DCA(std::shared_ptr<DCAModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~DCA();

	// --- ISynthComponent
	virtual bool initializeComponent(InitializeInfo& info);
	virtual bool startComponent();
	virtual bool stopComponent();
	virtual bool resetComponent();
	virtual bool validateComponent();
	virtual bool isComponentRunning() { return noteOn; }
	ModulatorControl* getModulatorControls(uint32_t modulatorIndex) { return &modifiers->modulationControls[modulatorIndex]; }

	// --- note event handlers
	virtual bool doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity);
	virtual bool doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity);

	// --- update and render methods
	virtual bool updateComponent();
	virtual bool renderComponent(bool update);

	// --- IAudioProcessor
	virtual bool processAudio(RenderInfo& renderInfo);

	// --- modifier getter
	std::shared_ptr<DCAModifiers> getModifiers() { return modifiers; }
	
	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(DCAModifiers* _modifiers) { modifiers.reset(_modifiers); }

	/// setter for pan value
	void setPanValue(double _panValue) { panValue = _panValue; }

protected:
	double gainRaw = 1.0;			///< the final raw gain value
	double panLeftGain = 0.707;		///< left channel gain
	double panRightGain = 0.707;	///< right channel gain
	double midiVelocityGain = 0.0;	

	// --- pan value is set internally by voice, or via MIDI/MIDI Channel
	double panValue = 0.0;			///< pan value is set internally by voice, or via MIDI/MIDI Channel

	// --- note on flag
	bool noteOn = false;

	// --- our modifiers
	std::shared_ptr<DCAModifiers> modifiers;
};

