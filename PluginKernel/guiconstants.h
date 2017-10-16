//
//  guiconstants.h
//
//  Created by Will Pirkle on 3/25/17.
//
//

#ifndef _guiconstants_h
#define _guiconstants_h

#define _MATH_DEFINES_DEFINED

#include <stdint.h>
#include <stdlib.h>

// --- RESERVED PARAMETER ID VALUES
const unsigned int PLUGIN_SIDE_BYPASS = 131072; // VST3 only
const unsigned int TRACKPAD = 131073;
const unsigned int VECTOR_JOYSTICK = 131074;
const unsigned int PRESET_NAME = 131075;
const unsigned int WRITE_PRESET_FILE = 131076;
const unsigned int SCALE_GUI_SIZE = 131077;

// --- enum for the GUI object's message processing
enum { tinyGUI, verySmallGUI, smallGUI, normalGUI, largeGUI, veryLargeGUI };

// --- can add to this as the above RESERVED PARAMETER ID VALUES are added
inline bool isReservedTag(int tag)
{
	if (tag == PLUGIN_SIDE_BYPASS ||
		tag == TRACKPAD ||
		tag == VECTOR_JOYSTICK ||
		tag == PRESET_NAME ||
		tag == WRITE_PRESET_FILE ||
		tag == SCALE_GUI_SIZE)
		return true;

	return false;
}

const double kCTCoefficient = 5.0 / 12.0;
const float kTwoPi = 2.f*3.14159265358979323846264338327950288419716939937510582097494459230781640628620899f;

const uint32_t ENVELOPE_DETECT_MODE_PEAK = 0;
const uint32_t ENVELOPE_DETECT_MODE_MS = 1;
const uint32_t ENVELOPE_DETECT_MODE_RMS = 2;
const uint32_t ENVELOPE_DETECT_MODE_NONE = 3;

const size_t XYPAD_TAG_X = 'XYTX';
const size_t XYPAD_TAG_Y = 'XYTY';
const size_t CUSTOMVIEW_DATA = 'CVDT';

const float ENVELOPE_DIGITAL_TC = -2.f; // log(1%)
const float ENVELOPE_ANALOG_TC = -0.43533393574791066201247090699309f; // (log(36.7%)
const float GUI_METER_UPDATE_INTERVAL_MSEC = 50.f;
const float GUI_METER_MIN_DB = -60.f;
#define FLT_EPSILON_PLUS      1.192092896e-07         /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_EPSILON_MINUS    -1.192092896e-07         /* smallest such that 1.0-FLT_EPSILON != 1.0 */
#define FLT_MIN_PLUS          1.175494351e-38         /* min positive value */
#define FLT_MIN_MINUS        -1.175494351e-38         /* min negative value */

// --- for math.h constants
#define _MATH_DEFINES_DEFINED

#define enumToInt(x) static_cast<int>(x)
#define compareEnumToInt(x,y) (static_cast<int>(x)==(y))
#define compareIntToEnum(x,y) ((x)==static_cast<int>(y))
#define convertEnum(x,y) static_cast<y>(x)

#include <math.h>
#include "guiconstants.h"

enum class smoothingMethod { kLinearSmoother, kLPFSmoother };
enum class taper { kLinearTaper, kLogTaper, kAntiLogTaper, kVoltOctaveTaper };
enum class meterCal { kLinearMeter, kLogMeter };
enum class controlVariableType { kFloat, kDouble, kInt, kTypedEnumStringList, kMeter, kNonVariableBoundControl };
enum class boundVariableType { kFloat, kDouble, kInt, kUInt };

template <class T>
class ParamSmoother
{
public:
	ParamSmoother() { a = 0.0; b = 0.0; z = 0.0; z2 = 0.0; }

	void setSampleRate(T samplingRate)
	{
		sampleRate = samplingRate;

		// --- for LPF smoother
		a = exp(-kTwoPi / (smoothingTimeInMSec * 0.001 * sampleRate));
		b = 1.0 - a;

		// --- for linear smoother
		linInc = (maxVal - minVal) / (smoothingTimeInMSec * 0.001 * sampleRate);
	}

	void initParamSmoother(T smoothingTimeInMs,
		T samplingRate,
		T initValue,
		T minControlValue,
		T maxControlValue,
		smoothingMethod smoother = smoothingMethod::kLPFSmoother)
	{
		minVal = minControlValue;
		maxVal = maxControlValue;
		sampleRate = samplingRate;
		smoothingTimeInMSec = smoothingTimeInMs;

		setSampleRate(samplingRate);

		// --- storage
		z = initValue;
		z2 = initValue;
	}

	inline bool smoothParameter(T in, T& out)
	{
		if (smootherType == smoothingMethod::kLPFSmoother)
		{
			z = (in * b) + (z * a);
			if (z == z2)
			{
				out = in;
				return false;
			}
			z2 = z;
			out = z2;
			return true;
		}
		else // if (smootherType == smoothingMethod::kLinearSmoother)
		{
			if (in == z)
			{
				out = in;
				return false;
			}
			if (in > z)
			{
				z += linInc;
				if (z > in) z = in;
			}
			else if (in < z)
			{
				z -= linInc;
				if (z < in) z = in;
			}
			out = z;
			return true;
		}
	}

private:
	T a = 0.0;
	T b = 0.0;
	T z = 0.0;
	T z2 = 0.0;

	T linInc = 0.0;

	T minVal = 0.0;
	T maxVal = 1.0;

	T sampleRate = 44100;
	T smoothingTimeInMSec = 100.0;

	smoothingMethod smootherType = smoothingMethod::kLPFSmoother;
};


#endif
