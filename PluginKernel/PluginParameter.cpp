#include "PluginParameter.h"

// --- constructor for continuous controls
PluginParameter::PluginParameter(int _controlID, const char* _controlName, const char* _controlUnits,
                                 controlVariableType _controlType, double _minValue, double _maxValue, double _defaultValue,
                                 taper _controlTaper, uint32_t _displayPrecision)
: controlID(_controlID)
, controlName(_controlName)
, controlUnits(_controlUnits)
, controlType(_controlType)
, minValue(_minValue)
, maxValue(_maxValue)
, defaultValue(_defaultValue)
, controlTaper(_controlTaper)
, displayPrecision(_displayPrecision)
{
    setControlValue(_defaultValue);
    setSmoothedTargetValue(_defaultValue);
    useParameterSmoothing = false;
    setIsWritable(false);
}

// --- constructor 1 for string-list controls
PluginParameter::PluginParameter(int _controlID, const char* _controlName, std::vector<std::string> _stringList, std::string _defaultString)
: controlID(_controlID)
, controlName(_controlName)
, stringList(_stringList)
{
    setControlValue(0.0);
    setSmoothedTargetValue(0.0);
    setControlVariableType(controlVariableType::kTypedEnumStringList);
    setMaxValue((double)stringList.size()-1);

    int defaultStringIndex = findStringIndex(_defaultString);
    if(defaultStringIndex >= 0)
    {
        setDefaultValue((double)defaultStringIndex);
        setControlValue((double)defaultStringIndex);
    }
    useParameterSmoothing = false;
    setIsWritable(false);
    setCommaSeparatedStringList();
}

// --- constructor 2 for string-list controls
PluginParameter::PluginParameter(int _controlID, const char* _controlName, const char* _commaSeparatedList, std::string _defaultString)
: controlID(_controlID)
, controlName(_controlName)
{
    setControlValue(0.0);
    setSmoothedTargetValue(0.0);

    std::stringstream ss(_commaSeparatedList);
    while(ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');
        stringList.push_back(substr);
    }
	
	// --- create csvlist
	setCommaSeparatedStringList();

    setControlVariableType(controlVariableType::kTypedEnumStringList);
    setMaxValue((double)stringList.size()-1);

    int defaultStringIndex = findStringIndex(_defaultString);
    if(defaultStringIndex >= 0)
    {
        setDefaultValue((double)defaultStringIndex);
        setControlValue((double)defaultStringIndex);
    }
    useParameterSmoothing = false;
    setIsWritable(false);
    setCommaSeparatedStringList();
}


// --- constructor for meter controls
PluginParameter::PluginParameter(int _controlID, const char* _controlName, double _meterAttack_ms, double _meterRelease_ms, uint32_t _detectorMode, meterCal _meterCal)
: controlID(_controlID)
, controlName(_controlName)
, meterAttack_ms(_meterAttack_ms)
, meterRelease_ms(_meterRelease_ms)
, detectorMode(_detectorMode)
{
	if (_meterCal == meterCal::kLinearMeter)
		setLogMeter(false);
	else
		setLogMeter(true);

    setControlValue(0.0);
    setSmoothedTargetValue(0.0);

    setControlVariableType(controlVariableType::kMeter);
    useParameterSmoothing = false;
    setIsWritable(true);
}

// --- constructor for NonVariableBoundControl
PluginParameter::PluginParameter(int _controlID, const char* _controlName, controlVariableType _controlType)
: controlID(_controlID)
, controlName(_controlName)
, controlType(_controlType)
{
    setControlValue(0.0);
    setSmoothedTargetValue(0.0);

    useParameterSmoothing = false;
    setIsWritable(false);
}

PluginParameter::PluginParameter()
{
    setControlValue(0.0);
    setSmoothedTargetValue(0.0);

    useParameterSmoothing = false;
    setIsWritable(false);
}

// --- copy constructor
PluginParameter::PluginParameter(const PluginParameter& initGuiControl)
{
    controlID = initGuiControl.controlID;
    controlName = initGuiControl.controlName;
	controlUnits = initGuiControl.controlUnits;
	commaSeparatedStringList = initGuiControl.commaSeparatedStringList;
	controlType = initGuiControl.controlType;
    minValue = initGuiControl.minValue;
    maxValue = initGuiControl.maxValue;
    defaultValue = initGuiControl.defaultValue;
    controlValueAtomic = initGuiControl.getAtomicControlValueFloat();
    controlTaper = initGuiControl.controlTaper;
    displayPrecision = initGuiControl.displayPrecision;
    stringList = initGuiControl.stringList;
    useParameterSmoothing = initGuiControl.useParameterSmoothing;
    smoothingType = initGuiControl.smoothingType;
    smoothingTimeMsec = initGuiControl.smoothingTimeMsec;
    meterAttack_ms = initGuiControl.meterAttack_ms;
    meterRelease_ms = initGuiControl.meterRelease_ms;
	detectorMode = initGuiControl.detectorMode;
    logMeter = initGuiControl.logMeter;
    isWritable = initGuiControl.isWritable;
	isDiscreteSwitch = initGuiControl.isDiscreteSwitch;
	invertedMeter = initGuiControl.invertedMeter;
}

PluginParameter::~PluginParameter()
{
	// --- clear aux attributes
	for (auxParameterAttributeMap::const_iterator it = auxAttributeMap.begin(), end = auxAttributeMap.end(); it != end; ++it)
	{
		delete it->second;
	}

	auxAttributeMap.clear();
}

void PluginParameter::setCommaSeparatedStringList()
{
    commaSeparatedStringList.clear();

    for(std::vector<std::string>::iterator it = stringList.begin(); it != stringList.end(); ++it)
    {
        std::string subStr = *it;
        if(commaSeparatedStringList.size() > 0)
            commaSeparatedStringList.append(",");
        commaSeparatedStringList.append(subStr);
    }
}

uint32_t PluginParameter::setAuxAttribute(uint32_t attributeID, const AuxParameterAttribute& auxParameterAtribute)
{
	AuxParameterAttribute* newAttribute = new AuxParameterAttribute(auxParameterAtribute);
	if (!newAttribute) return 0;

	auxAttributeMap.insert(std::make_pair(attributeID, newAttribute));

	return (uint32_t)auxAttributeMap.size();
}

AuxParameterAttribute* PluginParameter::getAuxAttribute(uint32_t attributeID)
{
	if (auxAttributeMap.find(attributeID) == auxAttributeMap.end()) {
		return nullptr;
	}

	return  auxAttributeMap[attributeID];
}


