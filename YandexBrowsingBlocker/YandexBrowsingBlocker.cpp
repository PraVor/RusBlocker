#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <tlhelp32.h>

const wchar_t CLASS_NAME[] = L"RusBlockerClass";
const wchar_t WINDOW_TITLE[] = L"Rus Blocker";
const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 500;

#define BTN_ENABLE_DOMAINS 1001
#define BTN_DISABLE_DOMAINS 1002
#define BTN_ENABLE_APPS 1003
#define BTN_DISABLE_APPS 1004

HFONT hFont = NULL;
HFONT hTitleFont = NULL;
HFONT hSmallFont = NULL;

const std::vector<std::string> YANDEX_PROCESSES = {
    "Yandex.exe",
    "YandexBrowser.exe",
    "browser.exe",
    "YandexMusic.exe",
    "YandexDisk.exe",
    "YandexNavigator.exe",
    "Alice.exe",
    "YandexStation.exe",
    "YaTaxi.exe",
    "YandexEda.exe",
    "YandexMarket.exe",
    "Max.exe"
};

const std::vector<std::string> BLOCKED_DOMAINS = {
    "yandex.ru",
    "ya.ru",
    "yastatic.net",
    "yandex.net",
    "yandex.by",
    "yandex.kz",
    "yandex.ua",
    "yandex.com",
    "yandex-team.ru",
    "www.yandex.ru",
    "www.ya.ru",
    "www.yandex.com",
    "download.max.ru",
    "max.ru",
    "yandex.team",
    "yandex.cloud",
    "yandex.st",
    "yandex.su",
    "yandex.taxi",
    "music.yandex.ru",
    "disk.yandex.ru",
    "mail.yandex.ru",
    "passport.yandex.ru",
    "kinopoisk.ru",
    "yandex360.ru",
    "vk.com",
    "vk.ru",
    "vkontakte.ru",
    "mail.ru",
    "my.mail.ru",
    "cloud.mail.ru",
    "ok.ru",
    "odnoklassniki.ru",
    "ok.me",
    "moymir.ru",
    "inbox.ru",
    "bk.ru",
    "list.ru",
    "games.mail.ru",
    "icq.com",
    "vkuseraudio.net",
    "rambler.ru",
    "lenta.ru",
    "gazeta.ru",
    "championat.com",
    "ria.ru",
    "rbc.ru",
    "tass.ru",
    "iz.ru",
    "kp.ru",
    "rg.ru",
    "rt.com",
    "ren.tv",
    "ntv.ru",
    "1tv.ru",
    "vesti.ru",
    "gosuslugi.ru",
    "gosuslugi.tech",
    "sber.ru",
    "sberbank.ru",
    "online.sberbank.ru",
    "tinkoff.ru",
    "acdn.tinkoff.ru",
    "vtb.ru",
    "alfabank.ru",
    "psbank.ru",
    "mts.ru",
    "mgts.ru",
    "beeline.ru",
    "tele2.ru",
    "rostelecom.ru",
    "rt.ru",
    "domru.ru",
    "ertelecom.ru",
    "ozon.ru",
    "wildberries.ru",
    "wb.ru",
    "dns-shop.ru",
    "mvideo.ru",
    "eldorado.ru",
    "cdek.ru",
    "ozon.travel",
    "citilink.ru",
    "yaplakal.com",
    "2gis.ru",
    "2gis.com",
    "auto.ru",
    "avito.ru",
    "fl.ru",
    "hh.ru",
    "hhcdn.ru",
    "superjob.ru",
    "amnezia.org",
    "amnezia.net",
    "adguard.com",
    "adguard.ru",
    "hidemy.name",
    "hide.me.ru",
    "sportmaster.ru"
};

const wchar_t* REG_PATH_IFEO = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";

const std::string BLOCK_START = "# RUS BLOCKER START";
const std::string BLOCK_END = "# RUS BLOCKER END";

std::string GetHostsFilePath() {
    char systemDir[MAX_PATH];
    if (GetSystemDirectoryA(systemDir, MAX_PATH) == 0) {
        return "";
    }
    return std::string(systemDir) + "\\drivers\\etc\\hosts";
}

bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

bool CreateBackup(const std::string& hostsPath) {
    std::string backupPath = hostsPath + ".backup";
    return CopyFileA(hostsPath.c_str(), backupPath.c_str(), FALSE) != 0;
}

bool EnableBlock() {
    std::string hostsPath = GetHostsFilePath();
    if (hostsPath.empty()) {
        return false;
    }

    CreateBackup(hostsPath);

    std::ifstream inFile(hostsPath);
    if (!inFile.is_open()) {
        return false;
    }

    std::stringstream buffer;
    std::string line;
    bool blockExists = false;

    while (std::getline(inFile, line)) {
        if (line.find(BLOCK_START) != std::string::npos) {
            blockExists = true;
            break;
        }
        buffer << line << "\n";
    }
    inFile.close();

    if (blockExists) {
        return true;
    }

    std::ofstream outFile(hostsPath, std::ios::app);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << "\n" << BLOCK_START << "\n";
    for (const auto& domain : BLOCKED_DOMAINS) {
        outFile << "0.0.0.0 " << domain << "\n";
    }
    outFile << BLOCK_END << "\n";
    outFile.close();

    return true;
}

bool DisableBlock() {
    std::string hostsPath = GetHostsFilePath();
    if (hostsPath.empty()) {
        return false;
    }

    CreateBackup(hostsPath);

    std::ifstream inFile(hostsPath);
    if (!inFile.is_open()) {
        return false;
    }

    std::stringstream buffer;
    std::string line;
    bool inBlockSection = false;
    bool blockFound = false;

    while (std::getline(inFile, line)) {
        if (line.find(BLOCK_START) != std::string::npos) {
            inBlockSection = true;
            blockFound = true;
            continue;
        }
        if (line.find(BLOCK_END) != std::string::npos) {
            inBlockSection = false;
            continue;
        }
        if (!inBlockSection) {
            buffer << line << "\n";
        }
    }
    inFile.close();

    if (!blockFound) {
        return true;
    }

    std::ofstream outFile(hostsPath, std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << buffer.str();
    outFile.close();

    return true;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

bool KillYandexProcesses() {
    bool anyKilled = false;

    for (const auto& procName : YANDEX_PROCESSES) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            continue;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring wProcName = StringToWString(procName);
                if (_wcsicmp(pe32.szExeFile, wProcName.c_str()) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess != NULL) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                        anyKilled = true;
                    }
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
    }

    return anyKilled;
}

bool EnableAppBlock() {
    HKEY hKey;
    bool success = true;

    for (const auto& procName : YANDEX_PROCESSES) {
        std::wstring wProcName = StringToWString(procName);
        std::wstring regPath = std::wstring(REG_PATH_IFEO) + L"\\" + wProcName;

        LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

        if (result == ERROR_SUCCESS) {
            const wchar_t* debugger = L"\"C:\\Windows\\System32\\systray.exe\"";
            RegSetValueExW(hKey, L"Debugger", 0, REG_SZ,
                (BYTE*)debugger, (DWORD)((wcslen(debugger) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
        else {
            success = false;
        }
    }

    KillYandexProcesses();

    return success;
}

bool DisableAppBlock() {
    bool success = true;

    for (const auto& procName : YANDEX_PROCESSES) {
        std::wstring wProcName = StringToWString(procName);
        std::wstring regPath = std::wstring(REG_PATH_IFEO) + L"\\" + wProcName;

        LONG result = RegDeleteTreeW(HKEY_LOCAL_MACHINE, regPath.c_str());
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
            success = false;
        }
    }

    return success;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hTitleFont = CreateFontW(
            32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );

        hFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );

        hSmallFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        HWND lblTitle = CreateWindowW(L"STATIC", L"Rus Blocker",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 20, 600, 40,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

        HWND lblDescription = CreateWindowW(L"STATIC",
            L"This application blocks Russian services (Yandex) by modifying the hosts file\n"
            L"and preventing applications from running. Safe and virus-free.\n"
            L"All changes are reversible. Requires administrator privileges.",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            30, 70, 540, 60,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblDescription, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

        CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
            30, 145, 540, 2,
            hwnd, NULL, NULL, NULL);

        HWND lblDomains = CreateWindowW(L"STATIC", L"Domain Blocking",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            30, 160, 540, 25,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblDomains, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND lblDomainsDesc = CreateWindowW(L"STATIC",
            L"Blocks access to Yandex domains (yandex.ru, ya.ru, etc.) via hosts file",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            30, 185, 540, 20,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblDomainsDesc, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

        HWND btnEnableDomains = CreateWindowW(L"BUTTON", L"Block",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            30, 215, 250, 40,
            hwnd, (HMENU)BTN_ENABLE_DOMAINS, NULL, NULL);
        SendMessage(btnEnableDomains, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND btnDisableDomains = CreateWindowW(L"BUTTON", L"Unblock",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            320, 215, 250, 40,
            hwnd, (HMENU)BTN_DISABLE_DOMAINS, NULL, NULL);
        SendMessage(btnDisableDomains, WM_SETFONT, (WPARAM)hFont, TRUE);

        CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
            30, 275, 540, 2,
            hwnd, NULL, NULL, NULL);

        HWND lblApps = CreateWindowW(L"STATIC", L"Application Blocking",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            30, 290, 540, 25,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblApps, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND lblAppsDesc = CreateWindowW(L"STATIC",
            L"Prevents Yandex applications from running (Browser, Music, Disk, etc.)\n"
            L"Will terminate all running Yandex processes",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            30, 315, 540, 35,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblAppsDesc, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

        HWND btnEnableApps = CreateWindowW(L"BUTTON", L"Block",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            30, 360, 250, 40,
            hwnd, (HMENU)BTN_ENABLE_APPS, NULL, NULL);
        SendMessage(btnEnableApps, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND btnDisableApps = CreateWindowW(L"BUTTON", L"Unblock",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            320, 360, 250, 40,
            hwnd, (HMENU)BTN_DISABLE_APPS, NULL, NULL);
        SendMessage(btnDisableApps, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND lblSafety = CreateWindowW(L"STATIC",
            L"✓ No viruses  ✓ Open source  ✓ Reversible changes",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            30, 425, 540, 25,
            hwnd, NULL, NULL, NULL);
        SendMessage(lblSafety, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        break;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case BTN_ENABLE_DOMAINS: {
            if (EnableBlock()) {
                MessageBoxW(hwnd,
                    L"Domain blocking successfully enabled!\n\n"
                    L"To apply changes:\n"
                    L"- Flush DNS cache (ipconfig /flushdns)\n"
                    L"- Restart your browser",
                    L"Success",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBoxW(hwnd,
                    L"Error enabling domain blocking.\n"
                    L"Make sure the program is running as administrator.",
                    L"Error",
                    MB_OK | MB_ICONERROR);
            }
            break;
        }

        case BTN_DISABLE_DOMAINS: {
            if (DisableBlock()) {
                MessageBoxW(hwnd,
                    L"Domain blocking successfully disabled!\n\n"
                    L"To apply changes:\n"
                    L"- Flush DNS cache (ipconfig /flushdns)\n"
                    L"- Restart your browser",
                    L"Success",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBoxW(hwnd,
                    L"Error disabling domain blocking.\n"
                    L"Make sure the program is running as administrator.",
                    L"Error",
                    MB_OK | MB_ICONERROR);
            }
            break;
        }

        case BTN_ENABLE_APPS: {
            int result = MessageBoxW(hwnd,
                L"All running Yandex applications will be forcefully terminated!\n\n"
                L"This includes:\n"
                L"• Yandex Browser\n"
                L"• Yandex Music\n"
                L"• Yandex Disk\n"
                L"• And other Yandex applications\n\n"
                L"Continue?",
                L"Warning",
                MB_YESNO | MB_ICONWARNING);

            if (result == IDYES) {
                if (EnableAppBlock()) {
                    MessageBoxW(hwnd,
                        L"Application blocking successfully enabled!\n\n"
                        L"All running processes have been terminated.\n"
                        L"Yandex applications are now blocked from starting.",
                        L"Success",
                        MB_OK | MB_ICONINFORMATION);
                }
                else {
                    MessageBoxW(hwnd,
                        L"Error enabling application blocking.\n"
                        L"Make sure the program is running as administrator.",
                        L"Error",
                        MB_OK | MB_ICONERROR);
                }
            }
            break;
        }

        case BTN_DISABLE_APPS: {
            if (DisableAppBlock()) {
                MessageBoxW(hwnd,
                    L"Application blocking successfully disabled!\n\n"
                    L"You can now run Yandex applications.",
                    L"Success",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBoxW(hwnd,
                    L"Error disabling application blocking.\n"
                    L"Make sure the program is running as administrator.",
                    L"Error",
                    MB_OK | MB_ICONERROR);
            }
            break;
        }
        }
        break;
    }

    case WM_DESTROY: {
        if (hFont) {
            DeleteObject(hFont);
        }
        if (hTitleFont) {
            DeleteObject(hTitleFont);
        }
        if (hSmallFont) {
            DeleteObject(hSmallFont);
        }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    if (!IsRunAsAdmin()) {
        MessageBoxW(NULL,
            L"This program must be run as administrator!\n\n"
            L"Right-click the program and select\n"
            L"\"Run as administrator\".",
            L"Administrator Privileges Required",
            MB_OK | MB_ICONWARNING);
        return 1;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Error registering window class!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Error creating window!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
