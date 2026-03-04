#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include<gnutls/gnutls.h>
#include<gnutls/x509.h>

typedef struct{
    int sockfd;
    gnutls_session_t session;
    gnutls_certificate_credentials_t creds;
} connection;


int n_init(connection *conn, int socket, int is_serv);


int n_send(connection *conn, const void *buffer, size_t length);


int n_recv(connection *conn, void *buffer, size_t length);

void n_close(connection *conn);

#endif