ALL:httpd client

httpd:
	gcc httpd.c -o httpd -W -g -pthread

client:
	gcc client.c -o httpd -W -g

clean:
	rm -rf httpd client

run:
	./httpd 8000 sources/

.PHONY:
	ALL clean
