#pragma once
#include "SynthVoice.h"
#include "DelayFX.h" // delay FX suite

#define MAX_VOICES 16 // in release mode with granularity at 128, can easily get > 50 voices
#define MAX_UNISON_VOICES 7 // --- see notes about unison panning and detuning!

// --- outputs[] indexes for this component
enum {
	kEngineLeftOutput,
	kEngineRightOutput,
	kNumEngineOutputs
};

enum class synthMode { kPoly, kMono, kUnison};

/**
	\struct SynthEngineModifiers
	\ingroup SynthStructures
	\brief Contains modifiers for the Synth Engine component. A "modifier" is any variable that *may* be connected to a
	GUI control, however modifiers are not required to be connected to anything and their default values are set in the structure.
	*A nodifier may also be used by an outer container object to modify parameters externally!*

	\param synthMode:					operational mode -- mono, poly or unison
	\param masterTuningRatio:			master tuning control as a ratio
	\param masterTuningOffset_cents:	master tuning control as an offset in cents(can also be used to tune up or down in semitones)
	\param masterVolume_dB:				master volume control in dB
	\param masterPitchBend:				master pitch bend control in semitones
	\param unisonDetune_Cents:			maximum detuning offset for unison mode in cents
*/
struct SynthEngineModifiers
{
	SynthEngineModifiers() {}

	// --- global synth mode
	synthMode synthMode = synthMode::kPoly;

	// --- master tuning stuff
	double masterTuningRatio = 1.0;			// --- for a global master tune value as a ratio
	double masterTuningOffset_cents = 0.0;	// --- for a global master tune value in cents

	// --- global master volume control, controls each output DCA's master volume
	double masterVolume_dB = 0.0;

	// --- master pitch bend, in semitones
	unsigned int masterPitchBend = 1;

	// --- unison Detune
	double unisonDetune_Cents = 0.0;

	// --- modifiers for our sub-components
	std::shared_ptr<SynthVoiceModifiers> voiceModifiers = std::make_shared<SynthVoiceModifiers>();

	// --- modifiers for master FX: Chorus
	std::shared_ptr<DelayFXModifiers> chorusFXModifiers = std::make_shared<DelayFXModifiers>();

	// --- modifiers for master FX: Delay
	std::shared_ptr<DelayFXModifiers> delayFXModifiers = std::make_shared<DelayFXModifiers>();
};

/**
	\class SynthEngine
	\ingroup SynthClasses
	\brief Encapsulates an entire synth engine, producing one type of sysnthesizer set of voices (e.g. Virtual Analog, Sample Based, FM, etc...) --
	This object contains an array of SynthVoice objects to render audio and also processes MIDI events.

	Outputs: contains 2 outputs
	- Left Channel
	- Right Channel

	Control I/F:
	Use SynthEngineModifiers structure; note that it's SynthVoiceModifiers pointer is shared across all voices

	\param synthMode:					operational mode -- mono, poly or unison
	\param masterTuningRatio:			master tuning control as a ratio
	\param masterTuningOffset_cents:	master tuning control as an offset in cents (can also be used to tune up or down in semitones)
	\param masterVolume_dB:				master volume control in dB
	\param masterPitchBend:				master pitch bend control in semitones
	\param unisonDetune_Cents:			maximum detuning offset for unison mode in cents

	Modulator indexes:
	- this component contains no modulators

	\author Will Pirkle
	\version Revision : 1.0
	\date Date : 2017 / 09 / 24
*/
class SynthEngine : public IPluginComponent, public IMIDIData
{
public:
	SynthEngine();
	virtual ~SynthEngine();

	// --- IPluginComponent
	virtual bool reset(ResetInfo& resetInfo);
	virtual bool update(UpdateInfo& updateInfo);
	virtual bool render(RenderInfo& renderInfo);
	virtual bool processMIDIEvent(midiEvent& event);

	// --- IMIDIData
	virtual uint32_t getMidiGlobalData(uint32_t index);
	virtual uint32_t getMidiCCData(uint32_t index);
	virtual bool setMIDIOutputEvent(midiEvent& event);

	// --- our modifiers
	std::shared_ptr<SynthEngineModifiers> modifiers = std::make_shared<SynthEngineModifiers>(); ///<engine modifiers

	/** get the Voice modifiers for this engine */
	std::shared_ptr<SynthVoiceModifiers> getSynthVoiceModifiers() { return modifiers->voiceModifiers; }

	/** get the Engine modifiers */
	std::shared_ptr<SynthEngineModifiers> getSynthEngineModifiers() { return modifiers; }

	// --- voice-stealing functions
	void incrementVoiceTimestamps();
	int getFreeVoiceIndex();
	int getVoiceIndexToSteal();
	int getVoiceIndexWithNote(unsigned int midiNoteNumber);
	int getVoiceIndexForNoteOffWithNote(unsigned int midiNoteNumber);

	// --- special functions for unison mode
	void doUnisonNoteOn(uint32_t midiNoteNumber, uint32_t midiNoteVelocity);
	void doUnisonNoteOff(uint32_t midiNoteNumber, uint32_t midiNoteVelocity);

protected:
	// --- reset subcomponents
	void resetEngine();

	// --- flush outputs
	void clearOutputs()
	{
		// --- flush output with memset()
		memset(&outputs[0], 0, kNumEngineOutputs * sizeof(double));
	}

	// --- our outputs, same number as synth voice!
	double outputs[kNumEngineOutputs] = { 0.0 };		///< the output array for the engine

	// --- shared MIDI tables, via IMIDIData
	uint32_t globalMIDIData[kNumMIDIGlobals] = { 0 };	///< the global MIDI table that is shared across the voices via the IMIDIData interface
	uint32_t ccMIDIData[kNumMIDICCs] = { 0 };			///< the global MIDI CC table that is shared across the voices via the IMIDIData interface

	// --- current mode
	synthMode synthMode = synthMode::kPoly;				///< current mode of the synth

	// --- array of voice object pointers
	std::unique_ptr<SynthVoice> synthVoices[MAX_VOICES] = { 0 };		///< array of voice objects for the engine

	DelayFX* masterFX_Chorus = nullptr;
	DelayFX* masterFX_Delay = nullptr;
};

