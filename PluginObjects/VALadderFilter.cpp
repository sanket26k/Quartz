#include "VALadderFilter.h"

/**
	\brief Object constructor specialized to properly and safely share the modifiers and MIDI data
	\param _modifiers -- the GUI modifiers structure for this component, to be shared with all similar components
	\param _globalMIDIData -- global MIDI data table, shared across all ISynthComponents
	\param _ccMIDIData -- global MIDI Continuous Controller (CC) table, shared across all ISynthComponents
	\param numOutputs -- the number of outputs for this component
	\param numModulators -- the number of modulators for this component
*/
VALadderFilter::VALadderFilter(std::shared_ptr<VALadderFilterModifiers> _modifiers, IMIDIData* _midiData, uint32_t numOutputs, uint32_t numModulators)
: ISynthAudioProcessor(_midiData, numOutputs, numModulators)
, modifiers(_modifiers)
{
	if (!modifiers) return;
	
	// --- set our type id
	componentType = componentType::kLadderFilter;

	// initialize taps to LPF4 config
	a = 0;
	b = 0;
	c = 0;
	d = 0;
	e = 1;

	// --- create all subfilters
	for (unsigned int i = 0; i<kNumMoogSubFilters; i++)
	{
		// --- create subFilterModifiers - not sharing modifiers due to homework addition of Half-Ladder filter
		std::shared_ptr<VA1FilterModifiers> subFilterModifiers(new VA1FilterModifiers);
		
		// --- set the filter type: note this is set individually for each sub-filter (HINT: for your homework...)
		if(i == kFilter5)
			subFilterModifiers->filter = filterType::kAPF1;

		// --- create sub-filters
		va1Filters[i] = new VA1Filter(subFilterModifiers, _midiData, kNumVA1FilterOutputs, kNumVA1FilterModulators);
	}

	// --- calculate the range of mod frequencies in semi-tones
	//
	//     the /2.0 is because we have bipolar modulation, so this is really
	//     a modulation range of +/- filterModulationRange in semitones
	filterModulationRange = semitonesBetweenFrequencies(kMinFilter_fc, kMaxFilter_fc) / 2.0;

	// --- create modulators
	modulators[kVALadderFilterFcMod] = new Modulator(kDefaultOutputValueOFF, filterModulationRange, modTransform::kNoTransform);

	// --- note this modulator is not used for first order filters
	modulators[kVALadderFilterQMod] = new Modulator(kDefaultOutputValueOFF, kFilterGUI_Q_Range / 2.0, modTransform::kNoTransform);

	// --- this is a priority modulator from the osc output (Tom Sawyer) -- note limiting range to 1/3 of the normal range to prevent aliasing from broken filter
	modulators[kVALadderFilterOscToFcMod] = new Modulator(kDefaultOutputValueOFF, filterModulationRange / 3.0, modTransform::kNoTransform, true); /* true = priority modulator */
	
	// --- validate all pointers
	validComponent = validateComponent();
}

/** Destructor: delete output array and modulators */
VALadderFilter::~VALadderFilter()
{
	// --- destroy subfilters
	for (unsigned int i = 0; i < kNumMoogSubFilters; i++)
	{
		// --- now delete the filter
		delete va1Filters[i];
	}

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
bool VALadderFilter::initializeComponent(InitializeInfo& info)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- init subcomponents
	for (unsigned int i = 0; i<kNumMoogSubFilters; i++)
	{
		va1Filters[i]->initializeComponent(info);
	}

	return true; // handled
}

/**
	\brief Perform startup operations for the component
	\return true if handled, false if not handled
*/
bool VALadderFilter::startComponent()
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
bool VALadderFilter::stopComponent()
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
bool VALadderFilter::resetComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- reset all subfilters
	for (unsigned int i = 0; i<kNumMoogSubFilters; i++)
	{
		va1Filters[i]->resetComponent();
	}

	return true; // handled
}

/**
	\brief Validate all shared pointers, dynamically declared objects (including modulators) and the output array;
	this function should be called once during construction to set the validComponent flag, which is used for future component validation.
	The component should also be re-validated if any new sub-components are dynamically created/destroyed.
	
	\return true if handled, false if not handled
*/
bool VALadderFilter::validateComponent()
{
	// --- sub-components
	for (unsigned int i = 0; i < kNumMoogSubFilters; i++)
	{
		if (!va1Filters[i])
			return false;
	}

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
bool VALadderFilter::doNoteOn(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- save for keytrack
	midiNotePitch = midiPitch;

	// --- start up filter
	startComponent();

	for (unsigned int i = 0; i<kNumMoogSubFilters; i++)
	{
		// --- do the update
		va1Filters[i]->doNoteOn(midiPitch, midiNoteNumber, midiNoteVelocity);
	}

	return true;
}

/**
	\brief Perform note-off operations for the component
	\return true if handled, false if not handled
*/
bool VALadderFilter::doNoteOff(double midiPitch, uint32_t midiNoteNumber, uint32_t midiNoteVelocity)
{
	// --- check valid flag
	if (!validComponent) return false;

	for (unsigned int i = 0; i<kNumMoogSubFilters; i++)
	{
		// --- do the update
		va1Filters[i]->doNoteOff(midiPitch, midiNoteNumber, midiNoteVelocity);
	}

	return true;
}

/**
	\brief Recalculate the component's internal variables based on GUI modifiers, modulators, and MIDI data
	\return true if handled, false if not handled
*/
bool VALadderFilter::updateComponent()
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- caculate fc based on fcControl modulated by fcModulator in semitones
	if (modifiers->enableKeyTrack)
		filter_fc_NoPriorityMod = midiNotePitch * modifiers->keytrackRatio * pitchShiftTableLookup(modulators[kVALadderFilterFcMod]->getModulatedValue());
	else
		filter_fc_NoPriorityMod = modifiers->fcControl * pitchShiftTableLookup(modulators[kVALadderFilterFcMod]->getModulatedValue());

	// --- update
	filter_fc = filter_fc_NoPriorityMod;
	
	// --- the Q
	filter_Q = modifiers->qControl + modulators[kVALadderFilterQMod]->getModulatedValue();

	// --- then do the final coeff calculations
	return calculateFilterCoeffs();
}


/**
	\brief Render the component;
	- for ISynthAudioProcessors, this checks and updates the component if needed
	- for ISynthComponents, this synthesizes the output data into the output array

	\param update -- a flag that is used to update the component; the voice's granularity timer sets/clears this variable

	\return true if handled, false if not handled
*/
bool VALadderFilter::renderComponent(bool update)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- run the modulators
	bool didModulate = runModuators(update);

	if (update)
		updateComponent();
	if (modulators[kVALadderFilterOscToFcMod]->isEnabled()) // then a priority modulation occurred
	{
		// --- we only have one for this component
		//
		// --- factor in the priority modulation; we saved the unmodulated version (filter_fc_NoOscMod) for this in-between priority modulation
		filter_fc = filter_fc_NoPriorityMod * pitchShiftTableLookup(modulators[kVALadderFilterOscToFcMod]->getModulatedValue());

		// --- do the update
		calculateFilterCoeffs();
	}

	// --- any other render-only operations here...
	return true;
}

/**
	\brief Process audio from renderInfo.inputData to output array.

	\param renderInfo contains information about the processing including the flag renderInfo.renderInternal; if this is true, we process audio
	into the output buffer only, otherwise copy the output data into the renderInfo.outputData array

	\return true if handled, false if not handled
*/
bool VALadderFilter::processAudio(RenderInfo& renderInfo)
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
	doFilter(modifiers->filter, renderInfo.inputData[0], CHANNEL_0);

	// --- process right channel if we have one
	if (renderInfo.numInputChannels == 2)
		doFilter(modifiers->filter, renderInfo.inputData[1], CHANNEL_1);

	// --- check for internal render only
	if (renderInfo.renderInternal)
		return true;

	// --- process left channel (must have it)
	renderInfo.outputData[0] = outputs[kVA1LadderFilterOutput_0];

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
		renderInfo.outputData[1] = outputs[kVA1LadderFilterOutput_1];
	}

	return true;
}

/**
	\brief Perform the filtering operation on the input data and write to output array

	\param filter the type of filter to be implemented
	\param input the audio input sample
	\param channel the channel (0 or 1) for the filter

	\return true if handled, false otherwise
*/
bool VALadderFilter::doFilter(filterType filter, double input, unsigned int channel)
{
	// --- check valid flag
	if (!validComponent) return false;

	// --- Moog Half Ladder filter
	if (filter == filterType::kLPF2)
	{
		// --- form sigma 1) the brute force way
		double sigma = calculateSigma(channel);

		// --- apply compensation
		input *= 1.0 + modifiers->gainCompensation*K;

		// --- calculate input to first filter
		double filterInput_u = (input - K*sigma)*alpha0;

		// --- apply non linear saturation
		if (modifiers->applyNLP)
			//	filterInput_u = tanh(modifiers->nlpSaturation*filterInput_u);  
			// --- normalized version
			filterInput_u = tanh(modifiers->nlpSaturation*filterInput_u) / tanh(modifiers->nlpSaturation); 

		// --- cascade of 3 filters
		double dLP1 = va1Filters[kFilter1]->doFilter(filterType::kLPF1, filterInput_u, channel);
		double dLP2 = va1Filters[kFilter2]->doFilter(filterType::kLPF1, dLP1, channel);
		double dAP1 = va1Filters[kFilter5]->doFilter(filterType::kLPF1, dLP2, channel);

		if (channel == CHANNEL_0)
			outputs[kVA1LadderFilterOutput_0] = dAP1;
		else if (channel == CHANNEL_1)
			outputs[kVA1LadderFilterOutput_1] = dAP1;

		// --- alternate, embedded cascade
		// return va1Filters[FILTER4].doFilter(filterType::kLPF1, va1Filters[FILTER3].doFilter(filterType::kLPF1, va1Filters[FILTER2].doFilter(filterType::kLPF1,  va1Filters[FILTER1].doFilter(filterType::kLPF1, filterInput_u, channel), channel), channel), channel);
		return true;
	}
	else //if (filter == filterType::kLPF4) ANY OBERHEIM
	{
		// --- form sigma 1) the brute force way
		double sigma = calculateSigma(channel);

		// --- apply compensation
		input *= 1.0 + modifiers->gainCompensation*K;

		// --- calculate input to first filter
		double filterInput_u = (input - K*sigma)*alpha0;

		// --- apply non linear saturation
		if (modifiers->applyNLP)
			//filterInput_u = tanh(modifiers->nlpSaturation*filterInput_u);  
			// --- normalized version
			filterInput_u = tanh(modifiers->nlpSaturation*filterInput_u) / tanh(modifiers->nlpSaturation);

		// --- cascade of 4 filters
		double dLP1 = va1Filters[kFilter1]->doFilter(filterType::kLPF1, filterInput_u, channel);
		double dLP2 = va1Filters[kFilter2]->doFilter(filterType::kLPF1, dLP1, channel);
		double dLP3 = va1Filters[kFilter3]->doFilter(filterType::kLPF1, dLP2, channel);
		double dLP4 = va1Filters[kFilter4]->doFilter(filterType::kLPF1, dLP3, channel);

		if (channel == CHANNEL_0)
			outputs[kVA1LadderFilterOutput_0] = a*filterInput_u + b*dLP1 + c*dLP2 + d*dLP3 + e*dLP4;
		else if (channel == CHANNEL_1)
			outputs[kVA1LadderFilterOutput_1] = a*filterInput_u + b*dLP1 + c*dLP2 + d*dLP3 + e*dLP4;


		// --- alternate, embedded cascade
		// return va1Filters[FILTER4].doFilter(filterType::kLPF1, va1Filters[FILTER3].doFilter(filterType::kLPF1, va1Filters[FILTER2].doFilter(filterType::kLPF1,  va1Filters[FILTER1].doFilter(filterType::kLPF1, filterInput_u, channel), channel), channel), channel);
	
		return true;
	}

	return false; // unknown filter type :(
}

/**
	\brief Calcualte sigma, the sum of all the storage feedbacks from each subfilter

	\param channel the channel (0 or 1) for the filter

	\return the sigma value
*/
double VALadderFilter::calculateSigma(unsigned int channel)
{
	// --- check valid flag
	if (!validComponent) return false;

	double sigma = 0.0;

	if (modifiers->filter == filterType::kLPF2)
	{
		sigma += va1Filters[kFilter1]->getFeedbackStorageValue(channel);
		sigma += va1Filters[kFilter2]->getFeedbackStorageValue(channel);
		sigma += va1Filters[kFilter5]->getFeedbackStorageValue(channel);
	}	
	else // (modifiers->filter == filterType::kLPF4 || modifiers->filter == filterType::kHPF4 )
	{
		for (unsigned int i = 0; i < kNumMoogSubFilters; i++)
		{
			sigma += va1Filters[i]->getFeedbackStorageValue(channel);
		}
	}

	return sigma;

	// --- verbose way - delete this once you understand loop above
	/*
	double sigma = va1Filters[FILTER1].getFeedbackStorageValue(channel) +
	va1Filters[FILTER2].getFeedbackStorageValue(channel) +
	va1Filters[FILTER2].getFeedbackStorageValue(channel) +
	va1Filters[FILTER2].getFeedbackStorageValue(channel);*/

}


/**
	\brief Recalculate and update the filter coefficients, including subfilters

	\return true if handled
*/
bool VALadderFilter::calculateFilterCoeffs()
{
	// --- limit in case fc control is biased
	boundValue(filter_fc, kMinFilter_fc, kMaxFilter_fc);
	boundValue(filter_Q, kMinFilter_Q, kMaxFilter_Q);

	// --- FIRST update our internal filters
	//
	//     Can take advamtage of the fact that all filters are sync-tuned; only need to run the full calculation for the first filter
	//
	va1Filters[kFilter1]->getModifiers()->fcControl = filter_fc;
	va1Filters[kFilter1]->updateComponent();
	double alpha = va1Filters[kFilter1]->getAlpha();
	double g = va1Filters[kFilter1]->getFilter_g();

	// --- now just copy the values from first filter
	for (unsigned int i = 1; i<kNumMoogSubFilters; i++) // note i=1...
	{
		// --- our sub-filters are sync-tuned to our Fc, so can just copy
		va1Filters[i]->setFilterCoeffs(filter_fc, alpha, g);
	}

	// --- THEN update our stuff
	if (modifiers->filter == filterType::kLPF2)
	{
		// --- Q controls always 1 -> 10  kFilterGUI_Q_Range = (10.0 - 1.0)
		//	   this maps qControl = 1 -> 10   to   K = 0 -> 2
		K = (2.0)*(filter_Q - 1.0) / (kFilterGUI_Q_Range);

		// the allpass G value
		double G = va1Filters[kFilter1]->getAlpha();		// --- all alphas are the same, so just use first filter
		double g = va1Filters[kFilter1]->getFilter_g();		// --- all g's are the same, so just use first filter

															// --- half ladder calcs
		double GA = 2.0*G - 1;

		// --- set beta values
		va1Filters[kFilter1]->setBeta(GA*G / (1.0 + g));
		va1Filters[kFilter2]->setBeta(GA / (1.0 + g));
		va1Filters[kFilter5]->setBeta(2.0 / (1.0 + g));

		// calculate alpha0
		alpha0 = 1.0 / (1.0 + K*GA*G*G);
	}
	else // (modifiers->filter == filterType::kLPF4)
	{
		// --- Q controls always 1 -> 10  kFilterGUI_Q_Range = (10.0 - 1.0)
		//	   this maps qControl = 1 -> 10   to   K = 0 -> 4
		K = (4.0)*(filter_Q - 1.0) / (kFilterGUI_Q_Range);

		// --- create feedback values; using G to match book
		double G = va1Filters[kFilter1]->getAlpha();		// --- all alphas are the same, so just use first filter
		double g = va1Filters[kFilter1]->getFilter_g();		// --- all g's are the same, so just use first filter

															// --- set the beta coefficients on subfilters
		va1Filters[kFilter1]->setBeta(G*G*G / (1.0 + g));
		va1Filters[kFilter2]->setBeta(G*G / (1.0 + g));
		va1Filters[kFilter3]->setBeta(G / (1.0 + g));
		va1Filters[kFilter4]->setBeta(1.0 / (1.0 + g));

		// --- set our coefficients
		//gamma = G*G*G*G; // --- G^4
		alpha0 = 1.0 / (1.0 + K*G*G*G*G);

		if( modifiers->filter == filterType::kLPF4 )
		{	
			a = 0;
			b = 0;
			c = 0;
			d = 0;
			e = 1;		
		}
		else if( modifiers->filter == filterType::kHPF4 )
		{
			a = 1;
			b = -4;
			c = 6;
			d = -4;
			e = 1;
		}
		else if( modifiers->filter == filterType::kHPF2 )
		{
			a = 1;
			b = -2;
			c = 1;
			d = 0;
			e = 0;
		}
		/*else if( modifiers->filter == filterType::kBPF2 )
		{
			a = 0;
			b = 2;
			c = -2;
			d = 0;
			e = 0;
		}
		else if( modifiers->filter == filterType::kBPF4 )
		{
			a = 0;
			b = 0;
			c = 4;
			d = -8;
			e = 4;
		} */
	}

	return true;
}

