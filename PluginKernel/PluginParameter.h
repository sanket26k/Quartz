#ifndef _PluginParameter_H_
#define _PluginParameter_H_

#include <string>
#include <vector>
#include <sstream>
#include <atomic>
#include <map>

#include <math.h>
#include "pluginstructures.h"
//#include "plugininterfaces.h"
#include "guiconstants.h"

class PluginParameter
{
public:
    // --- constructor for continuous controls
    PluginParameter(int _controlID, const char* _controlName, const char* _controlUnits,
                    controlVariableType _controlType, double _minValue, double _maxValue, double _defaultValue,
                    taper _controlTaper = taper::kLinearTaper, uint32_t _displayPrecision = 2);

    // --- constructor 1 for string-list controls
    PluginParameter(int _controlID, const char* _controlName, std::vector<std::string> _stringList, std::string _defaultString);

    // --- constructor 2 for string-list controls
    PluginParameter(int _controlID, const char* _controlName, const char* _commaSeparatedList, std::string _defaultString);

    // --- constructor for meter controls
    PluginParameter(int _controlID, const char* _controlName, double _meterAttack_ms, double _meterRelease_ms,
                    uint32_t _detectorMode, meterCal _meterCal = meterCal::kLinearMeter);

    // --- constructor for NonVariableBoundControl
    PluginParameter(int _controlID, const char* _controlName = "", controlVariableType _controlType = controlVariableType::kNonVariableBoundControl);

    // --- empty constructor
    PluginParameter();

    // --- copy constructor
    PluginParameter(const PluginParameter& initGuiControl);

    virtual ~PluginParameter();

    uint32_t getControlID() { return controlID; }
    void setControlID(uint32_t cid) { controlID = cid; }

    const char* getControlName() { return controlName.c_str(); }
    void setControlName(const char* name) { controlName.assign(name); }

    const char* getControlUnits() { return controlUnits.c_str(); }
    void setControlUnits(const char* units) { controlUnits.assign(units); }

    controlVariableType getControlVariableType() { return controlType; }
    void setControlVariableType(controlVariableType ctrlVarType) { controlType = ctrlVarType; }

    double getMinValue() { return minValue; }
    void setMinValue(double value) { minValue = value; }

    double getMaxValue() { return maxValue; }
    void setMaxValue(double value) { maxValue = value; }

    double getDefaultValue() { return defaultValue; }
    void setDefaultValue(double value) { defaultValue = value; }

	void setIsDiscreteSwitch(bool _isDiscreteSwitch) { isDiscreteSwitch = _isDiscreteSwitch; }
	bool getIsDiscreteSwitch() { return isDiscreteSwitch; }

	void setControlTaper(taper ctrlTaper) { controlTaper = ctrlTaper; }
	taper getControlTaper() { return controlTaper; }


    // -- taper getters
    bool isLinearTaper() { return controlTaper == taper::kLinearTaper ? true : false; }
    bool isLogTaper() { return controlTaper == taper::kLogTaper ? true : false; }
    bool isAntiLogTaper() { return controlTaper == taper::kAntiLogTaper ? true : false; }
    bool isVoltOctaveTaper() { return controlTaper == taper::kVoltOctaveTaper ? true : false; }

    // --- control type getters
    bool isMeterParam() { return controlType == controlVariableType::kMeter ? true : false; }
    bool isStringListParam() { return controlType == controlVariableType::kTypedEnumStringList ? true : false; }
    bool isFloatParam() { return controlType == controlVariableType::kFloat ? true : false; }
    bool isDoubleParam() { return controlType == controlVariableType::kDouble ? true : false; }
    bool isIntParam() { return controlType == controlVariableType::kInt ? true : false; }
    bool isNonVariableBoundParam() { return controlType == controlVariableType::kNonVariableBoundControl ? true : false; }

    uint32_t getDisplayPrecision() { return displayPrecision; }
    void setDisplayPrecision(uint32_t precision) { displayPrecision = precision; }

    double getMeterAttack_ms() { return meterAttack_ms;}
    void setMeterAttack_ms(double value) { meterAttack_ms = value; }

    double getMeterRelease_ms() { return meterRelease_ms; }
    void setMeterRelease_ms(double value) { meterRelease_ms = value; }

    uint32_t getDetectorMode() { return detectorMode; }
    void setMeterDetectorMode(uint32_t value) { detectorMode = value; }

	bool getLogMeter() { return logMeter; }
	void setLogMeter(bool value) { logMeter = value; }

	bool getInvertedMeter() { return invertedMeter; }
	void setInvertedMeter(bool value) { invertedMeter = value; }

	bool getParameterSmoothing() { return useParameterSmoothing; }
    void setParameterSmoothing(bool value) { useParameterSmoothing = value; }

    double getSmoothingTimeMsec() { return smoothingTimeMsec;}
    void setSmoothingTimeMsec(double value) { smoothingTimeMsec = value; }

    smoothingMethod getSmoothingMethod() { return smoothingType; }
    void setSmoothingMethod(smoothingMethod smoothingMethod) { smoothingType = smoothingMethod; }

    bool getIsWritable() { return isWritable; }
    void setIsWritable(bool value) { isWritable = value; }
    
    bool getEnableVSTSampleAccurateAutomation() { return enableVSTSampleAccurateAutomation; }
    void setEnableVSTSampleAccurateAutomation(bool value) { enableVSTSampleAccurateAutomation = value; }

	// --- for aux attributes
	AuxParameterAttribute* getAuxAttribute(uint32_t attributeID);
	uint32_t setAuxAttribute(uint32_t attributeID, const AuxParameterAttribute& auxParameterAtribute);

    // --- Ordinary accesses deal with the actual value ---
    //
    // --- getter
    inline double getControlValue()
    {
        return getAtomicControlValueDouble();
    }

    inline std::string getControlValueAsString()
    {
        std::string empty;
        if(controlType == controlVariableType::kTypedEnumStringList)
        {
            if((uint32_t)getAtomicControlValueFloat() >= stringList.size())
                return empty;

            return stringList[(uint32_t)getAtomicControlValueFloat()];
        }

        std::ostringstream ss;
        ss << getAtomicControlValueFloat();
        std::string numString(ss.str());

        if(numString == "-inf" || numString == "inf")
            return numString;

        size_t pos = numString.find('.');
        if(pos == std::string::npos)
        {
            numString += ".00000000";
            pos = numString.find('.');
        }

        numString += "00000000000000000000000000000000";

        if(controlType != controlVariableType::kInt)
            pos += displayPrecision+1;

        std::string formattedString = numString.substr(0, pos);
        if(appendUnits)
        {
            formattedString += " ";
            formattedString += controlUnits;
        }

        return formattedString;
    }

    size_t getStringCount(){return stringList.size();}
    const char* getCommaSeparatedStringList() {return commaSeparatedStringList.c_str();}
    void setCommaSeparatedStringList();

    void setStringList(std::vector<std::string> _stringList) {stringList = _stringList;}

    std::string getStringByIndex(uint32_t index)
    {
        std::string empty;
        if(index >= stringList.size())
            return empty;

        return stringList[index];
    }

    // --- setter
    inline void setControlValue(double actualParamValue, bool ignoreSmoothing = false)
    {
        if(controlType == controlVariableType::kDouble ||
           controlType == controlVariableType::kFloat)
        {
            if(useParameterSmoothing && !ignoreSmoothing)
                setSmoothedTargetValue(actualParamValue);
            else
                setAtomicControlValueDouble(actualParamValue);
        }
         else
             setAtomicControlValueDouble(actualParamValue);
    }


    // --- GUI Accesses ---
    //     all GUI values are normalized in and out of the object; object handles the tapering
    //
    // --- default value (AAX)
     inline double getDefaultValueNormalized()
    {
        // --- apply taper as needed
        switch (controlTaper)
        {
            case taper::kLinearTaper:
                return getNormalizedDefaultValue();
                
            case taper::kLogTaper:
                return getNormalizedLogDefaultValue();
                
            case taper::kAntiLogTaper:
                return getNormalizedAntiLogDefaultValue();
                
            case taper::kVoltOctaveTaper:
                return getNormalizedVoltOctaveDefaultValue();
                
            default:
                return 0.0;
        }
    }

    
    // --- getter
    inline double getControlValueNormalized()
    {
        // --- apply taper as needed
        switch (controlTaper)
        {
            case taper::kLinearTaper:
                return getNormalizedControlValue();

            case taper::kLogTaper:
                return getNormalizedLogControlValue();

            case taper::kAntiLogTaper:
                return getNormalizedAntiLogControlValue();

            case taper::kVoltOctaveTaper:
                return getNormalizedVoltOctaveControlValue();

            default:
                return 0.0;
        }
    }
    
    inline double getControlValueWithNormalizedValue(double normalizedValue, bool applyTaper = true)
    {
        if(!applyTaper)
            return getControlValueFromNormalizedValue(normalizedValue);
        
        double newValue = 0;

        // --- apply taper as needed
        switch (controlTaper)
        {
            case taper::kLinearTaper:
                newValue = getControlValueFromNormalizedValue(normalizedValue);
                break;

            case taper::kLogTaper:
                newValue = getControlValueFromNormalizedValue(normToLogNorm(normalizedValue));
                break;

            case taper::kAntiLogTaper:
                newValue = getControlValueFromNormalizedValue(normToAntiLogNorm(normalizedValue));
                break;

            case taper::kVoltOctaveTaper:
                newValue = getVoltOctaveControlValueFromNormValue(normalizedValue);
                break;

            default:
                break; // --- no update
        }

        return newValue;
    }

    // --- conversion routine, does not access control value!
    inline double getNormalizedControlValueWithActualValue(double actualValue)
    {
        double newValue = getNormalizedControlValueWithActual(actualValue);

        return newValue;
    }

    // --- setter: used in VST3 and RAFX as AU/AAX take actual values; returns actual value that was seet from normalized value
    inline double setControlValueNormalized(double normalizedValue, bool applyTaper = true, bool ignoreParameterSmoothing = false)
    {
        // --- set according to smoothing option
        double actualParamValue = getControlValueWithNormalizedValue(normalizedValue, applyTaper);

        if(controlType == controlVariableType::kDouble ||
           controlType == controlVariableType::kFloat)
        {
            if(useParameterSmoothing && !ignoreParameterSmoothing)
                setSmoothedTargetValue(actualParamValue);
            else
                setAtomicControlValueDouble(actualParamValue);
        }
        else
            setAtomicControlValueDouble(actualParamValue);

		return actualParamValue;
    }

    double getGUIMin()
    {
        return 0.0;
    }

    double getGUIMax()
    {
        if(controlType == controlVariableType::kTypedEnumStringList)
            return (double)getStringCount() - 1;

        return 1.0;
    }

    int findStringIndex(std::string searchString)
    {
        auto it = std::find(stringList.begin (), stringList.end (), searchString);

        if (it == stringList.end())
        {
            return -1;
        }
        else
        {
            return (int)std::distance(stringList.begin(), it);
        }

        return -1;
    }

    // --- normalized to Log-normalized version (convex transform)
    inline double normToLogNorm(double normalizedValue)
    {
        return 1.0 + kCTCoefficient*log10(normalizedValue);
    }

    // --- Log-normalized to normalized version (reverse-convex transform)
    inline double logNormToNorm(double logNormalizedValue)
    {
        return pow(10.0, (logNormalizedValue - 1.0) / kCTCoefficient);
    }

    //  --- normalized to AntiLog-normalized version
    inline double normToAntiLogNorm(double normalizedValue)
    {
		if (normalizedValue == 1.0)
			return 1.0;

		double aln = -kCTCoefficient*log10(1.0 - normalizedValue);
		aln = fmin(1.0, aln);
		return aln;
	}

    //  --- AntiLog-normalized to normalized version
    inline double antiLogNormToNorm(double aLogNormalizedValue)
    {
        return -pow(10.0, (-aLogNormalizedValue / kCTCoefficient)) + 1.0;
    }

    PluginParameter& operator=(const PluginParameter& aPluginParameter)	// need this override for collections to work
    {
        if(this == &aPluginParameter)
            return *this;

        controlID = aPluginParameter.controlID;
        controlName = aPluginParameter.controlName;
		controlUnits = aPluginParameter.controlUnits;
		commaSeparatedStringList = aPluginParameter.commaSeparatedStringList;
        controlType = aPluginParameter.controlType;
        minValue = aPluginParameter.minValue;
        maxValue = aPluginParameter.maxValue;
        defaultValue = aPluginParameter.defaultValue;
        controlTaper = aPluginParameter.controlTaper;
        controlValueAtomic = aPluginParameter.getAtomicControlValueFloat();
        displayPrecision = aPluginParameter.displayPrecision;
        stringList = aPluginParameter.stringList;
        useParameterSmoothing = aPluginParameter.useParameterSmoothing;
        smoothingType = aPluginParameter.smoothingType;
        smoothingTimeMsec = aPluginParameter.smoothingTimeMsec;
        meterAttack_ms = aPluginParameter.meterAttack_ms;
        meterRelease_ms = aPluginParameter.meterRelease_ms;
		detectorMode = aPluginParameter.detectorMode;
        logMeter = aPluginParameter.logMeter;
        isWritable = aPluginParameter.isWritable;
		isDiscreteSwitch = aPluginParameter.isDiscreteSwitch;
		invertedMeter = aPluginParameter.invertedMeter;

        return *this;
    }

    void initParamSmoother(double sampleRate)
    {
        paramSmoother.initParamSmoother(smoothingTimeMsec,
                                        sampleRate,
                                        getAtomicControlValueDouble(),
                                        minValue,
                                        maxValue,
                                        smoothingType);
    }

    void updateSampleRate(double sampleRate)
    {
        paramSmoother.setSampleRate(sampleRate);
    }

    bool smoothParameterValue()
    {
        if(!useParameterSmoothing) return false;
        double smoothedValue = 0.0;
        bool smoothed = paramSmoother.smoothParameter(getSmoothedTargetValue(), smoothedValue);
        setAtomicControlValueDouble(smoothedValue);
        return smoothed;
    }
  
	// --- link a bound variable for easy updating
    void setBoundVariable(void* boundVariable, boundVariableType dataType)
    {
		boundVariableDataType = dataType;

        if(dataType == boundVariableType::kDouble)
            boundVariableDouble = (double*)boundVariable;
        else if(dataType == boundVariableType::kFloat)
            boundVariableFloat = (float*)boundVariable;
        else if(dataType == boundVariableType::kInt)
            boundVariableInt = (int*)boundVariable;
        else if(dataType == boundVariableType::kUInt)
            boundVariableUInt = (uint32_t*)boundVariableUInt;
    }

	boundVariableType getBoundVariableType() { return boundVariableDataType; }

    // --- Bound Variable updaters
	bool updateInBoundVariable()
	{
		if (boundVariableUInt)
		{
			*boundVariableUInt = (uint32_t)getControlValue();
			return true;
		}
		else if (boundVariableInt)
		{
			*boundVariableInt = (int)getControlValue();
			return true;
		}
		else if (boundVariableFloat)
		{
			*boundVariableFloat = (float)getControlValue();
			return true;
		}
		else if (boundVariableDouble)
		{
			*boundVariableDouble = getControlValue();
			return true;
		}
		return false;
	}

	bool updateOutBoundVariable()
	{
		if (boundVariableUInt)
		{
			setControlValue((double)*boundVariableUInt);
			return true;
		}
		else if (boundVariableInt)
		{
			setControlValue((double)*boundVariableInt);
			return true;
		}
		else if (boundVariableFloat)
		{
			setControlValue((double)*boundVariableFloat);
			return true;
		}
		else if (boundVariableDouble)
		{
			setControlValue(*boundVariableDouble);
			return true;
		}
		return false;
	}

    void setParameterUpdateQueue(IParameterUpdateQueue* _parameterUpdateQueue) { parameterUpdateQueue = _parameterUpdateQueue; }
    IParameterUpdateQueue* getParameterUpdateQueue() { return parameterUpdateQueue; } // may be NULL - that is OK

protected:
    // --- control description stuff
    int controlID = -1;
    std::string controlName = "ControlName";
    std::string controlUnits = "Units";
    controlVariableType controlType = controlVariableType::kDouble; // --- default is Continuous Controller Type

    // --- min/max/def
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;

    // --- *the* control value
    // --- atomic float as control value
    //     atomic double will not behave properly between 32/64 bit
    std::atomic<float> controlValueAtomic;
    
    void setAtomicControlValueFloat(float value){ controlValueAtomic.store(value, std::memory_order_relaxed); }
    float getAtomicControlValueFloat() const { return controlValueAtomic.load(std::memory_order_relaxed); }
    
    void setAtomicControlValueDouble(double value){ controlValueAtomic.store((float)value, std::memory_order_relaxed); }
    double getAtomicControlValueDouble() const { return (double)controlValueAtomic.load(std::memory_order_relaxed); }
    
    std::atomic<float> smoothedTargetValueAtomic;
    void setSmoothedTargetValue(double value){ smoothedTargetValueAtomic.store((float)value); }
    double getSmoothedTargetValue() const { return (double)smoothedTargetValueAtomic.load(); }

    // --- control tweakers
    taper controlTaper = taper::kLinearTaper;
    uint32_t displayPrecision = 2;

    // --- for enumerated string list
    std::vector<std::string> stringList;
    std::string commaSeparatedStringList;

    // --- gui specific
    bool appendUnits = true;
    bool isWritable = false;
	bool isDiscreteSwitch = false;

    // --- for VU meters
    double meterAttack_ms = 10.0;
    double meterRelease_ms = 500.0;
    uint32_t detectorMode = ENVELOPE_DETECT_MODE_RMS;
	bool logMeter = false;
	bool invertedMeter = false;

    // --- parameter smoothing
    bool useParameterSmoothing = false;
    smoothingMethod smoothingType = smoothingMethod::kLPFSmoother;
    double smoothingTimeMsec = 100.0;
    ParamSmoother<double> paramSmoother;

	// --- variable binding
	boundVariableType boundVariableDataType = boundVariableType::kFloat;

    // --- our sample accurate interface for VST3
    IParameterUpdateQueue* parameterUpdateQueue = nullptr;
    
    // --- default is enabled; you can disable this for controls that have a long postUpdate cooking time
    bool enableVSTSampleAccurateAutomation = true;

    // --- get volt/octave control value from a normalized value
    inline double getVoltOctaveControlValueFromNormValue(double normalizedValue)
    {
        double octaves = log2(maxValue / minValue);
        if (normalizedValue == 0)
            return minValue;

        return minValue*pow(2.0, normalizedValue*octaves);
    }

    //  --- get log control value in normalized form
    inline double getNormalizedLogControlValue()
    {
        return logNormToNorm(getNormalizedControlValue());
    }

    //  --- get anti-log control value in normalized form
    inline double getNormalizedAntiLogControlValue()
    {
        return antiLogNormToNorm(getNormalizedControlValue());
    }

    //  --- get volt/octave control value in normalized form
    inline double getNormalizedVoltOctaveControlValue()
    {
        if (minValue == 0)
            return getAtomicControlValueDouble();

        return log2(getAtomicControlValueDouble() / minValue) / (log2(maxValue / minValue));
    }

    // --- for GUI accesses; these do not change underlying variable
    inline double getNormalizedControlValueWithActual(double actualValue)
    {
        // --- calculate normalized value from actual
        return (actualValue - minValue) / (maxValue - minValue);
    }

    // --- plain, no taper applied
    inline double getNormalizedControlValue()
    {
        // --- calculate normalized value from actual
        return (getAtomicControlValueDouble() - minValue) / (maxValue - minValue);
    }

    // --- plain, no taper applied
    inline double getControlValueFromNormalizedValue(double normalizedValue)
    {
        // --- calculate the control Value using normalized input
        return (maxValue - minValue)*normalizedValue + minValue;
    }

    // --- for default values (for AAX support only)
    inline double getNormalizedDefaultValue()
    {
        // --- calculate normalized value from actual
        return (defaultValue - minValue) / (maxValue - minValue);
    }
    
    //  --- get log default value in normalized form
    inline double getNormalizedLogDefaultValue()
    {
        return logNormToNorm(getNormalizedDefaultValue());
    }

    //  --- get anti-log default value in normalized form
    inline double getNormalizedAntiLogDefaultValue()
    {
        return antiLogNormToNorm(getNormalizedDefaultValue());
    }
    
    //  --- get volt/octave control value in normalized form
    inline double getNormalizedVoltOctaveDefaultValue()
    {
        if (minValue == 0)
            return defaultValue;
        
        return log2(defaultValue / minValue) / (log2(maxValue / minValue));
    }


private:
    // --- for variable-binding support
    uint32_t* boundVariableUInt = nullptr;
    int* boundVariableInt = nullptr;
    float* boundVariableFloat = nullptr;
    double* boundVariableDouble = nullptr;

	// --- Aux attributes that can be stored on this object (similar to VSTGUI4) makes it easy to add extra data in the future
	typedef std::map<uint32_t, AuxParameterAttribute*> auxParameterAttributeMap;
	auxParameterAttributeMap auxAttributeMap;

};

#endif

