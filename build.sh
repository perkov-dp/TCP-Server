rm -R ./build
mkdir ./build && cd ./build

DELIVERY_APP=bin/tcp_server

cmake .. -DCMAKE_BUILD_TYPE=Debug && \
	cmake --build .

# Запишем команду запуска программы в bash-скрипт и в дальнейшем будем вызывать его
echo $DELIVERY_APP > ./run.sh
# Делаем его исполняемым (пользователь и группа обладают полными правами, а остальные могут только читать)
chmod u+x ./run.sh
# Запускаем программу
./run.sh
