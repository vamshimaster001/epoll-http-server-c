#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

#define PORT 9999
#define BUFFER_SIZE 2048

struct client
{
  int fd;
  
  char recv_buffer[256];
  size_t recv_bufflen;
  
  char header_buffer[512];
  size_t header_total_len;
  size_t header_sent_len;

  FILE *fp;
  char send_buffer[BUFFER_SIZE];
  size_t file_total_len;
  size_t file_sent_len;
  
  int receive;

  size_t chunk_len;
  size_t chunk_sent;
  char connection_type[20];
};

void logging(char *level, char *msg, ...) 
{
  time_t now = time(NULL);
  
  fprintf(stderr, "%ld %s : ",now,level);
  
  va_list args;
  
  va_start(args,msg);
  
  vfprintf(stderr,msg,args);
  
  va_end(args);
  
  fprintf(stderr,"\n");
}

int set_sock_nonblock(int fd)
{
  int flags = fcntl(fd,F_GETFL, 0);
  if(flags < 0)
    {
      return -1;
    }
  if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) <0)
   {
    return -1;
   }
  return 0;
}

int main()
{
  int server_fd, client_fd;
  struct sockaddr_in server_address, client_address;
  socklen_t client_len = sizeof(client_address);
  int n;
  struct epoll_event mod_event;
    
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0)
    {
      logging("ERROR", strerror(errno));
      return 1;
    }
  logging("INFO", "Server socket is created successfully");

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(9999);
  server_address.sin_family = AF_INET;

  if(set_sock_nonblock(server_fd) == -1)
    {
      close(server_fd);
      logging("ERROR","setting socket to non blocking failed");
      return 1;
    }
  logging("INFO","server socket is set to non blocking succeeded");


  int opt =1;
  setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

  if(bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
      logging("ERROR", strerror(errno));
      close(server_fd);
      return 1;
    }
  logging("INFO", "Server is bind to IP address %s and port %d", inet_ntoa(server_address.sin_addr), PORT);

  

  if(listen(server_fd,5) < 0)
    {
      logging("ERROR", strerror(errno));
      close(server_fd);
      return 1;
    }
  logging("INFO", "SEREVR IS LISTENING");

  int epfd = epoll_create1(0);
  if(epfd < 0)
    {
      logging("ERROR",strerror(errno));
      close(server_fd);
      return 1;
    }
  logging("INFO", "epoll is created");

  struct epoll_event server_event;
  server_event.events = EPOLLIN;
  server_event.data.fd = server_fd;

  if(epoll_ctl(epfd,EPOLL_CTL_ADD,server_fd, &server_event) < 0)
    {
      logging("ERROR",strerror(errno));
      close(server_fd);
      return 1;
    }
  
  struct epoll_event event_all[100];
  
  while(1)
    {
      int nfds = epoll_wait(epfd,event_all, 100, -1);
      for(int i=0 ; i<nfds; i++)
	{
	  if(event_all[i].data.fd == server_fd)
	    {
	      while(1)
		{
		  client_fd = accept(server_fd, (struct sockaddr *)&client_address,&client_len);
	      
		  if(client_fd < 0)
		    {
		      if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
			  break;
			}
		      else
			{
			  logging("ERROR", strerror(errno));
			  continue;
			}
		    }

		  logging("INFO","client with IP address %s connected ",inet_ntoa(client_address.sin_addr));

		  if(set_sock_nonblock(client_fd) < 0)
		    {
		      logging("ERROR", strerror(errno));
		      close(client_fd);
		      continue;
		    }

		  logging("INFO", "Client socket is set to non blocking mode ");

		  struct client *new_client = malloc(sizeof(struct client));
		  if(!new_client)
		    {
		      logging("ERROR", "client malloc failed");
		      close(client_fd);
		      continue;
		    }
		  new_client->fd = client_fd;
		  new_client->recv_bufflen = 0;
		  new_client->recv_buffer[new_client->recv_bufflen] = '\0';
		  new_client->header_total_len = 0;
		  new_client->header_sent_len = 0;
		  new_client->header_buffer[new_client->header_total_len] = '\0';
		  new_client->fp = NULL;
		  new_client->file_sent_len = 0;
		  new_client->file_total_len = 0;
		  new_client->send_buffer[new_client->file_total_len] = '\0';
	          new_client->receive = 1;
		  new_client->chunk_len = 0;
		  new_client->chunk_sent = 0;
		  
		  strcpy(new_client->connection_type ,"close");
		  
		  struct epoll_event client_event;
		  client_event.events = EPOLLIN;
		  client_event.data.ptr = new_client;

		  if(epoll_ctl(epfd,EPOLL_CTL_ADD,client_fd,&client_event) < 0)
		    {
		      free(new_client);
		      logging("ERROR", strerror(errno));
		      close(client_fd);
		      continue;
		    }
		  logging("INFO", "client is successfully added to epoll polling");
		} 
	    }
	  else
	    {
	      struct client *c = event_all[i].data.ptr;
            
	      if(c->receive)
		{
		  if(c->recv_bufflen >= sizeof(c->recv_buffer) - 1)
		    {
		      logging("ERROR","Request too large");
		      epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
		      close(c->fd);
		      free(c);
		      continue;
		    }
		  
		  ssize_t bytes_read = recv(c->fd,c->recv_buffer+c->recv_bufflen,sizeof(c->recv_buffer)-c->recv_bufflen-1,0);

		  if(bytes_read < 0)
		    {
		      if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
			  continue;
			}
		      else
			{
			  epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
			  close(c->fd);
			  free(c);
			  logging("ERROR", "Receive failed , client socket closed");
			  continue;
			}
		    }
		  else      
		    {
		      char httprequest[512];
		      
		      c->recv_bufflen += bytes_read;
		      c->recv_buffer[c->recv_bufflen] = '\0';

		      char *ptr = strstr(c->recv_buffer,"\r\n\r\n");
		      if(ptr == NULL)
			{
			  continue;
			}
		      ptr = ptr+4;
		      int mem_offset = ptr-c->recv_buffer;
		      memcpy(httprequest,c->recv_buffer,mem_offset);
		      httprequest[mem_offset] = '\0';
		      size_t remaining = c->recv_bufflen - mem_offset;
		      memmove(c->recv_buffer, ptr, remaining);
		      c->recv_bufflen = remaining;
		      c->recv_buffer[c->recv_bufflen] = '\0';

		      char *start, *end, first_line[256];
		      start = httprequest;

		      char method[10];
		      char path[100];
		      char version[10];
		      char content_type[50] = "text/html";
		      int ret;
		      		      		      
		      end = strstr(httprequest,"\r\n");
		      {
		        mem_offset = end-start;
			memcpy(first_line,httprequest,mem_offset+2);
			first_line[mem_offset] = '\0';
			ret = sscanf(first_line,"%9s %99s %9s\r\n", method, path, version);
			logging("INFO"," %s Method : %s path : %s version : %s",inet_ntoa(client_address.sin_addr), method, path, version);
		      }
		      if((ret == 3))
			{
			  			  
			  if(strcmp(method,"GET") == 0)
			    {
			      start = strstr(httprequest,"Connection: ");
			      if(start != NULL)
				{
				  end = strstr(start,"\r\n");
				  {
				    mem_offset = end-start;
				    memcpy(first_line,start,mem_offset+2);
				    first_line[mem_offset] = '\0';
				    sscanf(first_line,"Connection: %s\r\n",c->connection_type);
				    logging("INFO","connection type : %s", c->connection_type);
				  }
				}
                      
			      c->receive = 0;
			      mod_event.events = EPOLLOUT;
			      mod_event.data.ptr = c;
			      epoll_ctl(epfd,EPOLL_CTL_MOD,c->fd,&mod_event);
                      
			      logging("INFO", "message from client is %s",httprequest);
                      		      
			      if(strcmp(path,"/") == 0)
				{
				  strcpy(path,"/root/PROJ2/index.html");
				  c->fp = fopen("/root/PROJ2/index.html","r");
				}
			      else if(strcmp(path,"/about")==0)
				{
				  strcpy(path,"/root/PROJ2/about.html");
				  c->fp = fopen("/root/PROJ2/about.html", "r");
				}
			      else if(strcmp(path, "/style.css")==0)
				{
				  strcpy(path,"/root/PROJ2/style.css");
				  c->fp = fopen("/root/PROJ2/style.css","r");
				}
			      else if(strcmp(path, "/app.js")==0)
				{
				  strcpy(path,"/root/PROJ2/app.js");
				  c->fp = fopen("/root/PROJ2/app.js","r");
				}
			      else
				{
				  c->fp = NULL;
				  logging("INFO","%s %s %s 500 Internal Server Error", inet_ntoa(client_address.sin_addr), method, path);
				  snprintf(c->header_buffer,sizeof(c->header_buffer),"HTTP/1.1 500 Internal Server Error\r\n"
                                                                             "Content-Type: text/plain\r\n"
                                                                             "Connection: %s\r\n"
					   "Content-Length: 25\r\n\r\n500 Internal Server Error",c->connection_type);
			  
				  c->header_total_len = strlen(c->header_buffer);
				  path[0] = '\0';   
				}

			      char *filetype = strrchr(path,'.');
		      
			      if(filetype != NULL)
				{
				  if(strcmp(filetype,".html") == 0)
				    {
				      strcpy(content_type,"text/html");
				    }
				  else if(strcmp(filetype,".css") == 0)
				    {
				      strcpy(content_type,"text/css");
				    }
				  else if(strcmp(filetype,".js") == 0)
				    {
				      strcpy(content_type,"application/javascript");
				    }
				  else if(strcmp(filetype,".txt") == 0)
				    {
				      strcpy(content_type,"text/plain");
				    }
				  else
				    {
				      strcpy(content_type,"text/html");
				    }
                                  logging("INFO","%s %s %s 200 OK", inet_ntoa(client_address.sin_addr), method, path);
				  snprintf(c->header_buffer, sizeof(c->header_buffer),
				  "HTTP/1.1 200 OK\r\n"
				  "Content-Type: %s\r\n"
				  "Connection: %s\r\n"				  
				  "Content-Length: ",
				  content_type, c->connection_type);
			  
				  logging("INFO","header : %s", c->header_buffer);
				  logging("INFO","strlen %d",strlen(c->header_buffer));

				  }
			        }
			        else
			        {
				    start = strstr(httprequest,"Connection: ");
				    if(start != NULL)
				    {
				      end = strstr(start,"\r\n");
				      {
					mem_offset = end-start;
					memcpy(first_line,start,mem_offset+2);
					first_line[mem_offset] = '\0';
					sscanf(first_line,"Connection: %s\r\n",c->connection_type);
					logging("INFO","connection type : %s", c->connection_type);
				      }
				    }
			            c->fp = NULL;
				    logging("INFO","%s %s %s 405 Method Not Allowed", inet_ntoa(client_address.sin_addr), method, path);
                                    snprintf(c->header_buffer,sizeof(c->header_buffer),"HTTP/1.1 405 Method Not Allowed\r\n"
                                                                             "Content-Type: text/plain\r\n"
                                                                             "Connection: %s\r\n"
                                           "Content-Length: 22\r\n\r\n405 Method Not Allowed",c->connection_type);

                                    c->header_total_len = strlen(c->header_buffer);
			        }
			  
			     }
			     else
			     {

	                      	start = strstr(httprequest,"Connection: ");
				if(start != NULL)
				  {
				    end = strstr(start,"\r\n");
				    {
				      mem_offset = end-start;
				      memcpy(first_line,start,mem_offset+2);
				      first_line[mem_offset] = '\0';
				      sscanf(first_line,"Connection: %s\r\n",c->connection_type);
				      logging("INFO","connection type : %s", c->connection_type);
				    }
				  }
				  c->fp = NULL;
				  logging("INFO","%s %s %s 400 Bad Request", inet_ntoa(client_address.sin_addr), method, path);
                                  snprintf(c->header_buffer,sizeof(c->header_buffer),"HTTP/1.1 400 Bad Request\r\n"
                                                                             "Content-Type: text/plain\r\n"
                                                                             "Connection: %s\r\n"
                                           "Content-Length: 15\r\n\r\n400 Bad Request",c->connection_type);

                                  c->header_total_len = strlen(c->header_buffer);
			       
			     }
			
			    if(c->fp != NULL)
			      {
			  
				fseek(c->fp,0,SEEK_END);
				c->file_total_len = ftell(c->fp);
				fseek(c->fp,0,SEEK_SET);
				logging("INFO","strlen %d",strlen(c->header_buffer));
				logging("INFO","strl %s",c->header_buffer);
				snprintf(c->header_buffer+strlen(c->header_buffer),sizeof(c->header_buffer)-strlen(c->header_buffer),"%ld\r\n\r\n",c->file_total_len);
				logging("INFO","strlen %d",strlen(c->header_buffer));
				c->header_total_len = strlen(c->header_buffer);
                              }
			}
		   }
	      if(!c->receive)
		{
		      
		      logging("INFO", "header total length %ld ",c->header_total_len);
		      logging("INFO", "header sent length %ld ",c->header_sent_len);
		      logging("INFO", "header buffer is %s", c->header_buffer);
	              logging("INFO", "file total length is %ld", c->file_total_len);
		      logging("INFO", "file sent length is %ld", c->file_sent_len);
		      
		      if(c->header_total_len != c->header_sent_len)
			{
			  
			  n = send(c->fd, c->header_buffer+c->header_sent_len, c->header_total_len-c->header_sent_len,0);
			  logging("INFO"," sent bytes : %d", n);
			  if(n < 0)
			    {
			      if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
				  continue;
				}
			      else
				{
                                  epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
                                  close(c->fd);
                                  free(c);
                                  logging("ERROR", "header send failed , client socket closed");
				  continue;
				}
			      
			    }
			  else
			    {
			      c->header_sent_len += n;
			      if(c->header_total_len != c->header_sent_len)
				continue;
			    }
			}
		      
			if(c->fp == NULL)
			{
			      mod_event.events = EPOLLIN ;
                              mod_event.data.ptr = c;
                              epoll_ctl(epfd,EPOLL_CTL_MOD,c->fd,&mod_event);
                              c->header_total_len = 0;
                              c->header_sent_len = 0;
                              c->header_buffer[c->header_total_len] = '\0';
                              c->file_total_len = 0;
                              c->file_sent_len = 0;
                              c->send_buffer[c->file_sent_len]= '\0';
                              c->receive= 1;
                              c->chunk_len= 0;
                              c->chunk_sent = 0;
			      if(strcmp(c->connection_type,"close")==0)
                                {
                                  c->recv_bufflen = 0;                                                                              
                                  c->recv_buffer[c->recv_bufflen] = '\0';
                                  epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
                                  close(c->fd);
                                  free(c);
                                }
			      continue;
			}
			
			  if(c->file_total_len != c->file_sent_len)
			    {
			      if(c->chunk_len == c->chunk_sent)
				{
				  int nread = fread(c->send_buffer,sizeof(char),BUFFER_SIZE-1,c->fp);
				  c->send_buffer[nread] = '\0';
				  c->chunk_len = nread;
				  c->chunk_sent = 0;
				}
			      n = send(c->fd, c->send_buffer+c->chunk_sent, c->chunk_len-c->chunk_sent,0);
			     
			      if(n < 0)
				{
				  if(errno == EAGAIN || errno == EWOULDBLOCK)
				    {
			              continue;
				    }
				  else
				    {
				      epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
				      close(c->fd);
				      free(c);
				      logging("ERROR", "send file failed , client socket closed");
				      break;
				    }

				}
			      else
				{
				  c->file_sent_len += n;
				  c->chunk_sent += n;
				}
			    }
			  else
			    {
			      
                              mod_event.events = EPOLLIN ;
                              mod_event.data.ptr = c;
                              epoll_ctl(epfd,EPOLL_CTL_MOD,c->fd,&mod_event);

			      c->header_total_len = 0;
			      c->header_sent_len = 0;
			      c->header_buffer[c->header_total_len] = '\0';
			      c->file_total_len = 0;
			      c->file_sent_len = 0;
			      c->send_buffer[c->file_sent_len]= '\0';
			      c->receive= 1;
			      c->chunk_len= 0;
			      c->chunk_sent = 0;
			      fclose(c->fp);
			      c->fp = NULL;

			      if(strcmp(c->connection_type,"close")==0)
				{
			          c->recv_bufflen = 0;                                                                                                                c->recv_buffer[c->recv_bufflen] = '\0';
				  epoll_ctl(epfd,EPOLL_CTL_DEL,c->fd,NULL);
                                  close(c->fd);
                                  free(c);
				}
			      
			    }
	         
		      }		      
		    }	   
	         }	  
	      }
  close(epfd);
  close(server_fd);
  return 0;
}
