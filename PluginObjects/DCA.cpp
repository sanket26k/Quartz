	#include "DCA.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
DCA::DCA(std::shared_ptr<DCAModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthAudioProcessor(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kDCA;

	// --- create modulators 
	modulators[kDCA_AmpMod] = new Modulator(kDefaultOutputValueON, kDCA_Amp_ModRange, modTransform::kAlwaysPositiveTransform);

	modulators[kDCA_MaxDownAmpMod] = new Modulator(kDefaultOutputValueON, kDCA_Amp_ModRange, modTransform::kMaxDownTransform);

	modulators[kDCA_PanMod] = new Modulator(kDefaultOutputValueOFF, kDCA_Pan_ModRange, modTransform::kNoTransform);

	// --- validate all pointers
	validComponent = validateComponent();
}

/** Destructor: delete output array and modulators */
DCA::~DCA()
{
	// --- delete our arrays and nullify
	if (outputs) delete[] outputs;
	if (modulators)delete[] modulators;

	outputs = nullptr;
	modulators = nullptr;
}

/** 
	\brief Initialize component with sample-rate dependent parameters
	\param info -- initialization information including sample rate
	\return true if handled, false if not handled
*/
bool DCA::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- nothing to do for this object
	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool DCA::startComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save flag
	noteOn = true;

	return true; // nothing to do
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool DCA::stopComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- recenter this, it may hae neen changed during a unison mode voice event
	panValue = 0.0;

	// --- clear flag
	noteOn = false;

	return true; // nothing to do
}

/**
	\brief Reset the component to a note-off state
	\return true if handled, false if not handled
*/
bool DCA::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	gainRaw = 1.0;			// --- unity
	panLeftGain = 0.707;	// --- center
	panRightGain = 0.707;	// --- center
	
	// --- clear the outputs
	clearOutputs();

	return true;
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array; 
	this function should be called once during construction to set the validComponent flag, which is used for future component validation. 
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.
	\return true if handled, false if not handled
*/
bool DCA::validateComponent()
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
	return false;
}

/**
	\brief Perform note-on operations for the component
	\return true if handled, false if not handled
*/
bool DCA::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- store our MIDI velocity
	midiVelocityGain = mmaMIDItoAtten(midiNoteVelocity);

	// --- start up
	startComponent();

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool DCA::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	return true;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool DCA::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- apply modulator 
	//double modGain_dB = doUnipolarModulationFromMax(ampMod, kMinModGain_dB, modifiers->gain_dB);
	double ampMod = doUnipolarModulationFromMax(modulators[kDCA_MaxDownAmpMod]->getModulatedValue(), 0.0, 1.0);

	// --- support for MIDI Volume CC
	double midiVolumeGain = mmaMIDItoAtten(midiData->getMidiCCData(VOLUME_CC07));

	// --- calculate the final raw gain value
	//gainRaw = midiVolumeGain * midiVelocityGain * egAmpMod * ampMod * pow(10.0, modGain_dB / 20.0);
	gainRaw = midiVolumeGain * midiVelocityGain * modulators[kDCA_AmpMod]->getModulatedValue() * ampMod * pow(10.0, modifiers->gain_dB / 20.0);

	// --- apply final output gain
	if (modifiers->gain_dB > kMinAbsoluteGain_dB) // note change from last project
		gainRaw *= pow(10.0, modifiers->gain_dB / 20.0);
	else
		gainRaw = 0.0; // OFF

	// --- is mute ON? 0 = OFF, 1 = ON
	if (modifiers->mute) gainRaw = 0.0;

	// --- now process pan modifiers
	double panTotal = panValue + modulators[kDCA_PanMod]->getModulatedValue();

	// --- limit in case pan control is biased
	boundValue(panTotal, -1.0, 1.0);

	// --- equal power calculation in synthfunction.h
	calculatePanValues(panTotal, panLeftGain, panRightGain);

	return true; // handled
}

/**
	\brief Render the component; 
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable 

	\return true if handled, false if not handled
*/
bool DCA::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- run the modulators
	runModuators(update);

	if (update)
		updateComponent();

	// --- any other render-only operations here...
	return true;
}

/**
	\brief Process audio from renderInfo.inputData to output array.

	\param renderInfo contains information about the processing including the flag renderInfo.renderInternal; if this is true, we process audio
	into the output buffer only, otherwise copy the output data into the renderInfo.outputData array

	\return true if handled, false if not handled
*/
bool DCA::processAudio(RenderInfo& renderInfo)
{
	// --- check render/update
	if (!renderComponent(renderInfo.updateComponent))
		return false;

	// --- always check for proper channel setup
	if (renderInfo.numInputChannels == 0 ||
		renderInfo.numInputChannels > 2 ||
		renderInfo.numOutputChannels == 0 ||
		renderInfo.numOutputChannels > 2)
		return false; // not handled

	// --- rendering into output array
	if (renderInfo.numInputChannels == 1 && renderInfo.numOutputChannels == 1)
		outputs[kDCALeftOutput] = renderInfo.inputData[0] * gainRaw;
	else
		outputs[kDCALeftOutput] = renderInfo.inputData[0] * panLeftGain * gainRaw;

	if (renderInfo.numInputChannels == 2 && renderInfo.numOutputChannels == 2)
		outputs[kDCARightOutput] = renderInfo.inputData[1] * panRightGain * gainRaw;

	// --- if rendering internal, we're done!
	if (renderInfo.renderInternal)
		return true;

	// --- process left channel (must have it)
	renderInfo.outputData[0] = outputs[kDCALeftOutput];

	// --- mono -> stereo
	if (renderInfo.numInputChannels == 1 && renderInfo.numOutputChannels == 2)
	{
		// --- copy left to right channel
		renderInfo.outputData[1] = renderInfo.outputData[0];
	}
	// --- stereo --> stereo
	else if (renderInfo.numInputChannels == 2 && renderInfo.numOutputChannels == 2)
	{
		// --- process right channel
		renderInfo.outputData[1] = outputs[kDCARightOutput];
	}
	return true;
}

