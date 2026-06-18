# Збірка Степан

Цей документ описує як підготувати оточення, встановити залежності та зібрати застосунок на macOS, Linux та Windows.

GeographicLib завантажується і збирається автоматично через CMake FetchContent — встановлювати його окремо **не потрібно**.

---

## Залежності

| Інструмент | Мінімальна версія | Примітка |
|---|---|---|
| CMake | 3.20 | |
| C++ компілятор | C++17 | Clang / GCC / MSVC |
| Qt 6 | 6.5 | Компоненти: Widgets, WebEngineWidgets, WebChannel, Network |
| Ninja | будь-яка | Рекомендовано; альтернатива — Make / Xcode / Visual Studio |
| Git | будь-яка | Для клонування і FetchContent |

---

## macOS

### 1. Встановлення залежностей

**Homebrew (рекомендовано):**
```bash
brew install cmake ninja qt
```

**Або Qt Online Installer** — завантажити з [qt.io/download](https://www.qt.io/download-qt-installer), обрати компоненти:
- Qt 6.x → macOS
- Qt WebEngine

### 2. Збірка

```bash
git clone https://github.com/keedhost/stepan.git
cd stepan
./scripts/build-macos.sh
```

Скрипт автоматично знаходить Qt (через Homebrew або стандартні шляхи), конфігурує CMake і запускає білд.

**Або вручну:**
```bash
export CMAKE_PREFIX_PATH=$(brew --prefix qt)   # або шлях до ~/Qt/6.x/macos
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 3. Результат

```
build/Stepan.app          ← готовий .app bundle
```

**Для розповсюдження** (вбудовує Qt-бібліотеки, створює DMG):
```bash
$(brew --prefix qt)/bin/macdeployqt build/Stepan.app -dmg
# → build/Stepan.dmg
```

---

## Linux

Протестовано на Ubuntu 22.04 / Debian 12.

### 1. Встановлення залежностей

```bash
sudo apt update
sudo apt install \
    cmake ninja-build git \
    qt6-base-dev \
    qt6-webengine-dev \
    libqt6webenginewidgets6 \
    libqt6webchannel6-dev \
    libgl-dev
```

> На Fedora/RHEL замість `apt` використовуйте `dnf`, пакунки мають назви на кшталт `qt6-qtbase-devel`, `qt6-qtwebengine-devel`.

### 2. Збірка

```bash
git clone https://github.com/keedhost/stepan.git
cd stepan
./scripts/build-linux.sh
```

**Або вручну:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 3. Результат

```
build/stepan              ← виконуваний файл
```

**Запуск:**
```bash
./build/stepan
```

**Встановлення у систему** (іконки + .desktop файл):
```bash
cmake --install build --prefix /usr/local
```

**Портативний AppImage** (потребує [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy)):
```bash
linuxdeploy --appdir AppDir \
            --executable build/stepan \
            --desktop-file build/stepan.desktop \
            --icon-file icons/icon-256x256.png \
            --output appimage
```

---

## Windows

### 1. Встановлення залежностей

1. **CMake** — [cmake.org/download](https://cmake.org/download/) (додати до PATH при встановленні)
2. **Git** — [git-scm.com](https://git-scm.com)
3. **Visual Studio 2022** (або 2019) — при встановленні обрати компонент **«Desktop development with C++»**
4. **Qt 6** — [Qt Online Installer](https://www.qt.io/download-qt-installer):
   - Qt 6.x → MSVC 2022 64-bit
   - Qt WebEngine

> **Ninja (опційно):** якщо встановлено, скрипт використає його замість Visual Studio — білд швидший. Встановити: `winget install Ninja-build.Ninja`

### 2. Збірка

Відкрийте **Developer Command Prompt for VS 2022** (або PowerShell зі встановленим середовищем MSVC) і виконайте:

```bat
git clone https://github.com/keedhost/stepan.git
cd stepan
scripts\build-windows.bat
```

Скрипт автоматично знаходить Qt у стандартних шляхах (`C:\Qt\6.x.x\msvc2022_64`).

Якщо Qt встановлено в нестандартне місце:
```bat
set CMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2022_64
scripts\build-windows.bat
```

**Або вручну (Visual Studio):**
```bat
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2022_64
cmake --build build --config Release --parallel
```

### 3. Результат

```
build\Release\Stepan.exe        ← виконуваний файл (Ninja: build\Stepan.exe)
```

**Для розповсюдження** — скрипт автоматично запускає `windeployqt`, який копіює всі потрібні Qt DLL поруч з .exe. Після цього папку `build\Release\` можна архівувати і передавати.

---

## Ручна конфігурація CMake (всі платформи)

| Параметр | Значення | Опис |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` / `Debug` | Тип збірки |
| `CMAKE_PREFIX_PATH` | шлях до Qt | Якщо Qt не знайдено автоматично |
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Префікс для `cmake --install` (Linux) |

**Приклад з явним шляхом до Qt:**
```bash
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/home/user/Qt/6.7.0/gcc_64
cmake --build build --parallel
```

---

## Часті проблеми

**`Qt6 not found`**
: Встановіть Qt і вкажіть `CMAKE_PREFIX_PATH` — шлях до директорії, що містить `lib/cmake/Qt6`.

**`Could not find QtWebEngine`**
: Перевірте, що при встановленні Qt було обрано компонент **Qt WebEngine**. Без нього вбудована карта не працює.

**`GeographicLib` не завантажується**
: Потрібен доступ до GitHub під час конфігурації CMake. FetchContent клонує репозиторій автоматично при першому запуску `cmake -B build`.

**На Linux: помилки OpenGL**
: Встановіть `libgl-dev` (Debian/Ubuntu) або `mesa-libGL-devel` (Fedora).
