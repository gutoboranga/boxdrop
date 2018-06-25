all:
	gcc -c src/dropboxUtil.c -I include -o bin/dropboxUtil
	gcc src/dropboxClient.c bin/dropboxUtil -I include -o bin/dropboxClient
	gcc src/dropboxServer.c bin/dropboxUtil -I include -o bin/server/dropboxServer
	
leader_test:
	gcc -c src/dropboxUtil.c -I include -o bin/dropboxUtil
	gcc -c src/list.c -I include -o bin/list
	gcc src/leader.c bin/dropboxUtil bin/list -I include -o leader