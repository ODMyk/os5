#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <gelf.h>
#include <libelf.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

uintptr_t find_input_offset(const char* binary_path) {
    if (elf_version(EV_CURRENT) == EV_NONE) {
        std::cerr << "ELF init error\n";
        return 0;
    }

    int fd = open(binary_path, O_RDONLY);
    if (fd < 0) {
        perror("open binary");
        return 0;
    }

    Elf* elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        std::cerr << "elf_begin failed\n";
        close(fd);
        return 0;
    }

    Elf_Scn* scn = nullptr;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn))) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data* data = elf_getdata(scn, nullptr);
            int symbols = shdr.sh_size / shdr.sh_entsize;

            for (int i = 0; i < symbols; ++i) {
                GElf_Sym sym;
                gelf_getsym(data, i, &sym);

                const char* name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                if (name && strcmp(name, "input") == 0) {
                    uintptr_t offset = sym.st_value;
                    elf_end(elf);
                    close(fd);
                    return offset;
                }
            }
        }
    }

    elf_end(elf);
    close(fd);
    return 0;
}

uintptr_t find_base_address(pid_t pid, const std::string& binary_path) {
    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(binary_path) != std::string::npos && line.find("r-xp") != std::string::npos) {
            std::stringstream ss(line);
            std::string address_range;
            ss >> address_range;
            size_t dash = address_range.find('-');
            std::string base_address_str = address_range.substr(0, dash);
            return std::stoull(base_address_str, nullptr, 16);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Використання: ./spy_full <pid>\n";
        return 1;
    }

    pid_t pid = std::stoi(argv[1]);

    // Отримати шлях до ELF
    std::string exe_link = "/proc/" + std::to_string(pid) + "/exe";
    char exe_path[1024];
    ssize_t len = readlink(exe_link.c_str(), exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink");
        return 1;
    }
    exe_path[len] = '\0';

    uintptr_t offset = find_input_offset(exe_path);
    if (!offset) {
        std::cerr << "Не вдалося знайти символ 'input'\n";
        return 1;
    }

    uintptr_t base = find_base_address(pid, exe_path);
    if (!base) {
        std::cerr << "Не вдалося знайти базову адресу\n";
        return 1;
    }

    uintptr_t address = base + offset;

    std::string mem_path = "/proc/" + std::to_string(pid) + "/mem";
    int mem_fd = open(mem_path.c_str(), O_RDONLY);
    if (mem_fd == -1) {
        perror("open mem");
        return 1;
    }

    unsigned int prev = 0, curr = 0;

    std::cout << "Спостерігаю за адресою: 0x" << std::hex << address << std::dec << "\n";

    while (true) {
        if (lseek(mem_fd, address, SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        ssize_t n = read(mem_fd, &curr, sizeof(curr));
        if (n != sizeof(curr)) {
            perror("read");
            break;
        }

        if (curr != prev) {
            std::cout << "Зміна: " << curr << "\n";
            prev = curr;
        }

        usleep(100 * 1000);
    }

    close(mem_fd);
    return 0;
}
