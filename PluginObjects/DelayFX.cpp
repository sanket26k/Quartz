#include "DelayFX.h"

/**
\brief constructor: null and reset
*/
DelayLine::DelayLine(void)
{
	// --- zero everything
	buffer = NULL;
	delay_ms = 0.0;
	delayInSamples = 0.0;
	sampleRate = 0;

	// --- reset
	resetDelay();
}

/**
\brief destructor: delete the delay line
*/
DelayLine::~DelayLine(void)
{
	if (buffer)
		delete buffer;

	buffer = NULL;
}

/**
\brief create and initialize the delay line (buffer of doubles)

\param delayLengthInSamples: the MAX length of the delay line
*/
void DelayLine::init(int delayLengthInSamples)
{
	// --- save for later
	bufferLength = delayLengthInSamples;

	// --- delete if existing
	if (buffer)
		delete buffer;

	// --- create
	buffer = new double[bufferLength];

	// --- flush buffer
	memset(buffer, 0, bufferLength * sizeof(double));
}

/**
\brief reset the index variables and flush the delay line
*/
void DelayLine::resetDelay()
{
	// --- flush buffer
	if (buffer)
		memset(buffer, 0, bufferLength * sizeof(double));

	// --- init read/write indices
	writeIndex = 0;
	readIndex = 0;

	// --- cook
	cookVariables();
}

/**
\brief set the delay time for fixed delay algorithms

\param _delay_mSec: the new fixed delay time in mSec
*/
void DelayLine::setDelay_mSec(double _delay_mSec)
{
	delay_ms = _delay_mSec;
	cookVariables();
}

/**
\brief update the variables based on the current delay settings
*/
void DelayLine::cookVariables()
{
	// --- calculate fractional delay
	delayInSamples = delay_ms*((double)sampleRate / 1000.0);

	// --- subtract to make read index
	readIndex = writeIndex - (int)delayInSamples;

	// --- the check and wrap BACKWARDS if the index is negative
	if (readIndex < 0)
		readIndex += bufferLength;	// amount of wrap is Read + Length
}

/**
\brief  read delay at the prescribed (fixed) delay value
*/
double DelayLine::readDelay()
{
	// --- Read the output of the delay at readIndex
	double yn = buffer[readIndex];

	// --- Read the location ONE BEHIND yn at y(n-1)
	int nReadIndex_1 = readIndex - 1;
	if (nReadIndex_1 < 0)
		nReadIndex_1 = bufferLength - 1; // bufferLength-1 is last location

	// --- get y(n-1)
	double yn_1 = buffer[nReadIndex_1];

	// --- get fractional component
	double dFracDelay = delayInSamples - (int)delayInSamples;

	// --- interpolate: (0, yn) and (1, yn_1) by the amount fracDelay
	return doLinearInterpolation(0, 1, yn, yn_1, dFracDelay); // interp frac between them
}

/**
\brief  read delay from an arbitrary location given in mSec

\param _delay_mSec: the location away from the write index value to read the delay line; for non-fixed delay algorithms - note this does not increment any index variables
*/
double DelayLine::readDelayAt(double _delay_mSec)
{
	// --- local variales
	double dDelayInSamples = _delay_mSec*((float)sampleRate) / 1000.0;

	// --- subtract to make read index
	int nReadIndex = writeIndex - (int)dDelayInSamples;

	// --- wrap if needed
	if (nReadIndex < 0)
		nReadIndex += bufferLength;	// amount of wrap is Read + Length

	//---  Read the output of the delay at readIndex
	double yn = buffer[nReadIndex];
	return yn;

	// --- Read the location ONE BEHIND yn at y(n-1)
	int nReadIndex_1 = nReadIndex - 1;
	if (nReadIndex_1 < 0)
		nReadIndex_1 = bufferLength - 1; // bufferLength-1 is last location

										 // -- get y(n-1)
	double yn_1 = buffer[nReadIndex_1];

	// --- get the fractional component
	double dFracDelay = dDelayInSamples - (int)dDelayInSamples;

	// --- interpolate: (0, yn) and (1, yn_1) by the amount fracDelay
	return doLinearInterpolation(0, 1, yn, yn_1, dFracDelay); // interp frac between them

}

/**
\brief  write input sample into the delay line and increment the index variables

\param inputSample: the curent input sample
*/
void DelayLine::writeDelayAndInc(double inputSample)
{
	// --- write to the delay line
	buffer[writeIndex] = inputSample; // external feedback sample

	// --- increment the pointers and wrap if necessary
	writeIndex++;
	if (writeIndex >= bufferLength)
		writeIndex = 0;

	readIndex++;
	if (readIndex >= bufferLength)
		readIndex = 0;
}

bool DelayLine::processAudio(double* input, double* output)
{
	// read delayed output
	*output = delayInSamples == 0 ? *input : readDelay();

	// write to the delay line
	writeDelayAndInc(*input);

	return true; // all OK
}

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
DelayFX::DelayFX(std::shared_ptr<DelayFXModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthAudioProcessor(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kFXProcessor;

	// --- create modulators
	//
	// --- delay feedback
	modulators[kDelayFX_FeedbackMod] = new Modulator(kDefaultOutputValueOFF, kDelayFX_Feedback_ModRange, modTransform::kNoTransform);
	
	// --- delay mix
	modulators[kDelayFX_MixMod] = new Modulator(kDefaultOutputValueOFF, kDelayFX_Mix_ModRange, modTransform::kNoTransform);

	// --- chorus depth
	modulators[kChorusFX_DepthMod] = new Modulator(kDefaultOutputValueOFF, kChorusFX_Depth_ModRange, modTransform::kNoTransform);

	// --- lfo
	lfo = new LFO(lfoModifiers, _midiData, kNumLFOOutputs, kNumLFOModulators);
	lfo->getModifiers()->oscWave = LFOWaveform::kSin;

	// --- validate all pointers
	validComponent = validateComponent();
}

/** Destructor: delete output array and modulators */
DelayFX::~DelayFX()
{
	// --- delete our arrays and nullify
	if (outputs) delete[] outputs;
	if (modulators)delete[] modulators;
	if (lfo) delete lfo;

	outputs = nullptr;
	modulators = nullptr;
}

/**
	\brief Initialize component with sample-rate dependent parameters
	\param info -- initialization information including sample rate
	\return true if handled, false if not handled
*/
bool DelayFX::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- set sample rate first
	leftDelay.setSampleRate(info.sampleRate);
	rightDelay.setSampleRate(info.sampleRate);

	// --- initialize to 2 sec max delay
	leftDelay.init(2.0*info.sampleRate);
	rightDelay.init(2.0*info.sampleRate);

	// --- init the LFO
	lfo->initializeComponent(info);

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool DelayFX::startComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save flag
	noteOn = true;
	lfo->startComponent();

	return true; // nothing to do
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool DelayFX::stopComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear flag
	noteOn = false;

	return true; // nothing to do
}

/**
	\brief Reset the component to a note-off state
	\return true if handled, false if not handled
*/
bool DelayFX::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear the outputs
	clearOutputs();

	// --- flush buffers
	leftDelay.resetDelay();
	rightDelay.resetDelay();

	// --- reset lfo
	lfo->resetComponent();

	return true;
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.
	\return true if handled, false if not handled
*/
bool DelayFX::validateComponent()
{
	// --- shared pointers and modifiers
	if (modifiers && midiData && lfo)
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
bool DelayFX::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- modulation idea: use midi pitch to tune length of the delay 
	resetComponent();

	// --- start up
	startComponent();

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool DelayFX::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	return true;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool DelayFX::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	enabled = modifiers->enabled;
	chorusRate_Hz = modifiers->chorusRate_Hz;
	
	// --- chorus depth modulation
	chorusDepth_Pct = modifiers->chorusDepth_Pct + modulators[kChorusFX_DepthMod]->getModulatedValue();
	
	// --- always bound modulated values!!
	boundValue(chorusDepth_Pct, kChorusFX_Depth_Min, kChorusFX_Depth_Max);

	delayFXMode = modifiers->delayFXMode;

	// --- set the LFO Frequency & Depth (Amplitude)
	lfo->getModifiers()->oscFreqControl = chorusRate_Hz;
	lfo->updateComponent();
	if (delayFXMode == delayFXMode::chorus)
		return true;

	if (modifiers->delayRatio < 0)
	{
		// --- note negation of ratio!
		leftDelay.setDelay_mSec(-modifiers->delayRatio*modifiers->delayTime_mSec);
		rightDelay.setDelay_mSec(modifiers->delayTime_mSec);
	}
	else if (modifiers->delayRatio > 0)
	{
		leftDelay.setDelay_mSec(modifiers->delayTime_mSec);
		rightDelay.setDelay_mSec(modifiers->delayRatio*modifiers->delayTime_mSec);
	}
	else
	{
		leftDelay.setDelay_mSec(modifiers->delayTime_mSec);
		rightDelay.setDelay_mSec(modifiers->delayTime_mSec);
	}

	// --- add feedback modulation (+/- 50%)
	feedback_Pct = modifiers->feedback_Pct + modulators[kDelayFX_FeedbackMod]->getModulatedValue();
	
	// --- always bound modulated values!!
	boundValue(feedback_Pct, kDelayFX_Feedback_Min, kDelayFX_Feedback_Max);

	// --- add mix (wet/dry) modulation
	delayMix_Pct = modifiers->delayMix_Pct + modulators[kDelayFX_MixMod]->getModulatedValue();

	// --- always bound modulated values!!
	boundValue(feedback_Pct, kDelayFX_Mix_Min, kDelayFX_Mix_Max);

	// --- save others
	delayTime_mSec = modifiers->delayTime_mSec;
	delayRatio = modifiers->delayRatio;

	return true; // handled
}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool DelayFX::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- sub-component
	lfo->renderComponent(update);

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
bool DelayFX::processAudio(RenderInfo& renderInfo)
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

	// --- pass through if not enabled
	if (!enabled)
	{
		// --- process left channel (must have it)
		renderInfo.outputData[0] = renderInfo.inputData[0];

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
			renderInfo.outputData[1] = renderInfo.inputData[1];
		}
		return true;
	}

	// --- rendering into output array
	processDelayFX(renderInfo);

	// --- if rendering internal, we're done!
	if (renderInfo.renderInternal)
		return true;

	// --- process left channel (must have it)
	renderInfo.outputData[0] = outputs[kDelayFXLeftOutput];

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
		renderInfo.outputData[1] = outputs[kDelayFXRightOutput];
	}
	return true;
}

// --- process into internal output buffers
// cross:
//		Left Input -> Left Delay; Left Feedback -> Right Delay
//		Right Input -> Right Delay; Right Feedback -> Left Delay
//
// pingpong
//		Left Input -> Right Delay; Left Feedback -> Right Delay
//		Right Input -> Left Delay; Right Feedback -> Left Delay
//
void DelayFX::processDelayFX(RenderInfo& renderInfo)
{
	// --- chorus is twin QP variety
	if (delayFXMode == delayFXMode::chorus)
	{
		// --- render the LFO sample
		lfo->renderComponent(true);

		// --- get normal and quad phase outputs
		double lfoOut = lfo->getOutputValue(kLFONormalOutput);
		double lfoOutQP = lfo->getOutputValue(kLFOQuadPhaseOutput);

		// --- left channel uses normal LFO
		double modValue_mSec = doBipolarModulation(lfoOut, kMinChorusDelay_mSec, kMaxChorusDelay_mSec);

		// --- read delay 
		double delay = leftDelay.readDelayAt(modValue_mSec);
		
		// --- form output
		outputs[kDelayFXLeftOutput] = renderInfo.inputData[0] + (chorusDepth_Pct / 100.0)*0.5*delay;

		// --- write delay
		leftDelay.writeDelayAndInc(renderInfo.inputData[0]);

		if (renderInfo.numInputChannels == 1 && renderInfo.numOutputChannels == 2)
		{
			// --- copy left to right channel
			outputs[kDelayFXRightOutput] = outputs[kDelayFXLeftOutput];
		}
		else if (renderInfo.numInputChannels == 2 && renderInfo.numOutputChannels == 2)
		{
			// --- right channel uses QP output
			double modValue_mSec = doBipolarModulation(lfoOutQP, kMinChorusDelay_mSec, kMaxChorusDelay_mSec);

			// --- read delay
			double delay = rightDelay.readDelayAt(modValue_mSec);

			// --- form output
			outputs[kDelayFXRightOutput] = renderInfo.inputData[1] + (chorusDepth_Pct / 100.0)*0.5*delay;

			// --- write input to delay line
			rightDelay.writeDelayAndInc(renderInfo.inputData[1]);
		}

		return;
	}

	// --- normal delay stuff
	double leftDelayOut = leftDelay.readDelay();
	double rightDelayOut = rightDelay.readDelay();

	double leftDelayIn = 0.0;
	double rightDelayIn = 0.0;

	// --- SETUP for NORMAL operation
	if (delayFXMode == delayFXMode::norm)
		leftDelayIn = renderInfo.inputData[0] + leftDelayOut*(feedback_Pct / 100.0);
	else if (delayFXMode == delayFXMode::cross)
		leftDelayIn = renderInfo.inputData[0] + rightDelayOut*(feedback_Pct / 100.0);
	else if (delayFXMode == delayFXMode::pingpong)
		leftDelayIn = renderInfo.inputData[1] + rightDelayOut*(feedback_Pct / 100.0);

	// --- mono -> stereo
	if (renderInfo.numInputChannels == 1 && renderInfo.numOutputChannels == 2)
	{
		// --- copy left to right channel
		rightDelayIn = leftDelayIn;
	}
	// --- stereo --> stereo
	else if (renderInfo.numInputChannels == 2 && renderInfo.numOutputChannels == 2)
	{
		// --- setup right channel
		if (delayFXMode == delayFXMode::norm)
			rightDelayIn = renderInfo.inputData[1] + rightDelayOut*(feedback_Pct / 100.0);
		else if (delayFXMode == delayFXMode::cross)
			rightDelayIn = renderInfo.inputData[1] + leftDelayOut*(feedback_Pct / 100.0);
		else if (delayFXMode == delayFXMode::pingpong)
			rightDelayIn = renderInfo.inputData[0] + leftDelayOut*(feedback_Pct / 100.0);
	}

	// --- intermediate variables
	double leftOut = 0.0;
	double rightOut = 0.0;

	// --- do the delay lines
	leftDelay.processAudio(&leftDelayIn, &leftOut);
	rightDelay.processAudio(&rightDelayIn, &rightOut);

	double wet = (delayMix_Pct / 100.0);
	double dry = 1.0 - wet;

	// --- form outputs
	outputs[kDelayFXLeftOutput] = dry*renderInfo.inputData[0] + wet*leftOut;

	if (renderInfo.numOutputChannels == 2)
		outputs[kDelayFXRightOutput] = dry*renderInfo.inputData[1] + wet*rightOut;
	else
		outputs[kDelayFXRightOutput] = outputs[kDelayFXLeftOutput];
}
