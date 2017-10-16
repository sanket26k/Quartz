#include "synthoscillator.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
SynthOscillator::SynthOscillator(std::shared_ptr<SynthOscModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
	: ISynthComponent(_midiData, numOutputs, numModulators)
	, modifiers(_modifiers)
{
	// --- clear out arrays
	memset(sawTables, 0, kNumWaveTables * sizeof(double*));
	memset(triangleTables, 0, kNumWaveTables * sizeof(double*));

	// --- create tables
	createWaveTables();

	// --- default to SINE
	currentTable = &sineTable[0];

	// --- seed the random number generator
	srand(time(NULL));

	if (!modifiers) return;

	// --- set our type id
	componentType = componentType::kPitchedOscillator;

	// --- create modulators
	modulators[kSynthOscPitchMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_Pitch_ModRange, modTransform::kNoTransform);
	modulators[kSynthOscToOscPitchMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_Pitch_ModRange, modTransform::kNoTransform, true);/* priority modulator */

	modulators[kSynthOscLinFreqMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_FM_PM_ModIndex, modTransform::kNoTransform, true);/* priority modulator */

	modulators[kSynthOscLinPhaseMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_FM_PM_ModIndex, modTransform::kNoTransform, true);/* priority modulator */

	modulators[kSynthOscPulseWidthMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_PW_ModRange, modTransform::kNoTransform);
	modulators[kSynthToOscOscPulseWidthMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_PW_ModRange, modTransform::kNoTransform, true);/* priority modulator */

	modulators[kSynthOscAmpMod] = new Modulator(kDefaultOutputValueON, kSynthOsc_Amp_ModRange, modTransform::kAlwaysPositiveTransform);

	modulators[kSynthOscMaxDownAmpMod] = new Modulator(kDefaultOutputValueON, kSynthOsc_Amp_ModRange, modTransform::kMaxDownTransform);

	/* the modulation range for this one will be changed for each portamento "session" */
	modulators[kSynthOscPortamentoMod] = new Modulator(kDefaultOutputValueOFF, kSynthOsc_Pitch_ModRange,  modTransform::kNoTransform);

	// --- validate all pointers
	validComponent = validateComponent();
}

/** Destructor: delete output array and modulators */
SynthOscillator::~SynthOscillator()
{
	// --- destroy wavetables
	destroyWaveTables();

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
bool SynthOscillator::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- wave tables are fs based
	bool bNewSR = info.sampleRate != sampleRate ? true : false;

	// --- retain sample rate for update calculations
	sampleRate = info.sampleRate;

	// --- bulk reset
	resetComponent();

	// --- recrate the tables only if sample rate has changed
	if (bNewSR)
	{
		// --- then recrate
		destroyWaveTables();
		createWaveTables();
	}

	return true;
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool SynthOscillator::startComponent()
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
bool SynthOscillator::stopComponent()
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
bool SynthOscillator::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- clear the outputs
	clearOutputs();

	// --- VA stuff
	modCounter = 0.0;

	// --- VA saw requires offset
	if (oscWave == synthOscWaveform::kSaw)
		modCounter = 0.5;

	// --- wavetable stuff
	waveTableReadIndex = 0.0;

	// --- common to both VA and WT
	phaseInc = 0.0;

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
bool SynthOscillator::validateComponent()
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
bool SynthOscillator::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	if (!validateComponent()) return false;

	// --- save pitch
	midiNotePitch = midiPitch;

	// --- start component
	startComponent();

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool SynthOscillator::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	if (!validateComponent()) return false;

	// --- we ignore note off
	return true;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool SynthOscillator::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save waveform
	oscWave = modifiers->oscWave;

	// --- calculate MIDI pitch bend in semitones (globalMIDIData[kMIDIPitchBendRange] is in semitones)
	double midiPitchBend = midiData->getMidiGlobalData(kMIDIPitchBendRange) * midiPitchBendToBipolar(midiData->getMidiGlobalData(kMIDIPitchBendData1),
																									   midiData->getMidiGlobalData(kMIDIPitchBendData2));
	
	// --- calculate entire combination of moduations ***in semitones***    pitchShiftTableLookup
	if (modifiers->useOscFreqControl)
		oscFrequencyNoPriorityMod = modifiers->oscFreqControl;
	else
		oscFrequencyNoPriorityMod = midiNotePitch*
		pitchShiftTableLookup(
		(modulators[kSynthOscPitchMod]->getModulatedValue()) +				/* ExpFreqModulation input */
			(modulators[kSynthOscPortamentoMod]->getModulatedValue()) +		/* portamento input */
			(midiPitchBend)+												/* [-1, +1]*MIDI_PB_range*/
			(modifiers->octave * 12) +										/* octave*12 = semitones */
			modifiers->semitones +											/* raw semitones */
			(modifiers->cents / 100.0) +									/* cents/100 = semitones */
			(modifiers->masterTuningOffset_cents / 100.0) +					/* master tuning offset */
			((unisonDetuneIntensity * modifiers->unisonDetune_cents) / 100.0));		/* unison detune in cents */

	// --- apply tuning ratio for FM and other exotics
	oscFrequencyNoPriorityMod *= (modifiers->oscFreqRatio * modifiers->masterTuningRatio);

	// --- set and save
	oscFrequency = oscFrequencyNoPriorityMod;

	// --- bound the frequency to our range
	boundValue(oscFrequency, kMinSynthOsc_fo, kMaxSynthOsc_fo);

	// --- calculate pulse width
	pulseWidth = (modifiers->pulseWidthControl_Pct / 100.0) + modulators[kSynthOscPulseWidthMod]->getModulatedValue();

	// --- bound the pulse width to our range [-1, +1]
	boundValue(pulseWidth, kMinSynthOsc_PW, kMaxSynthOsc_PW);

	// --- output gain control (could test for change and not calculate if changed to save CPU cycles)
	oscVolumeControlGain = modifiers->oscAmpControl_dB == -96.0 ? 0.0 : pow(10.0, modifiers->oscAmpControl_dB / 20.0);

	// --- calculate final amp gain with ampmod and maxdown ampmod
	oscAmplitude = modulators[kSynthOscAmpMod]->getModulatedValue() * modulators[kSynthOscMaxDownAmpMod]->getModulatedValue();

	// --- bound the amplitude to our range
	boundValue(oscAmplitude, kMinSynthOsc_amp, kMaxSynthOsc_amp);

	// --- calcualte phase inc
	calculatePhaseInc();

	// --- select the wave table
	selectTable();

	return true;
}

/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool SynthOscillator::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- run the modulators
	runModuators(update);

	// --- these are CRITICAL modulator values that must be updated each sample interval!
	//
	// --- save Phase Mod value for use in oscillate functions (not worth it to test if enabled; benign if not
	phaseModuator = modulators[kSynthOscLinPhaseMod]->getModulatedValue();

	// --- apply linear FM (not used Yamaha DX, but is used in Oddysei and other primitive synths)
	//     NOTE: check to see if enabled as it will require a phase inc update calculation
	if (modulators[kSynthOscLinFreqMod]->isEnabled())
	{
		// --- linear application of modulator
		oscFrequency += modulators[kSynthOscLinFreqMod]->getModulatedValue();

		// --- recalculate
		calculatePhaseInc();
	}

	if (modulators[kSynthOscToOscPitchMod]->isEnabled())
	{
		// --- pitch application of modulator
		oscFrequency = oscFrequencyNoPriorityMod * pitchShiftTableLookup(modulators[kSynthOscToOscPitchMod]->getModulatedValue());

		// --- recalculate
		calculatePhaseInc();
	}

	if (modulators[kSynthToOscOscPulseWidthMod]->isEnabled())
	{
		// --- calculate pulse width
		pulseWidth = (modifiers->pulseWidthControl_Pct / 100.0) + modulators[kSynthToOscOscPulseWidthMod]->getModulatedValue();

		// --- bound the pulse width to our range [-1, +1]
		boundValue(pulseWidth, kMinSynthOsc_PW, kMaxSynthOsc_PW);
	}

	// --- check update
	if (update)
		updateComponent();

	// --- render oscillators
	return doOscillate();
}


/**
	\brief Synthesize the pitched oscillator's output
	\return true if handled, false if not handled
*/
bool SynthOscillator::doOscillate()
{
	// --- check valid flag
	if (!validComponent) return false;

	double oscOutput = 0.0;

	// --- decode oscillator
	if (oscWave == synthOscWaveform::kSaw)
	{
		// --- VA oscillator
		outputs[kLeftOscOutput] = doSawtooth();
		outputs[kRightOscOutput] = outputs[kLeftOscOutput];
	}
	else if (oscWave == synthOscWaveform::kSquare)
	{
		// --- dual mono output for wavetable oscillator
		outputs[kLeftOscOutput] = doSquareWave();
		outputs[kRightOscOutput] = outputs[kLeftOscOutput];
	}
	else if (oscWave == synthOscWaveform::kTriangle)
	{
		// --- dual mono output for wavetable oscillator
		outputs[kLeftOscOutput] = doTriangleWave();
		outputs[kRightOscOutput] = outputs[kLeftOscOutput];
	}
	else if (oscWave == synthOscWaveform::kSin)
	{
		// --- dual mono output for wavetable oscillator
		outputs[kLeftOscOutput] = doSineWave();
		outputs[kRightOscOutput] = outputs[kLeftOscOutput];
	}
	else if (oscWave == synthOscWaveform::kWhiteNoise)
	{
		// --- dual mono output for noise oscillator
		outputs[kLeftOscOutput] = doWhiteNoise();
		outputs[kRightOscOutput] = outputs[kLeftOscOutput];
	}

	// --- additional outputs for AM
	outputs[kOscUnipolarOutputFromMax] = bipolarToUnipolar(outputs[kLeftOscOutput]);

	// --- special output for actingas a frequency modulator (true FM) where we multiply the modulator depth by the modulator fo
	outputs[kOscFModOutput] = oscFrequency * outputs[kLeftOscOutput];

	// --- next, apply amplitude gain/mod
	outputs[kLeftOscOutput] *= oscAmplitude;
	outputs[kRightOscOutput] *= oscAmplitude;
	outputs[kOscUnipolarOutputFromMax] *= oscAmplitude;

	// --- special output for amplitude modulation from max; after gain calc above!
	outputs[kOscUnipolarOutputFromMax] = 1.0 - outputs[kOscUnipolarOutputFromMax];

	// --- for hard-sync modulation
	outputs[kOscResetTrigger] = resetTrigger ? 1.0 : 0.0;

	// --- now apply final user gain at last step so that user gain does not affect modulation outputs
	outputs[kLeftOscOutputWithAmpGain] = oscVolumeControlGain * outputs[kLeftOscOutput];
	outputs[kRightOscOutputWithAmpGain] = oscVolumeControlGain * outputs[kRightOscOutput];

	return true;
}

/**
	\brief Synthesize the VA Sawtooth waveform

	\return the oscillator's output sample 
*/
double SynthOscillator::doSawtooth()
{
	// --- always first
	bool wrapped = checkAndWrapModulo(modCounter, phaseInc);

	// --- added for PHASE MODULATION on VA oscillators
	double finalModCounter = modCounter;

	// --- create final version
	resetTrigger = advanceAndCheckWrapModulo(finalModCounter, phaseModuator);

	// --- return variable
	double output = 0.0;

	// --- first create the trivial saw from modulo counter
	double trivialSaw = unipolarToBipolar(finalModCounter);

	// --- NOTE: Fs/8 = Nyquist/4
	if (oscFrequency <= sampleRate / 8.0)
	{
		output = trivialSaw + doBLEP_N(&dBLEPTable_8_BLKHAR[0], /* BLEP table */
			4096,					/* BLEP table length */
			finalModCounter,		/* current phase value */
			fabs(phaseInc),			/* abs(phaseInc) is for FM synthesis with negative frequencies */
			1.0,					/* sawtooth edge height = 1.0 */
			false,					/* falling edge */
			4,						/* 4 points per side */
			false);					/* no interpolation */
	}
	else if (oscFrequency <= sampleRate / 4.0) // Nyquist/2
	{
		output = trivialSaw + doBLEP_N(&dBLEPTable_8_BLKHAR[0], /* BLEP table */
			4096,					/* BLEP table length */
			finalModCounter,		/* current phase value */
			fabs(phaseInc),			/* abs(phaseInc) is for FM synthesis with negative frequencies */
			1.0,					/* sawtooth edge height = 1.0 */
			false,					/* falling edge */
			2,						/* 2 points per side */
			false);					/* no interpolation */
	}
	else // --- just 1 point per side
	{
		output = trivialSaw + doBLEP_N(&dBLEPTable[0],			/* BLEP table */
			4096,					/* BLEP table length */
			finalModCounter,		/* current phase value */
			fabs(phaseInc),			/* abs(phaseInc) is for FM synthesis with negative frequencies */
			1.0,					/* sawtooth edge height = 1.0 */
			false,					/* falling edge */
			1,						/* 1 point per side */
			true);					/* no interpolation */
	}

	// --- setup for next time around
	advanceModulo(modCounter, phaseInc);

	return output;
}

/**
	\brief Synthesize the Square wave using the sum-of-saws wavetable method

	\return the oscillator's output sample
*/
double SynthOscillator::doSquareWave()
{
	// --- find location of read index for our pulseWidth
	double pulseWidthReadIndex = waveTableReadIndex + pulseWidth*kWaveTableLength;

	// --- render first sawtooth using dReadIndex
	double saw1 = doSelectedWaveTable(waveTableReadIndex, phaseInc);

	// --- find the phase shifted output
	if (phaseInc >= 0)
	{
		if (pulseWidthReadIndex >= kWaveTableLength)
			pulseWidthReadIndex = pulseWidthReadIndex - kWaveTableLength;
	}
	else
	{
		if (pulseWidthReadIndex < 0)
			pulseWidthReadIndex = kWaveTableLength + pulseWidthReadIndex;
	}

	// --- render second sawtooth using dPWIndex (shifted)
	double saw2 = doSelectedWaveTable(pulseWidthReadIndex, phaseInc);

	// --- find the correction factor from the table
	double corrFactor = squareCorrFactor[currentTableIndex];

	// --- then subtract
	double output = corrFactor*saw1 - corrFactor*saw2;

	// --- calculate the DC correction factor
	double dcCorrFactor = 1.0 / pulseWidth;
	if (pulseWidth < 0.5)
		dcCorrFactor = 1.0 / (1.0 - pulseWidth);

	// --- apply correction
	output *= dcCorrFactor;

	return output;
}

/**
	\brief Synthesize the Triangle wave using the wavetable method

	\return the oscillator's output sample
*/
double SynthOscillator::doTriangleWave()
{
	return doSelectedWaveTable(waveTableReadIndex, phaseInc);
}

/**
	\brief Synthesize the Sine wave using the wavetable method

	\return the oscillator's output sample
*/
double SynthOscillator::doSineWave()
{
	return doSelectedWaveTable(waveTableReadIndex, phaseInc);
}

/**
	\brief Create the wavetables; this happens at construction time and whenever the sample rate changes
*/
void SynthOscillator::createWaveTables()
{
	// --- create the tables
	//
	// --- SINE: only need one table
	for (int i = 0; i < kWaveTableLength; i++)
	{
		// sample the sinusoid, kWaveTableLength points
		// sin(wnT) = sin(2pi*i/kWaveTableLength)
		sineTable[i] = sin(((double)i / kWaveTableLength)*(2 * pi));
	}

	// --- SAW, TRIANGLE: need 9 tables
	double seedFreq = 27.5; // Note A0, bottom of piano
	for (int j = 0; j < kNumWaveTables; j++)
	{
		double* sawTableAccumulator = new double[kWaveTableLength];
		memset(sawTableAccumulator, 0, kWaveTableLength * sizeof(double));

		double* triTableAccumulator = new double[kWaveTableLength];
		memset(triTableAccumulator, 0, kWaveTableLength * sizeof(double));

		int numHarmonics = (int)((sampleRate / 2.0 / seedFreq) - 1.0);
		int halfNumHarmonics = (int)((float)numHarmonics / 2.0);

		double maxSawValue = 0;
		double maxTriVavlue = 0;

		for (int i = 0; i < kWaveTableLength; i++)
		{
			// --- sawtooth: += (-1)^g+1(1/g)sin(wnT)
			for (int g = 1; g <= numHarmonics; g++)
			{
				// --- Lanczos Sigma Factor
				double x = g*pi / numHarmonics;
				double sigma = sin(x) / x;

				// --- only apply to partials above fundamental
				if (g == 1)
					sigma = 1.0;

				double n = double(g);
				sawTableAccumulator[i] += pow((float)-1.0, (float)(g + 1))*(1.0 / n)*sigma*sin(2.0*pi*i*n / kWaveTableLength);
			}

			// --- triangle: += (-1)^g(1/(2g+1+^2)sin(w(2n+1)T)
			//	   NOTE: the limit is halfNumHarmonics here because of the way the sum is constructed
			//	   (look at the (2n+1) components
			for (int g = 0; g <= halfNumHarmonics; g++)
			{
				double n = double(g);
				triTableAccumulator[i] += pow((float)-1.0, (float)n)*(1.0 / pow((float)(2 * n + 1), (float)2.0))*sin(2.0*pi*(2.0*n + 1)*i / kWaveTableLength);
			}

			// --- store the max values
			if (i == 0)
			{
				maxSawValue = sawTableAccumulator[i];
				maxTriVavlue = triTableAccumulator[i];
			}
			else
			{
				// --- test and store
				if (sawTableAccumulator[i] > maxSawValue)
					maxSawValue = sawTableAccumulator[i];

				if (triTableAccumulator[i] > maxTriVavlue)
					maxTriVavlue = triTableAccumulator[i];
			}
		}
		// --- normalize
		for (int i = 0; i < kWaveTableLength; i++)
		{
			// normalize it
			sawTableAccumulator[i] /= maxSawValue;
			triTableAccumulator[i] /= maxTriVavlue;
		}

		// --- store
		sawTables[j] = sawTableAccumulator;
		triangleTables[j] = triTableAccumulator;

		// --- next table is one octave up
		seedFreq *= 2.0;
	}
}

/**
	\brief Destroy the wavetables; this happens at construction time and whenever the sample rate changes
*/
void SynthOscillator::destroyWaveTables()
{
	for (int i = 0; i < kNumWaveTables; i++)
	{
		double* p = sawTables[i];
		if (p)
		{
			delete[] p;
			sawTables[i] = 0;
		}

		p = triangleTables[i];
		if (p)
		{
			delete[] p;
			triangleTables[i] = 0;
		}
	}
}

/**
	\brief Calculate the table index based on the current oscillator frequency for choosing the proper wavetable to avoid aliasing

	\return the index of the wave-table or -1 if the sine table is to be used
*/
int SynthOscillator::getTableIndex()
{
	if (oscWave == synthOscWaveform::kSin)
		return -1;

	double seedFreq = 27.5; // Note A0, bottom of piano
	for (int j = 0; j < kNumWaveTables; j++)
	{
		if (oscFrequency <= seedFreq)
		{
			return j;
		}

		seedFreq *= 2.0;
	}

	// --- this means the frequency was outside the limit and is a flag to use the sine table
	return -1;
}

/**
	\brief Select the wavetable based on the currently selected waveform
*/
void SynthOscillator::selectTable()
{
	currentTableIndex = getTableIndex();

	// --- if the frequency is high enough, the sine table will be returned
	//     even for non-sinusoidal waves; anything about 10548 Hz is one
	//     harmonic only (sine)
	if (currentTableIndex < 0 || !modifiers)
	{
		currentTable = &sineTable[0];
		return;
	}

	// --- choose table
	if (oscWave == synthOscWaveform::kSquare)
		currentTable = sawTables[currentTableIndex];
	else if (oscWave == synthOscWaveform::kTriangle)
		currentTable = triangleTables[currentTableIndex];
	else if (oscWave == synthOscWaveform::kSin)
		currentTable = &sineTable[0];
}

/**
	\brief Perform the wavetable lookup and interpolation operation on the currently selected tables.

	\param readIndex the current location of the read index which is updated during the operation
	\param phaseInc the wavetable increment value for this frequency

	\returns the oscillator's output sample
*/
double SynthOscillator::doSelectedWaveTable(double& readIndex, double phaseInc)
{
	// --- apply phase modulation, if any
	double phaseModReadIndex = readIndex + phaseModuator * kWaveTableLength;

	// --- check for multi-wrapping on new read index
	resetTrigger = checkAndWrapWaveTableIndex(phaseModReadIndex, kWaveTableLength);

	// --- get INT part
	int intReadIndex = abs((int)phaseModReadIndex);

	// --- get FRAC part
	double fractionalPart = phaseModReadIndex - intReadIndex;

	// --- setup second index for interpolation; wrap the buffer if needed
	int intReadIndexNext = intReadIndex + 1 > kWaveTableLength - 1 ? 0 : intReadIndex + 1;

	// --- interpolate the output
	double output = doLinearInterpolation(0.0, 1.0, currentTable[intReadIndex], currentTable[intReadIndexNext], fractionalPart);

	// --- add the increment for next time
	readIndex += phaseInc;

	// --- check for wrap
	checkAndWrapWaveTableIndex(readIndex, kWaveTableLength);

	return output;
}







