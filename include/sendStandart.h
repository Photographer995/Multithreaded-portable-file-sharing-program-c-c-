#ifndef SENDSTANDART_H
#define SENDSTANDART_H

#include <string>
#include <vector>
#include <filesystem>

// Структура, описывающая элемент каталога (1 – файл, 2 – каталог)
struct myDir {
    std::string name;
    int type;
};


std::string removelastElement(const std::string &path);


bool parseArguments(int argc, char* argv[], int &port, std::string &rootDir);

// Получает локальный IP-адрес сервера
std::string getLocalIpAddress();

#endif // SENDSTANDART_H
