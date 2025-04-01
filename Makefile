
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Iinclude


SRCDIR   := src
INCDIR   := include
OBJDIR   := obj
BINDIR   := bin

SOURCES := $(wildcard $(SRCDIR)/*.cpp)

OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))


SERVER_OBJ := $(OBJDIR)/server.o $(OBJDIR)/file_utils.o $(OBJDIR)/file_manager.o
CLIENT_OBJ := $(OBJDIR)/client.o $(OBJDIR)/file_utils.o $(OBJDIR)/file_manager.o

# Линковка с pthread (и на Windows при необходимости можно добавить -lws2_32)
LDFLAGS  := -lpthread

all: $(BINDIR)/server $(BINDIR)/client

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


clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
