<div align="center">

```
       #
       #         ###   #### ##### ####   ###   ###  ####  ####
      ###       #   # #       #   #   # #   # #   # #   # #   #
  ###########   #####  ###    #   ####  ##### ##### #   # ####
      ###       #   #     #   #   #  #  #   # #   # #   # #   #
       #        #   # ####    #   #   # #   # #   # ####  ####
       #
```

### Android tools made easy, right in your terminal.

Push and pull files, manage apps, control your screen size, mirror with scrcpy and run ADB commands, all from one friendly menu. **No flags to remember. No dependencies to install.**

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)
![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-4c9be8)
![Dependencies](https://img.shields.io/badge/dependencies-none-success)
![Single file](https://img.shields.io/badge/source-single%20file-blueviolet)
![License](https://img.shields.io/badge/license-GPL-green)

[Features](#-features) ·
[Quick start](#-quick-start) ·
[Usage](#-usage) ·
[Customize](#-customize-it) ·
[Troubleshooting](#-troubleshooting) ·
[Contributing](#-contributing)

</div>

---

## Why AstraADB?

ADB is powerful, but the commands are long and easy to forget. AstraADB wraps the everyday ones in a numbered menu so you can do the job in a few key presses, and it is built from one readable `.cpp` file that you can change in minutes.

- **Zero dependencies.** Plain C++17 and ANSI colors. No ncurses, no Qt, no package to install first.
- **Cross-platform.** Built for Windows, macOS and Linux (Debian/Ubuntu, Fedora, Arch, openSUSE, Alpine, Void, Gentoo, NixOS).
- **Knows your OS.** It detects your system and shows the right install command for `adb` and `scrcpy` if they are missing.
- **Copy and paste friendly.** It uses normal line input, so your terminal's own copy and paste just works.
- **Easy to hack.** Most things you will want to change are single lines in a table.

---

## ✨ Features

| | Feature | What it does |
|---|---|---|
| 📊 | **Device dashboard** | Brand, model, Android version, CPU, battery, storage, screen size and Wi-Fi IP in one screen |
| 📤 | **Push files** | Visual file browser: open folders by number, paste or drag a path, push files or whole folders |
| 📥 | **Pull files** | One-key shortcuts for Camera, Screenshots, Downloads, Documents and WhatsApp media, or browse the phone |
| 📦 | **App manager** | Pick an app from a numbered, searchable list. Launch, force-stop, clear data, uninstall, save its APK to your PC, disable or enable it |
| ⚡ | **ADB commands** | About 50 ready-made commands in categories: Info, Toggles, Buttons & Media, Open on phone, Power, Network & Server |
| 💻 | **Simple shell** | Shell shortcuts (`bat`, `ram`, `cpu`...), command history (`!3`), `:copy` and `:paste` for the clipboard, full interactive mode |
| 📐 | **Screen size (`wm`)** | Detects phone or tablet and recommends a size. Presets, custom size, custom DPI and a one-key reset |
| 🖥️ | **scrcpy view** | Mirror presets, mirror all devices in separate windows, several windows of one device, virtual display, recording |
| 📶 | **Wireless wizard** | Plug in once. It switches to Wi-Fi mode, finds the IP and connects for you |
| ⌨️ | **Send text to phone** | Type on your PC, or send your clipboard, into any text field on the phone |
| 📸 | **Screenshot and recording** | Saved straight to the folder you launched AstraADB from |
| 🧾 | **Save logcat** | Dump the phone's log to a text file |
| 🖋️ | **Typing effect** | Smooth 4 ms typewriter text, adjustable or switchable off in Settings |

---

## 🚀 Quick start

### 1. Requirements

| Tool | Needed for | Required? |
|---|---|---|
| A C++17 compiler (`g++`, `clang++` or MSVC) | Building AstraADB | Yes |
| [`adb`](https://developer.android.com/tools/releases/platform-tools) (Android platform-tools) | Everything | Yes |
| [`scrcpy`](https://github.com/Genymobile/scrcpy) | The screen mirror menu | Optional |
| `wl-clipboard`, `xclip` or `xsel` | `:copy` and `:paste` on Linux only | Optional |

> **Tip:** if `adb` or `scrcpy` is missing, AstraADB detects your OS and shows the best install command, then offers to run it for you.

### 2. Build

**Linux and macOS**

```bash
g++ -std=c++17 -O2 astraadb.cpp -o astraadb
./astraadb
```

**Windows (MSYS2 / MinGW)**

```bash
g++ -std=c++17 -O2 astraadb.cpp -o astraadb.exe
astraadb.exe
```

**Windows (Visual Studio Developer Prompt)**

```bat
cl /std:c++17 /EHsc astraadb.cpp
astraadb.exe
```

> On Windows, use **Windows Terminal** or any terminal with ANSI color support. AstraADB turns color support on automatically.

### 3. Prepare your phone

1. Open **Settings > About phone** and tap **Build number** 7 times to unlock Developer options.
2. Open **Settings > Developer options** and enable **USB debugging**.
3. Plug the phone in and accept the **Allow USB debugging?** prompt.

---

## 🧭 Usage

Run the program and type a number, then press Enter.

```
Main menu
 [1]  Device dashboard (model, battery, storage...)
 [2]  Select device
 [3]  Push file/folder  ->  phone
 [4]  Pull file/folder  <-  phone
 [5]  App manager (launch, uninstall, save APK...)
 [6]  ADB commands (toggles, buttons, info...)
 [7]  Simple shell
 [8]  Screen size (wm size / density)
 [9]  scrcpy view (mirror, multi-window)
 [10] Screenshot
 [11] Record screen
 [12] Install APK
 [13] Wireless setup wizard
 [14] Send text to phone (paste PC text)
 [15] Save logcat to a file
 [16] System check
 [17] Settings
 [0]  Exit
```

### File browser keys

| Key | Action |
|---|---|
| a number | Open that folder, or select that file |
| `u` | Go up one folder |
| `s` | Select the **current folder** (push or pull a whole folder) |
| `p` | Type, paste or drag-and-drop a path |
| `n` / `b` | Next or previous page |
| `q` | Cancel |

### App picker keys

| Key | Action |
|---|---|
| a number | Select that app |
| `/word` | Search by name (`/` alone clears the search) |
| `a` | Show or hide system apps |
| `n` / `b` / `q` | Next page, previous page, cancel |

### Shell commands

| Command | Action |
|---|---|
| `bat`, `ram`, `cpu`, `ip`, `apps`, `ver` | Built-in shortcuts for long commands |
| `adb ...` | Pasted adb commands are accepted as they are |
| `:h` and `!3` | Show history, rerun history item 3 |
| `:copy` | Copy the last output to your clipboard |
| `:paste` | Run the text in your clipboard (asks first) |
| `:i` | Open a full interactive `adb shell` |
| `:clear` / `:q` | Clear the screen / go back |

### Screen size (`wm`)

AstraADB reads your screen and works out whether it is a phone or a tablet from its smallest width in dp.

| Device type | Recommended short side |
|---|---|
| Phone | 1080 px |
| Tablet | 1200 px |

It also offers 90%, 75% and 60% presets, a custom `WxH`, a custom DPI and a **reset** that restores the original size and density. By default DPI is scaled together with the size so the layout stays the same.

> If a size ever looks wrong, run **Screen size > RESET**, or `adb shell wm size reset` and `adb shell wm density reset`.

### Wireless

Choose **Wireless setup wizard** while the phone is plugged in. When it says *Connected*, you can unplug the cable. For Android 11+ wireless debugging without a cable, use **ADB commands > Network & Server > Pair**.

---

## 🛠️ Customize it

Everything is in `astraadb.cpp`, and the parts you will want to change are simple tables near the top and middle of the file.

| I want to... | Edit this |
|---|---|
| Change typing speed, colors, page size, defaults | `namespace cfg` and `namespace col` at the top |
| Rename the logo text | `cfg::LOGO_TEXT` (letters must exist in `glyph()`) |
| Add a letter to the logo | Copy a block in `glyph()`: 5 rows, 5 characters wide |
| Add an ADB quick command | Add one line to `QUICK_CMDS` |
| Add a new command category | Use a new name in the first field of a `QUICK_CMDS` line |
| Add a shell shortcut | Add one line to `SHELL_ALIASES` |
| Add a quick-pull folder | Add one line to `PULL_SHORTCUTS` |
| Add a scrcpy preset | Add one line to `SCRCPY_PRESETS` |
| Add a main menu item | Add a label to `MAIN_ITEMS` and a `case` in `main()` |
| Change phone/tablet size targets | `PHONE_TARGET_SHORT`, `TABLET_TARGET_SHORT`, `TABLET_MIN_SW_DP` |

**Example: add a command that opens the camera.** Add this line to `QUICK_CMDS`:

```cpp
{"Open on phone", "Camera", "shell am start -a android.media.action.IMAGE_CAPTURE", nullptr, true},
```

The fields are: category, menu label, arguments after `adb -s SERIAL`, an optional question (`{}` in the arguments is replaced by the answer), and whether a phone is required.

---

## 🧩 Project structure

```
.
├── astraadb.cpp   # the entire program (one file)
├── README.md
└── LICENSE
```

The source is organised in this order: config, text and typing effect, logo, system helpers (commands, quoting, clipboard, OS detection), devices, file browser, features, main menu.

---

## ❓ Troubleshooting

<details>
<summary><b>"No device found"</b></summary>

Check that USB debugging is on, try another cable or port (some cables are charge-only), and run **ADB commands > Network & Server > Restart ADB server**.
</details>

<details>
<summary><b>Device shows as "unauthorized"</b></summary>

Unlock the phone and accept the **Allow USB debugging?** prompt. If it does not appear, revoke USB debugging authorizations in Developer options and reconnect.
</details>

<details>
<summary><b>Linux: device is listed as "no permissions"</b></summary>

Install your distro's `android-udev` / `android-tools` package or add a udev rule for your phone's vendor, then replug the device.
</details>

<details>
<summary><b>"scrcpy was not found"</b></summary>

AstraADB shows the install command for your OS when you open the scrcpy menu. The virtual display preset needs scrcpy 3.0 or newer.
</details>

<details>
<summary><b>:copy / :paste do nothing on Linux</b></summary>

Install `wl-clipboard` (Wayland) or `xclip` / `xsel` (X11). Run **System check** to see which clipboard tool AstraADB found.
</details>

<details>
<summary><b>Colors look broken on Windows</b></summary>

Use Windows Terminal or a recent Windows 10/11 console. Very old consoles do not support ANSI colors.
</details>

<details>
<summary><b>Some commands fail on my phone</b></summary>

Some commands depend on the Android version or the manufacturer's build. For example, airplane mode through ADB and wireless pairing need Android 11 or newer.
</details>

---

## 🗺️ Roadmap

- [ ] Fastboot menu
- [ ] Saved favorite commands
- [ ] Backup and restore helpers
- [ ] CMake build file

Ideas are welcome. Open an issue.

---

## 🤝 Contributing

Contributions are welcome.

1. Fork the repository
2. Create a branch: `git checkout -b feature/my-feature`
3. Keep the code simple: AstraADB is meant to stay easy to read and change
4. Make sure it builds with `g++ -std=c++17 -Wall -Wextra astraadb.cpp`
5. Open a pull request

---

## ⚠️ Disclaimer

AstraADB runs real ADB commands on your device. Actions such as **uninstall**, **clear data**, **reboot** and **screen size changes** can affect your phone. The tool asks before the riskiest ones, but you are responsible for what you run. Use it on devices you own or have permission to manage.

---

## 📄 License

Released under the **GPL License 3.0**. Add a `LICENSE` file to the repository with the GPL 3.0 text.

---

<div align="center">

If AstraADB saved you time, consider giving it a ⭐

</div>
