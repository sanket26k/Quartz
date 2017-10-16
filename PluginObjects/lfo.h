#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"

// --- LIMITS (always at top)
//
// --- frequency
const double kMinLFO_fo = 0.02;
const double kMaxLFO_fo = 20.0;	// 0.02Hz  - 20Hz is the Korg default; some use 0.03Hz - 30Hz -- of course you can customize however you like

// --- amplitude
const double kMinLFO_amp = 0.0;
const double kMaxLFO_amp = 1.0;

// --- pulse width
const double kMinLFO_PW = 0.05;
const double kMaxLFO_PW = +0.95;

// --- modulation ranges
const double kLFO_Freq_ModRange = (kMaxLFO_fo - kMinLFO_fo) / 2.0;		// --> +/- 50%
const double kLFO_PW_ModRange = 0.5;									// --> +/- 50%
const double kLFO_Amp_ModRange = 1.0;									// --> 100%

// --- other constants
const double quadPhaseInc = 0.25;	// +90 deg

// --- outputs[] indexes for this component
enum {  
	kLFONormalOutput,
	kLFONormalOutputInverted, 
	kLFOQuadPhaseOutput, 
	kLFOQuadPhaseOutputInverted,
	kLFOUnipolarOutputFromMax,	/* special output for doing Amplitude Modulation (AM) */
	kLFOUnipolarUpRamp,			/* special output of the modulo counter */
	kLFOUnipolarDownRamp,		/* special output of the inverse modulo counter */
	kNumLFOOutputs };

// --- modulator indexes for this component
enum {	
	kLFOFreqMod,
	kLFOMaxDownAmpMod,
	kLFOPulseWidthMod,
	kNumLFOModulators };

// --- strongly typed enum for trivial oscilator type & mode
enum class LFOWaveform { kSin, kUpSaw, kDownSaw, kSquare, kTriangle, kRSH, kQRSH, kExpUp, kExpDown, kWhiteNoise};
enum class LFOMode { kSync, kFreeRun, kOneShot };

/**
	\struct LFOModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the LFO component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param oscFreqControl:			frequency (fo) in Hz of the oscillator, usually from GUI control
	\param oscAmpControl:			amplitude ("Depth") control
	\param pulseWidthControl_Pct:	[0.0, 100.0] pulse width in percent for square waves only
	\param oscWave:					oscillator waveform (see enum class LFOWaveform)
	\param oscMode:					LFO mode: synchronized with note-on, free running, one-shot
	\param modControls:				intensity and range controls for each modulator object
*/
struct LFOModifiers
{
	LFOModifiers() {}

	// --- modifiers
	double oscFreqControl = 0.0;			// --- external osc freqency control
	double oscAmpControl = 1.0;				// --- external osc "depth" control (not in dB); default = 1.0 in case nothing connected
	double pulseWidthControl_Pct = 50.0;	// --- pulse width as a percent, usually from user control

	// --- strongly typed enum for trivial oscilator type
	LFOWaveform oscWave = LFOWaveform::kTriangle; // note default

	// --- oscillator mode; default is sync, which is most common
	LFOMode oscMode = LFOMode::kSync;

	// --- modulator controls
	ModulatorControl modulationControls[kNumLFOModulators];

};

/**
	\class LFO
	\ingroup SynthClasses
	\brief Encapsulates a LFO (trivial oscillator)

	Outputs: contains 7 outputs
	- Normal
	- Inverted
	- +90 deg quad phase
	- -90 deg quad phase
	- unipolar output from max, for use with max-down modulators (see DCA for an example)
	- unipolar up-ramp - the modulo counter timebase
	- unipolar down-ramp - the inverted modulo counter, used with the Glide LFO for portamento

	Control I/F:
	Use LFOModifiers structure

	\param oscFreqControl:			frequency (fo) in Hz of the oscillator, usually from GUI control 
	\param oscAmpControl:			amplitude ("Depth") control
	\param pulseWidthControl_Pct:	[0.0, 100.0] pulse width in percent for square waves only
	\param oscWave:					oscillator waveform (see enum class LFOWaveform)
	\param oscMode:					LFO mode: synchronized with note-on, free running, one-shot
	\param modControls:				intensity and range controls for each modulator object

	Modulator indexes:
	- kLFOFreqMod:				[-1, +1] frequency modulation -- modulation is linear in frequency
	- kLFOMaxDownAmpMod:		[ 0, +1] gain modulation -- modulation is from the max gain downwards (tremolo or AM effect)
	- kLFOPulseWidthMod:		[-1, +1] PW modulation for square waves only

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class LFO : public ISynthComponent
{
public:
	LFO(std::shared_ptr<LFOModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~LFO();

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

	/** modifier getter */
	std::shared_ptr<LFOModifiers> getModifiers() { return modifiers; }
	
	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(LFOModifiers* _modifiers) { modifiers.reset(_modifiers); }

	/** do the oscillator operation; may be called externally */
	bool doOscillate();

protected:
	/** true if this waveform requires a phse-offset to maintain correct phase alignment with other waveforms */
	inline bool needsPhaseAdvance(LFOWaveform oscWave)
	{
		if (oscWave == LFOWaveform::kUpSaw || oscWave == LFOWaveform::kDownSaw || oscWave == LFOWaveform::kTriangle)
			return true;
	}
	
	// --- our waveform/mode
	LFOWaveform oscWave = LFOWaveform::kTriangle;
	LFOMode oscMode = LFOMode::kSync;;

	// --- sample rate
	double sampleRate = 0.0;			///< sample rate

	// --- LFOs will modulate linearly, NOT via volt/octave scaling
	double oscillatorModulationRange = ( kMaxLFO_fo - kMinLFO_fo ) / 2.0;	///< LFOs will modulate linearly, NOT via volt/octave scaling

	// --- the FINAL oscillator frequency
	double oscFrequency = 0.0;			///< the FINAL oscillator frequency

	// --- the FINAL PW for square wave only
	double pulseWidth = 0.0;			///< the FINAL PW for square wave only

	// --- the FINAL output amplitude
	double oscAmplitude = 0.0;			///< the FINAL output amplitude

	// --- 32-bit register for RS&H
	uint32_t pnRegister = 0;			///< 32 bit register for PN oscillator
	int randomSHCounter = -1;			///< random sample/hold counter;  -1 is reset condition
	double randomSHValue = 0.0;			///< current output, needed because we hold this output for some number of samples = (sampleRate / oscFrequency)

	// --- timebase variables
	double modCounter = 0.0;			///< modulo counter [0.0, +1.0]
	double phaseInc = 0.0;				///< phase inc = fo/fs
	double modCounterQP = 0.0;			///<Quad Phase modulo counter [0.0, +1.0]

	// --- RUN/STOP flag
	bool noteOn = false;

	// --- our modifiers
	std::shared_ptr<LFOModifiers> modifiers = nullptr;
};

