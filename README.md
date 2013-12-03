uwsgi-wstcp
===========

uWSGI plugin mapping websockets to tcp sockets

- WORK IN PROGRESS -

Usage
-----

build it (requires uWSGI 1.9.21):

```sh
uwsgi --build-plugin <directory_of_uwsgi_wstcp>
```


run it (async mode):

```sh
uwsgi --http-socket :9191 --plugin wstcp --symcall uwsgi_wstcp --http-socket-modifier1 18 --async 10 --ugreen
```
