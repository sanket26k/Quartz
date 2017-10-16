#pragma once

#include "synthfunctions.h"
#include "synthobjects.h"
#include "guiconstants.h"

// --- LIMITS (always at top)
//
// --- modulation limits
const double kMinFilter_fc = 20.0;
const double kMaxFilter_fc = 20480.0;	// 10 octaves above 20.0 Hz
	
// --- other limits
const unsigned int maxChannels = 2;		// mono or stereo filter

// --- outputs[] indexes for this component
enum {	
	kVA1FilterOutput_0,
	kVA1FilterOutput_1,
	kNumVA1FilterOutputs
};

// --- modulatior indexes for this component
enum {	
	kVA1FilterFcMod,
	kNumVA1FilterModulators
};

// --- strongly typed enum for filter type
enum class filterType { kLPF1, kHPF1, kAPF1, kLPF2, kHPF2, kBPF2, kBSF2, kLPF4, kHPF4 };

/**
	\struct VA1FilterModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the filter components, including both the VA1Filter and VALadderFilter objects. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param fcControl:				fc control from GUI
	\param filter:					filte type -- see enum class filterType
	\param enableKeyTrack:			enable/disable keytracking
	\param keytrackRatio:			pitch multiplier for MIDI key pitch
	\param modControls:				intensity and range controls for each modulator object
*/
struct VA1FilterModifiers
{
	VA1FilterModifiers() {}

	// --- modifiers
	double fcControl = 0.0;

	// --- strongly typed enum for filter type
	filterType filter = filterType::kLPF1;

	// --- filter keytracking
	bool enableKeyTrack = false;
	double keytrackRatio = 1.0;

	// --- modulator controls
	ModulatorControl modulationControls[kNumVA1FilterModulators];

};

/**
	\class VA1Filter
	\ingroup SynthClasses
	\brief Encapsulates a MONO or STEREO 1st Order VA Filter -- For stereo operation, the two filters share the same fc setting; 
	if you need individual left/right filters with separate fc values, use two of these in mono mode each and control independently

	I/O: supports the following channel combinations:
	- mono -> mono
	- mono -> stereo
	- stereo -> stereo

	Outputs: contains 2 outputs
	- Left Channel
	- Right Channel

	Control I/F:
	Use VA1FilterModifiers structure

	\param fcControl:				fc control from GUI
	\param filter:					filte type -- see enum class filterType
	\param enableKeyTrack:			enable/disable keytracking
	\param keytrackRatio:			pitch multiplier for MIDI key pitch
	\param modControls:				intensity and range controls for each modulator object

	Modulator indexes:
	- kFilterFcMod:				[-1, +1] fc modulation

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class VA1Filter : public ISynthAudioProcessor
{
public:
	VA1Filter(std::shared_ptr<VA1FilterModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~VA1Filter();

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
	std::shared_ptr<VA1FilterModifiers> getModifiers() { return modifiers; }

	/** reset our shared modifier pointer to a new pointer  */
	void setModifiers(VA1FilterModifiers* _modifiers) { modifiers.reset(_modifiers); }

	// --- storage access to the "s" value for this va filter
	double getStorageValue(unsigned int channel) // --- enum {CHANNEL_1, CHANNEL_2}; in synthobjects.h
	{
		// --- read access to our "s" value
		return z1[channel];
	}
	// --- same but with beta coefficient applied
	double getFeedbackStorageValue(unsigned int channel) // --- enum {CHANNEL_1, CHANNEL_2}; in synthobjects.h
	{
		// --- read access to our "s" value, apply beta
		return z1[channel]* beta;
	}

	// --- for coefficients
	void setBeta(double _beta) { beta = _beta; }
	double getBeta() { return beta; }			// not normally used, but providing for future

	const double getAlpha() { return alpha; }		// --- read only
	const double getFilter_g() { return vaFilter_g; } // --- read only

	// --- do the filter operation (this is public so it can be called from an outer container)
	//     and it returns the filter output data
	double doFilter(filterType filter, double input, unsigned int channel);

	void setFilterCoeffs(double filter_fc, double _alpha, double _vaFilter_g)
	{
		alpha = _alpha;
		vaFilter_g = _vaFilter_g;
	}

protected:
	// --- our filter
	filterType filter = filterType::kLPF1;	///< stored filter type

	// --- sample rate
	double sampleRate = 0.0;			///< sample rate

	// --- midiPitch for key-tracking
	double midiNotePitch = 0.0;			///< midiPitch for key-tracking

	// --- coefficients
	double alpha = 0.0;
	double beta = 0.0;			// --- used for outer filters that use this filter
	double vaFilter_g = 0.0;	// --- the "g" value for this va filter

	// --- z^-1 storage array (left, right)
	double z1[maxChannels] = { 0.0 };	///< z^-1 storage array (left, right)

	// --- modulation by +/- RANGE of semitones (volt/octave based)
	double filterModulationRange = 0.0;	///< modulation by +/- RANGE of semitones (volt/octave based) --- see constructor

	// --- THE final filter fc
	double filter_fc = kMinFilter_fc;	///< the final fc after all modulations have been applied

	// --- RUN/STOP flag
	bool noteOn = false;

	// --- our modifiers
	std::shared_ptr<VA1FilterModifiers> modifiers = nullptr;
};

