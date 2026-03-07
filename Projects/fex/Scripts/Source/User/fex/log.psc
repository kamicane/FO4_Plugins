ScriptName fex:log hidden native

; Logging
; - Uses the current running script name as the log $NAME. File will be written to Documents/My Games/Fallout4/F4SE/$NAME.log
; - Before logging, reads `[Logging] Level` from settings. If level >= chosen level, it won't log
; - Settings are only read once

function info (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
function warn (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
function error (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
function debug (string aFmtBase, var a1 = none, var a2 = none, var a3 = none, var a4 = none, var a5 = none, var a6 = none, var a7 = none, var a8 = none, var a9 = none) global native
