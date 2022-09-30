ALL:httpd client http_epoll

http:
	gcc httpd.c -o httpd -W -g -pthread

client:
	gcc client.c -o client -W -g

http_epoll:
	gcc http_epoll.c -o http_epoll -W -g

clean:
	rm -rf httpd client http_epoll

run:
	./httpd 8000 sources/

run_epoll:
	./http_epoll 8000 sources/

run_client:
	./client 8000

.PHONY:
	ALL clean
