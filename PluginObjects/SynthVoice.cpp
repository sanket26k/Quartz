#include "SynthVoice.h"

/** overload for operator < so that ModulatorRouting structures can be used to index a map */
bool operator <(const ModulatorRouting& x, const ModulatorRouting& y)
{
	return std::tie(x.modSource, x.modDest) < std::tie(y.modSource, y.modDest);
}

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/

SynthVoice::SynthVoice(std::shared_ptr<SynthVoiceModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthComponent(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kVoice;

	/*  --- NOTE: these specialized constructors do two important things:
				  1) FORCE the components to share the modifiers and MIDI interface
				  2) SETUP the proper output and modulator arrays at the ISynthComponent base level for less repeated code
	*/

	// --- Voice Architecture: 2 synth oscillators
	osc1 = new SynthOscillator(modifiers->osc1Modifiers, _midiData, kNumSynthOscOutputs, kNumSynthOscModulators);
	osc2 = new SynthOscillator(modifiers->osc2Modifiers, _midiData, kNumSynthOscOutputs, kNumSynthOscModulators);

	// --->> 3. create the new object with the new modifiers for it
	subOsc = new SynthOscillator(modifiers->subOscModifiers, _midiData, kNumSynthOscOutputs, kNumSynthOscModulators);

	// --- Voice Architecture: 2 EGs
	outputEG = new EnvelopeGenerator(modifiers->eg1Modifiers, _midiData, kNumEGOutputs, kNumEGModulators);
	eg2 = new EnvelopeGenerator(modifiers->eg2Modifiers, _midiData, kNumEGOutputs, kNumEGModulators);

	// --- Voice Architecture: 3 LFOs
	lfo1 = new LFO(modifiers->lfo1Modifiers, _midiData, kNumLFOOutputs, kNumLFOModulators);
	lfo2 = new LFO(modifiers->lfo2Modifiers, _midiData, kNumLFOOutputs, kNumLFOModulators);
	glideLFO = new LFO(modifiers->glideLFOModifiers, _midiData, kNumLFOOutputs, kNumLFOModulators);

	// --- Voice Architecture: 1 Moog Filter (Stereo or Dual-Mono)
	filter1 = new VALadderFilter(modifiers->filter1Modifiers, _midiData, kNumVALadderFilterOutputs, kNumVALadderFilterModulators);
	filter2 = new VALadderFilter(modifiers->filter2Modifiers, _midiData, kNumVALadderFilterOutputs, kNumVALadderFilterModulators);

	// --- Voice Architecture: 1 DCA for output (Stereo or Dual-Mono)
	outputDCA = new DCA(modifiers->outputDCAModifiers, _midiData, kNumDCAOutputs, kNumDCAModulators);

	// --- delay FX as insert 
	// insertDelayFX = new DelayFX(modifiers->delayFXModifiers, _midiData, kNumDelayFXOutputs, kNumDelayFXModulators);

	// --- REGISTER MOD SOURCE COMPONENTS -------------------------------------------------------------------------
	//
	//     function arguments: (modulationSource, ISynthComponent*, modulator source output array index)
	registerModSourceComponent(modulationSource::kOsc1_Out, osc1, kLeftOscOutput);
	registerModSourceComponent(modulationSource::kOsc2_Out, osc2, kLeftOscOutput);
	registerModSourceComponent(modulationSource::kSubOsc_Out, subOsc, kLeftOscOutput);

	registerModSourceComponent(modulationSource::kLFO1_Out, lfo1, kLFONormalOutput);
	registerModSourceComponent(modulationSource::kLFO1_OutInv, lfo1, kLFONormalOutputInverted);

	registerModSourceComponent(modulationSource::kLFO2_Out, lfo2, kLFONormalOutput);
	registerModSourceComponent(modulationSource::kLFO2_OutInv, lfo2, kLFONormalOutputInverted);

	registerModSourceComponent(modulationSource::kEG1_Out, outputEG, kEGNormalOutput);
	registerModSourceComponent(modulationSource::kEG1_BiasedOut, outputEG, kEGBiasedOutput);

	registerModSourceComponent(modulationSource::kEG2_Out, eg2, kEGNormalOutput);
	registerModSourceComponent(modulationSource::kEG2_BiasedOut, eg2, kEGBiasedOutput);
	// ------------------------------------------------------------------------------------------------------------

	// --- REGISTER MOD DESTINATION COMPONENTS --------------------------------------------------------------------
	//
	//     function arguments: (modulationDestination, ISynthComponent*)
	registerModDestinationComponent(modulationDestination::kOsc1_Pitch, osc1);
	registerModDestinationComponent(modulationDestination::kOsc1_PW, osc1);

	registerModDestinationComponent(modulationDestination::kOsc2_Pitch, osc2);
	registerModDestinationComponent(modulationDestination::kOsc2_PW, osc2);

	registerModDestinationComponent(modulationDestination::kSubOsc_PW, subOsc);

	registerModDestinationComponent(modulationDestination::kFilter1_fc, filter1);		//MODIFY FOR LPF AND HPF
	registerModDestinationComponent(modulationDestination::kFilter1_Q, filter1);
	registerModDestinationComponent(modulationDestination::kFilter2_fc, filter2);		//MODIFY FOR LPF AND HPF
	registerModDestinationComponent(modulationDestination::kFilter2_Q, filter2);

	registerModDestinationComponent(modulationDestination::kEG1_Repeat_SubDiv, outputEG);
	registerModDestinationComponent(modulationDestination::kEG2_Repeat_SubDiv, eg2);
	registerModDestinationComponent(modulationDestination::kEG1_Repeat_mSec, outputEG);
	registerModDestinationComponent(modulationDestination::kEG2_Repeat_mSec, eg2);

	registerModDestinationComponent(modulationDestination::kDCA_Amp, outputDCA);
	registerModDestinationComponent(modulationDestination::kDCA_Pan, outputDCA);
	
	// --- insert delay FX
	//registerModDestinationComponent(modulationDestination::kDelayFX_FB, insertDelayFX);
	//registerModDestinationComponent(modulationDestination::kDelayFX_Mix, insertDelayFX);
	// ------------------------------------------------------------------------------------------------------------

	// --- REGISTER MODULATOR ARRAY INDEXES -----------------------------------------------------------------------
	//     (note use of Don't Care to mean any modulation source at all)
	//
	//    function arguments: (modulationSource, modulationDestination, destination modulator array index)
	//
	// --- OSC PITCH MOD
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kOsc1_Pitch, kSynthOscPitchMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kOsc2_Pitch, kSynthOscPitchMod);

	// --- OSC PW MOD
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kOsc1_PW, kSynthOscPulseWidthMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kOsc2_PW, kSynthOscPulseWidthMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kSubOsc_PW, kSynthOscPulseWidthMod);

	// --- FILTER FC and Q MOD				//MODIFY FOR LPF AND HPF
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kFilter1_fc, kVALadderFilterFcMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kFilter1_Q, kVALadderFilterQMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kFilter2_fc, kVALadderFilterFcMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kFilter2_Q, kVALadderFilterQMod);

	// --- EGS
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kEG1_Repeat_mSec, kEGRepeatTimeMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kEG2_Repeat_mSec, kEGRepeatTimeMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kEG1_Repeat_SubDiv, kEGRepeatTimeSDMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kEG2_Repeat_SubDiv, kEGRepeatTimeSDMod);

	// --- DCA AMP and PAN MOD
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDCA_Amp, kDCA_MaxDownAmpMod);
	registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDCA_Pan, kDCA_PanMod);

	// --- ADD SPECIFIC MODULATOR INDEXES (where the source is NOT don't care) HERE:
	//
	// --- OSC->FILTER FC MOD
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kFilter1_fc, kVALadderFilterOscToFcMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kFilter1_fc, kVALadderFilterOscToFcMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kFilter2_fc, kVALadderFilterOscToFcMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kFilter2_fc, kVALadderFilterOscToFcMod);/* special priority modulator */

	// --- OSC->OSC PITCH MOD (BEWARE: you may not want to enable this, or even allow the user to select it
	//     because it has very unpredicatable results, espceially self-oscillator-modulation -- however we support it here)
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kOsc1_Pitch, kSynthOscToOscPitchMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kOsc2_Pitch, kSynthOscToOscPitchMod);/* special priority modulator */

	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kOsc2_Pitch, kSynthOscToOscPitchMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kOsc1_Pitch, kSynthOscToOscPitchMod);/* special priority modulator */

	// --- OSC->OSC PW MOD (BEWARE)
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kOsc1_PW, kSynthToOscOscPulseWidthMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc1_Out, modulationDestination::kOsc2_PW, kSynthToOscOscPulseWidthMod);/* special priority modulator */

	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kOsc1_PW, kSynthToOscOscPulseWidthMod);/* special priority modulator */
	registerModulatorArrayIndex(modulationSource::kOsc2_Out, modulationDestination::kOsc2_PW, kSynthToOscOscPulseWidthMod);/* special priority modulator */

	// --- insert delay FX
	//registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDelayFX_FB, kDelayFX_FeedbackMod);
	//registerModulatorArrayIndex(modulationSource::kNoneDontCare, modulationDestination::kDelayFX_Mix, kDelayFX_MixMod);
	// ------------------------------------------------------------------------------------------------------------

	// --- final validation
	validComponent = validateComponent();

	// --- set the output EG
	outputEG->setIsOutputEG(true);

	// --- setup the default modulations
	setFixedModulationRoutings();

	// --- setup update granularity; could do this at Engine level, but maybe in future with an engine that mixed 
	//     various voice objects, we might want individual granularity control
	updateGranularity = 64;
	granularityCounter = -1;

	// --- setup the portamento (glide) LFO
	glideLFO->getModifiers()->oscMode = LFOMode::kOneShot;
}

/** delete all sub-component objects */
SynthVoice::~SynthVoice()
{
	if (osc1) delete osc1;
	if (osc2) delete osc2;
	if (subOsc) delete subOsc; 
	if (lfo1) delete lfo1;
	if (lfo2) delete lfo2;
	if (glideLFO) delete glideLFO;
	if (outputEG) delete outputEG;
	if (eg2) delete eg2;
	if (filter1) delete filter1;
	if (filter2) delete filter2;
	if (outputDCA) delete outputDCA;
//	if (insertDelayFX) delete insertDelayFX;

	// --- database
	modSourceComponents.clear();
	modDestinationComponents.clear();
	modulatorArrayIndexes.clear();
	modSourceOutputArrayIndexes.clear();
}

/**
	\brief Initialize component with sample-rate dependent parameters
	\param info -- initialization information including sample rate
	\return true if handled, false if not handled
*/
bool SynthVoice::initializeComponent(InitializeInfo& info)
{
	if (!validateComponent()) return false;

	osc1->initializeComponent(info);
	osc2->initializeComponent(info);
	subOsc->initializeComponent(info); 
	outputEG->initializeComponent(info);
	eg2->initializeComponent(info);
	lfo1->initializeComponent(info);
	lfo2->initializeComponent(info);
	glideLFO->initializeComponent(info);
	filter1->initializeComponent(info);
	filter2->initializeComponent(info);
	outputDCA->initializeComponent(info);
	//insertDelayFX->initializeComponent(info); // creates delay buffers

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool SynthVoice::startComponent()
{
	voiceRunning = true;
	granularityCounter = -1;
	return true;
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool SynthVoice::stopComponent()
{
	// --- stop all sub-components; check for re-trigger if stealing this voice
	if (voiceRunning)
	{
		osc1->stopComponent();
		osc2->stopComponent();
		subOsc->stopComponent();
		lfo1->stopComponent();
		lfo2->stopComponent();
		glideLFO->stopComponent();
		filter1->stopComponent();
		filter2->stopComponent();
		outputEG->stopComponent();
		eg2->stopComponent();
		outputDCA->stopComponent();
	//	insertDelayFX->stopComponent();

		clearTimestamp();

		// --- do we have a note pending from being stolen?
		if (midiNoteData[kPendingMIDINoteNumber] >= 0 && midiNoteData[kPendingMIDINoteVelocity] >= 0)
		{
			// --- start new note (non-stolen now)
			return doNoteOn(midiNoteData[kPendingMIDINoteNumber], midiNoteData[kPendingMIDINoteVelocity], false, unisonVoiceMode);
		}
		else
		{
			voiceRunning = false;

			midiNoteData[kMIDINoteNumber] = 0;
			midiNoteData[kMIDINoteVelocity] = 0;

			// --- clear all routings
			removeModRoutings();
		}

		return true;
	}
	return false; // component wasn't running
}

/**
	\brief Reset the component to a note-off state
	\return true if handled, false if not handled
*/
bool SynthVoice::resetComponent()
{
	osc1->resetComponent();
	osc2->resetComponent();
	subOsc->resetComponent();	
	lfo1->resetComponent();
	lfo2->resetComponent();
	glideLFO->resetComponent();
	filter1->resetComponent();
	filter2->resetComponent();
	outputEG->resetComponent();
	eg2->resetComponent();
	outputDCA->resetComponent();
//	insertDelayFX->resetComponent();

	return true;
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.
	
	\return true if handled, false if not handled
*/
bool SynthVoice::validateComponent()
{
	// --- sub-components
	if (osc1 && osc2 && subOsc && outputEG && eg2 && lfo1 && lfo2 && filter1 && filter2 && outputDCA && glideLFO)// && insertDelayFX)
	{
		// --- shared pointers and modifiers
		if (modifiers && midiData)
		{
			// --- test for modulators
			for (unsigned int i = 0; i < numModulators; i++)
			{
				if (!modulators[i])
					return false;
			}

			// --- test for outputs
			for (unsigned int i = 0; i < numOutputs; i++)
			{
				if (!getOutputPtr(i))
					return false;
			}
			return true;
		}
	}
	return false;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data;
	NOTE: this update function is not called in the MicroSynth product, but the function may be used in the future
	\return true if handled, false if not handled
*/
bool SynthVoice::updateComponent()
{
	return false; // not handled
}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool SynthVoice::renderComponent(bool update)
{
	if (!validComponent || !voiceRunning) return false;

	// --- setup granularity of updates - can make an enormous difference in polyphony!
	bool updateComponents = needsComponentUpdate();
	
	// --- check voice done; shut off if it is finished, and return
	if (isVoiceDone())
		return false;

	// --- check for modulation routing change; this should only happen sporadically
	if (updateComponents)
	{
		// --- do a fast memory block compare
		if (memcmp(&modulationRoutings[0], &modifiers->modulationRoutings[0], sizeof(modulationRoutings)) != 0)
		{
			// --- re-do the mod routings
			updateModRoutings();
		}
	}

	// --- render voice
	//
	// --- render all things that can modulate
	lfo1->renderComponent(updateComponents);
	lfo2->renderComponent(updateComponents);
	glideLFO->renderComponent(updateComponents);

	outputEG->renderComponent(updateComponents);
	eg2->renderComponent(updateComponents);

	// --- oscillators can be modulators also
	osc1->renderComponent(updateComponents);
	osc2->renderComponent(updateComponents);
	subOsc->renderComponent(updateComponents);
	
	// --- render the audio engine
	//     form the sum of the two outputs, i stereo from this point on
	outputs[kVoiceLeftOutput] = osc1->getOutputValue(kLeftOscOutputWithAmpGain) + osc2->getOutputValue(kLeftOscOutputWithAmpGain) + subOsc->getOutputValue(kLeftOscOutputWithAmpGain);
	outputs[kVoiceRightOutput] = osc1->getOutputValue(kRightOscOutputWithAmpGain) + osc2->getOutputValue(kRightOscOutputWithAmpGain) + subOsc->getOutputValue(kRightOscOutputWithAmpGain);

	// --- setup for the filter -> DCA render into buffers
	RenderInfo processAudioInfo;

	// --- set update flag for audio processors too
	processAudioInfo.updateComponent = updateComponents;

	// --- filter and DCA will processes audio "in-place" = input and output buffers are same: writes output over input data
	//
	//     Can do this for fastest processing and not requiring intermediate arrays because filter and DCA are IN SERIES
	//     Parallel operations require the intermediate arrays
	processAudioInfo.inputData = &outputs[0];
	processAudioInfo.outputData = &outputs[0];
	processAudioInfo.numInputChannels = kNumVoiceOutputs;
	processAudioInfo.numOutputChannels = kNumVoiceOutputs;

	// --- filter processes audio in-place
	if( modifiers->enableHPF )
		filter2->processAudio(processAudioInfo);
	if( modifiers->enableLPF )
		filter1->processAudio(processAudioInfo);
	
	// --- Delay FX processes audio in-place
	// insertDelayFX->processAudio(processAudioInfo);

	// --- DCA processes audio in-place
	outputDCA->processAudio(processAudioInfo);

	return true;
}

/**
	\brief Perform operations for voice-stealing operation
	\return true if handled, false if not handled
*/
bool SynthVoice::shutDownComponent()
{
	// --- place all outputEG devices into shutdown mode
	outputEG->shutDownComponent();

	return true;
}

/**
	\brief Begin a new note operation, or steal the voice if needed; called by the SynthEngine

	\param midiNoteNumber the note number of the key that was depressed
	\param midiNoteVelocity the note velocity
	\param stealVoice flag to steal the voice if needed
	\param _unisonVoiceMode flag to enable unison mode, which uses panning and detuning of multiple voices to achieve the effect

	\return true if handled, false if not
*/
bool SynthVoice::doNoteOn(uint32_t midiNoteNumber, uint32_t midiNoteVelocity, bool stealVoice, bool _unisonVoiceMode)
{
	// --- save unison flag
	unisonVoiceMode = _unisonVoiceMode;

	double zero = 0.0;
	if (!unisonVoiceMode)
	{
		setVoicePanValue(zero);
		setUnisonDetuneIntensity(zero);
	}

	// --- if we are stealing this voice, save the data and go to shutdown mode
	if (stealVoice)
	{
		// --- save note information
		midiNoteData[kPendingMIDINoteNumber] = midiNoteNumber;
		midiNoteData[kPendingMIDINoteVelocity] = midiNoteVelocity;

		// --- place the component in shutdown mode
		return shutDownComponent();
	}
	else
	{
		// --- save note information
		midiNoteData[kMIDINoteNumber] = midiNoteNumber;
		midiNoteData[kMIDINoteVelocity] = midiNoteVelocity;

		midiNoteData[kPendingMIDINoteNumber] = -1;
		midiNoteData[kPendingMIDINoteVelocity] = -1;
	}

	// --- lookup pitch value
	double midiPitch = midiFreqTable[midiNoteNumber];

	// --- we are running
	startComponent();

	// --- set octave = -1, then call doNoteOn() in SynthVoice::doNoteOn this is done BEFORE the portamento code chunk below to make sure the glide works correctly
	subOsc->getModifiers()->octave = -1; // set one octave down

	// --- setup the glide LFO if portamento is enabled
	if (modifiers->enablePortamento && midiData->getMidiGlobalData(kLastMIDINoteNumber) < 128 && modifiers->portamentoTime_mSec > 0.0)
	{
		// --- setup glideLFO
		double lastMIDIPitch = midiFreqTable[midiData->getMidiGlobalData(kLastMIDINoteNumber)];

		// --- semitones between lasst pitch and current pitch
		double glideRange = semitonesBetweenFrequencies(midiPitch, lastMIDIPitch);

		// --- set the range on the modulators for the correct glide to/from values
		osc1->getModifiers()->modulationControls[kSynthOscPortamentoMod].modulationRange = glideRange;
		osc2->getModifiers()->modulationControls[kSynthOscPortamentoMod].modulationRange = glideRange;
		subOsc->getModifiers()->modulationControls[kSynthOscPortamentoMod].modulationRange = glideRange;

		// --- glide fo = 1/time
		glideLFO->getModifiers()->oscFreqControl = 1.0 / (modifiers->portamentoTime_mSec / 1000.0);

		// --- trigger note on
		glideLFO->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	}

	// --- call noteOn() on all sub-components
	osc1->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	osc2->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	subOsc->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);

	lfo1->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	lfo2->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);

	if( modifiers->enableHPF )
		filter2->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	if( modifiers->enableLPF )
		filter1->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);

	outputEG->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	eg2->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);

	outputDCA->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
//	insertDelayFX->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);

	return true;
}

/**
	\brief Perform the note off operation on the voice
	- send egs into note-off mode
	- NOTE this does not shutdown the voice; wait until OUTPUT EG has expired (if there is one)

	\param midiNoteNumber the note number of the key that was depressed
	\param midiNoteVelocity the note velocity
	\return true if handled, false if not
*/  
bool SynthVoice::doNoteOff(uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	double midiPitch = midiFreqTable[midiNoteNumber];

	// --- send the EGs into release mode
	osc1->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	osc2->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	subOsc->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	lfo1->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	lfo2->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	glideLFO->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	filter1->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	filter2->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	outputEG->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	eg2->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	outputDCA->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
//	insertDelayFX->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);

	return true;
}

/**
\brief update the programmable modulation routing; this should only be called if the routings have changed
*/
void SynthVoice::updateModRoutings()
{
	for (unsigned int i = 0; i < MAX_MOD_ROUTINGS; i++)
	{
		if (!(modifiers->modulationRoutings[i] == modulationRoutings[i]))
		{
			// --- remove the routing for this slot
			ISynthComponent* destComponent = getModDestComponent(modulationRoutings[i].modDest);
			int32_t modulatorIndex = getModulatorIndex(modulationRoutings[i].modSource, modulationRoutings[i].modDest);

			if (destComponent && modulatorIndex >= 0)
			{
				destComponent->getModulator(modulatorIndex)->removeModulationRouting(i);
			}
			
			// --- add the new routing for this slot, set the row (slot) with the index i
			//
			//     Get routing from *modifiers*
			ISynthComponent* sourceComponent = getModSourceComponent(modifiers->modulationRoutings[i].modSource);
			int32_t sourceOutputArrayIndex = getModSourceOutputArrayIndex(modifiers->modulationRoutings[i].modSource);
			destComponent = getModDestComponent(modifiers->modulationRoutings[i].modDest);
			modulatorIndex = getModulatorIndex(modifiers->modulationRoutings[i].modSource, modifiers->modulationRoutings[i].modDest);
			
			ModulatorControl* controls = &modifiers->progModulationControls[i];

			if (sourceComponent && destComponent && modulatorIndex >= 0)
			{
				// --- add the routing, note use of i == matrix row for easy location
				destComponent->getModulator(modulatorIndex)->addModulationRouting(sourceComponent->getOutputPtr(sourceOutputArrayIndex), controls, i);
			}
		}
	}

	// --- bulk memory block copy FROM modifiers TO our storage array
	memcpy(&modulationRoutings[0], &modifiers->modulationRoutings[0], sizeof(modulationRoutings));

}


/**
	\brief Add modulation instructions to the modulation matrix (a vector of instructions); each instruction codes a unique modulation routing
*/
void SynthVoice::setFixedModulationRoutings()
{
	if (!validComponent) return;

	// --- SETUP FIXED (always) routings
	//
	// --- EG1 OUTPUT --> DCA AMP MOD
	addModulationRouting(outputEG, kEGNormalOutput, outputDCA, kDCA_AmpMod);

	// --- EG2 --> Filter Fc Mod
	addModulationRouting(eg2, kEGNormalOutput, filter1, kVALadderFilterFcMod);

	// --- glideLFO OUTPUT --> osc1 portamento modulator
	addModulationRouting(glideLFO, kLFOUnipolarDownRamp, osc1, kSynthOscPortamentoMod);
	addModulationRouting(glideLFO, kLFOUnipolarDownRamp, osc2, kSynthOscPortamentoMod);

	// --- because it is an oscillator, we want it to track the other oscillator's glide curve
	addModulationRouting(glideLFO, kLFOUnipolarDownRamp, subOsc, kSynthOscPortamentoMod);

	// --- add more FIXED routings here...
}


/**
	\brief Set the unison detune intensity value for each sub-component oscillator

	\param unisonDetuneIntensity the intensity from [0, +1]
*/
void SynthVoice::setUnisonDetuneIntensity(double& unisonDetuneIntensity)
{
	if (!validComponent) return;

	// --- set detune intensity of all pitched oscillators
	osc1->setUnisonDetuneIntensity(unisonDetuneIntensity);
	osc2->setUnisonDetuneIntensity(unisonDetuneIntensity);

	// --- NOT setting the sub to detune during Unison Mode - it would sound funny
	// NO NO NO subOsc->setUnisonDetuneIntensity(unisonDetuneIntensity);

}

/**
	\brief Set DCA pan value; called for unison mode and also from MIDI pan modulation

	\param panValue the new pan value from [-1, +1]
*/
void SynthVoice::setVoicePanValue(double& panValue)
{
	if (!validComponent) return;

	// --- set pan value [-1, +1]
	outputDCA->setPanValue(panValue);
}












