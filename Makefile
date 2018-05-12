all:
	gcc -c src/dropboxUtil.c -I include -o bin/dropboxUtil
	gcc src/dropboxClient.c bin/dropboxUtil -I include -o bin/dropboxClient
	gcc src/dropboxServer.c bin/dropboxUtil -I include -o bin/server/dropboxServer