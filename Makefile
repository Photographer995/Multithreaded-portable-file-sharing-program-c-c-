# Компилятор и флаги компиляции
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Iinclude

# Директории исходных файлов, объектов и бинарников
SRCDIR   := src
INCDIR   := include
OBJDIR   := obj
BINDIR   := bin

# Исходные файлы
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
# Объектные файлы (каждый cpp-файл компилируется отдельно)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

# Отдельно выделяем объектные файлы для сервера и клиента
SERVER_OBJ := $(OBJDIR)/server.o $(OBJDIR)/file_utils.o $(OBJDIR)/file_manager.o
CLIENT_OBJ := $(OBJDIR)/client.o $(OBJDIR)/file_utils.o $(OBJDIR)/file_manager.o

# Линковка с pthread (и на Windows при необходимости можно добавить -lws2_32)
LDFLAGS  := -lpthread

# Целевые исполняемые файлы
all: $(BINDIR)/server $(BINDIR)/client

# Правило компиляции для объектных файлов
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Сборка серверного приложения
$(BINDIR)/server: $(SERVER_OBJ)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Сборка клиентского приложения
$(BINDIR)/client: $(CLIENT_OBJ)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Очистка сгенерированных файлов
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
