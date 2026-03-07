ScriptName fex:debug hidden native

function printInventory (ObjectReference objectRef, Keyword hasKeyword) global native

struct InventoryFilter
	Keyword[] keywords = none
	string[] formTypes = none
	bool isLegendary = false
	bool isQuestItem = false
endStruct

; function transfer (ObjectReference sourceRef, ObjectReference targetRef, InventoryFilter inclusionFilter = none, InventoryFilter exclusionFilter = none) global native

bool function transfer (ObjectReference sourceRef, ObjectReference targetRef, Keyword transferKeyword = none) global native

string function getCallerName () global native

function searchRefs(ObjectReference sourceRef, float radius) global native

function scrapRef(ObjectReference objectRef) global native
