#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include "sendStandart.h"
#include "file_manager.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#endif

// Структура для передачи информации о клиенте в поток
struct ClientInfo {
    int clientSocket;
    std::string ip;
};

// Глобальная переменная для корневого каталога
std::string pathToRootdir;

// Глобальный указатель на объект FileManager
FileManager *globalFileManager = nullptr;

// Функция для получения текущего времени в виде строки
std::string getCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Убираем символ перевода строки из std::ctime()
    std::string timeStr = std::ctime(&now_time);
    timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), '\n'), timeStr.end());
    return timeStr;
}
// ... (включения, определения, структура ClientInfo, функции getCurrentTimeStr и т.д.)

void* handle_client(void* arg) {
    ClientInfo* info = static_cast<ClientInfo*>(arg);
    int clientSocket = info->clientSocket;
    std::string clientIp = info->ip;
    delete info; // освобождаем память

    std::cout << "[" << getCurrentTimeStr() << "] Клиент " << clientIp << " подключился." << std::endl;

    char buffer[1024];

    // Отправляем приветственное сообщение и список файлов через FileManager
    std::string welcome = "Добро пожаловать на сервер обмена файлами!\n";
    send(clientSocket, welcome.c_str(), welcome.size(), 0);
    globalFileManager->sendDirectoryListing(clientSocket);

    while(true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if(bytesReceived <= 0)
            break;
        std::string command(buffer);
        // Убираем символы переноса строки и возврата каретки
        command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
        command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());

        // Логирование активности клиента
        std::cout << "[" << getCurrentTimeStr() << "] Клиент " << clientIp << " отправил команду: " << command << std::endl;

        if(command == "exit") {
            std::string bye = "Соединение закрывается...\n";
            send(clientSocket, bye.c_str(), bye.size(), 0);
            break;
        } else if(command == "ls") {
            globalFileManager->sendDirectoryListing(clientSocket);
        } else if(command.substr(0,3) == "cd ") {
            std::string dir = command.substr(3);
            std::string newPath = globalFileManager->getCurrentPath() + "/" + dir;
            try {
                std::filesystem::path p(newPath);
                std::filesystem::path canonicalNew = std::filesystem::canonical(p);
                std::filesystem::path canonicalRoot = std::filesystem::canonical(pathToRootdir);
                if(canonicalNew.string().find(canonicalRoot.string()) != 0) {
                    std::string err = "Ошибка: доступ запрещён\n";
                    send(clientSocket, err.c_str(), err.size(), 0);
                } else if(std::filesystem::is_directory(canonicalNew)) {
                    if(globalFileManager->setCurrentPath(canonicalNew.string())) {
                        std::string ok = "Текущий каталог изменён: " + globalFileManager->getCurrentPath() + "\n";
                        send(clientSocket, ok.c_str(), ok.size(), 0);
                    } else {
                        std::string err = "Ошибка: не удалось сменить каталог\n";
                        send(clientSocket, err.c_str(), err.size(), 0);
                    }
                } else {
                    std::string err = "Ошибка: не является директорией\n";
                    send(clientSocket, err.c_str(), err.size(), 0);
                }
            } catch(...) {
                std::string err = "Ошибка: неверный путь\n";
                send(clientSocket, err.c_str(), err.size(), 0);
            }
        } else if(command.substr(0,4) == "get ") {
            // Обработка команды get <filename> [offset]
            std::istringstream iss(command);
            std::string cmd, filename;
            iss >> cmd >> filename;
            long long offset_val = 0;
            if(iss >> offset_val)
                ; // offset_val уже получен
                else
                    offset_val = 0;
            std::streampos offset = static_cast<std::streampos>(offset_val);
            globalFileManager->sendFileToClient(clientSocket, filename, offset);
        } else if(command.substr(0,5) == "stat ") {
            // Обработка команды stat <filename>
            std::istringstream iss(command);
            std::string cmd, filename;
            iss >> cmd >> filename;
            globalFileManager->sendFileInfo(clientSocket, filename);
        } else if(command == "help") {
            // Расширенная справка
            std::string helpText =
            "Доступные команды:\n"
            "ls               - показать список файлов и каталогов\n"
            "cd <dir>         - сменить текущий каталог\n"
            "get <file> [off] - загрузить файл с сервера (опционально с offset)\n"
            "stat <file>      - показать информацию о файле/каталоге\n"
            "find [опция] <шаблон> - поиск файлов\n"
            "    -f         - искать только файлы\n"
            "    -d         - искать только каталоги\n"
            "    без опций - искать и файлы, и каталоги\n"
            "help             - показать эту справку\n"
            "exit             - закрыть соединение\n";
            send(clientSocket, helpText.c_str(), helpText.size(), 0);
        } else if(command.substr(0,5) == "find ") {
            // Обработка команды find, формат: find [опция] <шаблон>
            std::istringstream iss(command);
            std::string cmd, token, pattern;
            iss >> cmd;
            FileManager::SearchCategory category = FileManager::SearchCategory::All;
            if(iss >> token) {
                if(token == "-f") {
                    category = FileManager::SearchCategory::Files;
                    if(!(iss >> pattern)) {
                        std::string error = "Ошибка: не указан шаблон для поиска\n";
                        send(clientSocket, error.c_str(), error.size(), 0);
                        continue;
                    }
                } else if(token == "-d") {
                    category = FileManager::SearchCategory::Directories;
                    if(!(iss >> pattern)) {
                        std::string error = "Ошибка: не указан шаблон для поиска\n";
                        send(clientSocket, error.c_str(), error.size(), 0);
                        continue;
                    }
                } else {
                    pattern = token;
                }
                globalFileManager->findFiles(clientSocket, category, pattern);
            } else {
                std::string error = "Ошибка: не указан шаблон для поиска\n";
                send(clientSocket, error.c_str(), error.size(), 0);
            }
        } else {
            std::string unknown = "Неизвестная команда\n";
            send(clientSocket, unknown.c_str(), unknown.size(), 0);
        }
    }
    std::cout << "[" << getCurrentTimeStr() << "] Клиент " << clientIp << " отключился." << std::endl;
    #ifdef _WIN32
    closesocket(clientSocket);
    #else
    close(clientSocket);
    #endif
    return nullptr;
}

// main() и остальной код остаются без изменений...


int main(int argc, char* argv[]) {
    int port;
    if(!parseArguments(argc, argv, port, pathToRootdir))
        return -1;

    // Инициализируем FileManager с корневым каталогом
    FileManager fileManager(pathToRootdir);
    globalFileManager = &fileManager;

    #ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "Ошибка инициализации Winsock" << std::endl;
        return -1;
    }
    #endif

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0) {
        std::cerr << "Не удалось создать сокет" << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if(bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        #ifdef _WIN32
        closesocket(serverSocket);
        #else
        close(serverSocket);
        #endif
        return -1;
    }

    listen(serverSocket, 5);
    std::cout << "[" << getCurrentTimeStr() << "] Сервер запущен на " << getLocalIpAddress() << ":" << port << std::endl;
    std::cout << "[" << getCurrentTimeStr() << "] Ожидание подключений..." << std::endl;

    while(true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if(clientSocket < 0) {
            std::cerr << "Ошибка accept" << std::endl;
            continue;
        }
        std::string clientIp = inet_ntoa(clientAddr.sin_addr);
        std::cout << "[" << getCurrentTimeStr() << "] Новое подключение от: " << clientIp << std::endl;
        ClientInfo *pClient = new ClientInfo;
        pClient->clientSocket = clientSocket;
        pClient->ip = clientIp;
        pthread_t threadId;
        if(pthread_create(&threadId, nullptr, handle_client, pClient) != 0) {
            std::cerr << "Не удалось создать поток для клиента" << std::endl;
            #ifdef _WIN32
            closesocket(clientSocket);
            #else
            close(clientSocket);
            #endif
            delete pClient;
        }
        pthread_detach(threadId);
    }

    #ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
    #else
    close(serverSocket);
    #endif

    return 0;
}
