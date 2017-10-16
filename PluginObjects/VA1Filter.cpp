#include "VA1Filter.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
VA1Filter::VA1Filter(std::shared_ptr<VA1FilterModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthAudioProcessor(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	// --- calculate the range of mod frequencies in semi-tones
	//
	//     the /2.0 is because we have bipolar modulation, so this is really
	//     a modulation range of +/- filterModulationRange in semitones
	filterModulationRange = semitonesBetweenFrequencies(kMinFilter_fc, kMaxFilter_fc) / 2.0;

	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kVA1Filter;

	// --- create modulators
	modulators[kVA1FilterFcMod] = new Modulator(kDefaultOutputValueOFF, filterModulationRange, modTransform::kNoTransform);

	// --- validate all pointers
	validComponent = validateComponent();
}

/** Destructor: delete output array and modulators */
VA1Filter::~VA1Filter()
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
bool VA1Filter::initializeComponent(InitializeInfo& info)
{
	// --- retain sample rate for update calculations
	sampleRate = info.sampleRate;

	// --- reset internals
	resetComponent();

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool VA1Filter::startComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- reset/flush registers
	resetComponent();

	// --- set our flag
	noteOn = true;

	return true;
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool VA1Filter::stopComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear our flag
	noteOn = false;

	return true;
}

/**
	\brief Reset the component to a note-off state
	\return true if handled, false if not handled
*/
bool VA1Filter::resetComponent()
{
	// --- flush delay with memset()
	memset(&z1[0],								/* pointer to top of array*/
		0,								/* memset value = 0.0 */
		maxChannels * sizeof(double));	/* size of the array IN BYTES = channels * (#bytes/double) */
										// --- clear modulator inputs
	return true; // handled
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.

	\return true if handled, false if not handled
*/
bool VA1Filter::validateComponent()
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
bool VA1Filter::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save for keytrack
	midiNotePitch = midiPitch;

	// --- start up
	startComponent();

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool VA1Filter::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- we ignore note off
	return true;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool VA1Filter::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	 // --- save filter
	filter = modifiers->filter;

	// --- caculate fc based on fcControl modulated by fcModulator in semitones
	double newFilterFc = 0.0;

	if(modifiers->enableKeyTrack)
		newFilterFc = midiNotePitch * modifiers->keytrackRatio * pitchShiftTableLookup(modulators[kVA1FilterFcMod]->getModulatedValue());
	else
		newFilterFc = modifiers->fcControl * pitchShiftTableLookup(modulators[kVA1FilterFcMod]->getModulatedValue());

	// --- only update if variable has changed to prevent tan() function calls
	if (!variableChanged(newFilterFc, filter_fc))
		return false; // not updated
	
	// --- update
	filter_fc = newFilterFc;

	// --- limit in case fc control is biased
	boundValue(filter_fc, kMinFilter_fc, kMaxFilter_fc);

	// --- prewarp
	double wd = 2.0 * pi*filter_fc;
	double T = 1.0 / sampleRate;
	double wa = (2.0 / T)*tan(wd*T / 2);
	vaFilter_g = wa*T / 2.0;

	// --- calculate alpha
	alpha = vaFilter_g / (1.0 + vaFilter_g);

	return true;
}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool VA1Filter::renderComponent(bool update)
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
bool VA1Filter::processAudio(RenderInfo& renderInfo)
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

	// --- process left channel (must have it)
	doFilter(filter, renderInfo.inputData[0], CHANNEL_0);

	// --- process right channel if we have one
	if (renderInfo.numInputChannels == 2)
		doFilter(modifiers->filter, renderInfo.inputData[1], CHANNEL_1);

	// --- check for internal render only
	if (renderInfo.renderInternal)
		return true;

	// --- render output
	renderInfo.outputData[0] = outputs[kVA1FilterOutput_0];

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
		renderInfo.outputData[1] = outputs[kVA1FilterOutput_1];
	}

	return true;
}

/**
	\brief Perform the filtering operation on the input data

	\param filter the type of filter to be implemented
	\param input the audio input sample
	\param channel the channel (0 or 1) for the filter

	\return the filter's output sample
*/
double VA1Filter::doFilter(filterType filter, double input, unsigned int channel)
{
	// --- create vn node
	double vn = (input - z1[channel])*alpha;

	// --- form LP output
	double lpf = vn + z1[channel];

	// --- update memory
	z1[channel] = vn + lpf;

	// --- form the HPF = INPUT = LPF
	double hpf = input - lpf;

	// --- form the APF = LPF - HPF
	double apf = lpf - hpf;

	// --- set the outputs
	if (filter == filterType::kLPF1)
		outputs[channel] = lpf;
	else if (filter == filterType::kHPF1)
		outputs[channel] = hpf;
	else if (filter == filterType::kAPF1)
		outputs[channel] = apf;

	return outputs[channel];
}




