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

// Удаляет последний элемент пути (например, имя файла или каталога)
std::string removelastElement(const std::string &path);

// Разбирает аргументы командной строки: ожидается порт и путь к корневому каталогу.
// При успешном разборе возвращает true, иначе – false.
bool parseArguments(int argc, char* argv[], int &port, std::string &rootDir);

// Получает локальный IP-адрес сервера
std::string getLocalIpAddress();

#endif // SENDSTANDART_H
