#include "Server.h"
#include <sys/wait.h>

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
		std::cout << "child " << pid << " terminated" << std::endl;
	}
	return;
}

int main(int argc, char *argv[]) {
	pid_t childpid;
	int connfd;
	struct sockaddr_in client_addr;
	constexpr uint16_t PORT_NUMBER = 34543;	//	сервер даты и времени
	constexpr SocketType SOCKET_TYPE = SocketType::SOCK_STREAM;	//	потоковый сокет
	
	Server server({"127.0.0.1", PORT_NUMBER}, SOCKET_TYPE);
	server.SignalInit(SIGCHLD, SigChild);

	std::cout << "Server start work" << std::endl;
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
		std::pair<std::string, int> p = server.GetClientId(client_addr);
		std::cout << "new client connected from <" << p.first << ", " << p.second << ">" << std::endl;

		//	запускается дочерний процесс
		if ((childpid = fork()) == 0) {
			server.Close(server.GetListenFd());	//	дочерний пр-сс закрывает прослушивамый сокет сервера

			/**
			 * Выполняем требуемые д-я.
			 * В данном случае отвечаем зеркалировнием на запрос клиента.
			 */
			server.StrEcho(connfd);	// process the request

			std::cout << "Client from <" << p.first << ", " << p.second << ">" << " disconnected!" << std::endl;
			exit(0);	//	завершение дочернего процесса с закрытием всех его дескрипторов
		} else if (childpid == -1) {
			perror("Fork error");
			return EXIT_FAILURE;
		}

		//	Закрываем соединение с клиентом
		server.Close(connfd);
	}

	return EXIT_SUCCESS;
}
