#include "envelopegenerator.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
EnvelopeGenerator::EnvelopeGenerator(std::shared_ptr<EGModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthComponent(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kEG;

	// --- validate all pointers
	validComponent = validateComponent();

	// --- set default mode
	setEGMode(egMode);

	// --- resetTimer
	egTimer.resetTimer();
	reTimer.resetTimer();
}

/** Destructor: delete output array and modulators */
EnvelopeGenerator::~EnvelopeGenerator()
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
bool EnvelopeGenerator::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- wave tables are fs based
	bool bNewSR = info.sampleRate != sampleRate ? true : false;

	// --- retain sample rate for update calculations
	sampleRate = info.sampleRate;

	// --- recrate the tables only if sample rate has changed
	if (bNewSR)
	{
		// --- recalc these, SR dependent
		calculateRepeatTime(repeatTime_mSec);
		calculateDelayTime(delayTime_mSec);
		calculateAttackTime(attackTime_mSec);
		calculateDecayTime(decayTime_mSec);
		calculateReleaseTime(releaseTime_mSec);
	}

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::startComponent()
{
	// reset and go to attack state

	// --- check valid flag
	if (!validComponent) return false;

	// --- ignore
	if (modifiers->legatoMode && state != egState::kOff && state != egState::kRelease)
		return false;

	// --- reset and go
	resetComponent();
	state = egState::kDelay;
	calculateDelayTime(delayTime_mSec);
	calculateRepeatTime(repeatTime_mSec);

	return true;
}

/**
	\brief Perform shut-off operations for the component
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::stopComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear output and go to OFF state
	envelopeOutput = 0.0;
	state = egState::kOff;

	return true;
}

/**
	\brief Reset the component to a note-off state
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear the outputs
	clearOutputs();

	// --- clear modulator inputs
	//  clearModInputs(); we don't have any

	// --- state
	state = egState::kOff;

	// --- reset 
	setEGMode(egMode);

	// --- may be modified in noteOff()
	calculateReleaseTime(releaseTime_mSec);

	// --- if reset to zero or we are not an output EG, clear envelopeOutput
	//     else let it stay frozen
	if (modifiers->resetToZero)
	{
		envelopeOutput = 0.0;
	}

	// --- reset run/stop flag
	noteOn = false;
	egTimer.resetTimer();
	reTimer.resetTimer();
	return true; // handled
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::validateComponent()
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
bool EnvelopeGenerator::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- start EG FSM
	startComponent();

	// --- apply MIDI based modulators - note that this only needs to happen once
	//     and did not belong in old modulation matrix
	if (modifiers->velocityToAttackScaling)
	{
		// --- normalize MIDI velocity and invert
		//     MIDI Velocity = 0   --> no change to attack, scalar = 1.0
		//     MIDI Velocity = 127 --> attack time is now 0mSec, scalar = 0.0
		double scalar = 1.0 - (double)midiNoteVelocity / 127.0;
		calculateAttackTime(attackTime_mSec, scalar);
	}
	if (modifiers->noteNumberToDecayScaling)
	{
		// --- normalize MIDI velocity and invert
		//     MIDI Note 0   --> no change to decay, scalar = 1.0
		//     MIDI Note 127 --> decay time is now 0mSec, scalar = 0.0
		double scalar = 1.0 - (double)midiNoteNumber / 127.0;
		calculateDecayTime(decayTime_mSec, scalar);
	}

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	if (sustainOverride)
	{
		// --- set releasePending flag
		releasePending = true;
		return true;
	}

	// --- go directly to release state
	if (envelopeOutput > 0)
		state = egState::kRelease;
	else // sustain was already at zero
		state = egState::kOff;

	repeatOn = false;

	return true; // handled

}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- sustain level first: other calculations depend on it
	bool sustainUpdate = false;
	if (variableChanged(sustainLevel, modifiers->sustainLevel))
		sustainUpdate = true;

	sustainLevel = modifiers->sustainLevel;

	// --- these are expensive functions, so only call when modified

	if (variableChanged(repeatTime_mSec, modifiers->repeatTime_mSec))
		calculateRepeatTime(modifiers->repeatTime_mSec);

	if (variableChanged(delayTime_mSec, modifiers->delayTime_mSec))
		calculateDelayTime(modifiers->delayTime_mSec);

	if (variableChanged(egMode, modifiers->egMode))
		setEGMode(modifiers->egMode);

	if (variableChanged(attackTime_mSec, modifiers->attackTime_mSec))
		calculateAttackTime(modifiers->attackTime_mSec);

	if (sustainUpdate || variableChanged(decayTime_mSec, modifiers->decayTime_mSec))
		calculateDecayTime(modifiers->decayTime_mSec);

	if (variableChanged(releaseTime_mSec, modifiers->releaseTime_mSec))
		calculateReleaseTime(modifiers->releaseTime_mSec);

	// --- update MIDI stuff
	setSustainOverride(midiData->getMidiCCData(SUSTAIN_PEDAL) > 63);

	return true;
}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- check update
	if (update)
		updateComponent();

	// --- render EG
	return doEnvelopeGenerator();
}

/**
	\brief Perform operations for voice-stealing operation
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::shutDownComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- legato mode - ignore
	if (modifiers->legatoMode)
		return false;

	// --- calculate the linear inc values based on current outputs
	incShutdown = -(1000.0*envelopeOutput) / shutdownTime_mSec / sampleRate;

	// --- set state and reset counter
	state = egState::kShutdown;
	// --- for sustain pedal
	sustainOverride = false;
	releasePending = false;

	return true; // handled
}

void EnvelopeGenerator::calculateRepeatTime(double repeatTime)
{
	reTimer.setTargetValueInSamples(sampleRate*repeatTime / 1000);
}

void EnvelopeGenerator::calculateDelayTime(double delayTime)
{
	egTimer.setTargetValueInSamples(sampleRate*delayTime / 1000);
}
/**
	\brief Calculate the attack time variables including the coefficient and offsets

	\param attackTime: the attack time in mSec
	\param attackTimeScalar: a scalar value on the range [0, +1] for using the velocity to attack time modulation
*/
void EnvelopeGenerator::calculateAttackTime(double attackTime, double attackTimeScalar)
{
	// --- store for comparing so don't need to waste cycles on updates
	attackTime_mSec = attackTime;

	// --- samples for the exponential rate
	double samples = sampleRate*( (attackTime_mSec*attackTimeScalar) / 1000.0);

	// --- coeff and base for iterative exponential calculation
	attackCoeff = exp(-log((1.0 + attackTCO) / attackTCO) / samples);
	attackOffset = (1.0 + attackTCO)*(1.0 - attackCoeff);
}

/**
	\brief Calculate the decay time variables including the coefficient and offsets

	\param decayTime: the decay time in mSec
	\param decayTimeScalar: a scalar value on the range [0, +1] for using the note number to decay time modulation
*/
void EnvelopeGenerator::calculateDecayTime(double decayTime, double decayTimeScalar)
{
	// --- store for comparing so don't need to waste cycles on updates
	decayTime_mSec = decayTime;

	// --- samples for the exponential rate
	double samples = sampleRate*( (decayTime_mSec*decayTimeScalar) / 1000.0);

	// --- coeff and base for iterative exponential calculation
	decayCoeff = exp(-log((1.0 + decayTCO) / decayTCO) / samples);
	decayOffset = (sustainLevel - decayTCO)*(1.0 - decayCoeff);
}

/**
	\brief Calculate the reease time variables including the coefficient and offsets

	\param releaseTime: the reease time in mSec
	\param releaseTimeScalar: a scalar value on the range [0, +1], currently not used but could be used for a MIDI modulation
*/
void EnvelopeGenerator::calculateReleaseTime(double releaseTime, double releaseTimeScalar)
{
	// --- store for comparing so don't need to waste cycles on updates
	releaseTime_mSec = releaseTime;

	// --- samples for the exponential rate
	double samples = sampleRate*( (releaseTime_mSec*releaseTimeScalar) / 1000.0);

	// --- coeff and base for iterative exponential calculation
	releaseCoeff = exp(-log((1.0 + releaseTCO) / releaseTCO) / samples);
	releaseOffset = -releaseTCO*(1.0 - releaseCoeff);
}

/**
	\brief Run the EG through one cycle of the finite state machine.
	\return true if handled, false if not handled
*/
bool EnvelopeGenerator::doEnvelopeGenerator()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- decode the state
	switch (state)
	{
		case egState::kOff:
		{
			// --- output is OFF
			if (!outputEG || modifiers->resetToZero)
			{
				envelopeOutput = 0.0;
				egTimer.resetTimer();
				reTimer.resetTimer();
			}
			break;
		}

		case egState::kDelay:
		{
			//
			if (doRepeat()) break;

			if (delayTime_mSec == 0 || egTimer.timerExpired())
			{
				
				state = egState::kAttack;
				egTimer.resetTimer();
				break;
			}
			else egTimer.advanceTimer();
			break;
			
		}


		case egState::kAttack:
		{
			// --- render value

			if (doRepeat()) break;
			envelopeOutput = attackOffset + envelopeOutput*attackCoeff;

			// --- check go to next state
			if (envelopeOutput >= 1.0 || attackTime_mSec <= 0.0)
			{
				envelopeOutput = 1.0;
				state = egState::kDecay;	// go to next state
				break;
			}
			break;
		}
		case egState::kDecay:
		{
			// --- render value

			if (doRepeat()) break;
			envelopeOutput = decayOffset + envelopeOutput*decayCoeff;

			// --- check go to next state
			if (envelopeOutput <= sustainLevel || decayTime_mSec <= 0.0)
			{
				envelopeOutput = sustainLevel;
				state = egState::kSustain;		// go to next state
				break;
			}
			break;
		}
		case egState::kSustain:
		{
			// --- render value
			
			if (doRepeat()) break;
			envelopeOutput = sustainLevel;
			break;
		}
		case egState::kRelease:
		{
			// --- if sustain pedal is down, override and return
			if (sustainOverride)
			{
				// --- leave envelopeOutput at whatever value is currently has
				//     this is in case the user hits the sustain pedal after the note is released
				break;
			}
			else
				// --- render value
				envelopeOutput = releaseOffset + envelopeOutput*releaseCoeff;

			// --- check go to next state
			if (envelopeOutput <= 0.0 || releaseTime_mSec <= 0.0)
			{
				envelopeOutput = 0.0;
				state = egState::kOff;		// go to OFF state
				break;
			}
			break;
		}
		case egState::kShutdown:
		{
			if (modifiers->resetToZero)
			{
				// --- the shutdown state is just a linear taper since it is so short
				envelopeOutput += incShutdown;

				// --- check go to next state
				if (envelopeOutput <= 0)
				{
					state = egState::kOff;		// go to next state
					envelopeOutput = 0.0;		// reset envelope
					break;
				}
			}
			else
			{
				// --- we are guaranteed to be retriggered
				//     just go to off state
				state = egState::kOff;
			}
			break;
		}
		case egState::kShutdownForRepeat:
		{
			envelopeOutput += incShutdown;

			if (envelopeOutput <= 0)
			{
				envelopeOutput = 0.0;		// reset envelope
				startComponent();
				break;
			}
		}
	}

	outputs[kEGNormalOutput] = envelopeOutput;
	outputs[kEGBiasedOutput] = envelopeOutput - sustainLevel;;

	return true;
}

/**
	\brief Recalculate the time constant offsets (TCOs) when the mode changes; this effectively sets the curvature of the eg segments
*/
void EnvelopeGenerator::setEGMode(egTCMode mode)
{
	// --- save it
	egMode = mode;

	// --- analog - use e^-1.5x, e^-4.95x
	if (mode == egTCMode::kAnalog)
	{
		attackTCO = exp(-1.5);  // fast attack
		decayTCO = exp(-4.95);
		releaseTCO = decayTCO;
	}
	else
	{
		// digital is linear-in-dB so use
		attackTCO = 0.99999;
		decayTCO = decayTCO = exp(-11.05);
		releaseTCO = decayTCO;
	}

	// --- recalc these
	calculateAttackTime(attackTime_mSec);
	calculateDecayTime(decayTime_mSec);
	calculateReleaseTime(releaseTime_mSec);
}
bool EnvelopeGenerator::doRepeat()
{
	if (/*repeatTime_mSec == 0 ||*/ reTimer.timerExpired())
	{
		shutDownComponent();
		state = egState::kShutdownForRepeat;
		return true;
	}
	else
	{
		reTimer.advanceTimer();
		return false;
	}
}