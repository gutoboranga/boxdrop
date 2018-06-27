all: clean compile
	
compile:
	@echo "> Compilando"
	@gcc -c src/dropboxUtil.c -I include -o bin/dropboxUtil
	@gcc -c src/dropboxServerFront.c -I include -o bin/dropboxServerFront
	@gcc -c src/list.c -I include -o bin/list
	@gcc -c src/backup.c -I include -o bin/backup
	@gcc -pthread src/dropboxServer.c bin/dropboxUtil bin/dropboxServerFront bin/list bin/backup -I include -o server/dropboxServer
	
	@mkdir server2/
	@mkdir server3/
	
	@cp -r server/ server2/
	@cp -r server/ server3/
	
	@gcc src/dropboxClient.c bin/dropboxUtil -I include -o client/dropboxClient
	
	@echo "> Tudo pronto!"

clean:
	@echo "> Limpando"
	@rm -rf bin/*
	@rm -rf server/*
	@rm -rf client/*
	@rm -rf server2/
	@rm -rf server3/
