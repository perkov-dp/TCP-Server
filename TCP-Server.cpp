#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include <time.h>
#include "Server.h"

int main(int argc, char *argv[]) {
	int connfd;
	char buff[128];
	time_t ticks;
	const uint16_t PORT_NUMBER = 34543;	//	сервер даты и времени
	Server server(PORT_NUMBER);

	/**
	 * Сервер обрабатывает т/один запрос за раз,
	 * т.е. это последовательный сервер.
	 */
	for(;;) {
		//	Соединение с клиентом
		connfd = server.Accept();

		ticks = time(NULL);
		bzero(buff, 0);
		//	проверяет переполнение буфера
		snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
		string str = buff;
		server.Writen(connfd, str.c_str(), str.size());
		cout << str << endl;

		//	Закрываем соединение с клиентом
		server.Close(connfd);
	}

	std::cout << "Welcome to the QNX Momentics IDE" << std::endl;
	return EXIT_SUCCESS;
}
