/*	handles loading a DLL and interfacing with the result

-load dll config during startup config (to get prototype)
-use moar templates for PROTOTYPE or default to windows style
-make interface more flexable so DLLs arent so specific

*/

#pragma once

#define LOG_MODULE_ERRORS

#include "winderps.h"

//ext types
const enum TYPES {
	TYPE_CAMERA = 1,	//camera extension
	TYPE_TRACKING,		//tracking extension
	TYPE_APPLICATION	//applicatoin
};
//extention constructor prototypes
const enum PROTOTYPES {
	PROTO_VOID = 1,	//void constructor				class(void);
	PROTO_STRING,	//string constructor prototype	class(std::string argv);
	PROTO_WINDOWS	//windows style prototype		class(std::vector<std::string> argv);
};
//one entry for each type of dll
const enum dirInds {
	D_CAM,
	D_TRACK,
	D_APP
};


//init info for a module
struct ModuleInit {
	//basic info and args
	std::string path = "", name = "", entry = "", args = "";
	std::vector<std::string> _args;
	//type of extension, function prototype
	int type = -1, proto = -1;
	bool load = false;
};

typedef std::vector<ModuleInit> ModuleConfigs;


template<typename T> class Module {
private:
	typedef T(__stdcall *f_alloc)();
	typedef T(__stdcall *f_allocStr)(std::string args);
	typedef T(__stdcall *f_allocWin)(std::vector<std::string> _args);
	typedef void(__stdcall *f_dest)(T);

	f_alloc create;
	f_allocStr createStr;
	f_allocWin createWin;
	f_dest destroy;

	ModuleInit info;
	HINSTANCE dll;
	T instance;
	bool open = false;
	
public:
	//construct
	Module<T>() = default;
	//construct module from init
	Module<T>(ModuleInit &modinit) : info(modinit) { init(); }

	//getters
	bool isOpen() { return open; }
	bool shouldLoad() { return info.load; }
	std::string getName() { return info.name; }
	T getInst() { if (open) return instance; else return nullptr; }

	//setters
	void shouldLoad(bool v) { info.load = v; }
	void setName(std::string &_name) { info.name = _name; }

	//allow conversions to T
	operator T() { if (open) return instance; else return nullptr; }
	//overload -> to return instance
	T operator ->() { if (open) return instance; else return nullptr; }


	//load construct and destruct functions from DLL
	bool getFuncProto() {
		//open DLL
		dll = openDLL(toWstr(info.path));
		if (dll == NULL) {
			return false;
		}
		//resolve function address
		FARPROC func = loadFunction(dll, info.entry);
		//check if exists
		if (!func) {
			closeDLL(dll);
			return false;
		}

		//resolve function prototype
		if (info.proto == PROTOTYPES::PROTO_VOID) create = (f_alloc)func;
		else if (info.proto == PROTOTYPES::PROTO_STRING) createStr = (f_allocStr)func;
		else if (info.proto == PROTOTYPES::PROTO_WINDOWS) createWin = (f_allocWin)func;
		else return false;

		//resolve destroy function address
		FARPROC dest = loadFunction(dll, std::string("destroy"));
		//check if exists
		if (!dest) {
			closeDLL(dll);
			return false;
		}
		destroy = (f_dest)dest;
		return true;
	}

	//init
	void init() {
		if (!info.load) return;
		//try to prototype the constructor function
		if (!getFuncProto()) return;

		//try to use the constructor
		if (info.proto == PROTO_VOID) instance = create();
		else if (info.proto == PROTO_STRING) instance = createStr(info.args);
		else if (info.proto == PROTO_WINDOWS) instance = createWin(info._args);
		else {
#ifdef LOG_MODULE_ERRORS
			std::string protoArgs[3] = { "void", "std::string args", "std::vector<std::string> _args" };
			printf("call to %s(%s) in %s failed\n", info.entry.c_str(), protoArgs[int(info.proto) - 1].c_str(), info.path.c_str());
#endif
			return;
		}

		//check if instance succeeded
		if (instance == nullptr) {
#ifdef LOG_MODULE_ERRORS
			printf("null instance of %s\n", info.path.c_str());
#endif
			return;
		}

		open = true;
	}

	//clone instance
	T newInst() {
		if (info.proto == PROTO_VOID) return create();
		if (info.proto == PROTO_STRING) return createStr(info.args);
		if (info.proto == PROTO_WINDOWS) return createWin(info._args);
		return nullptr;
	}

	//destruct
	~Module() {
		closeDLL(dll);
		if (instance != nullptr) destroy(instance);
	}
};


//load DLL configuration
static bool configure(ModuleInit &modInit) {
	//get configuration
	std::vector<std::string> opts;
	std::string cfgpath = modInit.path.substr(0, modInit.path.length() - 3) + "cfg";
	if (!openTxt(cfgpath, opts)) {
#ifdef LOG_MODULE_ERRORS
		printf("config file %s not found\n", cfgpath.c_str());
#endif
		return false;
	}
	//parse config
	for (int i = 0; i < opts.size(); i++) {
		std::string opt = opts[i];
		//ignore blanks and #comments
		if (opt.length() < 3 || opt[0] == '#') continue;
		int ofs = opt.find_first_of(':', 0);
		//try to split around ':'
		if (ofs < 1 || ofs > opt.length() - 1) continue;
		//get key/val pair
		std::string key = trim(opt.substr(0, ofs)),
			val = trim(opt.substr(ofs + 1, opt.length() - ofs - 1));
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		if (key == "name") {
			modInit.name = val;
		}
		else if (key == "type") {
			std::transform(val.begin(), val.end(), val.begin(), ::tolower);
			if (val == "camera") modInit.type = TYPES::TYPE_CAMERA;
			else if (val == "tracker") modInit.type = TYPES::TYPE_TRACKING;
			else if (val == "app") modInit.type = TYPES::TYPE_APPLICATION;
			else {
#ifdef LOG_MODULE_ERRORS
				printf("config %s unknown type '%s'\n", modInit.path.c_str(), val.c_str());
#endif
				return false;
			}
		}
		else if (key == "func") {
			modInit.entry = val;
			std::transform(val.begin(), val.end(), val.begin(), ::tolower);
			if (val == "allocate") modInit.proto = PROTOTYPES::PROTO_VOID;
			else if (val == "allocatestr") modInit.proto = PROTOTYPES::PROTO_STRING;
			else if (val == "allocatewin") modInit.proto = PROTOTYPES::PROTO_WINDOWS;
			else {
#ifdef LOG_MODULE_ERRORS
				printf("config %s unsupported function '%s'\n", modInit.path.c_str(), val.c_str());
#endif
				return false;
			}
		}
		else if (key == "args") {
			if (!modInit.proto) {
#ifdef LOG_MODULE_ERRORS
				printf("config %s error: first specify 'func' before 'args'\n", modInit.path.c_str());
#endif
				return false;
			}
			modInit.args = val;
		}
		else {
#ifdef LOG_MODULE_ERRORS
			printf("config %s unsupported option '%s'\n", modInit.path.c_str(), key.c_str());
#endif
			return false;
		}
	}
	return true;
}
//load startup.cfg
static bool loadStartupCFG(std::vector<ModuleConfigs> &directories) {
	directories.clear();
	directories.resize(3);
	//get configuration
	std::vector<std::string> opts;
	std::string path = "cfg\\startup.cfg";
	if (!openTxt(path, opts)) {
#ifdef LOG_MODULE_ERRORS
		std::printf("startup file %s not found\n", path.c_str());
#endif
		return false;
	}
	//parse config
	int dirInd = -1;
	std::vector<std::pair<bool, std::string>> conditionals;
	//global scope: true, no function
	conditionals.push_back(std::pair<bool, std::string>(true, ""));
	for (int i = 0; i < opts.size(); i++) {
		std::string opt = trim(opts[i]);
		//skip if line is too short, #comments
		if (opt.length() < 3 || opt[0] == '#') continue;
		if (opt[0] == '{') {
			//initialize condition
			std::string optname = opt.substr(1, opt.length() - 1);
			trim(optname);
			bool found = false;
			//look in cameras
			for (int j = 0; j < directories[D_CAM].size(); j++) {
				if (directories[D_CAM][j].name == optname) {
					conditionals.push_back(std::pair<bool, std::string>(true, optname));
					found = true; break;
				}
			}
			if (found) continue;
			//look in trackers
			for (int j = 0; j < directories[D_TRACK].size(); j++) {
				if (directories[D_TRACK][j].name == optname) {
					conditionals.push_back(std::pair<bool, std::string>(true, optname));
					found = true; break;
				}
			}
			if (found) continue;
			//look in apps last
			for (int j = 0; j < directories[D_APP].size(); j++) {
				if (directories[D_APP][j].name == optname) {
					conditionals.push_back(std::pair<bool, std::string>(true, optname));
					found = true; break;
				}
			}
			if (found) continue;
			conditionals.push_back(std::pair<bool, std::string>(false, optname));
			continue;
		}
		else if (opt[0] == '}') {
			std::string optname = opt.substr(1, opt.length() - 1);
			trim(optname);
			//search for opt in conditionals
			for (int j = 0; j < conditionals.size(); j++) {
				if (conditionals[j].second == optname) {
					std::swap(conditionals[j], conditionals.back());
					conditionals.pop_back();
					break;
				}
			}
			continue;
		}
		//if current condition does not permit processing
		if (!conditionals.back().first) continue;
		//lower case for comparison
		std::transform(opt.begin(), opt.end(), opt.begin(), ::tolower);
		//switches
		if (opt == "camera") {
			dirInd = D_CAM;
			continue;
		}
		else if (opt == "tracker") {
			dirInd = D_TRACK;
			continue;
		}
		else if (opt == "app") {
			dirInd = D_APP;
			continue;
		}
		//if state permits loading directories
		if (dirInd == -1) continue;
		bool shouldLoad = true;
		if (opt[0] == '-') shouldLoad = false;
		else if (opt[0] == '+') shouldLoad = true;
		else continue; //unknown option
		directories[dirInd].push_back(ModuleInit());
		ModuleInit &modref = directories[dirInd].back();
		modref.path = trim(opt.substr(1, opt.length() - 1));
		modref.load = shouldLoad;
		//if cant configure forget about it
		if (!configure(modref)) {
			printf("Failed to configure %s\n", modref.path.c_str());
			directories[dirInd].pop_back();
		}
	}
	return true;
}
//Must manually free successfully loaded modules!
template <typename T> static void loadModules(ModuleConfigs &conf, std::vector<Module<T>*> &outp) {
	for (int c = 0; c < conf.size(); c++) {
		printf("\t%s\n", conf[c].name.c_str());
		outp.push_back(new Module<T>(conf[c]));
	}
}
