#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"
#include "lfo.h"

// --- LIMITS (always at top)
//
// --- modulation limits
const double kDelayFX_Feedback_ModRange = 50.0; // bipolar, +/- 50%
const double kDelayFX_Feedback_Min = 0.0;
const double kDelayFX_Feedback_Max = 100.0;

const double kDelayFX_Mix_ModRange = 50.0; // bipolar, +/- 50%
const double kDelayFX_Mix_Min = 0.0;
const double kDelayFX_Mix_Max = 100.0;

const double kChorusFX_Depth_ModRange = 50.0; // bipolar, +/- 50%
const double kChorusFX_Depth_Min = 0.0;
const double kChorusFX_Depth_Max = 100.0;

const double kMinChorusDelay_mSec = 5.0;
const double kMaxChorusDelay_mSec = 30.0;

// --- outputs[] indexes for this component
enum {
	kDelayFXLeftOutput,
	kDelayFXRightOutput,
	kNumDelayFXOutputs
};

// --- modulator indexes for this component
enum {
	kDelayFX_FeedbackMod,
	kDelayFX_MixMod,
	kChorusFX_DepthMod,
	kNumDelayFXModulators
};

/** normal, crossed feedback or ping-pong delay types */
enum class delayFXMode { norm, cross, pingpong, chorus };

/**
	\struct DelayFXModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the DelayFX component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param delayTime_mSec:			delay time, set by user
	\param feedback_Pct:			delay feedback in %
	\param delayRatio:				delay ratio; for cross and pingpong, this sets the relative lengths of the left and right delay-lines
	\param delayMix_Pct:			delay FX mix ratio in %
	\param chorusRate_Hz:			rate for chorus effect
	\param chorusDepth_Pct:			depth (%) for chorus
	\param enabled:					enable/disable the FX
	\param modControls:				intensity and range controls for each modulator object
*/
struct DelayFXModifiers
{
	DelayFXModifiers() {}

	// --- modifiers
	double delayTime_mSec = 0.0;
	double feedback_Pct = 0.0;
	double delayRatio = 0.0;	// -0.9 to + 0.9
	double delayMix_Pct = 50.0;	// 0 to 1.0
	bool enabled = false;		// 0 to 1.0

	// --- chorus
	double chorusRate_Hz = 0.0;
	double chorusDepth_Pct = 0.0;

	// --- mode of operation
	delayFXMode delayFXMode = delayFXMode::norm;

	// --- modulator controls
	ModulatorControl modulationControls[kNumDelayFXModulators];
};

/**
\class DelayLine
\ingroup SynthClasses
\brief Encapsulates a single delay line of some number of samples in length

\author Will Pirkle
\version Revision : 1.0
\date Date : 2017 / 09 / 24
*/
class DelayLine
{
public:
	// constructor/destructor
	DelayLine(void);
	virtual ~DelayLine(void);  // base class MUST declare virtual destructor

	// functions for the owner plug-in to call
public:
	// --- declare buffer and zero
	void init(int delayLengthInSamples);

	// --- flush buffer, reset pointers to top
	void resetDelay();

	// --- set functions for Parent Plug In
	void setSampleRate(int fs) { sampleRate = fs; };
	void setDelay_mSec(double _delay_mSec);

	// --- function to cook variables
	void cookVariables();

	// --- read the delay without writing or incrementing
	double readDelay();

	// --- read the delay at an arbitrary time
	//     without writing or incrementing (optional)
	double readDelayAt(double _delay_mSec);

	// --- write the input and icrement pointers (optional)
	void writeDelayAndInc(double inputSample);

	// --- process audio -- this is the normal way to use the object
	bool processAudio(double* input, double* output);

protected:
	// member variables
	//
	// --- pointer to our circular buffer
	double* buffer;				///<  pointer to our circular buffer

	// --- delay in mSec, set by Parent object
	double delay_ms;			///<  delay in mSec, set by Parent object

	// --- delay in samples; float supports fractional delay
	double delayInSamples;		///<  delay in samples; float supports fractional delay

	// --- read/write index values for circ buffer
	int readIndex;				///< read index values for circ buffer
	int writeIndex;				///< write index values for circ buffer

	// --- max length of buffer	
	int bufferLength;			///< max length of buffer	

	// --- sample rate 
	int sampleRate;				///< sample rate 
};

/**
	\class DelayFX
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
	Use DelayFXModifiers structure

	\param delayTime_mSec:			delay time, set by user
	\param feedback_Pct:			delay feedback in %
	\param delayRatio:				delay ratio; for cross and pingpong, this sets the relative lengths of the left and right delay-lines
	\param delayMix_Pct:			delay FX mix ratio in %
	\param chorusRate_Hz:			rate for chorus effect
	\param chorusDepth_Pct:			depth (%) for chorus
	\param enabled:					enable/disable the FX
	\param modControls:				intensity and range controls for each modulator object

	Modulator indexes:
	- kDelayFX_FeedbackMod:			[ 0, +1] delay feedback modulation (!)

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class DelayFX : public ISynthAudioProcessor
{
public:
	DelayFX(std::shared_ptr<DelayFXModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~DelayFX();

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
	std::shared_ptr<DelayFXModifiers> getModifiers() { return modifiers; }

	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(DelayFXModifiers* _modifiers) { modifiers.reset(_modifiers); }

	/** do the delay FX */
	void processDelayFX(RenderInfo& renderInfo);

protected:
	// --- our delay lines
	DelayLine leftDelay;
	DelayLine rightDelay;

	// --- for modulated delay effects
	LFO* lfo = nullptr;
	std::shared_ptr<LFOModifiers> lfoModifiers = std::make_shared<LFOModifiers>();

	double delayTime_mSec = 0.0;
	double feedback_Pct = 0.0;
	double delayRatio = 0.0;		// -0.9 to + 0.9
	double delayMix_Pct = 0.5;		// 0 to 1.0
	delayFXMode delayFXMode = delayFXMode::norm;
	bool enabled = false;
	double chorusRate_Hz = 0.0;
	double chorusDepth_Pct = 0.0;

	// --- note on flag
	bool noteOn = false;

	// --- our modifiers
	std::shared_ptr<DelayFXModifiers> modifiers;
};

