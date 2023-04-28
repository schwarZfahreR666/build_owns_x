server_epoll: 06_server_epoll.cpp ./thread_pool/thread_pool.c ./thread_pool/queue_array.c
	g++ -Wall -Wextra -O2 -g 06_server_epoll.cpp ./thread_pool/thread_pool.c ./thread_pool/queue_array.c  -o server_epoll -lpthread

client: 06_client.cpp
	g++ -Wall -Wextra -O2 -g 06_client.cpp -o client

all: server_epoll client