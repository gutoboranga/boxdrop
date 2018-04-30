all:
	gcc -c src/dropboxUtil.c -I include -o bin/dropboxUtil
	gcc src/dropboxClient.c bin/dropboxUtil -I include -o bin/dropboxClient