#include "network.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#define CERT_FILE "server.crt"
#define KEY_FILE "server.key"

int n_init(connection *conn, int socket, int is_serv){
  int ret;
    conn->sockfd=socket;

    ret=gnutls_certificate_allocate_credentials(&conn->creds);
    if(ret<0)
    {fprintf(stderr,"Eroare: %s\n",gnutls_strerror(ret));
    return -1;}

    if(is_serv){
      ret=gnutls_certificate_set_x509_key_file(conn->creds,CERT_FILE,KEY_FILE,GNUTLS_X509_FMT_PEM);
      if(ret<0)
      {fprintf(stderr,"Eroare incarcare certificate:%s\n", gnutls_strerror(ret));
        return -1;

      }
    }

    unsigned flag;
    if (is_serv)
        flag=GNUTLS_SERVER;
        else flag=GNUTLS_CLIENT;
    
    ret=gnutls_init(&conn->session, flag);
    if(ret<0) 
    return -1;
    
    gnutls_priority_set_direct(conn->session, "NORMAL", NULL);

    gnutls_credentials_set(conn->session, GNUTLS_CRD_CERTIFICATE,conn->creds);

    gnutls_transport_set_int(conn->session, conn->sockfd);

    do {
      ret = gnutls_handshake(conn->session);
  } while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

  if (ret < 0) {
      fprintf(stderr, "Handshake esuat: %s\n", gnutls_strerror(ret));
      return -1;
  }
  return 0;

}

int n_send(connection *conn, const void *buffer, size_t length){
    const char *ptr=(const char *)buffer;
    size_t total_sent=0;
    
    while (total_sent<length){
      size_t bytes_left=length-total_sent;

    
      ssize_t sent = gnutls_record_send(conn->session, ptr + total_sent, bytes_left);

      if (sent < 0) {
          
          if (sent == GNUTLS_E_INTERRUPTED || sent == GNUTLS_E_AGAIN) {
              continue; 
          }
          fprintf(stderr, "Eroare la gnutls_send: %s\n", gnutls_strerror(sent));
          return -1;
      }

      if (sent<0){
        perror("Eroare la trimitere");
        return -1;
      }

      total_sent+=sent;
    }
    return total_sent;
}

int n_recv(connection *conn,  void *buffer, size_t length){
    char *ptr=(char*) buffer;
   size_t total_read=0;

   while(total_read<length){
    size_t bytes_left=length-total_read;
    ssize_t ret = gnutls_record_recv(conn->session, ptr + total_read, bytes_left);

        if(ret < 0){ 
            if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
                continue;
            }
            fprintf(stderr, "Eroare la gnutls_recv: %s\n", gnutls_strerror(ret));
            return -1;
        }
        
     
 if(ret==0) return total_read;
   total_read+=ret;  }

   return total_read;



}

void n_close( connection *conn){

if(conn->session){
  gnutls_bye(conn->session,GNUTLS_SHUT_RDWR);
  gnutls_deinit(conn->session);

}
if(conn->creds) {
  gnutls_certificate_free_credentials(conn->creds);
}

  if(conn->sockfd>0)
  {close(conn->sockfd);

  conn->sockfd=-1;}
}