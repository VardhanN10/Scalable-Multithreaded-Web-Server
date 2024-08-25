CXX = g++ -fsanitize=address
CXXFLAGS = -std=c++14 -Wall -I/usr/include -I/usr/local/include  
LDFLAGS = -lssl -lcrypto -lhiredis  

SRC_DIR = src
SERVER_DIR = server
THREADPOOL_DIR = threadpool
SSL_DIR = ssl_server
CACHE_DIR = cache


SRC_FILES = $(SRC_DIR)/main.cpp $(SERVER_DIR)/server.cpp $(THREADPOOL_DIR)/thread_pool.cpp $(SSL_DIR)/ssl_server.cpp $(CACHE_DIR)/cache.cpp
OBJ_FILES = $(SRC_FILES:.cpp=.o)


TARGET = my_server


$(TARGET): $(OBJ_FILES)
	$(CXX) -o $@ $(OBJ_FILES) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ_FILES)


rebuild: clean $(TARGET)

.PHONY: clean install rebuild
