all: proxy admin stripmime

proxy: Proxy/proxy.c Proxy/configuration.c Proxy/shellArgsParser.c Proxy/pop3parser.c Proxy/configParser.c
	gcc -o pop3filter Proxy/proxy.c Proxy/configuration.c Proxy/pop3parser.c Proxy/shellArgsParser.c Proxy/configParser.c -pedantic -Wall

admin: Admin/pop3filterctl.c
	gcc -o pop3filterctl Admin/pop3filterctl.c -pedantic -Wall

stripmime: Stripmime/stripmime.c Stripmime/headerAndBodyUtils.c Stripmime/stack.c
	gcc -o stripmime Stripmime/stripmime.c Stripmime/stack.c Stripmime/headerAndBodyUtils.c -Wall -pedantic

clean:
	rm pop3filter pop3filterctl stripmime
