BIN_dir = binary
INC_dir = include
OBJ_dir = object
SRC_dir = source
LIB_dir = lib

CC = gcc
CFLAGS = -Wall -I $(INC_dir)


SERVER_OBJECTS = htmessages.pb-c.o message.o zk_utils.o server_network.o client_network.o client_stub.o server_skeleton.o server_hashtable.o
CLIENT_OBJECTS = htmessages.pb-c.o message.o zk_utils.o client_network.o client_stub.o client_hashtable.o
LIB_OBJECTS = $(OBJ_dir)/table.o $(OBJ_dir)/block.o $(OBJ_dir)/entry.o $(OBJ_dir)/list.o

all: $(BIN_dir) $(OBJ_dir) libtable server_hashtable client_hashtable

libtable: $(LIB_OBJECTS) $(BIN_dir)
	ar -rcs $(LIB_dir)/libtable.a $(LIB_OBJECTS)

server_hashtable: $(addprefix $(OBJ_dir)/, $(SERVER_OBJECTS)) $(LIB_dir)/libtable.a | $(BIN_dir)
	$(CC) $^ -D THREADED -lzookeeper_mt -lprotobuf-c -o $(BIN_dir)/$@

client_hashtable: $(addprefix $(OBJ_dir)/, $(CLIENT_OBJECTS)) $(LIB_dir)/libtable.a | $(BIN_dir)
	$(CC) $^ -D THREADED -lzookeeper_mt -lprotobuf-c -o $(BIN_dir)/$@

$(BIN_dir):
	mkdir -p $(BIN_dir)

$(OBJ_dir):
	mkdir -p $(OBJ_dir)

$(OBJ_dir)/%.o: $(SRC_dir)/%.c | $(OBJ_dir)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_dir)/htmessages.pb-c.c: htmessages.proto
	protoc-c --c_out=. htmessages.proto
	mv htmessages.pb-c.c $(SRC_dir)/
	mv htmessages.pb-c.h $(INC_dir)/

clean: 
	rm -f $(OBJ_dir)/htmessages.pb-c.o $(OBJ_dir)/message.o $(OBJ_dir)/server_network.o $(OBJ_dir)/server_skeleton.o  $(OBJ_dir)/server_hashtable.o $(OBJ_dir)/client_network.o $(OBJ_dir)/client_stub.o $(OBJ_dir)/client_hashtable.o $(OBJ_dir)/zk_utils.o
	rm -f $(BIN_dir)/*
	rm -f $(LIB_dir)/*
	@echo "A remover os ficheiros objeto e os ficheiros executáveis"
