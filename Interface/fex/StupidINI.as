package fex {
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import flash.events.Event;
	import flash.events.IOErrorEvent;

	public class StupidINI {

		public var sections:Object;
		private var defaultsSections:Object;

		private var sectionRegex:RegExp = /^\s*\[\s*(.+?)\s*\]\s*$/;
		private var trimStartRegex:RegExp = /^\s+/;
		private var trimEndRegex:RegExp = /\s+$/;

		private var id:String;
		private var callback:Function;
		private var index:int = 0;
		private var fileNames:Array;

		public function StupidINI(modName:String, defaults:Object = null) {
			defaultsSections = defaults;
			id = modName;

			fileNames = [
					"../MCM/Config/" + id + "/settings.ini", // defaults
					"../MCM/Settings/" + id + ".ini" // overrides
				];
		}

		public function Dump():String {
			var dump:String = "";
			for (var sectionName:String in sections) {
				dump += "\n\t[" + sectionName + "]";
				var sectionObj:Object = sections[sectionName];
				for (var key:String in sectionObj) {
					dump += "\n\t" + key + " = " + sectionObj[key];
				}
			}
			return dump;
		}

		public function Reset ():void {
			sections = {};

			if (defaultsSections) {
				for (var section:String in defaultsSections) {
					sections[section] = {};
					for (var key:String in defaultsSections[section]) {
						sections[section][key] = defaultsSections[section][key];
					}
				}
			}
		}

		public function Load(onComplete:Function):void {
			index = 0;
			callback = onComplete;
			Reset();
			LoadNextFile();
		}

		private function LoadNextFile():void {
			if (index >= fileNames.length) {
				if (callback != null)
					callback();
				return;
			}

			var path:String = fileNames[index];
			var loader:URLLoader = new URLLoader();

			loader.addEventListener(Event.COMPLETE, function(e:Event):void {
					Parse(loader.data);
					index++;
					LoadNextFile();
				});

			loader.addEventListener(IOErrorEvent.IO_ERROR, function(e:IOErrorEvent):void {
					trace("[StupidINI] Failed to load file:", path, e.text);
					index++;
					LoadNextFile();
				});

			loader.load(new URLRequest(path));
		}

		public function Parse(text:String):void {
			var lines:Array = text.split(/\r?\n/);
			var currentSection:String = "Global";

			for each (var line:String in lines) {
				if (!line)
					continue;

				var commentIndex:int = line.indexOf(";");
				if (commentIndex >= 0)
					line = line.substr(0, commentIndex);

				line = Trim(line);
				if (!line)
					continue;

				var match:Array = sectionRegex.exec(line);
				if (match) {
					currentSection = Trim(match[1]);
					continue;
				}

				var indexEq:int = line.indexOf("=");
				if (indexEq > 0) {
					var rawKey:String = line.substr(0, indexEq);
					var rawValue:String = line.substr(indexEq + 1);
					var key:String = Trim(rawKey);
					var value:String = Trim(rawValue);
					if (!sections[currentSection]) {
						sections[currentSection] = {};
					}
					sections[currentSection][key] = value;
				}
			}
		}

		public function GetString(section:String, key:String, defaultValue:String = ""):String {
			if (sections[section] && sections[section][key] != undefined && sections[section][key] != null) {
				return sections[section][key];
			}

			return defaultValue;
		}

		public function GetNumber(section:String, key:String, defaultValue:Number = 0):Number {
			var numberRaw:String = GetString(section, key);

			if (!numberRaw) {
				return defaultValue;
			}

			var numberParsed:Number = Number(numberRaw);
			if (!isNaN(numberParsed)) {
				return numberParsed;
			}

			return defaultValue;
		}

		public function GetColor(section:String, key:String, defaultColor:uint = 0):uint {
			var colorRaw:String = GetString(section, key);

			if (!colorRaw) {
				return defaultColor;
			}

			if (colorRaw.indexOf("#") == 0) {
				colorRaw = colorRaw.substr(1);
			} else if (colorRaw.indexOf("0x") == 0) {
				colorRaw = colorRaw.substr(2);
			}

			// Accept 6 (RRGGBB) or 8 (RRGGBBAA) hex digits.
			// For 6-digit input assume alpha = 0xFF and return AARRGGBB.
			// For 8-digit input interpret as RRGGBBAA and return AARRGGBB.
			if (!/^[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$/.test(colorRaw)) {
				return defaultColor;
			}

			var parsed:uint = uint(parseInt(colorRaw, 16));

			if (colorRaw.length == 6) {
				return (0xFF << 24) | parsed;
			}

			// colorRaw.length == 8: parsed is 0xRRGGBBAA. Extract AA and RR GG BB,
			// then return 0xAARRGGBB.
			var aa:uint = parsed & 0xFF;
			var rrggbb:uint = parsed >>> 8;
			return (aa << 24) | rrggbb;
		}

		public function Trim(str:String):String {
			if (str == null)
				return "";

			return str.replace(trimStartRegex, "").replace(trimEndRegex, "");
		}
	}
}
