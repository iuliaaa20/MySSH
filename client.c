#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "network.h"
#include "msg.h"

#define PORT 8000

int send_packet(connection *conn, uint8_t type, const char *data){


    struct PackHeader header;
    uint32_t dlength;
    if(data!=NULL)
      dlength=strlen(data);
    else dlength=0;

    header.type=type;
    header.length=htonl(dlength);

    if(n_send(conn,&header,sizeof(header))<0) 
         return -1;
    
    if(dlength>0)
      if(n_send(conn,data,dlength)<0)
           return -1;


    return 0;
}

int main(int argc, char*argv[]){
    int sock=0;
    struct sockaddr_in serv_addr;
    char cmd_buffer[1024];

    if((sock=socket(AF_INET, SOCK_STREAM,0))<0){
       perror("eroare la crearea socket-ului");
       return -1;

    }
   
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);

    const char *ip_addr=(argc>1)? argv[1]: "127.0.0.1";

    if(inet_pton(AF_INET,ip_addr,&serv_addr.sin_addr)<=0){
        perror("Invalid adress");
        return -1;
    }

    if(connect(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
{
    perror("Nu s-a putut realiza conexiunea");
    return -1;
}

connection conn; 

if (n_init(&conn, sock, 0) < 0) {
    printf("Eroare la stabilirea conexiunii securizate.\n");
    return -1;
}


printf("Conectat la server (Port %d)\n", PORT);

int logged=0,maxi=3;

    char user[50];
while (!logged&&maxi>0) {

    char pass[50];
    char buffer[128];

    printf("Username: ");
    scanf("%49s", user);
    
    printf("Password: ");
    scanf("%49s", pass);

    getchar(); 

    
    snprintf(buffer, sizeof(buffer), "%s:%s", user, pass);

    if (send_packet(&conn, MSG_AUTH_REQ, buffer) < 0) {
        printf("Eroare la trimitere. Server deconectat.\n");
        return -1;
    }

    struct PackHeader header;
    if (n_recv(&conn, &header, sizeof(header)) != sizeof(header)) {
        printf("Serverul a inchis conexiunea.\n");
        return -1;
    }

    uint32_t len = ntohl(header.length);
    char *rasp = malloc(len + 1);
    n_recv(&conn, rasp, len);
    rasp[len] = '\0';

    printf("%s\n", rasp);
    if (strstr(rasp, "reusita") != NULL) {
        logged = 1; 
    } else { maxi--;

        if(maxi) printf(">> Mai incearca o data.\n");
        else printf("Ati atins numarul maxim de incercari. Deconectare de la server\n");
    }
    
    free(rasp);
}


if(maxi){

struct PackHeader header;
char current_path[256]="~";
char prompt[1024];

 while(1){
    snprintf(prompt,sizeof(prompt),"\033[1;32m%s@myssh\033[0m:\033[1;34m%s\033[0m$ ", user, current_path);
    
    char *input=readline(prompt);
    if(!input)
     break;

     if(strlen(input)>0)
     add_history(input);   

    strncpy(cmd_buffer,input,sizeof(cmd_buffer)-1);
    cmd_buffer[sizeof(cmd_buffer) - 1] = '\0';

    size_t len=strlen(cmd_buffer);

    if(len>0 && cmd_buffer[len-1]=='\n')
       cmd_buffer[len-1]='\0';

   free(input);

      send_packet(&conn, MSG_CMD,cmd_buffer);
  
    if(n_recv(&conn,&header,sizeof(header))!=sizeof(header)){
        printf("Serverul a inchis conexiunea\n");
        break;
    }

    uint32_t lungime=ntohl(header.length);
    char *rasp=malloc(lungime+1);
    n_recv(&conn, rasp,lungime);
    rasp[lungime]='\0';
    char *semnatura = "Director schimbat: ";
    char *locatie = NULL;
    char *cautare = strstr(rasp, semnatura); 

    while (cautare != NULL) {
        locatie = cautare; 
        
        cautare = strstr(locatie + 1, semnatura); 
    }
    if (locatie != NULL) {
        
        
        
        
        char *path_start = locatie + 19; 

        
        char *path_end = strchr(path_start, '\n');

        if (path_end != NULL) {
            
            char temp_save = *path_end; 
            *path_end = '\0';           
            
            strcpy(current_path, path_start); 
            
            *path_end = temp_save;      
        } else {
            
            strcpy(current_path, path_start);
        }
    }

    
    printf("%s", rasp);
    
    
    
    if (lungime > 0 && rasp[lungime-1] != '\n') {
        printf("\n");
    }
     free(rasp);
  
       if(strcmp(cmd_buffer,"exit")==0)
      break;

 }

   n_close(&conn);
        

}
    return 0;
}
