#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <filesystem>

// Класс, инкапсулирующий файловые операции
class FileManager {
public:
    // Перечисление для указания категории поиска (вложено в класс)
    enum class SearchCategory {
        All,
        Files,
        Directories
    };

    // Конструктор принимает путь к корневому каталогу и устанавливает текущий каталог равным ему
    FileManager(const std::string &rootPath);

    // Возвращает текущий рабочий каталог
    std::string getCurrentPath() const;

    // Устанавливает новый текущий каталог (если он находится внутри корневого каталога)
    // Возвращает true при успехе, иначе false
    bool setCurrentPath(const std::string &newPath);

    // Отправляет клиенту список файлов и каталогов текущего каталога
    void sendDirectoryListing(int clientSocket) const;

    // Отправляет файл клиенту с поддержкой возобновления передачи (offset)
    void sendFileToClient(int clientSocket, const std::string &filename, std::streampos offset) const;

    // Принимает файл с сервера с поддержкой возобновления передачи
    void receiveFile(int sock, const std::string &filename, std::streampos offset) const;

    // Отправляет информацию (характеристики) о файле клиенту
    void sendFileInfo(int clientSocket, const std::string &filename) const;

    // Ищет файлы по шаблону и отправляет результаты клиенту.
    // category - ограничение поиска: все, только файлы или только каталоги;
    // pattern - шаблон для поиска (поддерживаются символы * и ?)
    void findFiles(int clientSocket, SearchCategory category, const std::string &pattern) const;

private:
    std::string rootPath;
    std::string currentPath;

    // Вспомогательный метод: удаляет последний элемент пути (аналог removelastElement)
    std::string removeLastElement(const std::string &path) const;
};

#endif // FILE_MANAGER_H
