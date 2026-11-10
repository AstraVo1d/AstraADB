#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #define POPEN  _popen
  #define PCLOSE _pclose
#else
  #define POPEN  popen
  #define PCLOSE pclose
#endif

namespace fs = std::filesystem;
using std::string;
using std::vector;

namespace cfg {
    const char* LOGO_TEXT = "ASTRAADB";

    int  TYPE_DELAY_MS        = 4;
    bool TYPING_ON            = true;
    bool TYPE_COMMAND_OUTPUT  = false;

    int  PAGE_SIZE            = 25;
    const char* REMOTE_START        = "/sdcard/";
    const char* REMOTE_PUSH_DEFAULT = "/sdcard/Download/";
    int  REC_DEFAULT_SECONDS  = 30;
    int  WIRELESS_PORT        = 5555;

    bool   SCALE_DENSITY      = true;
    int    PHONE_TARGET_SHORT = 1080;
    int    TABLET_TARGET_SHORT= 1200;
    int    TABLET_MIN_SW_DP   = 600;
    const double WM_PRESETS[] = {0.90, 0.75, 0.60};
}

namespace col {
    const char* RESET  = "\033[0m";
    const char* BOLD   = "\033[1m";
    const char* DIM    = "\033[38;5;245m";
    const char* ACCENT = "\033[38;5;75m";
    const char* GOOD   = "\033[38;5;78m";
    const char* BAD    = "\033[38;5;203m";
    const char* STAR   = "\033[38;5;220m";
    const char* LOGO[5] = {"\033[38;5;213m", "\033[38;5;177m", "\033[38;5;141m",
                           "\033[38;5;105m", "\033[38;5;69m"};
}

static bool             g_instant = false;
static std::set<string> g_seen;
static string           g_serial, g_model, g_lastOutput;
static vector<string>   g_history;

struct OSInfo { string os, pretty, pm; } g_os;

void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

void say(const string& s) {
    bool slow = cfg::TYPING_ON && !g_instant && cfg::TYPE_DELAY_MS > 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == 27) {
            std::cout << c;
            while (i + 1 < s.size()) {
                char d = s[++i];
                std::cout << d;
                if (std::isalpha((unsigned char)d)) break;
            }
            continue;
        }
        std::cout << c;
        if (slow && c != ' ' && c != '\n' && c != '\t') { std::cout.flush(); sleepMs(cfg::TYPE_DELAY_MS); }
    }
    std::cout.flush();
}
void sayln(const string& s = "") { say(s + "\n"); }

string trim(string s) {
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    size_t i = 0;
    while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
    return s.substr(i);
}
bool contains(const string& h, const string& n) { return h.find(n) != string::npos; }
string lower(string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }

string ask(const string& prompt, const string& def = "") {
    say(string(col::ACCENT) + prompt + col::RESET);
    string line;
    if (!std::getline(std::cin, line)) { std::cout << "\n"; std::exit(0); }
    line = trim(line);
    return line.empty() ? def : line;
}
bool confirm(const string& msg) { return lower(ask(msg + " [y/N]: ")) == "y"; }
void pause() { ask(string("\n") + col::DIM + "Press Enter to continue..." + col::RESET); }
void good(const string& s) { sayln(string(col::GOOD) + s + col::RESET); }
void bad(const string& s)  { sayln(string(col::BAD) + s + col::RESET); }
void hint(const string& s) { sayln(string(col::DIM) + s + col::RESET); }

string cleanPath(string p) {
    p = trim(p);
    if (p.size() >= 2 && ((p.front() == '"' && p.back() == '"') || (p.front() == '\'' && p.back() == '\'')))
        p = p.substr(1, p.size() - 2);
#ifndef _WIN32
    string r;
    for (size_t i = 0; i < p.size(); ++i) {
        if (p[i] == '\\' && i + 1 < p.size() && p[i + 1] == ' ') continue;
        r += p[i];
    }
    p = r;
#endif
    return p;
}

int menu(const string& title, const vector<string>& items, const string& zeroLabel = "Back") {
    sayln(string(col::BOLD) + col::ACCENT + title + col::RESET);
    for (size_t i = 0; i < items.size(); ++i)
        sayln(string(col::DIM) + " [" + std::to_string(i + 1) + "] " + col::RESET + items[i]);
    sayln(string(col::DIM) + " [0] " + col::RESET + zeroLabel);
    while (true) {
        string s = ask("\n> ");
        if (!s.empty() && std::all_of(s.begin(), s.end(), ::isdigit)) {
            int n = std::atoi(s.c_str());
            if (n >= 0 && n <= (int)items.size()) return n;
        }
        bad("Pick a number from the list.");
    }
}

using Glyph = std::array<const char*, 5>;
Glyph glyph(char c) {
    switch (c) {
        case 'A': return {" ### ", "#   #", "#####", "#   #", "#   #"};
        case 'B': return {"#### ", "#   #", "#### ", "#   #", "#### "};
        case 'D': return {"#### ", "#   #", "#   #", "#   #", "#### "};
        case 'E': return {"#####", "#    ", "###  ", "#    ", "#####"};
        case 'I': return {"###", " # ", " # ", " # ", "###"};
        case 'L': return {"#    ", "#    ", "#    ", "#    ", "#####"};
        case 'M': return {"#   #", "## ##", "# # #", "#   #", "#   #"};
        case 'N': return {"#   #", "##  #", "# # #", "#  ##", "#   #"};
        case 'O': return {" ### ", "#   #", "#   #", "#   #", " ### "};
        case 'R': return {"#### ", "#   #", "#### ", "#  # ", "#   #"};
        case 'S': return {" ####", "#    ", " ### ", "    #", "#### "};
        case 'T': return {"#####", "  #  ", "  #  ", "  #  ", "  #  "};
        case 'V': return {"#   #", "#   #", "#   #", " # # ", "  #  "};
        default:  return {"     ", "     ", "     ", "     ", "     "};
    }
}

void printLogo() {
    static const char* STAR[7] = {
        "     #     ",
        "     #     ",
        "    ###    ",
        "###########",
        "    ###    ",
        "     #     ",
        "     #     "};
    std::array<string, 5> rows;
    for (char ch : string(cfg::LOGO_TEXT)) {
        Glyph g = glyph((char)std::toupper((unsigned char)ch));
        for (int r = 0; r < 5; ++r) rows[r] += string(g[r]) + " ";
    }
    sayln();
    for (int r = 0; r < 7; ++r) {
        string line = string("  ") + col::STAR + STAR[r] + col::RESET + "   ";
        if (r >= 1 && r <= 5) line += string(col::LOGO[r - 1]) + rows[r - 1] + col::RESET;
        sayln(line);
    }
    sayln(string("\n  ") + col::DIM + "Android tools made easy  -  files, apps, shell, screen, wireless" + col::RESET);
}

string run(const string& cmd, bool show = true) {
    string out;
    FILE* p = POPEN((cmd + " 2>&1").c_str(), "r");
    if (!p) return "";
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0) {
        out.append(buf, n);
        if (show) {
            if (cfg::TYPE_COMMAND_OUTPUT) say(string(buf, n));
            else { std::cout.write(buf, (std::streamsize)n); std::cout.flush(); }
        }
    }
    PCLOSE(p);
    if (show) g_lastOutput = out;
    return out;
}
int sh(const string& cmd) { return std::system(cmd.c_str()); }

void runDetached(const string& cmd) {
#ifdef _WIN32
    sh("start \"\" " + cmd);
#else
    sh(cmd + " >/dev/null 2>&1 &");
#endif
}

string shQuote(const string& s) {
    string r;
#ifdef _WIN32
    r = "\"";
    for (char c : s) { if (c == '"') r += "\\\""; else r += c; }
    return r + "\"";
#else
    r = "'";
    for (char c : s) { if (c == '\'') r += "'\\''"; else r += c; }
    return r + "'";
#endif
}
string remoteQuote(const string& s) {
    string r = "'";
    for (char c : s) { if (c == '\'') r += "'\\''"; else r += c; }
    return r + "'";
}

bool toolExists(const string& tool) {
#ifdef _WIN32
    return sh("where " + tool + " >nul 2>&1") == 0;
#else
    return sh("command -v " + tool + " >/dev/null 2>&1") == 0;
#endif
}

string adbp() { return g_serial.empty() ? "adb" : "adb -s " + shQuote(g_serial); }
string adbShell(const string& deviceCmd) { return adbp() + " shell " + shQuote(deviceCmd); }

string homeDir() {
    const char* h = std::getenv("HOME");
    if (!h) h = std::getenv("USERPROFILE");
    return h ? h : ".";
}
string startDir() {
    fs::path d = fs::path(homeDir()) / "Downloads";
    std::error_code ec;
    return fs::exists(d, ec) ? d.string() : homeDir();
}
string stamp() {
    char b[32]; std::time_t t = std::time(nullptr);
    std::strftime(b, sizeof b, "%Y%m%d_%H%M%S", std::localtime(&t));
    return b;
}
string baseName(const string& p) {
    size_t i = p.find_last_of('/');
    return i == string::npos ? p : p.substr(i + 1);
}

string clipTool(bool write) {
#ifdef _WIN32
    return write ? "clip" : "powershell -noprofile -command Get-Clipboard";
#elif __APPLE__
    return write ? "pbcopy" : "pbpaste";
#else
    if (toolExists("wl-copy"))  return write ? "wl-copy"  : "wl-paste";
    if (toolExists("xclip"))    return write ? "xclip -selection clipboard" : "xclip -selection clipboard -o";
    if (toolExists("xsel"))     return write ? "xsel -ib" : "xsel -ob";
    return "";
#endif
}
bool clipCopy(const string& text) {
    string c = clipTool(true);
    if (c.empty()) return false;
    FILE* p = POPEN(c.c_str(), "w");
    if (!p) return false;
    fwrite(text.data(), 1, text.size(), p);
    PCLOSE(p);
    return true;
}
string clipPaste() {
    string c = clipTool(false);
    return c.empty() ? "" : trim(run(c, false));
}

string osReleaseValue(const string& key) {
    std::ifstream f("/etc/os-release");
    string line;
    while (std::getline(f, line)) {
        if (line.rfind(key + "=", 0) == 0) {
            string v = line.substr(key.size() + 1);
            v.erase(std::remove(v.begin(), v.end(), '"'), v.end());
            return v;
        }
    }
    return "";
}
OSInfo detectOS() {
    OSInfo o;
#ifdef _WIN32
    o.os = "Windows"; o.pretty = "Windows"; o.pm = "winget";
#elif __APPLE__
    o.os = "macOS"; o.pretty = "macOS"; o.pm = "brew";
#else
    o.os = "Linux";
    o.pretty = osReleaseValue("PRETTY_NAME");
    if (o.pretty.empty()) o.pretty = "Linux";
    string ids = lower(osReleaseValue("ID") + " " + osReleaseValue("ID_LIKE"));
    if      (contains(ids, "debian") || contains(ids, "ubuntu"))                          o.pm = "apt";
    else if (contains(ids, "fedora") || contains(ids, "rhel") || contains(ids, "centos")) o.pm = "dnf";
    else if (contains(ids, "arch") || contains(ids, "manjaro"))                           o.pm = "pacman";
    else if (contains(ids, "suse"))                                                       o.pm = "zypper";
    else if (contains(ids, "alpine"))                                                     o.pm = "apk";
    else if (contains(ids, "void"))                                                       o.pm = "xbps";
    else if (contains(ids, "gentoo"))                                                     o.pm = "emerge";
    else if (contains(ids, "nixos"))                                                      o.pm = "nix";
    else for (const char* t : {"apt", "dnf", "pacman", "zypper", "apk"}) if (toolExists(t)) { o.pm = t; break; }
#endif
    return o;
}

string installCommand(const string& tool) {
    bool adb = (tool == "adb");
    const string& pm = g_os.pm;
    if (pm == "apt")     return string("sudo apt install -y ") + (adb ? "adb" : "scrcpy");
    if (pm == "dnf")     return string("sudo dnf install -y ") + (adb ? "android-tools" : "scrcpy");
    if (pm == "pacman")  return string("sudo pacman -S --noconfirm ") + (adb ? "android-tools" : "scrcpy");
    if (pm == "zypper")  return string("sudo zypper install -y ") + (adb ? "android-tools" : "scrcpy");
    if (pm == "apk")     return string("sudo apk add ") + (adb ? "android-tools" : "scrcpy");
    if (pm == "xbps")    return string("sudo xbps-install -y ") + (adb ? "android-tools" : "scrcpy");
    if (pm == "emerge")  return string("sudo emerge ") + (adb ? "dev-util/android-tools" : "media-gfx/scrcpy");
    if (pm == "nix")     return string("nix-env -iA nixpkgs.") + (adb ? "android-tools" : "scrcpy");
    if (pm == "brew")    return adb ? "brew install --cask android-platform-tools" : "brew install scrcpy";
    if (pm == "winget")  return adb ? "winget install -e --id Google.PlatformTools" : "winget install -e --id Genymobile.scrcpy";
    return "";
}

bool checkTool(const string& tool) {
    if (toolExists(tool)) return true;
    string cmd = installCommand(tool);
    bad("'" + tool + "' was not found on this system.");
    if (cmd.empty()) { sayln("Install it manually, then restart AstraADB."); pause(); return false; }
    sayln(string("Best command for ") + g_os.pretty + ":  " + col::GOOD + cmd + col::RESET);
    if (confirm("Run it now?")) {
        sh(cmd);
        return toolExists(tool);
    }
    return false;
}

void initTerminal() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | 0x0004);
#endif
}

void beginScreen(const string& id, bool logo = false) {
    std::cout << "\033[2J\033[H";
    g_instant = !g_seen.insert(id).second;
    if (logo) printLogo();
    else sayln(string("\n  ") + col::STAR + "*" + col::RESET + " " + col::BOLD + "AstraADB" + col::RESET);
    string dev = g_serial.empty() ? "none selected" : g_serial + (g_model.empty() ? "" : " (" + g_model + ")");
    sayln(string("  ") + col::DIM + "OS: " + g_os.pretty + "   Device: " + dev + col::RESET);
    sayln();
}

struct Device { string serial, state, model; };

vector<Device> listDevices() {
    vector<Device> v;
    std::istringstream in(run("adb devices -l", false));
    string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '*' || line.rfind("List of", 0) == 0) continue;
        std::istringstream ls(line);
        Device d; ls >> d.serial >> d.state;
        string tok;
        while (ls >> tok) if (tok.rfind("model:", 0) == 0) d.model = tok.substr(6);
        if (!d.serial.empty()) v.push_back(d);
    }
    return v;
}

bool selectDevice(bool forceMenu) {
    auto devs = listDevices();
    if (devs.empty()) {
        bad("No device found.");
        hint("Plug it in with USB and enable USB debugging, or use 'Wireless setup' from the main menu.");
        pause(); return false;
    }
    if (devs.size() == 1 && !forceMenu) {
        g_serial = devs[0].serial; g_model = devs[0].model;
    } else {
        vector<string> items;
        for (auto& d : devs) items.push_back(d.serial + "  " + d.model + "  (" + d.state + ")");
        int n = menu("Choose a device", items, "Cancel");
        if (n == 0) return false;
        g_serial = devs[n - 1].serial; g_model = devs[n - 1].model;
    }
    for (auto& d : devs) if (d.serial == g_serial && d.state != "device") {
        bad("Device state is '" + d.state + "'. Unlock the phone and accept the USB debugging prompt.");
        pause(); return false;
    }
    return true;
}

bool ensureDevice() {
    if (!g_serial.empty()) {
        for (auto& d : listDevices()) if (d.serial == g_serial && d.state == "device") { g_model = d.model; return true; }
        g_serial.clear(); g_model.clear();
    }
    return selectDevice(false);
}

struct Entry { string name; bool dir; };

vector<Entry> listLocal(const string& path, bool& ok) {
    vector<Entry> v; std::error_code ec;
    fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, ec);
    if (ec) { ok = false; return v; }
    for (auto& e : it) { std::error_code e2; v.push_back({e.path().filename().string(), e.is_directory(e2)}); }
    return v;
}
vector<Entry> listRemote(const string& path, bool& ok) {
    vector<Entry> v;
    string out = run(adbShell("ls -1p " + remoteQuote(path)), false);
    if (contains(out, "No such file") || contains(out, "Permission denied") || contains(out, "error:")) { ok = false; return v; }
    std::istringstream in(out); string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        bool d = line.back() == '/';
        if (d) line.pop_back();
        v.push_back({line, d});
    }
    return v;
}

string browse(bool remote, string cur, bool allowFolder, const string& title, const string& ext = "") {
    int page = 0;
    while (true) {
        bool ok = true;
        auto entries = remote ? listRemote(cur, ok) : listLocal(cur, ok);
        if (!ext.empty()) entries.erase(std::remove_if(entries.begin(), entries.end(),
            [&](const Entry& e) { string n = lower(e.name); return !e.dir && (n.size() < ext.size() || n.compare(n.size() - ext.size(), ext.size(), ext) != 0); }), entries.end());
        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            if (a.dir != b.dir) return a.dir;
            return lower(a.name) < lower(b.name); });

        beginScreen("browse");
        g_instant = true;
        sayln(string(col::BOLD) + col::ACCENT + title + col::RESET);
        sayln(string(col::GOOD) + (remote ? "Device: " : "Local:  ") + cur + col::RESET);
        if (!ok) bad("Cannot open this folder.");
        int total = (int)entries.size(), pages = std::max(1, (total + cfg::PAGE_SIZE - 1) / cfg::PAGE_SIZE);
        page = std::max(0, std::min(page, pages - 1));
        for (int i = page * cfg::PAGE_SIZE; i < std::min(total, (page + 1) * cfg::PAGE_SIZE); ++i)
            sayln(string(col::DIM) + " [" + std::to_string(i + 1) + "] " + col::RESET + (entries[i].dir ? string(col::ACCENT) + entries[i].name + "/" + col::RESET : entries[i].name));
        if (total == 0) hint("  (empty)");
        hint("\npage " + std::to_string(page + 1) + "/" + std::to_string(pages) +
             "   number=open/select  u=up  " + (allowFolder ? "s=select THIS folder  " : "") + "p=type/paste/drag a path  n/b=page  q=cancel");

        string in = lower(ask("\n> "));
        if (in == "q" || in == "0") return "";
        if (in == "n") { ++page; continue; }
        if (in == "b") { --page; continue; }
        if (in == "p") { string p = cleanPath(ask("Path (you can drag a file here): ")); if (!p.empty()) return p; continue; }
        if (in == "s" && allowFolder) return cur;
        if (in == "u" || in == ".." || !ok) {
            if (remote) {
                string t = cur; while (t.size() > 1 && t.back() == '/') t.pop_back();
                size_t pos = t.find_last_of('/');
                cur = (pos == string::npos || pos == 0) ? "/" : t.substr(0, pos);
            } else {
                string par = fs::path(cur).parent_path().string();
                if (!par.empty()) cur = par;
            }
            page = 0; continue;
        }
        if (!in.empty() && std::all_of(in.begin(), in.end(), ::isdigit)) {
            int n = std::atoi(in.c_str());
            if (n >= 1 && n <= total) {
                const Entry& e = entries[n - 1];
                string next = remote ? (cur.back() == '/' ? cur + e.name : cur + "/" + e.name)
                                     : (fs::path(cur) / e.name).string();
                if (e.dir) { cur = next; page = 0; } else return next;
            }
        }
    }
}

void pushFile() {
    if (!ensureDevice()) return;
    string local = browse(false, startDir(), true, "PUSH to device: pick a file (or s = push a whole folder)");
    if (local.empty()) return;
    string remote = ask(string("Destination on device [") + cfg::REMOTE_PUSH_DEFAULT + "]: ", cfg::REMOTE_PUSH_DEFAULT);
    sayln();
    sh(adbp() + " push " + shQuote(local) + " " + shQuote(remote));
    pause();
}

const vector<std::pair<string, string>> PULL_SHORTCUTS = {
    {"Camera photos",    "/sdcard/DCIM/Camera"},
    {"Screenshots",      "/sdcard/Pictures/Screenshots"},
    {"Downloads",        "/sdcard/Download"},
    {"Documents",        "/sdcard/Documents"},
    {"WhatsApp media",   "/sdcard/Android/media/com.whatsapp/WhatsApp/Media"},
};

void pullFile() {
    if (!ensureDevice()) return;
    beginScreen("pull");
    vector<string> items;
    for (auto& s : PULL_SHORTCUTS) items.push_back("Copy folder: " + s.first);
    size_t k = items.size();
    items.push_back("Browse all files on the device");
    items.push_back("Type / paste a path on the device");
    int n = menu("PULL from device", items, "Cancel");
    if (n == 0) return;
    string remote;
    if ((size_t)n <= k) remote = PULL_SHORTCUTS[n - 1].second;
    else if ((size_t)n == k + 1) remote = browse(true, cfg::REMOTE_START, true, "PULL from device: pick a file (or s = pull a whole folder)");
    else remote = cleanPath(ask("Path on device: "));
    if (remote.empty()) return;
    string defDest = startDir();
    string dest = cleanPath(ask("Save to [" + defDest + "]: ", defDest));
    sayln();
    sh(adbp() + " pull " + shQuote(remote) + " " + shQuote(dest));
    pause();
}

void installApk() {
    if (!ensureDevice()) return;
    string apk = browse(false, startDir(), false, "INSTALL: pick an .apk file", ".apk");
    if (apk.empty()) return;
    sh(adbp() + " install -r " + shQuote(apk));
    pause();
}

void screenshot() {
    if (!ensureDevice()) return;
    string name = "astraadb_shot_" + stamp() + ".png";
    run(adbShell("screencap -p /sdcard/astraadb_tmp.png"), false);
    sh(adbp() + " pull /sdcard/astraadb_tmp.png " + shQuote(name));
    run(adbShell("rm /sdcard/astraadb_tmp.png"), false);
    good("Saved: " + (fs::current_path() / name).string());
    pause();
}

void screenRecord() {
    if (!ensureDevice()) return;
    string secs = ask("Seconds to record, max 180 [" + std::to_string(cfg::REC_DEFAULT_SECONDS) + "]: ", std::to_string(cfg::REC_DEFAULT_SECONDS));
    string name = "astraadb_rec_" + stamp() + ".mp4";
    sayln("Recording... use your device now.");
    sh(adbp() + " shell screenrecord --time-limit " + secs + " /sdcard/astraadb_tmp.mp4");
    sh(adbp() + " pull /sdcard/astraadb_tmp.mp4 " + shQuote(name));
    run(adbShell("rm /sdcard/astraadb_tmp.mp4"), false);
    good("Saved: " + (fs::current_path() / name).string());
    pause();
}

void saveLogcat() {
    if (!ensureDevice()) return;
    string name = "astraadb_log_" + stamp() + ".txt";
    sh(adbp() + " logcat -d > " + shQuote(name));
    good("Saved: " + (fs::current_path() / name).string());
    pause();
}

void dashboard() {
    if (!ensureDevice()) return;
    beginScreen("dash");
    sayln(string(col::BOLD) + col::ACCENT + "Device dashboard" + col::RESET + "\n");
    run(adbShell(
        "echo Brand    : $(getprop ro.product.manufacturer); "
        "echo Model    : $(getprop ro.product.model); "
        "echo Android  : $(getprop ro.build.version.release) API $(getprop ro.build.version.sdk); "
        "echo CPU      : $(getprop ro.product.cpu.abi); "
        "echo Battery  : $(dumpsys battery | grep level | tr -d ' '); "
        "echo Storage  : $(df -h /sdcard | tail -n 1); "
        "echo Screen   : $(wm size | tail -n 1) $(wm density | tail -n 1); "
        "echo Wi-Fi IP : $(ip -f inet addr show wlan0 | grep inet)"));
    pause();
}

void pasteToPhone() {
    if (!ensureDevice()) return;
    beginScreen("typeText");
    sayln(string(col::BOLD) + col::ACCENT + "Send text to the phone" + col::RESET);
    hint("Click a text field on the phone first. Type text below, or leave empty to use your PC clipboard.");
    hint("Only plain English letters, numbers and symbols work (Android limitation).\n");
    string text = ask("Text: ");
    if (text.empty()) text = clipPaste();
    if (text.empty()) { bad("Nothing to send."); pause(); return; }
    for (auto& c : text) if (c == '\n' || c == '\r') c = ' ';
    string esc;
    for (char c : text) esc += (c == ' ') ? string("%s") : string(1, c);
    sayln("Sending: " + text);
    run(adbShell("input text " + remoteQuote(esc)));
    good("Done.");
    pause();
}

void wirelessWizard() {
    beginScreen("wireless");
    sayln(string(col::BOLD) + col::ACCENT + "Wireless setup wizard" + col::RESET);
    hint("1) Plug the phone with USB once   2) Both phone and PC on the same Wi-Fi   3) This wizard does the rest\n");
    if (!ensureDevice()) return;
    string port = std::to_string(cfg::WIRELESS_PORT);
    sayln("Switching the phone to wireless mode...");
    run(adbp() + " tcpip " + port);
    sleepMs(1500);
    string out = run(adbShell("ip -f inet addr show wlan0"), false);
    string ip;
    size_t p = out.find("inet ");
    if (p != string::npos) { size_t e = out.find('/', p); if (e != string::npos) ip = trim(out.substr(p + 5, e - p - 5)); }
    if (ip.empty()) ip = ask("Could not read the IP. Type the phone's IP: ");
    if (ip.empty()) return;
    sayln("Connecting to " + ip + ":" + port + " ...");
    string res = run("adb connect " + ip + ":" + port);
    if (contains(res, "connected")) {
        g_serial = ip + ":" + port; g_model.clear();
        good("\nConnected. You can unplug the USB cable now.");
    } else bad("\nCould not connect. Check that both devices share the same Wi-Fi.");
    pause();
}

vector<string> listPackages(bool withSystem) {
    vector<string> v;
    std::istringstream in(run(adbp() + " shell pm list packages" + (withSystem ? "" : " -3"), false));
    string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.rfind("package:", 0) == 0) v.push_back(line.substr(8));
    }
    std::sort(v.begin(), v.end());
    return v;
}

string pickPackage() {
    bool withSystem = false;
    string filter;
    int page = 0;
    auto all = listPackages(withSystem);
    while (true) {
        vector<string> v;
        for (auto& p : all) if (filter.empty() || contains(lower(p), lower(filter))) v.push_back(p);
        beginScreen("pkg");
        g_instant = true;
        sayln(string(col::BOLD) + col::ACCENT + "Pick an app" + col::RESET);
        hint(string("showing ") + (withSystem ? "ALL apps" : "your installed apps") + (filter.empty() ? "" : "   |   search: " + filter) + "   |   " + std::to_string(v.size()) + " found\n");
        int total = (int)v.size(), pages = std::max(1, (total + cfg::PAGE_SIZE - 1) / cfg::PAGE_SIZE);
        page = std::max(0, std::min(page, pages - 1));
        for (int i = page * cfg::PAGE_SIZE; i < std::min(total, (page + 1) * cfg::PAGE_SIZE); ++i)
            sayln(string(col::DIM) + " [" + std::to_string(i + 1) + "] " + col::RESET + v[i]);
        hint("\npage " + std::to_string(page + 1) + "/" + std::to_string(pages) +
             "   number=select   /word=search (/ alone clears)   a=show system apps on/off   n/b=page   q=cancel");
        string in = ask("\n> ");
        string l = lower(in);
        if (l == "q" || l == "0") return "";
        if (l == "n") { ++page; continue; }
        if (l == "b") { --page; continue; }
        if (l == "a") { withSystem = !withSystem; all = listPackages(withSystem); page = 0; continue; }
        if (in[0] == '/') { filter = trim(in.substr(1)); page = 0; continue; }
        if (std::all_of(in.begin(), in.end(), ::isdigit)) {
            int n = std::atoi(in.c_str());
            if (n >= 1 && n <= total) return v[n - 1];
        } else if (contains(in, ".")) return in;
    }
}

void appManager() {
    if (!ensureDevice()) return;
    string pkg = pickPackage();
    while (!pkg.empty()) {
        beginScreen("app");
        sayln(string("  App: ") + col::GOOD + pkg + col::RESET + "\n");
        int n = menu("What do you want to do?", {
            "Launch", "Force stop", "Clear data (reset the app)", "Uninstall", "Save its APK to this PC",
            "Disable", "Enable", "Version / install info", "Open its settings page on the phone", "Pick another app"});
        if (n == 0) return;
        switch (n) {
            case 1: run(adbp() + " shell monkey -p " + pkg + " -c android.intent.category.LAUNCHER 1"); break;
            case 2: run(adbp() + " shell am force-stop " + pkg); good("Stopped."); break;
            case 3: if (confirm("Delete ALL data of " + pkg + "?")) run(adbp() + " shell pm clear " + pkg); break;
            case 4: if (confirm("Uninstall " + pkg + "?")) { run(adbp() + " uninstall " + pkg); pkg.clear(); } break;
            case 5: {
                std::istringstream in(run(adbp() + " shell pm path " + pkg, false));
                string line;
                while (std::getline(in, line)) {
                    line = trim(line);
                    if (line.rfind("package:", 0) != 0) continue;
                    string path = line.substr(8);
                    sh(adbp() + " pull " + shQuote(path) + " " + shQuote(pkg + "_" + baseName(path)));
                }
                good("Saved in: " + fs::current_path().string());
                break;
            }
            case 6: run(adbp() + " shell pm disable-user --user 0 " + pkg); break;
            case 7: run(adbp() + " shell pm enable " + pkg); break;
            case 8: run(adbShell("dumpsys package " + pkg + " | grep -E 'versionName|versionCode|firstInstallTime|lastUpdateTime'")); break;
            case 9: run(adbp() + " shell am start -a android.settings.APPLICATION_DETAILS_SETTINGS -d package:" + pkg); break;
            case 10: pkg = pickPackage(); continue;
        }
        if (!pkg.empty()) pause();
    }
}

struct Cmd { const char* cat; const char* label; const char* args; const char* ask; bool needDevice; };
const vector<Cmd> QUICK_CMDS = {
    {"Info", "Device info",                         "shell \"getprop ro.product.model; getprop ro.build.version.release; getprop ro.product.cpu.abi\"", nullptr, true},
    {"Info", "Battery status",                      "shell dumpsys battery", nullptr, true},
    {"Info", "Storage usage",                       "shell df -h /sdcard", nullptr, true},
    {"Info", "Memory (RAM)",                        "shell \"head -n 4 /proc/meminfo\"", nullptr, true},
    {"Info", "Wi-Fi IP address",                    "shell ip -f inet addr show wlan0", nullptr, true},
    {"Info", "Is the screen on?",                   "shell \"dumpsys power | grep -i 'Display Power'\"", nullptr, true},
    {"Info", "Serial number",                       "get-serialno", nullptr, true},
    {"Info", "Last 200 log lines",                  "logcat -d -t 200", nullptr, true},
    {"Info", "List installed 3rd-party apps",       "shell pm list packages -3", nullptr, true},
    {"Info", "Search apps by keyword",              "shell \"pm list packages | grep {}\"", "Keyword: ", true},

    {"Toggles", "Wi-Fi ON",                         "shell svc wifi enable", nullptr, true},
    {"Toggles", "Wi-Fi OFF",                        "shell svc wifi disable", nullptr, true},
    {"Toggles", "Mobile data ON",                   "shell svc data enable", nullptr, true},
    {"Toggles", "Mobile data OFF",                  "shell svc data disable", nullptr, true},
    {"Toggles", "Airplane mode ON (Android 11+)",   "shell cmd connectivity airplane-mode enable", nullptr, true},
    {"Toggles", "Airplane mode OFF",                "shell cmd connectivity airplane-mode disable", nullptr, true},
    {"Toggles", "Dark mode ON",                     "shell cmd uimode night yes", nullptr, true},
    {"Toggles", "Dark mode OFF",                    "shell cmd uimode night no", nullptr, true},
    {"Toggles", "Auto-rotate ON",                   "shell settings put system accelerometer_rotation 1", nullptr, true},
    {"Toggles", "Auto-rotate OFF",                  "shell settings put system accelerometer_rotation 0", nullptr, true},
    {"Toggles", "Animations OFF (feels faster)",    "shell \"settings put global window_animation_scale 0; settings put global transition_animation_scale 0; settings put global animator_duration_scale 0\"", nullptr, true},
    {"Toggles", "Animations back to normal",        "shell \"settings put global window_animation_scale 1; settings put global transition_animation_scale 1; settings put global animator_duration_scale 1\"", nullptr, true},
    {"Toggles", "Stay awake while charging ON",     "shell settings put global stay_on_while_plugged_in 7", nullptr, true},
    {"Toggles", "Stay awake while charging OFF",    "shell settings put global stay_on_while_plugged_in 0", nullptr, true},
    {"Toggles", "Set brightness (0-255)",           "shell settings put system screen_brightness {}", "Brightness 0-255: ", true},

    {"Buttons & Media", "HOME",                     "shell input keyevent 3", nullptr, true},
    {"Buttons & Media", "BACK",                     "shell input keyevent 4", nullptr, true},
    {"Buttons & Media", "Recent apps",              "shell input keyevent 187", nullptr, true},
    {"Buttons & Media", "Power / lock screen",      "shell input keyevent 26", nullptr, true},
    {"Buttons & Media", "Wake up screen",           "shell input keyevent 224", nullptr, true},
    {"Buttons & Media", "Volume up",                "shell input keyevent 24", nullptr, true},
    {"Buttons & Media", "Volume down",              "shell input keyevent 25", nullptr, true},
    {"Buttons & Media", "Mute",                     "shell input keyevent 164", nullptr, true},
    {"Buttons & Media", "Play / pause",             "shell input keyevent 85", nullptr, true},
    {"Buttons & Media", "Next track",               "shell input keyevent 87", nullptr, true},
    {"Buttons & Media", "Previous track",           "shell input keyevent 88", nullptr, true},

    {"Open on phone", "Settings",                   "shell am start -a android.settings.SETTINGS", nullptr, true},
    {"Open on phone", "Wi-Fi settings",             "shell am start -a android.settings.WIFI_SETTINGS", nullptr, true},
    {"Open on phone", "Developer options",          "shell am start -a android.settings.APPLICATION_DEVELOPMENT_SETTINGS", nullptr, true},
    {"Open on phone", "A website (URL)",            "shell am start -a android.intent.action.VIEW -d {}", "URL: ", true},

    {"Power", "Reboot",                             "reboot", nullptr, true},
    {"Power", "Reboot to recovery",                 "reboot recovery", nullptr, true},
    {"Power", "Reboot to bootloader",               "reboot bootloader", nullptr, true},
    {"Power", "Power off",                          "shell reboot -p", nullptr, true},

    {"Network & Server", "List devices",            "devices -l", nullptr, false},
    {"Network & Server", "Connect wirelessly",      "connect {}", "IP:PORT (example 192.168.1.20:5555): ", false},
    {"Network & Server", "Pair (Android 11+ wireless debugging)", "pair {}", "IP:PORT shown on the phone: ", false},
    {"Network & Server", "Back to USB mode",        "usb", nullptr, true},
    {"Network & Server", "Disconnect all wireless", "disconnect", nullptr, false},
    {"Network & Server", "Restart ADB server",      "kill-server && adb start-server", nullptr, false},
    {"Network & Server", "ADB version",             "version", nullptr, false},

    {"Any command", "Type ANY adb command",         "{}", "adb ", false},
};

void categoryMenu(const string& cat) {
    vector<const Cmd*> list;
    vector<string> labels;
    for (auto& c : QUICK_CMDS) if (cat == c.cat) { list.push_back(&c); labels.push_back(c.label); }
    while (true) {
        beginScreen("cat:" + cat);
        int n = menu(cat, labels);
        if (n == 0) return;
        const Cmd& c = *list[n - 1];
        if (c.needDevice && !ensureDevice()) continue;
        string args = c.args;
        if (c.ask) {
            string a = ask(c.ask);
            if (a.empty()) continue;
            size_t p = args.find("{}");
            if (p != string::npos) args.replace(p, 2, a);
        }
        if (contains(args, "reboot") && !confirm("Really " + lower(c.label) + "?")) continue;
        string full = (c.needDevice ? adbp() : string("adb")) + " " + args;
        sayln(string("\n") + col::DIM + "$ " + full + col::RESET + "\n");
        run(full);
        pause();
    }
}

void quickCommands() {
    vector<string> cats;
    for (auto& c : QUICK_CMDS) if (std::find(cats.begin(), cats.end(), c.cat) == cats.end()) cats.push_back(c.cat);
    while (true) {
        beginScreen("quick");
        int n = menu("ADB commands - pick a category", cats);
        if (n == 0) return;
        categoryMenu(cats[n - 1]);
    }
}

const vector<std::pair<string, string>> SHELL_ALIASES = {
    {"apps", "pm list packages -3"},
    {"bat",  "dumpsys battery"},
    {"ram",  "cat /proc/meminfo | head -n 5"},
    {"cpu",  "top -n 1 -b | head -n 15"},
    {"ip",   "ip -f inet addr show wlan0"},
    {"ver",  "getprop ro.build.version.release"},
};

void shellMode() {
    if (!ensureDevice()) return;
    beginScreen("shell");
    sayln(string(col::BOLD) + col::ACCENT + "Simple Shell" + col::RESET + "  (commands run on your phone)");
    hint("  Paste with your terminal's paste (Ctrl+Shift+V / Cmd+V).  Pasted 'adb ...' commands work too.");
    hint("  :copy copy last output   :paste run clipboard   :h history   !3 run history item 3");
    hint("  :i full interactive shell   :clear clear screen   :q back");
    string al; for (auto& a : SHELL_ALIASES) al += a.first + " ";
    hint("  shortcuts: " + al + "\n");
    while (true) {
        string line = ask("adb$ ");
        if (line.empty()) continue;
        if (line == ":q" || line == "exit") return;
        if (line == ":clear") { std::cout << "\033[2J\033[H"; continue; }
        if (line == ":i") { sh(adbp() + " shell"); continue; }
        if (line == ":h") {
            for (size_t i = 0; i < g_history.size(); ++i) sayln("  " + std::to_string(i + 1) + "  " + g_history[i]);
            continue;
        }
        if (line[0] == '!' && line.size() > 1 && std::all_of(line.begin() + 1, line.end(), ::isdigit)) {
            size_t i = (size_t)std::atoi(line.c_str() + 1);
            if (i < 1 || i > g_history.size()) { bad("No such history item."); continue; }
            line = g_history[i - 1];
            sayln("  " + line);
        }
        if (line == ":copy") {
            if (clipCopy(g_lastOutput)) good("Copied."); else bad("No clipboard tool found (Linux: install wl-clipboard or xclip).");
            continue;
        }
        if (line == ":paste") {
            string c = clipPaste();
            if (c.empty()) { bad("Clipboard empty or unavailable."); continue; }
            sayln("Clipboard: " + c);
            if (!confirm("Run it?")) continue;
            line = c;
        }
        g_history.push_back(line);
        if (line.rfind("adb shell ", 0) == 0) line = line.substr(10);
        else if (line.rfind("adb ", 0) == 0) { run(adbp() + " " + line.substr(4)); continue; }
        for (auto& a : SHELL_ALIASES) if (line == a.first) line = a.second;
        run(adbShell(line));
        sayln();
    }
}

int evenInt(double v) { return ((int)(v / 2.0 + 0.5)) * 2; }

void applyWm(int w, int h, int dpi) {
    run(adbp() + " shell wm size " + std::to_string(w) + "x" + std::to_string(h));
    if (dpi > 0) run(adbp() + " shell wm density " + std::to_string(dpi));
}
void applyScale(int pw, int ph, int pd, double f) {
    applyWm(evenInt(pw * f), evenInt(ph * f), cfg::SCALE_DENSITY ? (int)std::lround(pd * f) : 0);
}

void wmMenu() {
    if (!ensureDevice()) return;
    while (true) {
        beginScreen("wm");
        string sz = run(adbp() + " shell wm size", false), dn = run(adbp() + " shell wm density", false);
        int pw = 0, ph = 0, pd = 0;
        size_t a = sz.find("Physical size:"), b = dn.find("Physical density:");
        if (a != string::npos) std::sscanf(sz.c_str() + a + 14, " %dx%d", &pw, &ph);
        if (b != string::npos) std::sscanf(dn.c_str() + b + 17, " %d", &pd);
        if (!pw || !ph || !pd) { bad("Could not read screen info:\n" + sz + dn); pause(); return; }

        int shortSide = std::min(pw, ph);
        int swdp = shortSide * 160 / pd;
        bool tablet = swdp >= cfg::TABLET_MIN_SW_DP;
        int target = tablet ? cfg::TABLET_TARGET_SHORT : cfg::PHONE_TARGET_SHORT;
        double rf = std::min(1.0, (double)target / shortSide);

        sayln(trim(sz)); sayln(trim(dn));
        good(string("Detected: ") + (tablet ? "TABLET" : "PHONE") + " (" + std::to_string(swdp) + "dp wide)");
        good("Recommended: " + std::to_string(evenInt(pw * rf)) + "x" + std::to_string(evenInt(ph * rf)) +
             (rf >= 1.0 ? "  (already at the sweet spot)" : "  (sharp but lighter to mirror/stream)"));
        sayln();

        vector<string> items = {"Apply RECOMMENDED size for my " + string(tablet ? "tablet" : "phone")};
        for (double f : cfg::WM_PRESETS)
            items.push_back(std::to_string((int)std::lround(f * 100)) + "% of physical  ->  " + std::to_string(evenInt(pw * f)) + "x" + std::to_string(evenInt(ph * f)));
        size_t k = items.size();
        items.push_back("Custom size (WxH)");
        items.push_back("Custom density (DPI)");
        items.push_back("RESET size + density to normal");
        int n = menu("Screen size (wm)", items);
        if (n == 0) return;
        if (n == 1) applyScale(pw, ph, pd, rf);
        else if ((size_t)n <= k) applyScale(pw, ph, pd, cfg::WM_PRESETS[n - 2]);
        else if ((size_t)n == k + 1) {
            string s = ask("Size WxH (example 1080x2400): ");
            int w, h; if (std::sscanf(s.c_str(), "%dx%d", &w, &h) == 2) applyWm(w, h, 0);
        } else if ((size_t)n == k + 2) {
            string d = ask("Density (example 420): ");
            if (!d.empty()) run(adbp() + " shell wm density " + d);
        } else {
            run(adbp() + " shell wm size reset"); run(adbp() + " shell wm density reset");
            good("Reset.");
        }
        sleepMs(400);
    }
}

struct Preset { const char* label; const char* args; };
const vector<Preset> SCRCPY_PRESETS = {
    {"Mirror (default)",                       ""},
    {"Smooth: 1280px, 8M bitrate, 60 fps",     "-m 1280 -b 8M --max-fps 60"},
    {"Screen off on phone + stay awake",       "--turn-screen-off --stay-awake"},
    {"View only (no mouse/keyboard control)",  "--no-control"},
    {"Always on top",                          "--always-on-top"},
    {"Virtual display window (scrcpy 3.0+)",   "--new-display=1280x720/160"},
};

void scrcpyMenu() {
    if (!checkTool("scrcpy")) { pause(); return; }
    if (!ensureDevice()) return;
    while (true) {
        beginScreen("scrcpy");
        vector<string> items;
        for (auto& p : SCRCPY_PRESETS) items.push_back(p.label);
        size_t k = items.size();
        items.push_back("Custom (max size + bitrate)");
        items.push_back("Mirror ALL connected devices (one window each)");
        items.push_back("Open several windows of this device");
        items.push_back("Record screen to a file");
        int n = menu("scrcpy - see your phone on the PC", items);
        if (n == 0) return;
        string base = "scrcpy -s " + shQuote(g_serial);
        if ((size_t)n <= k) {
            sayln("Close the scrcpy window to come back here.");
            sh(base + " " + SCRCPY_PRESETS[n - 1].args);
        } else if ((size_t)n == k + 1) {
            string m = ask("Max size in px (example 1024, empty = auto): ");
            string br = ask("Bitrate (example 8M, empty = default): ");
            sh(base + (m.empty() ? "" : " -m " + m) + (br.empty() ? "" : " -b " + br));
        } else if ((size_t)n == k + 2) {
            int i = 0;
            for (auto& d : listDevices()) if (d.state == "device") {
                runDetached("scrcpy -s " + shQuote(d.serial) + " --window-title " + shQuote("AstraADB " + d.serial + " " + d.model));
                ++i;
            }
            sayln(std::to_string(i) + " window(s) launched."); sleepMs(800);
        } else if ((size_t)n == k + 3) {
            int c = std::max(1, std::atoi(ask("How many windows [2]: ", "2").c_str()));
            for (int i = 1; i <= c; ++i) { runDetached(base + " --window-title " + shQuote("AstraADB #" + std::to_string(i))); sleepMs(600); }
        } else {
            string f = ask("File name [astraadb_" + stamp() + ".mp4]: ", "astraadb_" + stamp() + ".mp4");
            sayln("Recording... close the scrcpy window to stop.");
            sh(base + " --record " + shQuote(f));
            good("Saved: " + f); pause();
        }
    }
}

void systemInfo() {
    beginScreen("info");
    sayln(string(col::BOLD) + col::ACCENT + "System check" + col::RESET);
    sayln("Operating system : " + g_os.pretty + " (" + g_os.os + ")");
    sayln("Package manager  : " + (g_os.pm.empty() ? string("unknown") : g_os.pm));
    sayln("adb              : " + string(toolExists("adb") ? "installed" : "MISSING  ->  " + installCommand("adb")));
    sayln("scrcpy           : " + string(toolExists("scrcpy") ? "installed" : "MISSING  ->  " + installCommand("scrcpy")));
    string clip = clipTool(true);
    sayln("clipboard tool   : " + (clip.empty() ? string("none (Linux: install wl-clipboard or xclip)") : clip));
    pause();
}

void settings() {
    while (true) {
        beginScreen("settings");
        int n = menu("Settings", {
            string("Typing effect: ") + (cfg::TYPING_ON ? "ON" : "OFF") + "  (toggle)",
            "Typing delay: " + std::to_string(cfg::TYPE_DELAY_MS) + " ms  (change)",
            string("Type adb output too: ") + (cfg::TYPE_COMMAND_OUTPUT ? "ON" : "OFF") + "  (toggle)",
            string("Scale DPI with screen size: ") + (cfg::SCALE_DENSITY ? "ON" : "OFF") + "  (toggle)"});
        if (n == 0) return;
        if (n == 1) cfg::TYPING_ON = !cfg::TYPING_ON;
        if (n == 2) cfg::TYPE_DELAY_MS = std::max(0, std::atoi(ask("Delay in ms [4]: ", "4").c_str()));
        if (n == 3) cfg::TYPE_COMMAND_OUTPUT = !cfg::TYPE_COMMAND_OUTPUT;
        if (n == 4) cfg::SCALE_DENSITY = !cfg::SCALE_DENSITY;
    }
}

int main() {
    initTerminal();
    g_os = detectOS();

    beginScreen("main", true);
    sayln(string("  ") + col::DIM + "Detected: " + g_os.pretty + "  |  package manager: " + (g_os.pm.empty() ? "unknown" : g_os.pm) + col::RESET + "\n");
    checkTool("adb");

    const vector<string> MAIN_ITEMS = {
        "Device dashboard (model, battery, storage...)",
        "Select device",
        "Push file/folder  ->  phone",
        "Pull file/folder  <-  phone",
        "App manager (launch, uninstall, save APK...)",
        "ADB commands (toggles, buttons, info...)",
        "Simple shell",
        "Screen size (wm size / density)",
        "scrcpy view (mirror, multi-window)",
        "Screenshot",
        "Record screen",
        "Install APK",
        "Wireless setup wizard",
        "Send text to phone (paste PC text)",
        "Save logcat to a file",
        "System check",
        "Settings"};

    while (true) {
        beginScreen("main", true);
        hint("  Tip: type a number + Enter.  In file lists you can paste or drag a path with 'p'.\n");
        int n = menu("Main menu", MAIN_ITEMS, "Exit");
        switch (n) {
            case 0:  sayln("\nBye from AstraADB."); return 0;
            case 1:  dashboard();        break;
            case 2:  selectDevice(true); break;
            case 3:  pushFile();         break;
            case 4:  pullFile();         break;
            case 5:  appManager();       break;
            case 6:  quickCommands();    break;
            case 7:  shellMode();        break;
            case 8:  wmMenu();           break;
            case 9:  scrcpyMenu();       break;
            case 10: screenshot();       break;
            case 11: screenRecord();     break;
            case 12: installApk();       break;
            case 13: wirelessWizard();   break;
            case 14: pasteToPhone();     break;
            case 15: saveLogcat();       break;
            case 16: systemInfo();       break;
            case 17: settings();         break;
        }
    }
}
