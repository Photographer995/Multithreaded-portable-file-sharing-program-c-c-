#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <clocale>         // Для установки локали
#include "file_manager.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#endif

// Глобальный флаг, сигнализирующий о том, что идёт приём файла
std::atomic_bool g_receivingFile(false);

// Поток для приёма текстовых сообщений от сервера
void* recv_thread(void* arg) {
    int sock = *(int*)arg;
    char buffer[1024];
    while (true) {
        if (g_receivingFile.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
            break;
        std::cout << buffer;
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Устанавливаем локаль для корректной работы с UTF-8
    std::setlocale(LC_ALL, "ru_RU.UTF-8");

    if(argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <IP_сервера> <порт>" << std::endl;
        return -1;
    }
    std::string serverIP = argv[1];
    int port = std::atoi(argv[2]);
    #ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "Ошибка инициализации Winsock" << std::endl;
        return -1;
    }
    #endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        std::cerr << "Не удалось создать сокет" << std::endl;
        return -1;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if(inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Неверный IP-адрес" << std::endl;
        return -1;
    }
    if(connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Не удалось подключиться к серверу" << std::endl;
        return -1;
    }
    std::cout << "Подключение к серверу " << serverIP << ":" << port << " установлено." << std::endl;

    // Создаём объект FileManager (для клиента корневой каталог не используется)
    FileManager fileManager(".");

    pthread_t threadId;
    if(pthread_create(&threadId, nullptr, recv_thread, &sock) != 0) {
        std::cerr << "Не удалось создать поток для приёма сообщений" << std::endl;
        #ifdef _WIN32
        closesocket(sock);
        #else
        close(sock);
        #endif
        return -1;
    }

    // Основной цикл взаимодействия с сервером
    std::string input;
    while(true) {
        std::cout << "\nВведите команду (ls, cd <dir>, get <file>, exit): ";
        std::getline(std::cin, input);
        if(input.empty())
            continue;
        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;
        if(cmd == "exit") {
            send(sock, input.c_str(), input.size(), 0);
            break;
        } else if(cmd == "get") {
            std::string filename;
            iss >> filename;
            if(filename.empty()) {
                std::cout << "Укажите имя файла" << std::endl;
                continue;
            }
            // Перед началом загрузки файла ставим флаг
            g_receivingFile.store(true);
            send(sock, input.c_str(), input.size(), 0);

            std::ofstream outFile(filename, std::ios::binary);
            if (!outFile) {
                std::cerr << "Ошибка создания файла " << filename << std::endl;
                g_receivingFile.store(false);
                continue;
            }

            char buffer[1024];
            int bytesReceived;
            // Читаем данные для файла до тех пор, пока они приходят
            while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
                outFile.write(buffer, bytesReceived);
                // Если пришло меньше, чем размер буфера, предполагаем окончание передачи
                if (bytesReceived < (int)sizeof(buffer))
                    break;
            }
            outFile.close();
            g_receivingFile.store(false);
            std::cout << "Файл " << filename << " успешно загружен." << std::endl;
        } else {
            input += "\n";
            send(sock, input.c_str(), input.size(), 0);
        }
    }
    #ifdef _WIN32
    closesocket(sock);
    WSACleanup();
    #else
    close(sock);
    #endif
    pthread_join(threadId, nullptr);
    return 0;
}
