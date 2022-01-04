#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include <time.h>
#include "Server.h"

int main(int argc, char *argv[]) {
	pid_t childpid;
	int connfd;
	struct sockaddr_in client_addr;
	const uint16_t PORT_NUMBER = 34543;	//	сервер даты и времени
	Server server(PORT_NUMBER);

	/**
	 * Сервер обрабатывает т/один запрос за раз,
	 * т.е. это последовательный сервер.
	 */
	for(;;) {
		//	Соединение с клиентом
		connfd = server.Accept(client_addr);
		pair <string, int> p = server.GetClientId(client_addr);
		cout << "new client connected from <" << p.first << ", " << p.second << ">" << endl;

		//	запускается дочерний процесс
		if ((childpid = fork()) == 0) {
			server.Close(server.GetListenFd());	//	дочерний пр-сс закрывает прослушивамый сокет сервера

			/**
			 * Выполняем требуемые д-я.
			 * В данном случае отвечаем зеркалировнием на запрос клиента.
			 */
			char buf[128];
			int n = server.Readn(connfd, buf, 15);
			cout << "Read: " << buf << endl;
			server.Writen(connfd, buf, n-1);
			cout << "Write: " << buf << endl;
			//server.str_echo(connfd);	// process the request

			cout << "Cliend disconnected!" << endl;
			exit(0);	//	завершение дочернего процесса с закрытием всех его дескрипторов
		} else if (childpid == -1) {
			perror("Fork error");
			return EXIT_FAILURE;
		}

		//	Закрываем соединение с клиентом
		server.Close(connfd);
	}

	std::cout << "Welcome to the QNX Momentics IDE" << std::endl;
	return EXIT_SUCCESS;
}
