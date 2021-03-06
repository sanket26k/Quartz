//
//  plugindescription.h
//  VTestA
//
//  Created by Will Pirkle on 5/16/17.
//
//

#ifndef _plugindescription_h
#define _plugindescription_h

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define AU_COCOA_VIEWFACTORY_STRING STR(AU_COCOA_VIEWFACTORY_NAME)
#define AU_COCOA_VIEW_STRING STR(AU_COCOA_VIEW_NAME)

// --- STEP 1 for all plugins: change these variables accordingly

// --- FOR AU PLUGINS ONLY ------------------------------------------------- //
//     try to make this as unique as possible (Cocoa has a flat namespace)
//     here I'm appending the VST3 FUID string (unique but must be generated for each plugin)
#define AU_COCOA_VIEWFACTORY_NAME AUCocoaViewFactory_B34142F90D054816853D4B8E445B1E4F
#define AU_COCOA_VIEW_NAME AUCocoaView_B34142F90D054816853D4B8E445B1E4F

// --- MacOS Bundle Identifiers
//     NOTE: these ID strings must EXACTLY MATCH the PRODUCT_BUNDLE_IDENTIFIER setting in Build Settings for the Plugin Target
const char* kAAXBundleID = "prismatechnologies.aax.Quartz.bundleID";
const char* kVST3BundleID = "prismatechnologies.vst3.Quartz.bundleID";

// --- For AU: make sure BOTH the Plugin Target and the CocoaUI Target PRODUCT_BUNDLE_IDENTIFIER settings match these strings
const char* kAUBundleID = "prismatechnologies.au.Quartz.bundleID";

// --- plugin NAME
/* AU only: this MUST EXACLTY match the Product Name in Build Settings for BOTH the CocoaUI and your Plugin Targets! */
const char* kPluginName = "Quartz";     // 31 chars max for AAX

// --- other
const char* kShortPluginName = "Quartz";    // 15 chars max for AAX
const pluginType kPluginType = pluginType::kSynthPlugin;

// --- vendor info
const char* kVendorName = "Prisma Technologies";
const char* kVendorURL = "";
const char* kVendorEmail = "";

// --- VST3 & AAX only
const int32_t kFourCharCode = 'QRTZ'; /// must be unique for each plugin in your company

// --- VST3 specific, see www.willpirkle.com/forum/ for information on generating FUIDs
//     use GUIDGEN.exe from compiler/tools (Win) or UUID Generator (free Mac App)
const char* kVSTFUID = "{B34142F9-0D05-4816-853D-4B8E445B1E4F}"; /// NOTE: you need the enclosing { }

// --- AAX specific
const int32_t kAAXManufacturerID = 'PRSM';
const int32_t kAAXProductID = 'qrtz';


#endif

