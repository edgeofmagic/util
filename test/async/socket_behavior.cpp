#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <doctest.h>

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128

uv_loop_t *loop;
struct sockaddr_in addr;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t* handle) {
    free(handle);
}

void on_server_close( uv_handle_t* shandle )
{
    fprintf(stderr, "%s\n", "in server close callback" );	
	int refc = uv_has_ref( shandle );
	if ( refc )
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_has_ref is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_has_ref is false");
	}

	int actv = uv_is_active( shandle );
	if ( actv )
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_is_active is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_is_active is false");
	}

	int closing = uv_is_closing( shandle );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "in server close callback, uv_is_closing is false");
	}
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, nread);
        uv_write((uv_write_t*) req, client, &req->buf, 1, echo_write);
        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) client, on_close);
    }
}

// int main() 
TEST_CASE( "socket/behaivor/smoke")
{
    loop = uv_default_loop();

	int refc;
	int closing;
	int actv;

    uv_tcp_t server;
	
    uv_tcp_init(loop, &server);

	refc = uv_has_ref( (uv_handle_t*) &server );
	if ( refc )
	{
        fprintf(stderr, "%s\n", "after init, uv_has_ref is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after init, uv_has_ref is false");
	}

	actv = uv_is_active( (uv_handle_t*) &server );
	if ( actv )
	{
        fprintf(stderr, "%s\n", "after init, uv_is_active is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after init, uv_is_active is false");
	}


	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after init, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after init, uv_is_closing is false");
	}

/*
	uv_close( (uv_handle_t*) &server, on_server_close );

	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after init, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after init, uv_is_closing is false");
	}
*/

/*
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

	refc = uv_has_ref( (uv_handle_t*) &server );
	if ( refc )
	{
        fprintf(stderr, "%s\n", "after bind, uv_has_ref is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after bind, uv_has_ref is false");
	}

	actv = uv_is_active( (uv_handle_t*) &server );
	if ( actv )
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_active is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_active is false");
	}
*/
/*
	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_closing is false");
	}

	uv_close( (uv_handle_t*) &server, on_server_close );

	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after close, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after close, uv_is_closing is false");
	}
*/

/*
    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        // return 1;
    }

	refc = uv_has_ref( (uv_handle_t*) &server );

	if ( refc )
	{
        fprintf(stderr, "%s\n", "after listen, uv_has_ref is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after listen, uv_has_ref is false");
	}

	actv = uv_is_active( (uv_handle_t*) &server );
	if ( actv )
	{
        fprintf(stderr, "%s\n", "after listen, uv_is_active is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after listen, uv_is_active is false");
	}

	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after listen, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after listen, uv_is_closing is false");
	}

	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after bind, uv_is_closing is false");
	}

	uv_close( (uv_handle_t*) &server, on_server_close );

	closing = uv_is_closing( (uv_handle_t*) &server );
	if ( closing )
	{
        fprintf(stderr, "%s\n", "after close, uv_is_closing is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after close, uv_is_closing is false");
	}

	actv = uv_is_active( (uv_handle_t*) &server );
	if ( actv )
	{
        fprintf(stderr, "%s\n", "after close, uv_is_active is true");
	}
	else
	{
        fprintf(stderr, "%s\n", "after close, uv_is_active is false");
	}
*/
    // return uv_run(loop, UV_RUN_DEFAULT);
	uv_run(loop, UV_RUN_DEFAULT);
}
