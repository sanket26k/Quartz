//
//  PluginBase.cpp
//  VolGUI_I
//
//  Created by Will Pirkle on 3/31/17.
//
//

#include "PluginBase.h"

PluginBase::PluginBase()
{
    memset(&inputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&outputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&auxInputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&auxOutputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);

    pluginHostConnector = nullptr;
}

PluginBase::~PluginBase()
{
	if (pluginDescriptor.supportedIOCombinations)
		delete[] pluginDescriptor.supportedIOCombinations;
	pluginDescriptor.supportedIOCombinations = 0;

	if (pluginDescriptor.supportedAuxIOCombinations)
		delete[] pluginDescriptor.supportedAuxIOCombinations;
	pluginDescriptor.supportedAuxIOCombinations = 0;

    removeAllPresets();

    for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it != pluginParameters.end(); ++it) {
        delete *it;
    }
    pluginParameters.clear();
    pluginParameterMap.clear();
	delete [] pluginParameterArray;
}



PluginParameter* PluginBase::getPluginParameterByIndex(int32_t index)
{
    return pluginParameters[index];
}

PluginParameter* PluginBase::getPluginParameterByControlID(int32_t controlID)
{
    return pluginParameterMap[controlID];
}

/*
PluginParameter* PluginBase::getGUIParameterByControlID(int32_t controlID)
{
    return guiParameterMap[controlID];

}*/

double PluginBase::getPIParamValueDouble(int32_t controlID)
{
    PluginParameter* piParam = getPluginParameterByControlID(controlID);
    if(!piParam) return 0.0;

    return piParam->getControlValue();
}

float PluginBase::getPIParamValueFloat(int32_t controlID)
{
    PluginParameter* piParam = getPluginParameterByControlID(controlID);
    if(!piParam) return 0.f;

    return (float)piParam->getControlValue();
}

int PluginBase::getPIParamValueInt(int32_t controlID)
{
    PluginParameter* piParam = getPluginParameterByControlID(controlID);
    if(!piParam) return 0;

    return (int)piParam->getControlValue();
}

uint32_t PluginBase::getPIParamValueUInt(int32_t controlID)
{
    PluginParameter* piParam = getPluginParameterByControlID(controlID);
    if(!piParam) return 0;

    return (uint32_t)piParam->getControlValue();
}

bool PluginBase::compareSelectedString(int32_t controlID, const char* compareString)
{
    PluginParameter* piParam = getPluginParameterByControlID(controlID);
    if(!piParam) return false;

    std::string str = piParam->getControlValueAsString();
    if(str.compare(compareString) == 0)
        return true;

    return false;
}

PluginParameter* PluginBase::getNextParameterOfType(int32_t& startIndex, controlVariableType controlType)
{
    PluginParameter* piParam = nullptr;
	for (uint32_t i = startIndex; i < getPluginParameterCount(); i++)
    {
        piParam = getPluginParameterByIndex(i);
        {
            if(piParam->getControlVariableType() == controlType)
            {
                startIndex = i+1;
                return piParam;
            }
        }
    }

    startIndex = -1;
    return nullptr;
}

// --- combines VST sample accurate updates and parameter smoothing for fastest realtime updates
void PluginBase::doSampleAccurateParameterUpdates()
{
	// --- do updates
	double value = 0;
	bool vstSAAutomated = false;

	// --- rip through the array
	for (unsigned int i = 0; i < numPluginParameters; i++)
	{
		PluginParameter* piParam = pluginParameterArray[i];
		if (piParam)
		{
			// --- VST sample accurate stuff
			if (wantsVST3SampleAccurateAutomation())
			{
				// --- if we get here getParameterUpdateQueue() should be non-null
				//     NOTE you can disable sample accurate automation for each parameter when you set them up if needed
				if (piParam->getParameterUpdateQueue() && piParam->getEnableVSTSampleAccurateAutomation())
				{
					if (piParam->getParameterUpdateQueue()->getNextValue(value))
					{
						piParam->setControlValueNormalized(value, false, true); // false = do not apply taper, true = ignore smoothing (not needed here)
						vstSAAutomated = true;

						ParameterUpdateInfo info(false, true); /// false = this is NOT called from smoothing operation, true: this is a VST sample accurate update
						
						// --- now update the bound variable
						if (pluginParameterArray[i]->updateInBoundVariable())
						{
							info.bufferProcUpdate = false;
							info.boundVariableUpdate = true;
						}
						postUpdatePluginParameter(piParam->getControlID(), piParam->getControlValue(), info);
					}
				}
			}

			// --- do smoothing, but not if we did a sample accurate automation update! 
			if (!vstSAAutomated && piParam->smoothParameterValue())
			{
				ParameterUpdateInfo info(true, false); /// true = this is called from smoothing operation, false = NOT VST sample accurate update

				// --- update bound variable, if there is one
				if (pluginParameterArray[i]->updateInBoundVariable())
				{
					info.bufferProcUpdate = false;
					info.boundVariableUpdate = true;
				}
				postUpdatePluginParameter(pluginParameterArray[i]->getControlID(), pluginParameterArray[i]->getControlValue(), info);
			}
		}
	}
}

void PluginBase::syncInBoundVariables()
{
    // --- rip through and synch em
	for (unsigned int i = 0; i < numPluginParameters; i++)
	{
		if (pluginParameterArray[i] && pluginParameterArray[i]->updateInBoundVariable())
		{
			ParameterUpdateInfo info;
			info.bufferProcUpdate = true;
			info.boundVariableUpdate = true;
			postUpdatePluginParameter(pluginParameterArray[i]->getControlID(), pluginParameterArray[i]->getControlValue(), info);
		}
	}
}

bool PluginBase::updateOutBoundVariables()
{
	bool updated = false;

	// --- rip through and synch em
	for (unsigned int i = 0; i < numPluginParameters; i++)
	{
		if (pluginParameterArray[i] && pluginParameterArray[i]->getControlVariableType() == controlVariableType::kMeter)
		{
			pluginParameterArray[i]->updateOutBoundVariable();
			updated = true; // sticky
		}
	}
	return updated;
}
// --- returns array index for vector access
int32_t PluginBase::addPluginParameter(PluginParameter* piParam, double sampleRate)
{
    // --- map for controlID-indexing
    pluginParameterMap.insert(std::make_pair(piParam->getControlID(), piParam));

    // --- vector for fast iteration and 0-indexing
    pluginParameters.push_back(piParam);

    // --- first intialization, this can change
    piParam->initParamSmoother(sampleRate);

	return (int32_t)pluginParameters.size() - 1;
}

void PluginBase::setParamAuxAttribute(uint32_t controlID, const AuxParameterAttribute& auxAttribute)
{
	PluginParameter* piParam = getPluginParameterByControlID(controlID);
	if (!piParam) return;

	piParam->setAuxAttribute(auxAttribute.attributeID, auxAttribute);
}

bool PluginBase::setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData)
{
	// --- override for use
	return false;
}

std::vector<PluginParameter*>* PluginBase::makePluginParameterVectorCopy(bool disableSmoothing)
{
    std::vector<PluginParameter*>* PluginParameterPtr = new std::vector<PluginParameter*>;

    // --- copy params, do not use copy constructor; it copies pointers
    for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it !=  pluginParameters.end(); ++it)
    {
        PluginParameter* piParam = new PluginParameter(**it);

        if(disableSmoothing)
            piParam->setParameterSmoothing(false);

        PluginParameterPtr->push_back(piParam);
    }

    return PluginParameterPtr;
}

// --- which one is better?
bool PluginBase::initPresetParameters(std::vector<PresetParameter>& presetParameters, bool disableSmoothing)
{
    // --- copy params, do not use copy constructor; it copies pointers
    for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it !=  pluginParameters.end(); ++it)
    {
        PresetParameter preParam((*it)->getControlID(), (*it)->getControlValue());

        presetParameters.push_back(preParam);
    }

    return true;
}

bool PluginBase::setPresetParameter(std::vector<PresetParameter>& presetParameters, uint32_t _controlID, double _controlValue)
{
    bool foundIt = false;
    for(std::vector<PresetParameter>::iterator it = presetParameters.begin(); it !=  presetParameters.end(); ++it)
    {
        //PresetParameter preParam = *it;
        if((*it).controlID == _controlID)
        {
            (*it).actualValue = _controlValue;
            foundIt = true;
        }
    }

    return foundIt;
}


bool PluginBase::processMessage(MessageInfo& messageInfo)
{
    return true;
}

bool PluginBase::processMIDIEvent(midiEvent& event)
{
    return true;
}

bool PluginBase::reset(ResetInfo& resetInfo)
{
    // --- update param smoothers
    for(std::vector<PluginParameter*>::iterator it = pluginParameters.begin(); it !=  pluginParameters.end(); ++it)
    {
        PluginParameter* piParam = *it;
        if(piParam)
            piParam->updateSampleRate(resetInfo.sampleRate);
    }

    return true;
}

void PluginBase::initPluginParameterArray()
{
	if (pluginParameterArray)
		pluginParameterArray;

	numPluginParameters = pluginParameters.size();

	pluginParameterArray = new PluginParameter*[numPluginParameters];
	for (unsigned int i = 0; i < numPluginParameters; i++)
	{
		pluginParameterArray[i] = pluginParameters[i];
	}
}

bool PluginBase::processAudioBuffers(ProcessBufferInfo& processBufferInfo)
{
    memset(&inputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&outputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&auxInputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);
    memset(&auxOutputFrame, 0, sizeof(float)*MAX_CHANNEL_COUNT);

    /* info.inputAPISpecificData = processBufferInfo.inputAPISpecificData;
     info.outputAPISpecificData = processBufferInfo.outputAPISpecificData;
     info.auxInputAPISpecificData = processBufferInfo.auxInputAPISpecificData;
     info.auxOutputAPISpecificData = processBufferInfo.auxOutputAPISpecificData;*/

    double sampleInterval = 1.0/audioProcDescriptor.sampleRate;

    if(pluginDescriptor.processFrames)
    {
        ProcessFrameInfo info;

        info.inputData = &inputFrame[0];
        info.outputData = &outputFrame[0];
        info.auxInputData = &auxInputFrame[0];
        info.auxOutputData = &auxOutputFrame[0];

        info.channelIOConfig = processBufferInfo.channelIOConfig;
        info.auxChannelIOConfig = processBufferInfo.auxChannelIOConfig;

        info.numInputChannels = processBufferInfo.numInputChannels;
        info.numOutputChannels = processBufferInfo.numOutputChannels;
        info.numAuxInputChannels = processBufferInfo.numAuxInputChannels;
        info.numAuxOutputChannels = processBufferInfo.numAuxOutputChannels;

        info.hostInfo = processBufferInfo.hostInfo;
        info.midiEventQueue = processBufferInfo.midiEventQueue;

        // --- sync internal bound variables
        preProcessAudioBuffers(processBufferInfo);

        // --- build frames, one sample from each channel
		for (uint32_t frame = 0; frame<processBufferInfo.numFramesToProcess; frame++)
        {
			for (uint32_t i = 0; i<processBufferInfo.numInputChannels; i++)
            {
                inputFrame[i] = processBufferInfo.inputs[i][frame];
            }

			for (uint32_t i = 0; i<processBufferInfo.numAuxInputChannels; i++)
            {
                auxInputFrame[i] = processBufferInfo.auxInputs[i][frame];
            }

            info.currentFrame = frame;

            // -- process the frame of data
            processAudioFrame(info);

			for (uint32_t i = 0; i<processBufferInfo.numOutputChannels; i++)
            {
                processBufferInfo.outputs[i][frame] = outputFrame[i];
            }
			for (uint32_t i = 0; i<processBufferInfo.numAuxInputChannels; i++)
            {
                processBufferInfo.auxOutputs[i][frame] = auxOutputFrame[i];
            }

            // --- update per-frame
            info.hostInfo->uAbsoluteFrameBufferIndex += 1;
            info.hostInfo->dAbsoluteFrameBufferTime += sampleInterval;
        }

        // --- generally not used
        postProcessAudioBuffers(processBufferInfo);

        return true; /// processed
    }

    return false; /// processed
}

// --- currently not used
bool PluginBase::guiParameterChanged(int32_t controlID, double actualValue)
{
    return true;
}
