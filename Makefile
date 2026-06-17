ifneq ($(OS),Windows_NT) 
    CXX = x86_64-w64-mingw32-g++-win32
    WINDRES = x86_64-w64-mingw32-windres
    CXXFLAGS = -Wall -O2 -std=c++11 -DWIN32_LEAN_AND_MEAN -mwindows
    LDFLAGS = -static -mwindows -lws2_32 -lcomctl32 -lgdi32 -lwinmm -lcomdlg32 -luuid
    TARGET = messenger.exe
else
    CXX = g++
    WINDRES = windres
    CXXFLAGS = -Wall -O2 -std=c++11 -mwindows
    LDFLAGS = -static -mwindows -lws2_32 -lcomctl32 -lgdi32 -lcomdlg32 -lwinpthread
    TARGET = messenger.exe
endif

SOURCES = src/main.cpp \
          src/core/Logger.cpp \
          src/core/HttpClient.cpp \
          src/core/CryptoManager.cpp \
          src/core/Database.cpp \
          src/core/MessageServer.cpp \
          src/core/NetworkClient.cpp \
          src/gui/MainWindow.cpp \
          src/gui/LoginDialog.cpp

OBJECTS = $(SOURCES:.cpp=.o)
RESOURCE = resources/resource.o

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(CXX) $(OBJECTS) $(RESOURCE) -o $(TARGET) $(LDFLAGS)

$(RESOURCE): resources/resource.rc
	$(WINDRES) resources/resource.rc -O coff -o $(RESOURCE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(RESOURCE) $(TARGET) 2>/dev/null || del /Q $(OBJECTS) $(RESOURCE) $(TARGET) 2>nul

run: $(TARGET)
	wine $(TARGET) 2>/dev/null || ./$(TARGET)
