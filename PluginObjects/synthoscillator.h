#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"

// --- LIMITS (always at top)
//
// --- frequency
const double kMinSynthOsc_fo = -20480.0; // --- yes we make negative frequenciea
const double kMaxSynthOsc_fo = 20480.0;	 // --- 10 octaves of sound

// --- amplitude
const double kMinSynthOsc_amp = 0.0;
const double kMaxSynthOsc_amp = 1.0;

// --- pulse width
const double kMinSynthOsc_PW = 0.05;
const double kMaxSynthOsc_PW = +0.95;

// --- modulation ranges
const double kSynthOsc_PW_ModRange = 0.5;			// --> +/- 50%
const double kSynthOsc_Amp_ModRange = 1.0;			// --> unipolar, 100%
const double kSynthOsc_FM_PM_ModIndex = 4.0;		// --> FM Index of Modulation; need to defaut to 4 to match Chowning DX IMAX = 4
const double kSynthOsc_Pitch_ModRange = 12.0;		// --> +/- 12 semitones, could also let user modify this, but I don't see that on HW synths

// --- other constants
const unsigned int kNumWaveTables = 9;
const unsigned int kWaveTableLength = 512;
const unsigned int kNumOscAudioOutputs = 2;

// --- outputs[] indexes for this component
enum {
	kLeftOscOutputWithAmpGain,	/* output with user amp-gain applied */
	kRightOscOutputWithAmpGain,	/* output with user amp-gain applied */
	kLeftOscOutput,				/* for using osc as modulation source */
	kRightOscOutput,			/* for using osc as modulation source */
	kOscFModOutput,				/* for using osc specifically as a pitched Frequency Modulation (FM) source */
	kOscUnipolarOutputFromMax,	/* special output for doing Amplitude Modulation (AM) */
	kOscResetTrigger,			/* special output: 1.0 when oscillator retriggers */
	kNumSynthOscOutputs
};

// --- modulator indexes for this component
enum {  
	kSynthOscPitchMod, 
	kSynthOscToOscPitchMod,		 /* special priority modulator*/
	kSynthOscLinFreqMod, 
	kSynthOscLinPhaseMod, 
	kSynthOscPulseWidthMod,
	kSynthToOscOscPulseWidthMod, /* special priority modulator*/
	kSynthOscAmpMod,
	kSynthOscMaxDownAmpMod,
	kSynthOscPortamentoMod,
	kNumSynthOscModulators };

// --- strongly typed enum for trivial oscillator type & mode
enum class synthOscWaveform { kSaw, kSquare, kTriangle, kSin, kWhiteNoise};

/**
	\struct SynthOscModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the SynthOscillator (aka pitched oscillator) component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param octave:						octave offset from true MIDI pitch
	\param semitones:					semitones offset from true MIDI pitch
	\param cents:						cents offset from true MIDI pitch
	\param oscFreqControl:				frequency control -- generally not used since pitch is set with MIDI note number
	\param oscFreqRatio:				frequency ratio -- generally not used, but allows all oscillators to be retuned via a ratio
	\param masterTuningRatio:			master tuning control using a ratio
	\param masterTuningOffset_cents:	master tuning control using a pitch offset in cents
	\param unisonDetune_cents:			master unison detuning amount in cents
	\param oscWave:						waveform for this oscillator; see enum class synthOscWaveform
	\param pulseWidthControl_Pct:		pulse width in percent [2%, 98%]
	\param oscAmpControl_dB:			user controlled output control in dB
	\param useOscFreqControl:			a flag that notifies the object to take its frequency value from the oscFreqControl rather than MIDI pitch
	\param modControls:					intensity and range controls for each modulator object
*/
struct SynthOscModifiers
{
	SynthOscModifiers() {}
	
	// --- CONTROL INPUTS: these individual vaiables are changed via the User Interface
	int octave = 0.0;					// --- external osc octave control
	int semitones = 0.0;				// --- external osc semitones control
	double cents = 0.0;					// --- external osc cents control (note: double value)
	double oscFreqControl = 440.0;		// --- for controlling oscillator manually rather than with MIDI pitch
	double oscFreqRatio = 1.0;			// --- for FM where modulatorFc = carrierFc*FMRatio; or exotic tuning ratios, or a global tuning offset
	double masterTuningRatio = 1.0;			// --- for a global master tune value as a ratio
	double masterTuningOffset_cents = 0.0;	// --- for a global master tune value in cents
	double unisonDetune_cents = 0.0; // --- unison detune (master value)

	// --- strongly typed enum for trivial oscilator type
	synthOscWaveform oscWave = synthOscWaveform::kSaw; // note default

	// --- pulse width
	double pulseWidthControl_Pct = 50.0;	// --- pulse width as a percent, usually from user control [1%, 99%]

	// --- amplitude
	double oscAmpControl_dB = 0.0;			// --- external osc output control in dB (may or may not be used)

	// --- flag to use manual control
	bool useOscFreqControl = false;

	// --- modulator controls
	ModulatorControl modulationControls[kNumSynthOscModulators];

};

/**
	\class SynthOscillator
	\ingroup SynthClasses
	\brief Encapsulates a synth (pitched) oscillator
	- uses Virtual Analog for Sawtooth waveform
	- uses wavetable sum-of-saws for PWM square wave
	- uses wavetable for triangle and sin waveforms

	Outputs: contains 7 outputs
	- Left Output with user-controlled gain (in dB) applied
	- Right Output with user-controlled gain (in dB) applied
	- Pure Left Output, no gain applied (for use as a modulation source)
	- Pure Right Output, no gain applied (for use as a modulation source)
	- unipolar output from max, for use with max-down modulators (see DCA for an example)
	- unipolar up-ramp - the modulo counter timebase
	- unipolar down-ramp - the inverted modulo counter, used with the Glide LFO for portamento

	Control I/F:
	Use SynthOscModifiers structure

	\param octave:						octave offset from true MIDI pitch
	\param semitones:					semitones offset from true MIDI pitch
	\param cents:						cents offset from true MIDI pitch
	\param oscFreqControl:				frequency control -- generally not used since pitch is set with MIDI note number
	\param oscFreqRatio:				frequency ratio -- generally not used, but allows all oscillators to be retuned via a ratio
	\param masterTuningRatio:			master tuning control using a ratio
	\param masterTuningOffset_cents:	master tuning control using a pitch offset in cents
	\param oscWave:						waveform for this oscillator; see enum class synthOscWaveform
	\param pulseWidthControl_Pct:		pulse width in percent [2%, 98%]
	\param oscAmpControl_dB:			user controlled output control in dB
	\param useOscFreqControl:			a flag that notifies the object to take its frequency value from the oscFreqControl rather than MIDI pitch
	\param modControls:					intensity and range controls for each modulator object

	Modulator indexes:
	- kSynthOscPitchMod:		[-1, +1] pitch modulation in semitones
	- kSynthOscLinFreqMod:		[-1, +1] linear frequency modulation (for FM)
	- kSynthOscLinPhaseMod:		[-1, +1] linear phase modulation (for PM)
	- kSynthOscPulseWidthMod:	[-1, +1] pulse width modulation for square waves only
	- kSynthOscAmpMod:	[ 0, +1] gain modulation, usually from an EG
	- kSynthOscMaxDownAmpMod:	[ 0, +1] gain modulation -- modulation is from the max gain downwards (tremolo or AM effect)
	- kSynthOscPortamentoMod:	[-1, +1] for glide (portamento) effect

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class SynthOscillator : public ISynthComponent
{
public:
	// --- NOTE: the only constructor requires modifiers and midi arrays when being shared for global parameters
	SynthOscillator(std::shared_ptr<SynthOscModifiers>, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~SynthOscillator();
	
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

	// --- modifier getter
	std::shared_ptr<SynthOscModifiers> getModifiers() { return modifiers; }
	
	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(SynthOscModifiers* _modifiers) { modifiers.reset(_modifiers); }

	// --- waveform getter
	synthOscWaveform getWaveform() { return oscWave; }

	// --- access to our frequency & MIDI pitch
	double getOscFrequency() { return oscFrequency; }
	double getMIDIPitchFrequency() { return midiNotePitch; }

	// --- set our unison detune
//	void setUnisonDetune(double _unisonDetune_Cents) { unisonDetune_Cents = _unisonDetune_Cents; }
	void setUnisonDetuneIntensity(double _unisonDetuneIntensity) { unisonDetuneIntensity = _unisonDetuneIntensity; }
	
protected:
	// --- do the oscillator operation; may be called externally
	bool doOscillate();
	
	// --- waveform render functions
	double doSawtooth();
	double doSquareWave();
	double doTriangleWave();
	double doSineWave();

	/** saw requires phase advance due to VA nature; other VA oscillators can be added here if needed */
	bool needsPhaseAdvance(synthOscWaveform oscWave)
	{
		if (oscWave == synthOscWaveform::kSaw)
			return true;
	}

	/** calculate the current phase inc */
	inline void calculatePhaseInc()
	{
		// --- calcualte phase inc = fo/fs
		phaseInc = oscFrequency / sampleRate;

		// --- for wavetables, inc = (kWaveTableLength)*(fo/fs) = kWaveTableLength*phaseInc
		if (oscWave == synthOscWaveform::kSquare || oscWave == synthOscWaveform::kTriangle || oscWave == synthOscWaveform::kSin)
			phaseInc = kWaveTableLength*phaseInc;
	}

	// --- for wavetables
	void createWaveTables();
	void destroyWaveTables();

	// --- get index of multi-table to use based on note pitch frequency
	int getTableIndex();

	// --- set the currentTable pointer based on the type of oscillator we are implementing
	void selectTable();

	// --- do the selected wavetable
	double doSelectedWaveTable(double& readIndex, double phaseInc);
	
	// --- correction factor table sum-of-sawtooth  (empirical)
	double squareCorrFactor[kNumWaveTables] = { 0.5 , 0.5 , 0.5, 0.49 , 0.48 , 0.468 ,0.43 ,0.43, 0.25 }; ///< emperical correction factors

	// --- the current osc waveform
	synthOscWaveform oscWave = synthOscWaveform::kSaw; ///< the current osc waveform

	// --- sample rate
	double sampleRate = 0.0;				///< sample rate

	// --- the midi pitch, will need to save for portamento
	double midiNotePitch = 0.0;				///<the midi pitch, will need to save for portamento

	// --- the oscillator frequency with all modulations applied *EXCEPT* priority modulators (Frequency Modulation (FM), etc...)
	double oscFrequencyNoPriorityMod = 0.0;				///<the oscillator frequency with all modulations applied *EXCEPT* Frequency Modulation (FM)

	// --- the oscillator frequency with all modulations applied *EXCEPT* Frequency Modulation (FM)
	double oscFrequency = 0.0;				///<the oscillator frequency with all modulations applied *EXCEPT* Frequency Modulation (FM)

	// --- unison Detune, varies with each voice and not shared!
	double unisonDetuneIntensity = 0.0;		///< unison detuning intensity scalar [0, +1]

	// --- the FINAL PW for square wave only
	double pulseWidth = 0.0;				///< the FINAL PW for square wave only, after modulations are applied

	// --- the volume control gain
	double oscVolumeControlGain = 0.0;		///< volume control variable for GUI linked variable, applied after the oscillator is rendered

	// --- the FINAL output amplitude
	double oscAmplitude = 0.0;				///< the final amplitude after all modulations have been applied

	// --- the current Phase Modulation value; need to be stored because it is used differently in different oscillator algorithms
	double phaseModuator = 0.0;

	// --- timebase variables	
	double modCounter = 0.0;				///<  modulo counter 0 to 1.0
	double phaseInc = 0.0;					///<  phase inc = fo/fs

	// --- WaveRable oscillator variables
	double waveTableReadIndex = 0.0;		///< wavetable read location

	// --- the tables
	double sineTable[kWaveTableLength];		///< the single sine table

	// --- multi-tables; 9 of them for 9 octaves each starting on an A pitch
	double* sawTables[kNumWaveTables];		///< saw multi-tables; 9 of them for 9 octaves each starting on an A pitch
	double* triangleTables[kNumWaveTables]; ///< triangle multi-tables; 9 of them for 9 octaves each starting on an A pitch

	// --- for storing current table
	double* currentTable = nullptr;			///< the currently select4ed table

	// --- table index for the 9 octaves of tables
	int currentTableIndex = 0;				///< 0 - 9, or -1 if use sine table

	// --- flag indicating state (running or not)
	bool noteOn = false;

	// --- flag indicating that this oscillator is starting a new oscillation cycle
	bool resetTrigger = false;

	// --- our modifiers
	std::shared_ptr<SynthOscModifiers> modifiers = nullptr;
};

