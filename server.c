#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h> 
#include<sys/stat.h>
#include<sys/types.h>
#include <errno.h>
#include <time.h>
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
    if(n_send(conn, &header, sizeof(header))==-1)
             return -1;

    if(dlength>0)
        if (n_send(conn, data, dlength)==-1) return -1;
 
    return 0;
}


void clean_for_login(char *rez, char* start)
{ const char *ptr=start;
    int k=0, inside=0;
    while(*ptr){
           if(*ptr=='"')
           { inside=!inside;  }
           else if (inside)
              rez[k++]=*ptr;
          ptr++;
    }

   rez[k]='\0';

}

int verify_user(const char *client_user, const char *client_pass){
  FILE *f=fopen("users.json","r");
  if(!f) {perror("Eroare la deschiderea users.json"); return 0;}
  
  char user[100], pass[100],line[500];

  int found_user=0;

  while (fgets(line,sizeof(line),f))
{  char *user_ptr=strstr(line,"\"user\"");
    if (user_ptr) {
        char *val_ptr = strchr(user_ptr, ':');
        if (val_ptr) {
            clean_for_login(user, val_ptr);
            if (strcmp(user, client_user) == 0) {
                found_user = 1;
            }
        }
    }

    char *pass_ptr = strstr(line, "\"password\"");
        if (pass_ptr && found_user) {

            char *val_ptr = strchr(pass_ptr, ':');
            if (val_ptr) {
                clean_for_login(pass, val_ptr);
                if (strcmp(pass, client_pass) == 0) {
                    fclose(f);
                    return 1; 
                }
            }
            
            found_user = 0; 
        }

        
        if (strstr(line, "}")) {
            found_user = 0;
        }
    }

    fclose(f);
    return 0; 
}

void user_dir(const char*nume){
    char path[256];
    if(mkdir("users_dir",0700)<0&& errno!=EEXIST)
       perror("Eroare la crearea folderului pentru users");

  
snprintf(path, sizeof(path), "users_dir/%s", nume);
path[strlen(path)]='\0';

    if(mkdir(path,0700)<0&& errno!=EEXIST)
     perror("Eroare la crearea folderului utilizatorului");

     if (chdir(path) < 0) {
        perror("Nu am putut intra in folderul utilizatorului");
        
    } else {
        printf("Folder curent setat la: %s\n", path);
    }

}
int check_quotes(const char *cmd) {
    int count = 0;
    const char *p = cmd;
    while (*p) {
        if (*p == '"') {
            count++;
        }
        p++;
    }
    return (count % 2 == 0);
}

int is_safe(const char *target, const char *home){
    char abs_target[4096], abs_home[4096];
    if(realpath(home,abs_home)==NULL)
      return 0;
    
      realpath(target, abs_target);

      if(strncmp(abs_target,abs_home,strlen(abs_home))==0)
        return 1;

    return 0;
}

int is_blocked_command(const char *cmd) {
    const char *blocked[] = {
        "nano", "vim", "vi", "emacs", "pico",   
        "top", "htop", "btop",               
        "less", "more", "man",                
        "ssh", "telnet", "ftp",                
        "watch", "bash", "sh", "zsh", "python", 
        NULL 
    };

    char program_name[256];
    strncpy(program_name, cmd, sizeof(program_name) - 1);
    program_name[sizeof(program_name) - 1] = '\0';

    char *first_space = strchr(program_name, ' ');
    if (first_space) {
        *first_space = '\0';
    }

    for (int i = 0; blocked[i] != NULL; i++) {
        if (strcmp(program_name, blocked[i]) == 0) {
            return 1; 
        }
    }
    return 0; 
}

char* trim(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
    return str;
}
void executa_cmd(const char *cmd_input, char *buffer, size_t size, const char *home_path) {
    char cmd_copy[2048];
    strncpy(cmd_copy, cmd_input, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    buffer[0] = '\0'; 
    size_t current_len = 0;
    char *cursor = cmd_copy;

    int should_run = 1;      
    int last_status = 1;   

   

    while (cursor && *cursor) {
        
        
        char *p_and  = strstr(cursor, "&&");
        char *p_or   = strstr(cursor, "||");
        char *p_semi = strstr(cursor, ";");
        
        char *next_sep = NULL;
        int sep_type = 0; 

        char *min_pos = cursor + 4096; 

        if (p_and && p_and < min_pos)  { min_pos = p_and;  sep_type = 1; }
        if (p_or  && p_or  < min_pos)  { min_pos = p_or;   sep_type = 2; }
        if (p_semi && p_semi < min_pos){ min_pos = p_semi; sep_type = 3; }

        if (sep_type != 0) {
            next_sep = min_pos;
            *next_sep = '\0'; 
        }

        char *token = trim(cursor);
        
        if (strlen(token) > 0) {
            if (should_run) {
               
                
                int current_success = 1; 

                if (strncmp(token, "cd", 2) == 0 && (token[2] == ' ' || token[2] == '\0')) {
                    char target[1024];
                    char *arg = trim(token + 2);
                    if (strlen(arg) == 0 || strcmp(arg, "~") == 0) strcpy(target, home_path);
                    else strcpy(target, arg);

                    if (chdir(target) == 0) {
                        char cwd[1024];
                        getcwd(cwd, sizeof(cwd));
                        if (is_safe(cwd, home_path)) {
                            char *rel = cwd + strlen(home_path);
                            
                            char msg[512];
                            snprintf(msg, sizeof(msg), "Director schimbat: ~%s\n", rel);
                            if (current_len + strlen(msg) < size - 1) {
                                strcpy(buffer + current_len, msg);
                                current_len += strlen(msg);
                            }
                        } else {
                            chdir(home_path);
                            current_success = 0;
                            char *err = "Access denied\n";
                            if (current_len + strlen(err) < size - 1) {
                                strcpy(buffer + current_len, err);
                                current_len += strlen(err);
                            }
                        }
                    } else {
                        current_success = 0;
                        char err[256];
                        snprintf(err, sizeof(err), "Eroare cd: %s\n", strerror(errno));
                        if (current_len + strlen(err) < size - 1) {
                            strcpy(buffer + current_len, err);
                            current_len += strlen(err);
                        }
                    }
                }
                else if (strcmp(token, "exit") == 0) {
                    char *msg = "Deconectare...\n";
                    if (current_len + strlen(msg) < size - 1) strcpy(buffer + current_len, msg);
                    return;
                }
                else if (strcmp(token, "pwd") == 0) {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        if (strstr(cwd, home_path) == cwd) {
                            char *relative = cwd + strlen(home_path);
                            if (*relative == '\0') relative = "/";
                            char msg[1024];
                            snprintf(msg, sizeof(msg), "%s\n", relative);
                            if (current_len + strlen(msg) < size - 1) {
                                strcpy(buffer + current_len, msg);
                                current_len += strlen(msg);
                            }
                        } else {
                            snprintf(buffer + current_len, size - current_len, "%s\n", cwd);
                            current_len += strlen(cwd) + 1;
                        }
                    } else {
                        current_success = 0;
                    }
                }
                else {
                    if (strstr(token, "sudo")) {
                        char *err = "Comanda nepermisa: sudo\n";
                        if (current_len + strlen(err) < size - 1) {
                            strcpy(buffer + current_len, err);
                            current_len += strlen(err);
                        }
                        current_success = 0;
                    }
                    else if (is_blocked_command(token)) {
                        char err[128];
                        snprintf(err, sizeof(err), "Comanda nepermisa (interactiva): %s\n", token);
                        if (current_len + strlen(err) < size - 1) {
                            strcpy(buffer + current_len, err);
                            current_len += strlen(err);
                        }
                        current_success = 0;
                    }
                    else {
                        char full_cmd[2048];
                        snprintf(full_cmd, sizeof(full_cmd), "(%s) 2>&1", token);
                        
                        FILE *fp = popen(full_cmd, "r");
                        if (fp) {
                            if (size > current_len + 1) {
                                size_t bytes_read = fread(buffer + current_len, 1, size - current_len - 1, fp);
                                current_len += bytes_read;
                                buffer[current_len] = '\0';
                            }
                            int status = pclose(fp);
                            if (status != 0) {
                                current_success = 0;
                              
                            }
                        } else {
                            current_success = 0;
                        }
                    }
                }

                last_status = current_success;

            } else {
               
            }
        }

        
        if (sep_type == 0) { 
            cursor = NULL; 
        } else {
            
            if (sep_type == 3) {
                should_run = 1; 
            } 
            else if (sep_type == 1) { 
                should_run = (last_status == 1);
            } 
            else if (sep_type == 2) { 
                should_run = (last_status == 0);
            }

            if (sep_type == 3) cursor = next_sep + 1;     
            else               cursor = next_sep + 2;     
        }
    }

    if (current_len == 0) {
        strcpy(buffer, "(fara output)\n");
    }
}

void log_activity(const char *user, const char *command, const char * path){
     char filename[1024];
     snprintf(filename, sizeof(filename), "%s/log.json", path);
    FILE *f = fopen(filename, "r+");
    if (!f) {
        f = fopen(filename, "w");
        if (!f) return;
        fprintf(f, "[]");
        fclose(f);
        f = fopen(filename, "r+"); 
    }
    time_t now=time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fseek(f,0,SEEK_END);
    long fsize = ftell(f);

    if (fsize > 2) {
        fseek(f, -1, SEEK_END);
        
        fprintf(f, ",\n");
       
    } else {
        fseek(f, -1, SEEK_END); 
    }
    char cmd[1024];
    strncpy(cmd, command, sizeof(cmd)-1);
    cmd[sizeof(cmd)-1] = '\0';
    char *newline = strchr(cmd, '\n');
    if(newline) *newline = '\0';

    fprintf(f, "  {\n");
    fprintf(f, "    \"timestamp\": \"%s\",\n", timestamp);
    fprintf(f, "    \"user\": \"%s\",\n", user);
    fprintf(f, "    \"command\": \"%s\"\n", cmd);
    fprintf(f, "  }]"); 

    fclose(f);

}

void handle_client(int client_sock){

    connection conn;
    if(n_init(&conn, client_sock,1)<0){
        close(client_sock);
        return;
    }
    struct PackHeader header;
     int logged=0;
     char user[100] = "";
     char home_path[2048]="";
     char server_root[1024];
    if (getcwd(server_root, sizeof(server_root)) == NULL) {
        perror("Eroare la citirea server_root");
        strcpy(server_root, "."); 
    }

    while(1){

        int primit=n_recv(&conn, &header, sizeof(header));

        if(primit!=sizeof(header)) 
        {printf("Eroare header sau client deconectat\n");
            break;
        }

        uint32_t payload_len=ntohl(header.length);

        char *payload= NULL;
        if(payload_len>0){
         payload=malloc(payload_len+1);
         if(!payload) break;
         if(n_recv(&conn, payload, payload_len)!=payload_len){ free(payload); break;}
         payload[payload_len]='\0';}

        switch(header.type){
            case MSG_AUTH_REQ:
            {
                if(payload==NULL) {send_packet(&conn,MSG_AUTH_RESP, "Lipsesc datele de autentificare");
                break;}

                char *sep = strchr(payload, ':');
            
                if (sep) {
                    
                    *sep= '\0'; 
                    
                    char *temp_user = payload;       
                    char *temp_pass = sep + 1;   
            
                    if (verify_user(temp_user, temp_pass)) {
                        strcpy(user,temp_user);
                        logged = 1; 
                        user_dir(user);
                        snprintf(home_path, sizeof(home_path), "%s/users_dir/%s",server_root, user);
                        send_packet(&conn, MSG_AUTH_RESP, "Autentificare reusita!");
                    } else { 
                       
                        send_packet(&conn, MSG_AUTH_RESP, "User sau parola gresite!");
                    }
                } 
            
            break;
            }
    
            case MSG_CMD:
            {char msg[2048];
                if (payload == NULL || payload_len == 0) {
                    send_packet(&conn, MSG_ERROR, "Ati trimis o comanda vida. Scrieti ceva!");
               }
                else if(logged)
                { log_activity(user,payload, server_root);
                    executa_cmd(payload,msg,sizeof(msg),home_path);
                    if(strstr(msg,"Eroare"))
                    send_packet(&conn, MSG_ERROR, msg);
                    else send_packet(&conn, MSG_CMD_RESP, msg);
                }
                else send_packet(&conn, MSG_ERROR,"NU puteti trimite aceasta comanda fara sa fiti autentificat\n");
                break;}
            default:
            printf("Tip mesaj necunoscut: %d\n", header.type);
            send_packet(&conn, MSG_ERROR, "Tip de mesaj necunoscut/neimplementat.");
        }

        if(payload) free(payload);
    
    }
    n_close(&conn);
}

int main(){
   int s_fd, c_sock;
   struct sockaddr_in address;
   socklen_t length_address=sizeof(address);

   s_fd=socket(AF_INET, SOCK_STREAM,0);
   signal(SIGCHLD, SIG_IGN);
   
   int option=1;

   setsockopt(s_fd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(PORT);
   if (bind(s_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    return 1;
    }

   if (listen(s_fd, 6) < 0) {
    perror("Listen failed");
    return 1;
    }
   printf("Server pornit pe port %d\n", PORT);

   while(1){
    c_sock=accept(s_fd,(struct sockaddr *)&address, &length_address);
    if(fork()==0){
        close(s_fd);
        signal(SIGCHLD, SIG_DFL);
        handle_client(c_sock);
        exit(0);
    }
    close(c_sock);
   }

   return 0;

}