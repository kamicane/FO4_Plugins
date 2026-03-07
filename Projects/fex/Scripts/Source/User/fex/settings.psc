ScriptName fex:settings hidden native

; Settings
; Uses the current running script name as the settings $NAME

; Loads settings from disk into memory cache
; Reads both MCM/Config/$NAME/settings.ini (the defaults) and MCM/Settings/$NAME.ini (the overrides)
bool function load() global native

; Saves current in-memory settings to disk
; Reads defaults from MCM/Config and writes only to user MCM/Settings
; Values equal to defaults are removed from user settings
bool function save() global native

; Returns string setting value from memory cache, or default if key is missing/invalid
string function getString (string aSection, string aKey, string aDefaultValue = "") global native
; Sets string setting value in memory cache
function setString (string aSection, string aKey, string aValue) global native

; Returns bool setting value from memory cache, or default if key is missing/invalid
bool function getBool (string aSection, string aKey, bool aDefaultValue = false) global native
; Sets bool setting value in memory cache
function setBool (string aSection, string aKey, bool aValue) global native

; Returns int setting value from memory cache, clamped to [aMinValue, aMaxValue], or default
int function getInt (string aSection, string aKey, int aDefaultValue = 0, int aMinValue = -2147483647, int aMaxValue = 2147483647) global native
; Sets int setting value in memory cache
function setInt (string aSection, string aKey, int aValue) global native

; Returns float setting value from memory cache, clamped to [aMinValue, aMaxValue], or default
float function getFloat (string aSection, string aKey, float aDefaultValue = 0.0, float aMinValue = -340282346638528860000000000000000000000.0, float aMaxValue = 340282346638528860000000000000000000000.0) global native
; Sets float setting value in memory cache
function setFloat (string aSection, string aKey, float aValue) global native

; Reads all supported struct fields from memory cache section into aStruct
function getStruct (string aSection, var aStruct) global native
; Writes all supported struct fields from aStruct into section
function setStruct (string aSection, var aStruct) global native
