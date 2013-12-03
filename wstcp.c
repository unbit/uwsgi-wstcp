#include <uwsgi.h>

// access uwsgi configuration
extern struct uwsgi_server uwsgi;

int uwsgi_wstcp(struct wsgi_request *wsgi_req) {
	if (uwsgi_parse_vars(wsgi_req)) return -1;

	uint16_t ws_key_len = 0;
	uint16_t ws_origin_len = 0;

	char *ws_key = uwsgi_get_var(wsgi_req, "HTTP_SEC_WEBSOCKET_KEY", 22, &ws_key_len);
	if (!ws_key) return -1;

	char *ws_origin = uwsgi_get_var(wsgi_req, "HTTP_ORIGIN", 11, &ws_origin_len);
	if (!ws_origin) return -1;

	// handshake the websocket connection
	if (uwsgi_websocket_handshake(wsgi_req, ws_key, ws_key_len, ws_origin, ws_origin_len)) return -1;

	// async connect to the tcp server
	int fd = uwsgi_connect("127.0.0.1:9090", 0, 1);
	if (fd < 0) return -1;

	// wait for connection
	int ret = uwsgi.wait_write_hook(fd, 30);
	if (ret <= 0) {
		close(fd);
		return -1;
	}

	// ok we are connected, enter the main loop
	for(;;) {
		int ready_fd = -1;
		// wait for both websockets and tcp
		int ret = uwsgi.wait_read2_hook(wsgi_req->fd, fd, 30, &ready_fd);
		if (ret <= 0) break;
		// websocket message
		if (ready_fd == wsgi_req->fd) {
			// read websocket message
			struct uwsgi_buffer *ub = uwsgi_websocket_recv_nb(wsgi_req);
			if (!ub) break;
			// send to tcp
			if (uwsgi_write_true_nb(fd, ub->buf, ub->pos, 30)) break;
		}
		// packet from tcp
		else if (ready_fd == fd) {
			char buf[8192];
			// read from the tcp socket
			ssize_t rlen = uwsgi_read_true_nb(fd, buf, 8192, 30);
			if (rlen <= 0) break;
			// send to the websocket
			if (uwsgi_websocket_send_binary(wsgi_req, buf, rlen)) break;
		}
	}

	// end of the session
	close(fd);
	return UWSGI_OK;
}


struct uwsgi_plugin wstcp_plugin = {
	.name = "wstcp",
};
