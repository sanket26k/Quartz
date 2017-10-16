#include "SynthEngine.h"
#include <memory>

//#include "trace.h"

/** Generic constructor for engine which creates all shared modifiers and populates the voice array */
SynthEngine::SynthEngine()
{
	// --- reset the engine modifiers
	modifiers.reset(new SynthEngineModifiers);

	// --- global MIDI
	globalMIDIData[kCurrentMIDINoteNumber] = 128;	// --- 128 = not set (yet)
	globalMIDIData[kLastMIDINoteNumber] = 128;		// --- 128 = not set (yet)

	globalMIDIData[kMIDIPitchBendData2] = 64;		// --- this is for 0 pitchbend at startup

	// --- CCs
	ccMIDIData[VOLUME_CC07] = 127;	// --- MIDI VOLUME; default this to ON
	ccMIDIData[PAN_CC10] = 64;		// --- MIDI PAN; default this to CENTER
	
	// --- master FX: chorus
	modifiers->chorusFXModifiers->delayFXMode = delayFXMode::chorus;
	masterFX_Chorus = new DelayFX(modifiers->chorusFXModifiers, this, kNumDelayFXOutputs, kNumDelayFXModulators);

	// --- master FX: delay
	modifiers->delayFXModifiers->delayFXMode = delayFXMode::norm;
	masterFX_Delay = new DelayFX(modifiers->delayFXModifiers, this, kNumDelayFXOutputs, kNumDelayFXModulators);

	// --- create array of voices
	for (unsigned int i = 0; i < MAX_VOICES; i++)
	{
		synthVoices[i].reset(new SynthVoice(modifiers->voiceModifiers, this, kNumVoiceOutputs, kNumVoiceModulators));

		// --- can register master level FX with voices for modulation from their components (beware, can get very tricky)
		//     NOTE: you can't have multiple modulation components/indexes with the same source or destination
		//           So, the delay FX as insert routings would need to be un-registered (see the functions)
		synthVoices[i]->registerModDestinationComponent(modulationDestination::kDelayFX_FB, masterFX_Delay);
		synthVoices[i]->registerModDestinationComponent(modulationDestination::kDelayFX_Mix, masterFX_Delay);
		synthVoices[i]->registerModDestinationComponent(modulationDestination::kChorus_Depth, masterFX_Chorus);
		
		synthVoices[i]->registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDelayFX_FB, kDelayFX_FeedbackMod);
		synthVoices[i]->registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDelayFX_Mix, kDelayFX_MixMod);
		synthVoices[i]->registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kChorus_Depth, kChorusFX_DepthMod);
	}


}

/**
	\brief Destroy all modifiers and voices that were allocated in constructor
*/
SynthEngine::~SynthEngine()
{
	if (masterFX_Chorus) delete masterFX_Chorus;
	if (masterFX_Delay) delete masterFX_Delay;
}

/**
	\brief Reset the plugin component by initializing all sub-components

	\param resetInfo contains reset information including sample rate

	\return true if handled, false otherwise
*/
bool SynthEngine::reset(ResetInfo& resetInfo)
{
	InitializeInfo info(resetInfo.sampleRate, resetInfo.bitDepth);

	for (unsigned int i = 0; i < MAX_VOICES; i++)
	{
		synthVoices[i]->initializeComponent(info);
	}

	// --- Master FX: init and start running
	masterFX_Chorus->initializeComponent(info);
	masterFX_Chorus->startComponent();

	masterFX_Delay->initializeComponent(info);
	masterFX_Delay->startComponent();

	return true;
}

/**
	\brief Update the synth sub-components with master (voice) level parameters

	\param updateInfo currently not used

	\return true if handled, false otherwise
*/
bool SynthEngine::update(UpdateInfo& updateInfo)
{
	// --- MODE
	synthMode = modifiers->synthMode;

	// --- store pitch bend range in midi data table
	globalMIDIData[kMIDIPitchBendRange] = modifiers->masterPitchBend;

	masterFX_Chorus->updateComponent();
	masterFX_Delay->updateComponent();

	return true;
}

/**
	\brief Render the synth output for this sample interval; loop through the voices and render/accumulate them

	\param renderInfo information about current render cycle

	\return true if handled, false otherwise
*/
bool SynthEngine::render(RenderInfo& renderInfo)
{
	if (renderInfo.numOutputChannels != kNumEngineOutputs)
		return false; // not handled

	// --- flush
	clearOutputs();

	// --- voices render into their internal buffers
	renderInfo.renderInternal = true;

	// --- for unison mode we need to scale back the gains of each voice
	double gainFactor = 1.0;
	if (synthMode == synthMode::kUnison)
		gainFactor = 1.0 / (MAX_UNISON_VOICES);

	// --- loop through voices and render/accumulate them
	for (unsigned int i = 0; i < MAX_VOICES; i++) 
	{
		if (synthVoices[i]->renderComponent(false))
		{
			outputs[kEngineLeftOutput] += gainFactor * synthVoices[i]->getOutputValue(kVoiceLeftOutput);
			outputs[kEngineRightOutput] += gainFactor * synthVoices[i]->getOutputValue(kVoiceRightOutput);
		}
	}

	// --- setup FX render
	RenderInfo masterFXRender;
	masterFXRender.inputData = &outputs[kEngineLeftOutput]; // NOTE: processing "in place"
	masterFXRender.outputData = &outputs[kEngineLeftOutput];// NOTE: processing "in place"
	masterFXRender.numInputChannels = kNumEngineOutputs;
	masterFXRender.numOutputChannels = kNumEngineOutputs;
	masterFXRender.renderInternal = false;
	masterFXRender.updateComponent = true;

	// --- master FX: CHORUS
	if (masterFX_Chorus->getModifiers()->enabled)
	{
		masterFX_Chorus->processAudio(masterFXRender);
	}

	// --- master FX: DELAY
	if (masterFX_Delay->getModifiers()->enabled)
	{
		masterFX_Delay->processAudio(masterFXRender);
	}

	// --- copy to output arrays
	renderInfo.outputData[0] = outputs[kEngineLeftOutput];
	renderInfo.outputData[1] = outputs[kEngineRightOutput];

	return true;
}

/**
	\brief The MIDI event handler function; for note on/off messages it finds the voices to turn on/off.
	MIDI CC information is placed in the shared CC array.

	\param event a single MIDI Event to decode and process

	\return true if handled, false otherwise
*/
bool SynthEngine::processMIDIEvent(midiEvent& event)
{
	// --- note on and note off are specialized functions
	if (event.midiMessage == NOTE_ON)
	{
		// TRACE("-- Note On Ch:%d Note:%d Vel:%d \n", event.midiChannel, event.midiData1, event.midiData2);

		// --- mono mode
		if (synthMode == synthMode::kMono)
		{
			// --- just use voice 0 and do the note EG variables will handle the rest
			synthVoices[0]->doNoteOn(event.midiData1, event.midiData2, synthVoices[0]->isComponentRunning());
			return true;
		}
		else if (synthMode == synthMode::kUnison)
		{
			// --- create our stacked unison (aka supersaw when using sawtooth)
			doUnisonNoteOn(event.midiData1, event.midiData2);
			return true;
		}

		// --- is this note already playing?
		int index = getVoiceIndexWithNote(event.midiData1);

		// --- if not try to find a free one
		if (index < 0)
			index = getFreeVoiceIndex();

		// --- if no free voice, find one to steal
		bool stealVoice = false;
		if (index < 0)
		{
			index = getVoiceIndexToSteal();
			stealVoice = true;
		}

		// --- should always have an index to work with
		if (index >= 0)
		{
			//if(stealVoice)
			// TRACE("-- Stealing Voice Index:%d \n", index);

			// --- advance timestamps
			incrementVoiceTimestamps();

			// --- store global data for note ON event: set previous note-on data
			globalMIDIData[kLastMIDINoteNumber] = globalMIDIData[kCurrentMIDINoteNumber];
			globalMIDIData[kLastMIDINoteVelocity] = globalMIDIData[kCurrentMIDINoteVelocity];

			// --- current data
			globalMIDIData[kCurrentMIDINoteNumber] = event.midiData1;
			globalMIDIData[kCurrentMIDINoteVelocity] = event.midiData2;

			// --- call note on with steal flag
			synthVoices[index]->doNoteOn(event.midiData1, event.midiData2, stealVoice);
		}
	}
	else if (event.midiMessage == NOTE_OFF)
	{
		// TRACE("-- Note Off Ch:%d Note:%d Vel:%d \n", event.midiChannel, event.midiData1, event.midiData2);

		if (synthMode == synthMode::kMono)
		{
			if (synthVoices[0]->isComponentRunning())
			{
				synthVoices[0]->doNoteOff(event.midiData1, event.midiData2);
				//	return true;
			}
		}
		else if (synthMode == synthMode::kUnison)
		{
			// --- kill our stacked unison (aka supersaw when using sawtooth)
			doUnisonNoteOff(event.midiData1, event.midiData2);
			//return true;
		}

		int index = getVoiceIndexForNoteOffWithNote(event.midiData1);
		if (index >= 0)
		{
			// TRACE("-- FOUND NOTE OFF index:%d \n", index);

			synthVoices[index]->doNoteOff(event.midiData1, event.midiData2);
		}
		else
			int t = 0;
		// TRACE("-- DID NOT FOUND NOTE OFF index:%d \n", index);

	}
	else // --- non-note stuff here!
	{
		if (event.midiMessage == PITCH_BEND)
		{
			globalMIDIData[kMIDIPitchBendData1] = event.midiData1;
			globalMIDIData[kMIDIPitchBendData2] = event.midiData2;
		}
		if (event.midiMessage == CONTROL_CHANGE)
		{
			// --- store CC event in globally shared array
			ccMIDIData[event.midiData1] = event.midiData2;
		}

	}

	return true;
}

/**
	\brief Retrieve a MIDI global value

	\param index: the index of the value in the table

	\return the value in the MIDI Global table, or 0 if the table index is out of bounds
*/
uint32_t SynthEngine::getMidiGlobalData(uint32_t index)
{
	if (index >= kNumMIDIGlobals)
		return 0;

	return globalMIDIData[index];
}

/**
	\brief Retrieve a MIDI global CC value

	\param index: the index of the value in the table
	
	\return the value in the MIDI Global CC table, or 0 if the table index is out of bounds
*/
uint32_t SynthEngine::getMidiCCData(uint32_t index)
{
	if (index >= 128)
		return 0;

	return ccMIDIData[index];
}

/**
	\brief Set MIDI output data

	\return true if handled, false otherwise *NOTE* currently not handled
*/
bool SynthEngine::setMIDIOutputEvent(midiEvent& event)
{
	return false;
}

/**
	\brief Increment the timestamps of currently running voices.

	\return true if handled, false otherwise
*/
void SynthEngine::incrementVoiceTimestamps()
{
	for (unsigned int i = 0; i < MAX_VOICES; i++)
	{
		if (synthVoices[i]->isComponentRunning())
			synthVoices[i]->incrementTimestamp();
	}
}

/**
	\brief Get the array index of a free voice that is ready for note on event.

	\return array index or -1 if no voices are free
*/
int SynthEngine::getFreeVoiceIndex()
{
	for (unsigned int i = 0; i < MAX_VOICES; i++)
	{
		if (!synthVoices[i]->isComponentRunning())
			return i;
	}

	return -1;
}

/**
	\brief Get the array index of a voice to steal; the heuristic used here is to steal the oldest note.

	\return array index or -1 if no voices are free for stealing
*/
int SynthEngine::getVoiceIndexToSteal()
{
	int nTimeStamp = -1;
	int foundIndex = -1;
	for (int i = 0; i<MAX_VOICES; i++)
	{
		if (synthVoices[i])
		{
			// if on and older, save
			// highest timestamp is oldest
			if (synthVoices[i]->isComponentRunning() && (int)synthVoices[i]->getTimestamp() > nTimeStamp)
			{
				foundIndex = i;
				nTimeStamp = (int)synthVoices[i]->getTimestamp();
			}
		}
	}

	return foundIndex;
}

/**
	\brief Get the array index of a voice with a given MIDI note number.

	\param midiNoteNumber the note number to test

	\return array index or -1 if no voices are playing the current note
*/
int SynthEngine::getVoiceIndexWithNote(unsigned int midiNoteNumber)
{
	for (int i = 0; i < MAX_VOICES; i++)
	{
		if (synthVoices[i] && synthVoices[i]->isComponentRunning() && synthVoices[i]->getMidiNoteNumber() == midiNoteNumber)
			return i;
	}

	return -1;
}

/**
	\brief Get the array index of a voice with a given MIDI note number that is available to process note off message.

	\param midiNoteNumber the note number to test

	\return array index or -1 if no voices have the MIDI note number, either playing or stealing
*/
int SynthEngine::getVoiceIndexForNoteOffWithNote(unsigned int midiNoteNumber)
{
	for (int i = 0; i < MAX_VOICES; i++)
	{
		if (synthVoices[i] && synthVoices[i]->isComponentRunning() && ( synthVoices[i]->getMidiNoteNumber() == midiNoteNumber || synthVoices[i]->getMidiNoteStealNumber() == midiNoteNumber) )
			return i;
	}

	return -1;
}

/**
	\brief Create the unison voice by using multiple panned and detuned voices.

	\param midiNoteNumber the note number to tes
	\param midiNoteVelocity the note velocity
*/
void SynthEngine::doUnisonNoteOn(uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	double panValue = -1.0;
	double panInc = 1.0 / ((MAX_UNISON_VOICES - 1) / 2.0); // assumes ODD number of voices so center voice is center panned and not detuned

	double unisonDetuneIntensity = -1.0;
	double unisonDetuneInc = 1.0 / ((MAX_UNISON_VOICES - 1) / 2.0); // assumes ODD number of voices so center voice is center panned and not detuned

	for (unsigned int i = 0; i < MAX_UNISON_VOICES; i++)
	{
		// --- check if running?
		synthVoices[i]->setVoicePanValue(panValue);
		synthVoices[i]->setUnisonDetuneIntensity(unisonDetuneIntensity);

		synthVoices[i]->doNoteOn(midiNoteNumber, midiNoteVelocity, synthVoices[i]->isComponentRunning(), true);

		panValue += panInc;
		unisonDetuneIntensity += unisonDetuneInc;
	}

}

/**
	\brief Place the unison voice into note-off mode.

	\param midiNoteNumber the note number to tes
	\param midiNoteVelocity the note velocity
*/
void SynthEngine::doUnisonNoteOff(uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	for (unsigned int i = 0; i < MAX_UNISON_VOICES; i++)
	{
		synthVoices[i]->doNoteOff(midiNoteNumber, midiNoteVelocity); 
	}

}

/**
	\brief Reset the engine to all-notes-off state by shutting down all components
*/
void SynthEngine::resetEngine()
{
	for (unsigned int i = 0; i < MAX_VOICES; i++)
	{
		synthVoices[i]->stopComponent();
	}
}
