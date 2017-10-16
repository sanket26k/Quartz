//
//  PluginBase.h
//  VolGUI_I
//
//  Created by Will Pirkle on 3/31/17.
//
//

#ifndef __PluginBase__
#define __PluginBase__

#include "PluginParameter.h"

#include <map>

class PluginBase
{
public:
    PluginBase();
    virtual ~PluginBase();

    // --- overrides: pure virtual
	virtual bool updatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo) = 0;
	virtual bool updatePluginParameterNormalized(int32_t controlID, double normalizedValue, ParameterUpdateInfo& paramInfo) = 0;
	virtual bool postUpdatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo) = 0;
    virtual bool processAudioFrame(ProcessFrameInfo& processFrameInfo) = 0;

    virtual bool guiParameterChanged(int32_t controlID, double actualValue); //, doubvle normalizedValue) = 0;

    // --- overrides: non-pure-virtual
    virtual bool reset(ResetInfo& resetInfo);
	virtual bool setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData); 

    // --- messaging system for GUI
    virtual bool processMessage(MessageInfo& messageInfo);
    virtual bool processMIDIEvent(midiEvent& event);

    // --- override theis to process your own buffers
    virtual bool processAudioBuffers(ProcessBufferInfo& processInfo);

    // --- must implement, but may not be used
    virtual bool postProcessAudioBuffers(ProcessBufferInfo& processInfo) = 0;
    virtual bool preProcessAudioBuffers(ProcessBufferInfo& processInfo) = 0;

    // --- frame-accurate updates
  //  void doVSTSampleAccurateParamUpdates();
//	void smoothParameterValues();
	void doSampleAccurateParameterUpdates();

	// --- bound variables
	void syncInBoundVariables();
	bool updateOutBoundVariables();

    // --- the add-method that the derived class uses to add the param
    int32_t addPluginParameter(PluginParameter* piParam, double sampleRate = 44100);
	void setParamAuxAttribute(uint32_t controlID, const AuxParameterAttribute& auxAttribute);

    // --- getters
    size_t getPluginParameterCount(){ return pluginParameters.size(); }

    // --- different ways of getting a PluginParameter
    PluginParameter* getPluginParameterByIndex(int32_t index);
    PluginParameter* getPluginParameterByControlID(int32_t controlID);
    PluginParameter* getNextParameterOfType(int32_t& startIndex, controlVariableType controlType);

    // --- get control value in various formats
    double getPIParamValueDouble(int32_t controlID);
    float getPIParamValueFloat(int32_t controlID);
    int getPIParamValueInt(int32_t controlID);
    uint32_t getPIParamValueUInt(int32_t controlID);

    // --- compare the string with the control value's selection (string list variables)
    bool compareSelectedString(int32_t controlID, const char* compareString);

    // --- description stuff
    bool hasSidechain(){ return pluginDescriptor.hasSidechain; }
    bool wantsMIDI(){ return pluginDescriptor.wantsMIDI; }
    bool hasCustomGUI(){ return pluginDescriptor.hasCustomGUI; }
    double getLatencyInSamples(){ return pluginDescriptor.latencyInSamples; }
    double getTailTimeInMSec(){ return pluginDescriptor.tailTimeInMSec; }
    bool wantsInfiniteTailVST3(){ return pluginDescriptor.infiniteTailVST3; }

    const char* getPluginName(){ return pluginDescriptor.pluginName.c_str(); }
    const char* getShortPluginName(){ return pluginDescriptor.shortPluginName.c_str(); }
    const char* getVendorName(){ return pluginDescriptor.vendorName.c_str(); }
    uint32_t getPluginType() { return pluginDescriptor.pluginTypeCode; }
    double getSampleRate() { return audioProcDescriptor.sampleRate; }


    // --- API/Platform Specific info; setup in the PluginCore constructor
    int getFourCharCode(){ return apiSpecificInfo.fourCharCode; }

    const uint32_t getAAXManufacturerID() { return apiSpecificInfo.aaxManufacturerID; }    // --- change this to your own code
    const uint32_t getAAXProductID() { return apiSpecificInfo.aaxProductID; }         // --- must be unique among your plugins
    const char* getAAXBundleID() { return apiSpecificInfo.aaxBundleID.c_str(); }
    const char* getAAXEffectID(){ return apiSpecificInfo.aaxEffectID.c_str(); }
    uint32_t getAAXPluginCategory() { return apiSpecificInfo.aaxPluginCategoryCode; }

    //static const char* getVST3_FUID(){ return apiSpecificInfo.vst3FUID.c_str(); }
    const char* getVST3_FUID(){ return apiSpecificInfo.vst3FUID.c_str(); }
    bool wantsVST3SampleAccurateAutomation() { return apiSpecificInfo.enableVST3SampleAccurateAutomation; }
    uint32_t getVST3SampleAccuracyGranularity() { return apiSpecificInfo.vst3SampleAccurateGranularity; }
    const char* getVST3BundleID() { return apiSpecificInfo.vst3BundleID.c_str(); }


    const char* getAUBundleID() { return apiSpecificInfo.auBundleID.c_str(); }


    // --- channel IO support
    uint32_t getNumSupportedIOCombinations() {return pluginDescriptor.numSupportedIOCombinations;}
    bool hasSupportedInputChannelFormat(uint32_t channelFormat)
    {
        for(uint32_t i=0; i< getNumSupportedIOCombinations(); i++)
        {
            if(getChannelInputFormat(i) == channelFormat)
                return true;
        }
        return false;
    }

    bool hasSupportedOutputChannelFormat(uint32_t channelFormat)
    {
		for (uint32_t i = 0; i< getNumSupportedIOCombinations(); i++)
        {
            if(getChannelOutputFormat(i) == channelFormat)
                return true;
        }
        return false;
    }

    int32_t getChannelInputFormat(uint32_t ioConfigIndex) {return pluginDescriptor.supportedIOCombinations[ioConfigIndex].inputChannelFormat;}
    int32_t getChannelOutputFormat(uint32_t ioConfigIndex) {return pluginDescriptor.supportedIOCombinations[ioConfigIndex].outputChannelFormat;}

    uint32_t getInputChannelCount(uint32_t ioConfigIndex)
    {
        if(ioConfigIndex < pluginDescriptor.numSupportedIOCombinations)
            return pluginDescriptor.getChannelCountForChannelIOConfig(pluginDescriptor.supportedIOCombinations[ioConfigIndex].inputChannelFormat);

        return 0;
    }

    uint32_t getOutputChannelCount(uint32_t ioConfigIndex)
    {
        if(ioConfigIndex < pluginDescriptor.numSupportedIOCombinations)
            return pluginDescriptor.getChannelCountForChannelIOConfig(pluginDescriptor.supportedIOCombinations[ioConfigIndex].outputChannelFormat);

        return 0;
    }

    uint32_t getDefaultChannelIOConfigForChannelCount(uint32_t channelCount)
    {
        return pluginDescriptor.getDefaultChannelIOConfigForChannelCount(channelCount);
    }

    // --- setters
	void setPIParamValue(uint32_t _controlID, double _controlValue)
	{
		PluginParameter* piParam = getPluginParameterByControlID(_controlID);
		if (!piParam) return; /// not handled

		// --- set value
		piParam->setControlValue(_controlValue);
	}

	double setPIParamValueNormalized(uint32_t _controlID, double _normalizedValue, bool applyTaper = true)
	{
		PluginParameter* piParam = getPluginParameterByControlID(_controlID);
		if (!piParam) return 0.0; /// not handled

		// --- set value
		return piParam->setControlValueNormalized(_normalizedValue, applyTaper);
	}

	void clearUpdateGUIParameters(std::vector<GUIParameter*>& guiParameters)
    {
        // --- copy params, do not use copy constructor; it copies pointers
        for(std::vector<GUIParameter*>::iterator it = guiParameters.begin(); it !=  guiParameters.end(); ++it)
             delete *it;
    }

    std::vector<PluginParameter*>* makePluginParameterVectorCopy(bool disableSmoothing = true);
    bool initPresetParameters(std::vector<PresetParameter>& presetParameters, bool disableSmoothing = true);
    bool setPresetParameter(std::vector<PresetParameter>& presetParameters, uint32_t _controlID, double _controlValue);

    void setPluginHostConnector(IPluginHostConnector* _pluginHostConnector){pluginHostConnector = _pluginHostConnector;}

    size_t getPresetCount(){return presets.size();}
    const char* getPresetName(uint32_t index)
    {
        if(index < presets.size())
        {
            return presets[index]->presetName.c_str();
        }
        return "";
    }

    size_t addPreset(PresetInfo* preset)
    {
        presets.push_back(preset);
        return presets.size();
    }

    void removePreset(uint32_t index)
    {
        if(index < presets.size())
        {
            PresetInfo* preset = presets[index];
            // --- clean up
			for (uint32_t i = 0; i < preset->presetParameters.size(); i++)
                preset->presetParameters.pop_back();

            delete preset;
            presets.erase(presets.begin() + index);
        }
    }
    void removeAllPresets()
    {
        for(std::vector<PresetInfo*>::iterator it = presets.begin(); it != presets.end(); ++it)
        {
            //for(std::vector<PluginParameter*>::iterator it2 = (*it)->piParameters.begin(); it2 != (*it)->piParameters.end(); ++it2)
            //   delete *it2;
            // --- clean up
			for (uint32_t i = 0; i < (*it)->presetParameters.size(); i++)
                (*it)->presetParameters.pop_back();


            delete *it;
        }
        presets.clear();
    }

    PresetInfo* getPreset(uint32_t index)
    {
        if(index < presets.size())
            return presets[index];

        return nullptr;
    }

	void initPluginParameterArray();

protected:
    PluginDescriptor pluginDescriptor;
    AudioProcDescriptor audioProcDescriptor;
    APISpecificInfo apiSpecificInfo;

    // --- arrays for frame processing
    float inputFrame[MAX_CHANNEL_COUNT];
    float outputFrame[MAX_CHANNEL_COUNT];
    float auxInputFrame[MAX_CHANNEL_COUNT];
    float auxOutputFrame[MAX_CHANNEL_COUNT];

	// --- ultra-fast access for real-time audio processing
	PluginParameter** pluginParameterArray = nullptr;
	uint32_t numPluginParameters = 0;

    // --- vectorized version of pluginParameterMap for fast iteration when key not needed
    std::vector<PluginParameter*> pluginParameters;

    // --- map<controlID , PluginParameter*>
    typedef std::map<uint32_t, PluginParameter*> pluginParameterControlIDMap;
    pluginParameterControlIDMap pluginParameterMap;

    // --- plugin core -> host (wrap) connector
    IPluginHostConnector* pluginHostConnector; // created and destroyed on host

    // --- PRESETS
    std::vector<PresetInfo*> presets;
};

#endif /* defined(__PluginBase__) */
