#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"

// --- LIMITS (always at top)
//
// --- outputs[] indexes for this component
enum {	kEGNormalOutput,
		kEGBiasedOutput, 
		kNumEGOutputs }; // normal, biased, number of outputs (last)

// --- modulatior indexes for this component
enum {
	kEGRepeatTimeMod,
	kEGRepeatTimeSDMod,
	kNumEGModulators };

// --- strongly typed enum for trivial oscillator type & mode
enum class egTCMode { kAnalog, kDigital };
enum class egState { kOff, kDelay, kAttack, kDecay, kSustain, kRelease, kShutdown, kShutdownForRepeat };
enum class egSubDiv { kOff, kWhole, kDottedHalf, kHalf, kDottedQuarter, kQuarter, kDottedEigth, kTripletQuarter, kEigth, kTripletEigth, kSixteenth};

/**
	\struct EGModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the EG component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param egMode:						analog (based on the CEM3310 IC http://curtiselectromusic.com/uploads/CEM_3310_Long.pdf) or digital (based on linear-in-dB curve)
	\param resetToZero:					enable/disable reset to zero mode
	\param legatoMode:					enable/disable legato mode
	\param velocityToAttackScaling:		enable/disable MIDI velocity to attack time scaling; as velocity increases, attack time decreases
	\param noteNumberToDecayScaling:	enable/disable MIDI note number to decay time scaling; as note number increases, decay time decreases
	\param attackTime_mSec:				attack time in mSec
	\param decayTime_mSec:				decay time in mSec (see synth book or DLS Level 1 spec for more information)
	\param releaseTime_mSec:			release time in mSec (see synth book or DLS Level 1 spec for more information)
	\param sustainLevel:				sustain level as a raw value (not dB)
*/
struct EGModifiers
{
	EGModifiers() {}

	egTCMode egMode = egTCMode::kAnalog;
	bool resetToZero = false;
	bool legatoMode = false;
	bool velocityToAttackScaling = false;
	bool noteNumberToDecayScaling = false;

	// tempo info for repeat
	bool subdivide = false;
	double bpm = 120.0;
	uint32_t sigDenominator = 4;
	egSubDiv repeatSubDiv = egSubDiv::kWhole; 

	//--- ADSR times from user
	double repeatTime_mSec = 1000.0;
	double delayTime_mSec = 1000.0;
	double attackTime_mSec = 1000.0;	// att: is a time duration
	double decayTime_mSec = 1000.0;		// dcy: is a time to decay 1->0
	double releaseTime_mSec = 1000.0;	// rel: is a time to decay 1->0
	double sustainLevel = 1.0;

	// --- modulator controls (none yet)
	ModulatorControl modulationControls[kNumEGModulators];
};


/**
	\class EnvelopeGenerator
	\ingroup SynthClasses
	\brief Encapsulates a virtual analog ADSR envelope generator with adjustable segment curvature

	Outputs: contains 2 outputs
	- Normal Output
	- Biased Output (for using the EG to control pitch -- this way the note will hit its MIDI pitch during the sustain segment

	Control I/F:
	Use EGModifiers structure

	\param egMode:						analog (based on the CEM3310 IC http://curtiselectromusic.com/uploads/CEM_3310_Long.pdf) or digital (based on linear-in-dB curve)
	\param resetToZero:					enable/disable reset to zero mode
	\param legatoMode:					enable/disable legato mode 
	\param velocityToAttackScaling:		enable/disable MIDI velocity to attack time scaling; as velocity increases, attack time decreases
	\param noteNumberToDecayScaling:	enable/disable MIDI note number to decay time scaling; as note number increases, decay time decreases
	\param attackTime_mSec:				attack time in mSec
	\param decayTime_mSec:				decay time in mSec (see synth book or DLS Level 1 spec for more information)
	\param releaseTime_mSec:			release time in mSec (see synth book or DLS Level 1 spec for more information)
	\param sustainLevel:				sustain level as a raw value (not dB)

	Modulator indexes:
	- this component has no modulators

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class EnvelopeGenerator : public ISynthComponent
{
public:
	// --- NOTE: the only constructor requires modifiers and midi arrays when being shared for global parameters
	EnvelopeGenerator(std::shared_ptr<EGModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~EnvelopeGenerator();

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

	// --- specific to the EG component
	virtual bool shutDownComponent();

	// --- modifier getter
	std::shared_ptr<EGModifiers> getModifiers() { return modifiers; }
	
	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(EGModifiers* _modifiers) { modifiers.reset(_modifiers); }

	// --- accessors - allow owner to get our state
	egState getState() { return state; }			///< returns current state of the EG finite state machine

	// --- output EG identifer access
	bool isOutputEG() { return outputEG; }			///< returns true if this EG is connected to the output DCA
	void setIsOutputEG(bool b) { outputEG = b; }	///< set/clear the output DCA flag

protected:
	// --- calculate time params
	void calculateRepeatTime(double repeatTime);
	void calculateRepeatTimeFromSubDiv(egSubDiv subDiv);
	void calculateDelayTime(double delayTime);
	void calculateAttackTime(double attackTime, double attackTimeScalar = 1.0);
	void calculateDecayTime(double decayTime, double decayTimeScalar = 1.0);
	void calculateReleaseTime(double releaseTime, double releaseTimeScalar = 1.0);
	
	// --- generate it
	bool doRepeat();
	bool doEnvelopeGenerator();

	// --- function to set the time constants
	void setEGMode(egTCMode mode);
		
	/// set the sustain pedal override to keep the EG stuck in the sustain state until the pedal is released
	void setSustainOverride(bool b)
	{
		sustainOverride = b;

		if (releasePending && !sustainOverride)
		{
			releasePending = false;
			doNoteOff(0.0, 0, 0);
		}
	}

	// --- sample rate for time calculations
	double sampleRate = 0.0;			///< sample rate

	// --- the current output of the EG
	double envelopeOutput = 0.0;		///< the current envelope output sample

	/** true if this EG is connected to the output DCA, or more generally
	//     if the note-off state of the voice depends on this EG expiring */
	bool outputEG = false;

	//--- ADSR times from user
	double repeatTime_mSec = 1000.0;
	double delayTime_mSec = 1000.0;
	double attackTime_mSec = 1000.0;	///< att: is a time duration
	double decayTime_mSec = 1000.0;		///< dcy: is a time to decay from max output to 0.0
	double releaseTime_mSec = 1000.0;	///< rel: is a time to decay from max output to 0.0

	//--- Coefficient, offset and TCO values
	//    for each state
	double attackCoeff = 0.0;
	double attackOffset = 0.0;
	double attackTCO = 0.0;

	double decayCoeff = 0.0;
	double decayOffset = 0.0;
	double decayTCO = 0.0;

	double releaseCoeff = 0.0;
	double releaseOffset = 0.0;
	double releaseTCO = 0.0;

	// --- this is set internally; user normally not allowed to adjust
	double shutdownTime_mSec = 5.0;	///< short shutdown time when stealing a voice

	// --- sustain is a level, not a time
	double sustainLevel = 1.0;			///< the sustain level as a raw value (not dB)

	// --- for sustain pedal input; these two bools keep track of the override, 
	//     and eventual release when the sus pedal is released after user released key
	bool sustainOverride = false;		///< if true, places the EG into sustain mode
	bool releasePending = false;		///< a flag set when a note off event occurs while the sustain pedal is held, telling the EG to go to the release state once the pedal is released

	// --- inc value for shutdown
	double incShutdown = 0.0;			///< shutdown linear incrementer

	// --- stage variable
	egState state = egState::kOff;		///< EG state variable

	// --- analog/digital mode
	egTCMode egMode = egTCMode::kAnalog; ///< analog or digital (linear in dB)

	// --- RUN/STOP flag
	bool noteOn = false;

	// --- Timer
	Timer egTimer = Timer();
	Timer reTimer = Timer();

	// tempo information for repeat
	bool subdivide = false;
	double bpm = 120.0;
	uint32_t sigDenominator = 4;
	egSubDiv repeatSubDiv = egSubDiv::kWhole;

	// --- our modifiers
	std::shared_ptr<EGModifiers> modifiers = nullptr;

	// Modulator Range
	double maxRepeatTime = 5000.0;
	double minRepeatTime = 0.0;
	double maxRepeatTimeSD = 6000.0 / bpm * 16.0;
	double minRepeatTimeSD = 6000.0 / bpm * 0.0625;
	double repeatTimeModRange = ( maxRepeatTime - minRepeatTime ) / 4.0;	// +/- 25% bipolar range
	double repeatTimeSDModRange = ( maxRepeatTimeSD - minRepeatTimeSD ) / 2.0;	// +/- 50% bipolar range
};

