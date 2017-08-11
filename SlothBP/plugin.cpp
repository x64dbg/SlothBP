#include "plugin.h"
#include "Utf8Ini.h"
#include "resource.h"
#include "strUtil.h"

//Original idea by https://github.com/blaquee

struct API
{
    std::string apiName;
    std::string bpName;
    std::string category;
    bool enabled = false;
};

enum MenuEntries
{
    MENU_RELOAD = 99999,
    MENU_ABOUT = 100000
};

static std::vector<API> apis;
static wchar_t apiFile[MAX_PATH];

static bool LoadApis(const wchar_t* apiFile)
{
    apis.clear();
    auto hFile = CreateFileW(apiFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    auto result = false;
    if(hFile != INVALID_HANDLE_VALUE)
    {
        auto size = GetFileSize(hFile, nullptr);
        if(size)
        {
            std::vector<char> iniData(size + 1, '\0');
            DWORD read = 0;
            if(ReadFile(hFile, iniData.data(), size, &read, nullptr))
            {
                Utf8Ini ini;
                int errorLine;
                if(ini.Deserialize(iniData.data(), errorLine))
                {
                    API curApi;
                    for(auto & section : ini.Sections())
                    {
                        curApi.category = section;
                        for(auto & key : ini.Keys(section))
                        {
                            curApi.bpName = key;
                            curApi.apiName = ini.GetValue(section, key);
                            apis.push_back(curApi);
                        }
                    }
                    _plugin_logprintf("[" PLUGIN_NAME "] Loaded %d APIs in %d categories!\n", int(apis.size()), int(ini.Sections().size()));
                    result = true;
                }
                else
                    _plugin_logprintf("[" PLUGIN_NAME "] Error in API file at line %d...\n", errorLine);
            }
            else
                _plugin_logputs("[" PLUGIN_NAME "] Failed to read file...");
        }
        else
            _plugin_logputs("[" PLUGIN_NAME "] File is empty...");
        CloseHandle(hFile);
    }
    else
        _plugin_logputs("[" PLUGIN_NAME "] Failed to open file...");
    return result;
}

static bool SetupMenus()
{
    if(apis.empty())
        return false;
    std::unordered_map<std::string, int> categories;
    for(size_t i = 0; i < apis.size(); i++)
    {
        const auto & api = apis[i];
        if(!categories.count(api.category))
        {
            auto cMenu = _plugin_menuadd(hMenu, api.category.c_str());
            if(cMenu == -1)
            {
                _plugin_logputs("[" PLUGIN_NAME "] Failed to add menu item");
                return false;
            }
                
            categories[api.category] = cMenu;
        }
        auto cMenu = categories[api.category];
        auto hEntry = int(i);
        if(!_plugin_menuaddentry(cMenu, hEntry, api.bpName.c_str()))
        {
            _plugin_logputs("[" PLUGIN_NAME "] Failed to add sub menu item");
            return false;
        }
        _plugin_menuentrysetchecked(pluginHandle, hEntry, false);
    }
    auto hResInfo = FindResourceW(hInst, MAKEINTRESOURCEW(IDB_SLOTH), L"PNG");
    auto hResData = LoadResource(hInst, hResInfo);
    auto pData = LockResource(hResData);
    ICONDATA icon;
    icon.data = pData;
    icon.size = SizeofResource(hInst, hResInfo);
    _plugin_menuseticon(hMenu, &icon);
    _plugin_menuaddentry(hMenu, MENU_RELOAD, "Reload Config");
    _plugin_menuaddentry(hMenu, MENU_ABOUT, "About");
    return true;
}

bool ReloadConfig()
{
    bool ret = true;

    // Clear the menu
    if(_plugin_menuclear(hMenu))
    {
        // Load the API file
        if(LoadApis(apiFile))
        {
            // Setup the menu items for new config
            if(SetupMenus())
                return ret;
            else
            {
                _plugin_logputs("SLOTHBP Menu setup failed");
                ret = false;
            }
        }
    }
    return ret;
}

static void refreshStatus(CBTYPE type, const char* modulename)
{
    if(apis.empty())
        return;
    if(!modulename)
        return;
    if(type == CB_STOPDEBUG)
    {
        // Check for any invalid entries to remove
        // TODO: find a better way to do this if needed.
        for(size_t i = 0; i < apis.size(); ++i)
        {
            auto & api = apis[i];
            auto addr = Eval(api.apiName.c_str());
            auto oldenabled = api.enabled;
            api.enabled = addr ? (DbgGetBpxTypeAt(addr) & bp_normal) != 0 : false;
            if(api.enabled != oldenabled) //only waste GUI time if an update is needed
                _plugin_menuentrysetchecked(pluginHandle, int(i), api.enabled);
        }
		return;
    }

    for(size_t i = 0; i < apis.size(); ++i)
    {
        //determine if we care about this module.
        auto modnameList = split(apis[i].apiName, ':');
        auto modname = modnameList.at(0);
        if(strstr(modulename, modname.c_str()) == NULL)
        {
            //skip this module, we don't need to incur cycles to resolve it
            continue;
        }
        else
        {
            auto & api = apis[i];
            auto addr = Eval(api.apiName.c_str());
            auto oldenabled = api.enabled;
            api.enabled = addr ? (DbgGetBpxTypeAt(addr) & bp_normal) != 0 : false;
            if(api.enabled != oldenabled) //only waste GUI time if an update is needed
                _plugin_menuentrysetchecked(pluginHandle, int(i), api.enabled);
        }
    }
}

PLUG_EXPORT void CBCREATEPROCESS(CBTYPE cbType, PLUG_CB_CREATEPROCESS* info)
{
    refreshStatus(cbType, info->DebugFileName);
}

PLUG_EXPORT void CBLOADDLL(CBTYPE cbType, PLUG_CB_LOADDLL* info)
{
    refreshStatus(cbType, info->modname);
}

PLUG_EXPORT void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info)
{
    refreshStatus(cbType, nullptr);
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
    if(info->hEntry >= 0 && info->hEntry < int(apis.size()))
    {
        auto & api = apis[info->hEntry];
        if(DbgIsDebugging())
        {
            auto addr = Eval(api.apiName.c_str());
            if(addr)
            {
                auto bpType = DbgGetBpxTypeAt(addr);
                char cmd[256] = "";
                if(api.enabled) //already enabled -> try to disable
                {
                    if(bpType & bp_normal) //enabled in the debuggee -> try to disable
                    {
                        sprintf_s(cmd, "bc %p", addr);
                        if(DbgCmdExecDirect(cmd))
                            api.enabled = false;
                    }
                    else //already disabled in the debuggee -> nothing to do
                    {
                        _plugin_logputs("Breakpoint already disabled...");
                        api.enabled = false;
                    }
                }
                else //not yet enabled -> try to enable
                {
                    if(bpType & bp_normal) //already enabled in debuggee -> nothing to do
                    {
                        _plugin_logputs("Breakpoint already enabled...");
                        api.enabled = true;
                    }
                    else
                    {
                        sprintf_s(cmd, "bp %p", addr);
                        if(DbgCmdExecDirect(cmd))
                            api.enabled = true;
                    }
                }
            }
            else
                _plugin_logprintf("[" PLUGIN_NAME "] Failed to resolve api %s (%s)...\n", api.bpName.c_str(), api.apiName.c_str());
        }
        else
            _plugin_logputs("[" PLUGIN_NAME "] Not debugging...");
        _plugin_menuentrysetchecked(pluginHandle, info->hEntry, api.enabled);
    }
    else if(info->hEntry == MENU_RELOAD)
    {
        if(!ReloadConfig())
            MessageBoxW(GuiGetWindowHandle(), L"Error Loading new config", L"Error", MB_ICONERROR);
    }
    else if(info->hEntry == MENU_ABOUT)
        MessageBoxW(GuiGetWindowHandle(), L"SlothBP\n\nIcon from shareicon.net", L"About", MB_ICONINFORMATION);
    else
        _plugin_logprintf("[" PLUGIN_NAME "] Unknown menu entry %d...\n", info->hEntry);
}

#define EXPAND(x) L ## x
#define DOWIDEN(x) EXPAND(x)

bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    GetModuleFileNameW(hInst, apiFile, _countof(apiFile));
    auto l = wcsrchr(apiFile, L'\\');
    if(l)
        *l = L'\0';
    wcsncat_s(apiFile, L"\\" DOWIDEN(PLUGIN_NAME) L".ini", _TRUNCATE);

    if(!LoadApis(apiFile))
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Failed to load API file %S...\n", apiFile);
        return false;
    }
    return true;
}

bool pluginStop()
{
    return true;
}

void pluginSetup()
{
    if(!SetupMenus())
        _plugin_menuclear(hMenu);
}
