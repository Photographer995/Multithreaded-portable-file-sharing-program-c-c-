#include "file_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <ctime>
#include <regex>
#include <cctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Конструктор: инициализирует корневой и текущий каталоги
FileManager::FileManager(const std::string &rootPath) : rootPath(rootPath) {
    currentPath = std::filesystem::canonical(rootPath).string();
}

std::string FileManager::getCurrentPath() const {
    return currentPath;
}

bool FileManager::setCurrentPath(const std::string &newPath) {
    try {
        std::filesystem::path p(newPath);
        std::filesystem::path canonicalNew = std::filesystem::canonical(p);
        std::filesystem::path canonicalRoot = std::filesystem::canonical(rootPath);
        // Проверка: новый путь должен начинаться с корневого каталога
        if(canonicalNew.string().find(canonicalRoot.string()) != 0)
            return false;
        if(std::filesystem::is_directory(canonicalNew)) {
            currentPath = canonicalNew.string();
            return true;
        } else {
            return false;
        }
    } catch(...) {
        return false;
    }
}

std::string FileManager::removeLastElement(const std::string &path) const {
    if(path.empty()) return path;
    std::string trimmed = path;
    if(trimmed.back() == '/' || trimmed.back() == '\\')
        trimmed.pop_back();
    size_t pos = trimmed.find_last_of("/\\");
    if(pos == std::string::npos)
        return trimmed;
    return trimmed.substr(0, pos);
}

void FileManager::sendDirectoryListing(int clientSocket) const {
    std::stringstream ss;
    ss << "Текущий каталог: " << currentPath << "\n";
    for(const auto &entry: std::filesystem::directory_iterator(currentPath)) {
        std::string name = entry.path().filename().string();
        if(entry.is_directory())
            name += "/";
        ss << name << "\n";
    }
    std::string listStr = ss.str();
    send(clientSocket, listStr.c_str(), listStr.size(), 0);
}

void FileManager::sendFileToClient(int clientSocket, const std::string &filename, std::streampos offset) const {
    std::string filepath = currentPath + "/" + filename;
    if(!std::filesystem::exists(filepath)) {
        std::string err = "Ошибка: файл не найден\n";
        send(clientSocket, err.c_str(), err.size(), 0);
        return;
    }

    std::ifstream file(filepath, std::ios::binary);
    if(!file) {
        std::string err = "Ошибка: не удалось открыть файл\n";
        send(clientSocket, err.c_str(), err.size(), 0);
        return;
    }

    file.seekg(0, std::ios::end);
    std::streampos filesize = file.tellg();
    if(offset > filesize) {
        std::string err = "Ошибка: неверный offset\n";
        send(clientSocket, err.c_str(), err.size(), 0);
        return;
    }
    std::streamoff remaining = filesize - offset;

    // Отправляем заголовок с информацией о размере передаваемых данных
    std::string header = "FILE " + std::to_string(remaining) + "\n";
    send(clientSocket, header.c_str(), header.size(), 0);

    file.seekg(offset, std::ios::beg);
    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    while(file && remaining > 0) {
        file.read(buffer, bufferSize);
        std::streamsize bytesRead = file.gcount();
        if(bytesRead <= 0)
            break;
        send(clientSocket, buffer, bytesRead, 0);
        remaining -= bytesRead;
    }
    file.close();
}

void FileManager::receiveFile(int sock, const std::string &filename, std::streampos offset) const {
    std::streampos resumeOffset = offset;
    // Если offset равен 0 и файл уже существует, спрашиваем, нужно ли возобновлять передачу
    if(offset == 0 && std::filesystem::exists(filename)) {
        std::cout << "Файл " << filename << " уже существует. Возобновить передачу? (y/n): ";
        std::string answer;
        std::getline(std::cin, answer);
        if(answer == "y" || answer == "Y") {
            resumeOffset = std::filesystem::file_size(filename);
        } else {
            resumeOffset = 0;
        }
    }
    // Формируем команду с указанием offset
    std::string command = "get " + filename + " " + std::to_string(static_cast<long long>(resumeOffset)) + "\n";
    send(sock, command.c_str(), command.size(), 0);

    // Получаем заголовок, содержащий размер передаваемых данных
    char header[128];
    memset(header, 0, sizeof(header));
    int bytes = recv(sock, header, sizeof(header)-1, 0);
    if(bytes <= 0) {
        std::cout << "Ошибка получения заголовка файла" << std::endl;
        return;
    }
    std::string headerStr(header);
    if(headerStr.find("FILE ") != 0) {
        std::cout << headerStr;
        return;
    }
    std::istringstream iss(headerStr);
    std::string fileTag;
    size_t fileSize;
    iss >> fileTag >> fileSize;

    std::ofstream file;
    if(resumeOffset > 0)
        file.open(filename, std::ios::binary | std::ios::app);
    else
        file.open(filename, std::ios::binary);

    if(!file) {
        std::cout << "Не удалось открыть файл для записи" << std::endl;
        return;
    }

    const size_t bufferSize = 1024;
    char bufferData[bufferSize];
    size_t remaining = fileSize;
    while(remaining > 0) {
        int chunk = recv(sock, bufferData, std::min(bufferSize, remaining), 0);
        if(chunk <= 0)
            break;
        file.write(bufferData, chunk);
        remaining -= chunk;
    }
    file.close();
    std::cout << "\nПередача файла завершена." << std::endl;
}

void FileManager::sendFileInfo(int clientSocket, const std::string &filename) const {
    std::string filepath = currentPath + "/" + filename;
    std::stringstream ss;
    if(!std::filesystem::exists(filepath)) {
        ss << "Ошибка: файл или каталог не найден\n";
    } else {
        ss << "Информация о " << filename << ":\n";
        auto status = std::filesystem::status(filepath);
        auto ftype = status.type();
        // Определяем тип файла
        switch (ftype) {
            case std::filesystem::file_type::regular:
                ss << "Тип: обычный файл\n";
                break;
            case std::filesystem::file_type::directory:
                ss << "Тип: каталог\n";
                break;
            case std::filesystem::file_type::symlink:
                ss << "Тип: символическая ссылка\n";
                break;
            case std::filesystem::file_type::block:
                ss << "Тип: блочное устройство\n";
                break;
            case std::filesystem::file_type::character:
                ss << "Тип: символьное устройство\n";
                break;
            case std::filesystem::file_type::fifo:
                ss << "Тип: FIFO (канал)\n";
                break;
            case std::filesystem::file_type::socket:
                ss << "Тип: сокет\n";
                break;
            default:
                ss << "Тип: неизвестный\n";
                break;
        }
        // Если это обычный файл – получаем размер
        if(ftype == std::filesystem::file_type::regular) {
            try {
                uintmax_t size = std::filesystem::file_size(filepath);
                ss << "Размер: " << size << " байт\n";
            } catch(...) {
                ss << "Размер: неизвестен\n";
            }
        }
        // Получаем права доступа и формируем строку, похожую на Unix-формат
        auto perms = status.permissions();
        std::string permissionStr;
        // Владелец
        permissionStr.push_back((perms & std::filesystem::perms::owner_read)  != std::filesystem::perms::none ? 'r' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? 'w' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::owner_exec)  != std::filesystem::perms::none ? 'x' : '-');
        permissionStr.push_back(' ');
        // Группа
        permissionStr.push_back((perms & std::filesystem::perms::group_read)  != std::filesystem::perms::none ? 'r' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::group_write) != std::filesystem::perms::none ? 'w' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::group_exec)  != std::filesystem::perms::none ? 'x' : '-');
        permissionStr.push_back(' ');
        // Остальные
        permissionStr.push_back((perms & std::filesystem::perms::others_read)  != std::filesystem::perms::none ? 'r' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::others_write) != std::filesystem::perms::none ? 'w' : '-');
        permissionStr.push_back((perms & std::filesystem::perms::others_exec)  != std::filesystem::perms::none ? 'x' : '-');
        ss << "Права доступа: " << permissionStr << "\n";

        // Получаем время последнего изменения
        try {
            auto ftime = std::filesystem::last_write_time(filepath);
            auto sctp = std::chrono::system_clock::to_time_t(
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
                )
            );
            ss << "Последнее изменение: " << std::ctime(&sctp);
        } catch(...) {
            ss << "Не удалось получить время последнего изменения\n";
        }
    }
    std::string infoStr = ss.str();
    send(clientSocket, infoStr.c_str(), infoStr.size(), 0);
}

// Вспомогательная функция для конвертации glob-шаблона в регулярное выражение
static std::string globToRegex(const std::string &glob) {
    std::string regex;
    regex.push_back('^');
    for (char c : glob) {
        switch (c) {
            case '*':
                regex.append(".*");
                break;
            case '?':
                regex.push_back('.');
                break;
            case '.':
                regex.append("\\.");
                break;
            default:
                // Экранируем специальные символы, если необходимо
                if(std::isalnum(c)) {
                    regex.push_back(c);
                } else {
                    regex.push_back('\\');
                    regex.push_back(c);
                }
                break;
        }
    }
    regex.push_back('$');
    return regex;
}

void FileManager::findFiles(int clientSocket, SearchCategory category, const std::string &pattern) const {
    std::stringstream ss;
    std::regex re(globToRegex(pattern), std::regex::icase);
    bool found = false;

    for (const auto &entry : std::filesystem::recursive_directory_iterator(currentPath)) {
        // Фильтрация по категории
        if (category == SearchCategory::Files && !entry.is_regular_file())
            continue;
        if (category == SearchCategory::Directories && !entry.is_directory())
            continue;

        std::string name = entry.path().filename().string();
        if (std::regex_match(name, re)) {
            found = true;
            ss << entry.path().string() << "\n";
        }
    }
    if (!found) {
        ss << "Ничего не найдено по шаблону \"" << pattern << "\"\n";
    }
    std::string result = ss.str();
    send(clientSocket, result.c_str(), result.size(), 0);
}
