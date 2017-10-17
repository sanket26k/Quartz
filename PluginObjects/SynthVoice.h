#pragma once
#include "synthobjects.h"

#include "synthoscillator.h"
#include "envelopegenerator.h"
#include "lfo.h"
#include "valadderfilter.h"
#include "dca.h"
#include "DelayFX.h" // delay FX suite

#include <vector>
#include <map>


//#define USE_UNIQUE_PTRS

// --- outputs[] indexes for this component
enum {
	kVoiceLeftOutput,
	kVoiceRightOutput,
	kNumVoiceOutputs
};

// --- modulator indexes for this component
const unsigned int kNumVoiceModulators = 0;

/*  Modify this list if you customize the component objects; it's easy to add output channels
	--------------------------------------------------------------------------------------------------------------------------- 
	--- MODULATION SOURCES 
	--------------------------------------------------------------------------------------------------------------------------- 

	LFO:
	- normal output
	- quad phase output
	- inverted output
	- quad phase inverted output
	- unipolar from max down
	- unipolar up-ramp (the modulo counter) -- specifically used for the glide-LFO
	- unipolar down-ramp (inverted modulo counter)

	EG:
	- normal output
	- biased output (for pitch modulation)

	OSCILLATOR:
	- left osc output
	- right osc output
	- osc FM output (specifically for FM)
	- uni-polar output from max, for AM
	- reset trigger pulse, for synchronizing oscillators to each other, or other timing events

	FILTERS:
	- currently none

	DCA:
	- currently none

	DELAY FX:
	- currently none


	---------------------------------------------------------------------------------------------------------------------------
	--- MODULATION DESTINATIONS 
	---------------------------------------------------------------------------------------------------------------------------

	LFO:
	- linear frequency modulation
	- max-downward amp modulation
	- pulse-width modulation

	EG:
	- currently none (however does support MIDI modulation with key and note to attack/decay)
	- Repeat time in mSec after being set by subdivision

	OSCILLATOR:
	- pitch modulation
	- linear FM
	- linear PM
	- pulse-width modulation
	- unipolar amp modulation (usually from EG)
	- max-down amp modulation (AM)
	- portamento modulation (from glide LFO)

	FILTERS:
	- fc modulation
	- Q modulation (2nd order and higher filters only)

	DCA:
	- unipolar-amp modulation (usually from EG)
	- max-down amp modulation
	- pan (balance( modulation

	DELAY FX:
	- feedback (%) modulation

*/


/*---------------------------------------------------------------------------------------------------------------------------
  --- MASTER MODULATION ENUM 
  ---------------------------------------------------------------------------------------------------------------------------

  - identical to the GUI version; NOTE: these must be synchronized when adding new sources and/or destinations
  - These MUST exist for the GUI to make sense of the controls */

enum class modulationSource { 
	kNoneDontCare, 
	kLFO1_Out, 
	kLFO1_OutInv, 
	kLFO2_Out, 
	kLFO2_OutInv, 
	kEG1_Out, 
	kEG1_BiasedOut, 
	kEG2_Out, 
	kEG2_BiasedOut, 
	kOsc1_Out, 
	kOsc2_Out, 
	kSubOsc_Out, 
	kNumModulationSources };	

// --- note: not adding the subOsc for pitch mod
enum class modulationDestination { 
	kNoneDontCare, 
	kOsc1_Pitch, 
	kOsc2_Pitch, 
	kAll_Osc_Pitch, 
	kOsc1_PW,
	kOsc2_PW, 
	kSubOsc_PW, 
	kAll_Osc_PW, 
	kFilter1_fc, 
	kFilter1_Q,
	kFilter2_fc, 
	kFilter2_Q,
	kEG1_Repeat_mSec,
	kEG2_Repeat_mSec,
	kEG1_Repeat_SubDiv,
	kEG2_Repeat_SubDiv,
	kDCA_Amp, 
	kDCA_Pan, 
	kDelayFX_Mix, 
	kDelayFX_FB, 
	kChorus_Depth, 
	kNumModulationDestinations};

/**
\struct ModulatorDestination
\ingroup SynthStructures
\brief A structure to hold a single routing from a mod source to a mod destination; also used in the database to identify modulator index values																																						*/
struct ModulatorRouting
{
	ModulatorRouting() {}
	ModulatorRouting(modulationSource _modSource, modulationDestination _modDest) 
	: modSource(_modSource)
	, modDest(_modDest){}

	modulationSource modSource = modulationSource::kNoneDontCare;
	modulationDestination modDest = modulationDestination::kNoneDontCare;

	bool operator==(const ModulatorRouting testRouting)
	{
		return (testRouting.modSource == modSource &&
				testRouting.modDest == modDest);
	}
};

/** overload for operator < so that ModulatorRouting structures can be used to index a map */
bool operator <(const ModulatorRouting& x, const ModulatorRouting& y);

/**
	\struct SynthVoiceModifiers
	\ingroup SynthStructures
	\brief Contains voice-level modifiers and component modifer pointers that are shared across all voices. A "modifier" is any variable that *may* be connected to a 
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure. 
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param enablePortamento:		turn glide (portamento) on/off
	\param portamentoTime_mSec:		glide time in mSec
	\param legatoMode:				turn legato mode on/off
	\param modulationRoutings:		a set of programmable modulation routings for this voice
	\param progModulationControls:	a set of controls (intensity, range, invert) for each modulation routing
*/
struct SynthVoiceModifiers
{
	SynthVoiceModifiers() {}
	SynthVoiceModifiers(SynthVoiceModifiers& initModifiers) { memcpy(this, &initModifiers, sizeof(SynthVoiceModifiers)); }

	// --- function to copy modifiers; useful when combining objects like the filters
	void SynthVoiceModifiers::copyModifiers(SynthVoiceModifiers* modifiers) { memcpy(this, &modifiers, sizeof(SynthVoiceModifiers)); }

	// --- portamento (glide)
	bool enablePortamento = false;

	// --- glide time
	double portamentoTime_mSec = 0.0; 

	// --- legato mode
	bool legatoMode = false; 

	// --- modifiers for our sub-components
	std::shared_ptr<SynthOscModifiers> osc1Modifiers = std::make_shared<SynthOscModifiers>();	///<modifiers for osc1, shared across voices
	std::shared_ptr<SynthOscModifiers> osc2Modifiers = std::make_shared<SynthOscModifiers>();	///<modifiers for osc2 shared across voices
	std::shared_ptr<SynthOscModifiers> subOscModifiers = std::make_shared<SynthOscModifiers>();	///<modifiers for osc2 shared across voices

	std::shared_ptr<EGModifiers> eg1Modifiers = std::make_shared<EGModifiers>();				///<modifiers for eg1, shared across voices
	std::shared_ptr<EGModifiers> eg2Modifiers = std::make_shared<EGModifiers>();				///<modifiers for eg2 shared across voices

	std::shared_ptr<LFOModifiers> lfo1Modifiers = std::make_shared<LFOModifiers>();				///<modifiers for lfo1, shared across voices
	std::shared_ptr<LFOModifiers> lfo2Modifiers = std::make_shared<LFOModifiers>();				///<modifiers for lfo2, shared across voices
	std::shared_ptr<LFOModifiers> glideLFOModifiers = std::make_shared<LFOModifiers>();			///<modifiers for glide LFO, shared across voices

	// --- Filter Bypass
	bool enableLPF = true;
	bool enableHPF = true;

	std::shared_ptr<VALadderFilterModifiers> filter1Modifiers = std::make_shared<VALadderFilterModifiers>();	///<modifiers for Ladder Filter, shared across voices
	std::shared_ptr<VALadderFilterModifiers> filter2Modifiers = std::make_shared<VALadderFilterModifiers>();	///<modifiers for Ladder Filter, shared across voices

	std::shared_ptr<DCAModifiers> outputDCAModifiers = std::make_shared<DCAModifiers>();		///<modifiers for Output DCA, shared across voices

	std::shared_ptr<DelayFXModifiers> delayFXModifiers = std::make_shared<DelayFXModifiers>();

	// --- for programmable modulation routing/matrix
	ModulatorRouting modulationRoutings[MAX_MOD_ROUTINGS];		///<a set of modulation routings for the voice
	ModulatorControl progModulationControls[MAX_MOD_ROUTINGS];	///<a set of modulation controls (intensity, range, invert) for each modulation routing
};

/**
	\class SynthVoice
	\ingroup SynthClasses
	\brief Encapsulates one voice: a set of ISynthComponents, a modulation matrix to connect them and all updating/rendring code

	Outputs: contains 2 outputs
	- Left Channel
	- Right Channel

	Control I/F:
	Use SynthVoiceModifiers structure; note that it's sub-component pointers are shared across all voices

	\param enablePortamento:		turn glide (portamento) on/off
	\param portamentoTime_mSec:		glide time in mSec
	\param legatoMode:				turn legato mode on/off
	\param modulationRoutings:		a set of programmable modulation routings for this voice
	\param progModulationControls:	a set of controls (intensity, range, invert) for each modulation routing

	Modulator indexes:
	- this component contains no modulators

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class SynthVoice : public ISynthComponent 
{
public:
	SynthVoice(std::shared_ptr<SynthVoiceModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators);
	virtual ~SynthVoice();
	
	// --- ISynthComponent
	virtual bool initializeComponent(InitializeInfo& info);
	virtual bool startComponent();
	virtual bool stopComponent();
	virtual bool resetComponent();
	virtual bool validateComponent();
	virtual bool isComponentRunning() { return voiceRunning; }

	// --- note event handlers
	virtual bool doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity) { return false; } /// voice has separate note on/off methods
	virtual bool doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity) { return false; } /// voice has separate note on/off methods
	
	// --- update and render methods
	virtual bool updateComponent();
	virtual bool renderComponent(bool update);

	// --- shutdown component
	virtual bool shutDownComponent();

	// --- note these are specialized voice functions
	virtual bool doNoteOn(uint32_t midiNoteNumber, uint32_t midiNoteVelocity, bool stealVoice, bool _unisonVoiceMode = false);
	virtual bool doNoteOff(uint32_t midiNoteNumber, uint32_t midiNoteVelocity);

	/** called when modulation routings have changed*/
	void updateModRoutings();
	
	/** \brief add a new routing, may be fixed or dynamic (programmable) 
		
		\param sourceComponent: a pointer to the ISynthComponent that acts as the modulation source
		\param outputArrayIndex: the index (slot) in the modulator source's output array
		\param destComponent: a pointer to the ISynthComponent that acts as the modulation destination
		\param modulatorIndex: the index (slot) in the modulator destination's modulator object array

	*/
	void addModulationRouting(ISynthComponent* sourceComponent, uint32_t outputArrayIndex, ISynthComponent* destComponent, uint32_t modulatorIndex)
	{
		destComponent->addModulationRouting(sourceComponent->getOutputPtr(outputArrayIndex), modulatorIndex, destComponent->getModulatorControls(modulatorIndex));
	}
	
	/** test the output EG to see if it has expired; returns true if the voice is finished playing its note */
	virtual bool isVoiceDone() 
	{
		if (outputEG->getState() == egState::kOff)
		{
			stopComponent();
			return true;
		}

		return false;
	}

	/** one-time function to setup the initial (fexed/default) modulation routings */
	void setFixedModulationRoutings();

	/** get a synth component interface pointer based on the modulation source */
	ISynthComponent* getModSourceComponent(modulationSource source)
	{
		if (source >= modulationSource::kNumModulationSources) return nullptr;
		return modSourceComponents[source];
	}

	/** get a synth component interface pointer based on the modulation destination */
	ISynthComponent* getModDestComponent(modulationDestination dest)
	{
		if (dest >= modulationDestination::kNumModulationDestinations) return nullptr;
		return modDestinationComponents[dest];
	}

	/** get the output array index of a specific mod source */
	int32_t getModSourceOutputArrayIndex(modulationSource source)
	{
		if (source >= modulationSource::kNumModulationSources) return -1;
		return modSourceOutputArrayIndexes[source];
	}

	/** get the index of a specific modulator */
	int32_t getModulatorIndex(modulationSource source, modulationDestination dest)
	{
		if (source >= modulationSource::kNumModulationSources || dest >= modulationDestination::kNumModulationDestinations) return -1;
		ModulatorRouting routing(source, dest);
		return modulatorArrayIndexes[routing];
	}

	// --- timestamps for determining note age
	unsigned int getTimestamp() { return timestamp; }	///< get current timestamp, the higher the value, the older the voice has been running
	void incrementTimestamp() { timestamp++; }			///< increment timestamp when a new note is triggered
	void clearTimestamp() { timestamp = 0; }			///< reset timestamp after voice is turned off
	
	// --- midi getters for other objects to use
	uint32_t getMidiNoteNumber() { return midiNoteData[kMIDINoteNumber]; }				///< get the MIDI note number of the voice
	uint32_t getMidiNoteStealNumber() { return midiNoteData[kPendingMIDINoteNumber]; }	///< get the pending MIDI note number for a voice that is being stolen
	uint32_t getMidiNoteVelocity() { return midiNoteData[kMIDINoteVelocity]; }			///< get the current MIDI velocity of the voice

	// --- for unison detune
	void setUnisonDetuneIntensity(double& unisonDetuneIntensity);
	void setVoicePanValue(double& panValue);

	/// get an output value from the output array
	double getOutputValue(uint32_t index)
	{
		if (index >= kNumVoiceOutputs) return 0.0;
		return outputs[index];
	}

	// --- database functions for easy addition of components, aggreagated componets, and modulators
	//
	/**
	\struct registerModSourceComponent
	\ingroup SynthFunctions
	\brief register a modulation source component with the database

	\param source:					the modulation source enum
	\param component:				the synth component that implements the modulation
	\param sourceOutputArrayIndex:	the index in the source component's output array
	*/
	void registerModSourceComponent(modulationSource source, ISynthComponent* component, int32_t sourceOutputArrayIndex)
	{
		modSourceComponents.insert(std::make_pair(source, component));
		registerModSourceOutputIndex(source, sourceOutputArrayIndex);
	}

	/**
	\struct unRegisterModSourceComponent
	\ingroup SynthFunctions
	\brief un-registers (removes) a modulation source component with the database

	\param source:					the modulation source enum
	*/
	void unRegisterModSourceComponent(modulationSource source)
	{
		modSourceComponents.erase(source);
		unRegisterModSourceOutputIndex(source);
	}


	/**
	\struct registerModDestinationComponent
	\ingroup SynthFunctions
	\brief register a modulation destination component with the database

	\param dest:					the modulation destination enum
	\param component:				the destination synth component that receives the modulation
	*/
	void registerModDestinationComponent(modulationDestination dest, ISynthComponent* component)
	{
		modDestinationComponents.insert(std::make_pair(dest, component));
	}

	/**
	\struct unRegisterModDestinationComponent
	\ingroup SynthFunctions
	\brief un-registers (removes) a modulation destination component with the database

	\param dest:					the modulation destination enum
	*/
	void unRegisterModDestinationComponent(modulationDestination dest)
	{
		modDestinationComponents.erase(dest);
	}


	/**
	\struct registerModSourceOutputIndex
	\ingroup SynthFunctions
	\brief register a modulation source output index (called internally by registerModSourceComponent() above)

	\param source:					the modulation destination enum
	\param index:					the index in the source component's output array
	*/
	void registerModSourceOutputIndex(modulationSource source, int32_t index)
	{
		modSourceOutputArrayIndexes.insert(std::make_pair(source, index));
	}

	/**
	\struct unRegisterModSourceOutputIndex
	\ingroup SynthFunctions
	\brief un-registers (removes) a modulation source output index (called internally by registerModSourceComponent() above)

	\param source:					the modulation destination enum
	*/
	void unRegisterModSourceOutputIndex(modulationSource source)
	{
		modSourceOutputArrayIndexes.erase(source);
	}


	/**
	\brief remove all modulation routings
	*/
	void removeModRoutings()
	{
		for (unsigned int i = 0; i < MAX_MOD_ROUTINGS; i++)
		{
			// --- remove the routing for this slot
			ISynthComponent* destComponent = getModDestComponent(modulationRoutings[i].modDest);
			int32_t modulatorIndex = getModulatorIndex(modulationRoutings[i].modSource, modulationRoutings[i].modDest);

			if (destComponent && modulatorIndex >= 0)
			{
				destComponent->getModulator(modulatorIndex)->removeModulationRouting(i);
			}

			modulationRoutings[i].modSource = modulationSource::kNoneDontCare;
			modulationRoutings[i].modDest = modulationDestination::kNoneDontCare;
		}
	}

	/**
		\struct findModulatorRoutingInIndexMap
		\ingroup SynthFunctions
		\brief find an existing modulation routing (in case it needs to be overwritten)

		\param _routing:				the ModulatorRouting to find
	*/
	int32_t findModulatorRoutingInIndexMap(ModulatorRouting _routing)
	{
		int index = 0;
		for (modulatorArrayIndexMap::const_iterator it = modulatorArrayIndexes.begin(), end = modulatorArrayIndexes.end(); it != end; ++it)
		{
			// it->second is receiver
			ModulatorRouting routing = it->first;
			if (routing == _routing)
			{
				return index;
			}
			index++;
		}
		return -1;
	}

	// --- some C++11 fun with iterators and strongly typed enums
	typedef Iterator<modulationSource, modulationSource::kNoneDontCare, modulationSource::kNumModulationSources> modSourceIterator;
	
	/**
		\struct registerModulatorArrayIndex
		\ingroup SynthFunctions
		\brief register a modulator array index with the database

		\param source:				the source enum
		\param dest:				the destination enum
		\param index:				the modulator index on the destination component
	*/
	void registerModulatorArrayIndex(modulationSource source, modulationDestination dest, int32_t index)
	{
		// --- the destination can NEVER be kNoneDontCare - must be a specific destination always
		if (dest == modulationDestination::kNoneDontCare) return;

		// --- register a specific routing
		if (source != modulationSource::kNoneDontCare && dest != modulationDestination::kNoneDontCare)
		{
			// --- see if it exists due to one of the blank DontCare entries
			ModulatorRouting routing(source, dest);

			if (findModulatorRoutingInIndexMap(routing) >= 0)
				modulatorArrayIndexes.erase(routing);

			modulatorArrayIndexes.insert(std::make_pair(routing, index));
		}
		// --- register all sources when source == kNoneDontCare (the destination can NEVER be kNoneDontCare)
		if (source == modulationSource::kNoneDontCare)
		{
			for (modulationSource newSource : modSourceIterator())
			{
				if (newSource != modulationSource::kNoneDontCare && newSource != modulationSource::kNumModulationSources)
				{
					ModulatorRouting routing(newSource, dest);
					modulatorArrayIndexes.insert(std::make_pair(routing, index));
				}
			}
		}
	}
	
protected:
	// --- granularity counter and rollover updater
	bool needsComponentUpdate()						///< uses the granularity counter to generate component update timing; if the updateGranularity == 1, the components are updated on every sample interval
	{
		bool update = false;

		// --- always update on first render pass
		if (granularityCounter < 0)
			update = true;

		// --- check counter
		granularityCounter++;
		if (granularityCounter == updateGranularity)
		{
			granularityCounter = 0;
			update = true;
		}

		return update;
	}

	// --- SYNTH COMPONENTS; note that these are old-fashioned (raw) pointers - this will be needed for aggregating components or swapping interface pointers
	//
	SynthOscillator* osc1 = nullptr;
	SynthOscillator* osc2 = nullptr;
	SynthOscillator* subOsc = nullptr; // -->> 1. add member to .h file

	// --- 2 EGs
	EnvelopeGenerator* outputEG = nullptr;
	EnvelopeGenerator* eg2 = nullptr;

	// --- 3 LFOs
	LFO* lfo1 = nullptr;
	LFO* lfo2 = nullptr;
	LFO* glideLFO = nullptr;

	// --- 2 Moog filters LPF: 1 HPF: 2 
	VALadderFilter* filter1 = nullptr;
	VALadderFilter* filter2 = nullptr;

	// --- 1 output DCA
	DCA* outputDCA = nullptr;

	// --- the delayFX processor as insert (example)
	// DelayFX* insertDelayFX = nullptr;

	// --- modifier pointer
	std::shared_ptr<SynthVoiceModifiers> modifiers = nullptr;

	// --- current source/destination routings
	ModulatorRouting modulationRoutings[MAX_MOD_ROUTINGS];

	// --- voice timestamp, for knowing the age of a voice
	unsigned int timestamp = 0;						///<voice timestamp, for knowing the age of a voice

	// --- MIDI note data
	int midiNoteData[kNumMIDINoteData] = { -1 };	///< MIDI note information for this voice; contains CURRENT MIDI note information and PENDING information for voice-steal operation

	// --- granularity counter
	uint32_t updateGranularity = 1;					///< number of sample invervals to wait between component updates
	int granularityCounter = -1;					///< the counter for gramular updating; -1 = update NOW

	bool unisonVoiceMode = false;					///< true if voice is in unison mode
	bool voiceRunning = false;						///< NOTE: this is different from noteOn; after the note turns off, we still are running until the output EG has expired

	// --- databases
	typedef std::map<modulationSource, ISynthComponent*> modSourceComponentMap;
	modSourceComponentMap modSourceComponents;

	typedef std::map<modulationDestination, ISynthComponent*> modDestComponentMap;
	modDestComponentMap modDestinationComponents;

	typedef std::map<modulationSource, int32_t> modSourceOutputArrayIndexMap;
	modSourceOutputArrayIndexMap modSourceOutputArrayIndexes;

	typedef std::map<ModulatorRouting, int32_t> modulatorArrayIndexMap;
	modulatorArrayIndexMap modulatorArrayIndexes;

};

