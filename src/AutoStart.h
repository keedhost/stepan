#pragma once

class AutoStart {
public:
    static bool isSupported();
    static bool isEnabled();
    static bool setEnabled(bool enable);
};
