# Встановлення Степан

---

## macOS

### 1. Завантажте DMG

Завантажте файл `Stepan-<версія>-macos.dmg` зі сторінки [Releases](https://github.com/keedhost/stepan/releases).

### 2. Змонтуйте та скопіюйте

Двічі клацніть `.dmg`, перетягніть `Stepan.app` до папки `/Applications`.

### 3. Перший запуск (важливо!)

Оскільки програма **не підписана** сертифікатом Apple Developer, macOS за замовчуванням заблокує запуск із повідомленням:

> *"Stepan" не вдалось відкрити, оскільки Apple не може перевірити...*

Або Gatekeeper просто не дозволить запуститися без пояснень.

**Виконайте у Terminal один раз:**

```bash
sudo xattr -r -c /Applications/Stepan.app
```

Після цього програма відкривається звичайним подвійним кліком.

#### Альтернативний спосіб (без sudo)

1. Натисніть правою кнопкою на `Stepan.app` → **Відкрити**
2. У діалозі підтвердження натисніть **Відкрити**

Цей спосіб спрацьовує лише один раз — після нього Gatekeeper запам'ятовує вибір.

---

## Windows

### 1. Завантажте ZIP

Завантажте `Stepan-<версія>-windows-x64.zip` зі сторінки [Releases](https://github.com/keedhost/stepan/releases).

### 2. Розпакуйте

Розпакуйте архів у будь-яке зручне місце, наприклад `C:\Programs\Stepan\`.

### 3. Запустіть

Двічі клацніть `Stepan.exe`. Qt DLL та всі залежності вже включені у ZIP — встановлення не потрібне.

> **Windows SmartScreen** може показати попередження при першому запуску. Натисніть **Докладніше → Все одно виконати**.

---

## Linux

### AppImage (рекомендовано — без встановлення)

```bash
chmod +x Stepan-<версія>-linux-x64.AppImage
./Stepan-<версія>-linux-x64.AppImage
```

> На деяких дистрибутивах потрібен FUSE: `sudo apt install libfuse2` (Ubuntu/Debian).

### DEB (Debian / Ubuntu)

```bash
sudo dpkg -i stepan_<версія>_amd64.deb
sudo apt-get install -f   # встановить залежності, якщо потрібно
```

### RPM (Fedora / openSUSE / RHEL)

```bash
sudo rpm -i stepan-<версія>-1.x86_64.rpm
```

### tar.xz (портативно)

```bash
tar -xJf Stepan-<версія>-linux-x64.tar.xz
./bin/stepan
```

---

## Збірка з вихідного коду

Дивіться [BUILD.md](BUILD.md) для покрокових інструкцій збірки на macOS, Linux та Windows.
