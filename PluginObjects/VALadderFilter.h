#pragma once

#include "VA1Filter.h"

// --- LIMITS (always at top)
//

// --- filter enums
enum {
	kFilter1, 
	kFilter2,
	kFilter3,
	kFilter4,
	kFilter5, // APF for half-ladder filter
	kNumMoogSubFilters}; // --- NUM_MOOG_SUBFILTERS = ALWAYS LAST = 4 here, used for counters, arrays

// --- modulatior indexes for this component
enum {
	kVALadderFilterFcMod,
	kVALadderFilterQMod,
	kVALadderFilterOscToFcMod, /* oscillator -> filter fc (priority modulator) */
	kNumVALadderFilterModulators };

// --- outputs[] indexes for this component
enum {
	kVA1LadderFilterOutput_0,
	kVA1LadderFilterOutput_1,
	kNumVALadderFilterOutputs
};

/**
	\struct VALadderFilterModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the filter components, including both the VA1Filter and VALadderFilter objects. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param fcControl:				fc control from GUI
	\param qControl:				Q control from GUI - note all Q controls on GUIs range from [1, 10] and are mapped to individual filter paramaters
	\param applyNLP:				apply non-linear processing (aka NLP) (ladder filter only)
	\param nlpSaturation:			NLP saturation
	\param gainCompensation:		gain compensation for Moog ladder filter only
	\param filter:					filte type -- see enum class filterType
	\param enableKeyTrack:			enable/disable keytracking
	\param keytrackRatio:			pitch multiplier for MIDI key pitch
	\param modControls:				intensity and range controls for each modulator object
*/
struct VALadderFilterModifiers
{
	VALadderFilterModifiers() {}

	// --- modifiers
	double fcControl = 0.0;

	// --- for higher order filters
	double qControl = 1.0;

	// --- specialized ladder stuff
	//
	// --- nonlinear saturation
	bool applyNLP = false;
	double nlpSaturation = 1.0; // --- higher saturation = more oscillation

	// --- LF Gain Boosting (due to ladder cut of LF)
	double gainCompensation = 0.0;

	// --- strongly typed enum for filter type
	filterType filter = filterType::kLPF4; // note defaut is 4th order Moog

	// --- filter keytracking
	bool enableKeyTrack = false;
	double keytrackRatio = 1.0;

	// --- modulator controls
	ModulatorControl modulationControls[kNumVALadderFilterModulators];
};
/**
	\class VALadderFilter
	\ingroup SynthClasses
	\brief Encapsulates a MONO or STEREO Moog Ladder Filter -- For stereo operation, the two filters share the same fc setting;
	if you need individual left/right filters with separate fc values, use two of these in mono mode each and control independently

	I/O: supports the following channel combinations:
	- mono -> mono
	- mono -> stereo
	- stereo -> stereo

	Outputs: contains 2 outputs
	- Left Channel
	- Right Channel

	Control I/F:
	Use FilterModifiers structure

	\param fcControl:				fc control from GUI
	\param qControl:				Q control from GUI - note all Q controls on GUIs range from [1, 10] and are mapped to individual filter paramaters
	\param applyNLP:				apply non-linear processing (aka NLP) (ladder filter only)
	\param nlpSaturation:			NLP saturation
	\param gainCompensation:		gain compensation for Moog ladder filter only
	\param filter:					filte type -- see enum class filterType
	\param enableKeyTrack:			enable/disable keytracking
	\param keytrackRatio:			pitch multiplier for MIDI key pitch
	\param modControls:				intensity and range controls for each modulator object

	Modulator indexes:
	- kFilterFcMod:				[-1, +1] fc modulation
	- kFilterQMod:				[-1, +1] Q modulation

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class VALadderFilter : public ISynthAudioProcessor
{
public:
	VALadderFilter(std::shared_ptr<VALadderFilterModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~VALadderFilter();

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

	// --- do the filter and update the z1 register (pass by reference)
	//     This is a public function so that outer container filters can call it
	bool doFilter(filterType filter, double input, unsigned int channel);

	// --- modifier getter
	std::shared_ptr<VALadderFilterModifiers> getModifiers() { return modifiers; }

	/** reset our shared modifier pointer to a new pointer */
	void setModifiers(VALadderFilterModifiers* _modifiers) { modifiers.reset(_modifiers); }

protected:
	// --- calculate the sigma variable (see book)
	double calculateSigma(unsigned int channel);

	// --- do the final filter calculations
	bool calculateFilterCoeffs();

	// --- coeffs
	double K = 0.0;			// --- global feedback value
	double alpha0 = 0.0;	// --- input gain compensator from delay free loop resolution
	//double gamma = 0.0;		// --- internal variable for Q

	// --- beta coefficients, one per filter (see block diagram)
	double beta[kNumMoogSubFilters] = { 0.0 };			///< beta coefficients, one per filter (see block diagram)

	// --- our subfilters, up to 4
	VA1Filter* va1Filters[kNumMoogSubFilters] = { 0 };	///<  our subfilters, up to 4
	
	// --- modulation by +/- RANGE of semitones (volt/octave based)
	double filterModulationRange = 0.0;					///< modulation by +/- RANGE of semitones (volt/octave based) --- see constructor

	// --- THE final filter fc
	double filter_fc = kMinFilter_fc;					///< the final filter fc after all modulations have been applied
	double filter_fc_NoPriorityMod = kMinFilter_fc;			///< the final filter fc WITHOUT the osc to Fc mod, so that it may be applied during priority moduator runs
	double filter_Q = kMinFilter_Q;						///< the final filter Q after all modulations have been applied

	// --- midiPitch for key-tracking
	double midiNotePitch = 0.0;							///< midiPitch for key-tracking

	// --- RUN/STOP flag
	bool noteOn = false;

	// --- our modifiers
	std::shared_ptr<VALadderFilterModifiers> modifiers = nullptr;

	// --- Overheim taps
	int a = 0;
	int b = 0;
	int c = 0;
	int d = 0;
	int e = 1;

};

