#include "lfo.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
LFO::LFO(std::shared_ptr<LFOModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthComponent(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	// --- seed the random number generator
	srand(time(NULL));

	// --- randomize the PN register
	pnRegister = rand();

	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kLFO;

	// --- create modulators
	modulators[kLFOFreqMod] = new Modulator(kDefaultOutputValueOFF, kLFO_Freq_ModRange, modTransform::kNoTransform);

	modulators[kLFOMaxDownAmpMod] = new Modulator(kDefaultOutputValueON, kLFO_Amp_ModRange, modTransform::kMaxDownTransform);

	modulators[kLFOPulseWidthMod] = new Modulator(kDefaultOutputValueOFF, kLFO_PW_ModRange, modTransform::kNoTransform);
	
	// --- validate all pointers
	validComponent = validateComponent();

}

/** Destructor: delete output array and modulators */
LFO::~LFO()
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
bool LFO::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- retain sample rate for update calculations
	sampleRate = info.sampleRate;

	// --- bulk reset
	resetComponent();

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool LFO::startComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- reset counters, etc...
	resetComponent();

	// --- set our flag
	noteOn = true;

	return true;
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool LFO::stopComponent()
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
bool LFO::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear the outputs
	clearOutputs();

	// --- reset modulo counter timebase unless free running, then just let it goooo
	if (modifiers->oscMode != LFOMode::kFreeRun)
	{
		modCounter = 0.0;
		phaseInc = 0.0;
		modCounterQP = 0.25f;
	}

	// --- reset random S&H flag/counter
	randomSHCounter = -1; // -1 = reset

	// --- reset run/stop flag
	noteOn = false;

	return true; // handled
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.

	\return true if handled, false if not handled
*/
bool LFO::validateComponent()
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
bool LFO::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- start LFO
	startComponent();

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool LFO::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
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
bool LFO::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save here for doOscillate() and safety when using granularity
	oscWave = modifiers->oscWave;
	oscMode = modifiers->oscMode;

	// --- if oscMaxDownAmpModulator is enabled, pull from its input
	double ampMod = modulators[kLFOMaxDownAmpMod]->getModulatedValue();

	// --- calculate the amplitude with control value * mod value
	oscAmplitude = ampMod * modifiers->oscAmpControl;

	// --- bound the amplitude to our range
	boundValue(oscAmplitude, kMinLFO_amp, kMaxLFO_amp);

	// --- caculate osc freq with control value modulated by modulator LINEARLY; just add the modulator*range
	oscFrequency = modifiers->oscFreqControl + modulators[kLFOFreqMod]->getModulatedValue();

	// --- bound the frequency to our range
	boundValue(oscFrequency, kMinLFO_fo, kMaxLFO_fo);

	// --- calcualate pulse width modulated by modulator LINEARLY; just add the modulator*range
	pulseWidth = (modifiers->pulseWidthControl_Pct / 100.0) + modulators[kLFOPulseWidthMod]->getModulatedValue();

	// --- bound the pulse width to our range [-1, +1]
	boundValue(pulseWidth, kMinLFO_PW, kMaxLFO_PW);

	// --- calcualte phase inc = fo/fs
	phaseInc = oscFrequency / sampleRate;

	return true; // handled

}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool LFO::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- run the modulators
	runModuators(update);

	// --- check update
	if (update)
		updateComponent();

	// --- render oscillators
	return doOscillate();
}

/**
	\brief Synthesize the trivial oscillator's output
	\return true if handled, false if not handled
*/
bool LFO::doOscillate()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- check run/stop flag
	if (!noteOn)
	{
		// --- set to 0 just to be safe/nice
		clearOutputs();
		return true;
	}

	// --- always first!
	bool wrapped = checkAndWrapModulo(modCounter, phaseInc);

	// --- one shot LFO?
	if (oscMode == LFOMode::kOneShot && wrapped)
	{
		// --- clear the outputs
		clearOutputs();

		// --- no more
		noteOn = false;

		// --- done
		return true;
	}

	// --- for QP output
	//
	// --- QP output always follows location of current modulo; first set equal
	modCounterQP = modCounter;

	// --- then, advance modulo by quadPhaseInc = 0.25 = 90 degrees, AND wrap if needed
	advanceAndCheckWrapModulo(modCounterQP, quadPhaseInc);

	// --- setup for switch/case statement
	int oscillatorWaveform = enumToInt(oscWave);

	// --- calculate the oscillator value
	switch (oscillatorWaveform)
	{
		case enumToInt(LFOWaveform::kSin):
		{
			// --- calculate normal angle
			double angle = modCounter*2.0*pi - pi;

			// --- norm output with parabolicSine approximation
			outputs[kLFONormalOutput] = parabolicSine(-angle);

			// --- calculate QP angle
			angle = modCounterQP*2.0*pi - pi;

			// --- calc QP outputs
			outputs[kLFOQuadPhaseOutput] = parabolicSine(-angle);

			break;
		}
		// --- can combine these
		case enumToInt(LFOWaveform::kUpSaw):
		case enumToInt(LFOWaveform::kDownSaw):
		{
			// --- one shot is unipolar for saw
			if (oscMode == LFOMode::kOneShot)
			{
				outputs[kLFONormalOutput] = modCounter - 1.0;
				outputs[kLFOQuadPhaseOutput] = modCounterQP - 1.0;
			}
			else // --- bipolar for sync or free running, use helper function
			{
				outputs[kLFONormalOutput] = unipolarToBipolar(modCounter);
				outputs[kLFOQuadPhaseOutput] = unipolarToBipolar(modCounterQP);
			}

			// --- invert for downsaw
			if (oscillatorWaveform == enumToInt(LFOWaveform::kDownSaw))
			{
				outputs[kLFONormalOutput] *= -1.0;
				outputs[kLFOQuadPhaseOutput] *= -1.0;
			}

			break;
		}
		case enumToInt(LFOWaveform::kSquare):
		{
			// check pulse width and output either +1 or -1
			outputs[kLFONormalOutput] = modCounter > pulseWidth ? -1.0 : +1.0;
			outputs[kLFOQuadPhaseOutput] = modCounterQP > pulseWidth ? -1.0 : +1.0;

			break;
		}
		case enumToInt(LFOWaveform::kTriangle):
		{
			// triv saw
			outputs[kLFONormalOutput] = unipolarToBipolar(modCounter);

			// bipolar triagle
			outputs[kLFONormalOutput] = 2.0*fabs(outputs[kLFONormalOutput]) - 1.0;

			if (oscMode == LFOMode::kOneShot)
				// convert to unipolar
				outputs[kLFONormalOutput] = bipolarToUnipolar(outputs[kLFONormalOutput]);

			// -- quad phase
			outputs[kLFOQuadPhaseOutput] = unipolarToBipolar(modCounterQP);

			// bipolar triagle
			outputs[kLFOQuadPhaseOutput] = 2.0*fabs(outputs[kLFOQuadPhaseOutput]) - 1.0;

			if (oscMode == LFOMode::kOneShot)
				// convert to unipolar
				outputs[kLFOQuadPhaseOutput] = bipolarToUnipolar(outputs[kLFOQuadPhaseOutput]);

			break;
		}
		case enumToInt(LFOWaveform::kWhiteNoise):
		{
			// --- white noise has no real "quad phase"
			outputs[kLFONormalOutput] = doWhiteNoise();
			outputs[kLFOQuadPhaseOutput] = outputs[kLFONormalOutput];

			break;
		}
		case enumToInt(LFOWaveform::kRSH):
		case enumToInt(LFOWaveform::kQRSH):
		{
			// --- is this is the very first run? if so, form first output sample
			if (randomSHCounter < 0)
			{
				if (oscillatorWaveform == enumToInt(LFOWaveform::kRSH))
					randomSHValue = doWhiteNoise();
				else
					randomSHValue = doPNSequence(pnRegister);

				// --- init the sample counter, will be advanced below
				randomSHCounter = 1.0;
			}
			// --- has hold time been exceeded? if so, generate next output sample
			else if (randomSHCounter > (sampleRate / oscFrequency) )
			{
				// --- wrap counter
				randomSHCounter -= sampleRate / oscFrequency;

				if (oscillatorWaveform == enumToInt(LFOWaveform::kRSH))
					randomSHValue = doWhiteNoise();
				else
					randomSHValue = doPNSequence(pnRegister);
			}

			// --- advance the sample counter
			randomSHCounter += 1.0;

			// output held value
			outputs[kLFONormalOutput] = randomSHValue;

			// not meaningful for this output
			outputs[kLFOQuadPhaseOutput] = outputs[kLFONormalOutput];

			break;
		}
		// --- expo is unipolar!
		case enumToInt(LFOWaveform::kExpUp):
		{
			// calculate the output directly
			outputs[kLFONormalOutput] = concaveInvertedTransform(modCounter);
			outputs[kLFOQuadPhaseOutput] = concaveInvertedTransform(modCounterQP);

			break;
		}
		case enumToInt(LFOWaveform::kExpDown):
		{
			// calculate the output directly
			outputs[kLFONormalOutput] = -concaveInvertedTransform(modCounter);
			outputs[kLFOQuadPhaseOutput] = -concaveInvertedTransform(modCounterQP);

			break;
		}

		// --- if we get here, we don't know what kind of osc they want
		default:
			return false;
	}

	// --- invert these
	outputs[kLFOQuadPhaseOutputInverted] = -outputs[kLFOQuadPhaseOutput];
	outputs[kLFONormalOutputInverted] = -outputs[kLFONormalOutput];
	outputs[kLFOUnipolarOutputFromMax] = bipolarToUnipolar(outputs[kLFONormalOutput]);

	// --- then, apply amplitude gain/mod
	for (unsigned int i = 0; i < kNumLFOOutputs; i++)
	{
		outputs[i] *= oscAmplitude;
	}

	// --- special output for amplitude modulation from max; after gain calc above!
	outputs[kLFOUnipolarOutputFromMax] = 1.0 - outputs[kLFOUnipolarOutputFromMax]; 

	// --- for use in glideLFO or other ramp-type LFO
	outputs[kLFOUnipolarUpRamp] = modCounter;
	outputs[kLFOUnipolarDownRamp] = 1.0 - modCounter;

	// --- setup for next time around
	advanceModulo(modCounter, phaseInc);

	return true;
}




