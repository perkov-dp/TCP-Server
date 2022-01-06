#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include <time.h>
#include "Server.h"

/**
 * Обработка сигналов от процессов.
 * Сигнал SIGCHLD - для ожидания завершения зомби-процесса
 */
void SigChild(int signo) {
	pid_t pid;	//	идентификатор завершенного пр-сса
	int stat = 0;	//	статус завершения дочеренего пр-сса
	//	Ф-я waitpid возвращает идентификатор завершенного пр-сса
	//	1-й пар-р говорит о том, что н/дождаться завершения первого дочернего пр-сса
	//	2-й пар-р возвращает статус завершения дочеренего пр-сса
	//	статус мб следующим:
	//	- пр-сс завершен нормально
	//	- уничтожен сигналом
	//	- только приостановлен программой управления заданиями
	//	3-й пар-р сообщает ядру, что не нужно вып-ть блокирование, если нет завершенных дочерних пр-ссов
	//	Ф-я waitpid с флагом WNOHANG ставит сигналы в очередь и обрабатывает их по порядку.
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated\n", pid);
	}
	return;
}

int main(int argc, char *argv[]) {
	pid_t childpid;
	int connfd;
	struct sockaddr_in client_addr;
	const uint16_t PORT_NUMBER = 34543;	//	сервер даты и времени
	Server server(PORT_NUMBER);
	server.SignalInit(SIGCHLD, SigChild);

	/**
	 * Сервер обрабатывает т/один запрос за раз,
	 * т.е. это последовательный сервер.
	 */
	for(;;) {
		//	Соединение с клиентом
		connfd = server.Accept(client_addr);
		//	Во время вып-я Accept пришел сигнал и прервал ее выполнение -> перезапускаем Accept
		if (connfd == -1) {
			continue;
		}
		pair <string, int> p = server.GetClientId(client_addr);
		cout << "new client connected from <" << p.first << ", " << p.second << ">" << endl;

		//	запускается дочерний процесс
		if ((childpid = fork()) == 0) {
			server.Close(server.GetListenFd());	//	дочерний пр-сс закрывает прослушивамый сокет сервера

			/**
			 * Выполняем требуемые д-я.
			 * В данном случае отвечаем зеркалировнием на запрос клиента.
			 */
			server.str_echo(connfd);	// process the request

			cout << "Client from <" << p.first << ", " << p.second << ">" << " disconnected!" << endl;
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
