ScriptName fex hidden native

; Messages
; Supports fmt::format placeholders

function notification (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
function consoleLog (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
string function format (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native



; Register new name for $NAME
; must be called once per session, on every script using this
; otherwise will default to the name of the running script
; string function Register (string aMapName) global native

; string function RequestText () global
; 	bool ok = _OpenTextInputMenu()
; 	if (!ok)
; 		return ""
; 	endif

; 	return _GetLastTextInputResult()
; endFunction

bool function updatePowerArmor3d (ObjectReference aRef) global native

Form function getForm (string editorId) global native
