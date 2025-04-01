#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <filesystem>


class FileManager {
public:
    enum class SearchCategory {
        All,
        Files,
        Directories
    };

    
    FileManager(const std::string &rootPath);

    
    std::string getCurrentPath() const;

    
    bool setCurrentPath(const std::string &newPath);

    
    void sendDirectoryListing(int clientSocket) const;

    // Отправляет файл клиенту с поддержкой возобновления передачи (offset)
    void sendFileToClient(int clientSocket, const std::string &filename, std::streampos offset) const;

    
    void receiveFile(int sock, const std::string &filename, std::streampos offset) const;

    
    void sendFileInfo(int clientSocket, const std::string &filename) const;

    
    void findFiles(int clientSocket, SearchCategory category, const std::string &pattern) const;

private:
    std::string rootPath;
    std::string currentPath;

    
    std::string removeLastElement(const std::string &path) const;
};

#endif // FILE_MANAGER_H
