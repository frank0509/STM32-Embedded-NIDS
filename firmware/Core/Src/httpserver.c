/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : http.c
  * @brief          : Web server request handler
  ******************************************************************************
  * Copyright (c) 2025 Adrian Ciorita
  ********************************************************************************
  *	This code present how to handle and how to manage API request from a webserver
  *	created with HTML and CSS
  ******************************************************************************
  * Created by Adi Ciorita on 10.02.2025
  ******************************************************************************
  * Last update: 21.03.2025
  ******************************************************************************
    */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "httpserver.h"
#include "motor_control.h"
#include "inttypes.h"

//External variables
extern TIM_HandleTypeDef htim3;


//Global variables
 uint32_t  motor_status = 1;
 uint32_t motor_speed = 0;
 int previous_motor_speed = 0;
 char motor_direction[10] = "Nedefinit";
 int motor_runtime = 0;

 //---------------------


 // Function to serve http server
 static void http_server(struct netconn *conn)
{
	struct netbuf *inbuf;
	err_t recv_err;
	char* buf;
	u16_t buflen;
	struct fs_file file;

	/* Read the data from the port, blocking if nothing yet there */
	recv_err = netconn_recv(conn, &inbuf);

	if (recv_err == ERR_OK)
	{
		if (netconn_err(conn) == ERR_OK)
		{
			/* Get the data pointer and length of the data inside a netbuf */
			netbuf_data(inbuf, (void**)&buf, &buflen);

			/* Check if request to get the index.html */
			if (strncmp((char const *)buf,"GET /home.html",14)==0)
			{
				fs_open(&file, "/home.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}
			else if(strncmp((char const *)buf,"GET /status.html",16)==0)
			{
				fs_open(&file, "/status.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}
			else if(strncmp((char const *)buf,"GET /control.html",17)==0)
			{
				fs_open(&file, "/control.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
				}
			else if (strncmp((char const *)buf, "GET /api/motor_status", 21) == 0)
			{
				send_motor_status(conn);
			}
			else if (strncmp((char const *)buf, "PATCH /api/motor_control", 24) == 0)
			{
			    char *body = strstr(buf, "\r\n\r\n");  // Găsește începutul corpului JSON
			    if (body != NULL)
			    {
			        body += 4;  // Sare peste `\r\n\r\n`

			        // Verificăm dacă există ceva după delimitator (evităm trimiterea unui corp gol)
			        if (*body != '\0' && strlen(body) > 2)  // Verificăm că body nu e gol
			        {
			            get_motor_status(conn, body);

			            if(motor_speed> previous_motor_speed)
			            {
			            	accelerate_to_RPM(motor_speed);
			            }
			            else if(motor_speed < previous_motor_speed)
			            {
			            	decelerate_to_RPM(motor_speed);
			            }
			            else if(motor_speed == 0)
			            {
			            	decelerate_to_RPM(0);
			            }
			            previous_motor_speed = motor_speed;
			        }
			        else
			        {
			            // Dacă nu avem body valid, trimitem un răspuns de eroare
			            char error_response[] = "HTTP/1.1 400 Bad Request\r\n"
			                                    "Content-Type: text/plain\r\n"
			                                    "Content-Length: 22\r\n"
			                                    "Connection: close\r\n"
			                                    "\r\n"
			                                    "Eroare: Body invalid!";
			            netconn_write(conn, error_response, sizeof(error_response) - 1, NETCONN_COPY);
			            netconn_close(conn);
			            netconn_delete(conn);
			        }
			    }
			    else
			    {
			        // Dacă nu găsim `\r\n\r\n`, returnăm un răspuns de eroare
			        char error_response[] = "HTTP/1.1 400 Bad Request\r\n"
			                                "Content-Type: text/plain\r\n"
			                                "Content-Length: 32\r\n"
			                                "Connection: close\r\n"
			                                "\r\n"
			                                "Eroare: Lipseste corpul cererii!";
			        netconn_write(conn, error_response, sizeof(error_response) - 1, NETCONN_COPY);
			    }
			}
			else
			{
				DynWebPage(conn);
				/* Load Error page */
			}
		}
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	/* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
}


static void http_thread(void *arg)
{ 
  struct netconn *conn, *newconn;
  err_t err, accept_err;

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);

  if (conn!= NULL)
  {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, IP_ADDR_ANY, 80);

    if (err == ERR_OK)
    {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);

      while(1)
      {
        /* accept any incoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if(accept_err == ERR_OK)
        {
          /* serve connection */
          http_server(newconn);

          /* delete connection */
          netconn_delete(newconn);
        }
      }
    }
  }
}


void http_server_init()
{
  sys_thread_new("http_thread", http_thread, NULL, DEFAULT_THREAD_STACKSIZE, osPriorityNormal);
}

static uint32_t nPageHits = 0;


void DynWebPage(struct netconn *conn)
{
    char PAGE_BODY[512];
    char pagehits[10] = {0};
    osThreadId_t thread_ids[10];
    uint32_t thread_count;

    memset(PAGE_BODY, 0, 512);

    /* Add HTML start and auto-refresh meta tag */
    strcat(PAGE_BODY, "<!DOCTYPE html><html><head>");
    strcat(PAGE_BODY, "<title>Thread Status</title></head><body>");

    /* Update the hit count */
    nPageHits++;
    //sprintf(pagehits, "Page hits: %d", (int)nPageHits);
    strcat(PAGE_BODY, pagehits);

    /* Add thread info */
    strcat(PAGE_BODY, "<pre><br>Name          State  Priority   Stack<br>");
    strcat(PAGE_BODY, "---------------------------------------------<br>");

    thread_count = osThreadEnumerate(thread_ids, 10);

    for (uint32_t i = 0; i < thread_count; i++)
    {
        const char *name = osThreadGetName(thread_ids[i]);
        osThreadState_t state = osThreadGetState(thread_ids[i]);
        osPriority_t priority = osThreadGetPriority(thread_ids[i]);

        char thread_info[100];
        snprintf(thread_info, sizeof(thread_info), "%-15s %-5d %-5d<br>",
                  name ? name : "Unknown", state, priority);
        strcat(PAGE_BODY, thread_info);
    }

    strcat(PAGE_BODY, "<br>---------------------------------------------");
    strcat(PAGE_BODY, "<br>0: Inactive, 1: Ready, 2: Running, 3: Blocked, 4: Terminated, 5: Error, 6: Reserved<br>");
    strcat(PAGE_BODY, "</body></html>");

    /* Send the dynamically generated page */
    netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}



void send_motor_status(struct netconn *conn) {
    char response[256];
    char http_header[128];

    memset(response, 0, sizeof(response));
    memset(http_header, 0, sizeof(http_header));

    int json_length = snprintf(response, sizeof(response),
        "{ \"status\": \"%s\", \"speed\": %ul, \"direction\": \"%s\", \"runtime\": %ul }",
        motor_status ? "Pornit" : "Oprit", motor_speed, motor_direction, motor_runtime);

    if (json_length < 0 || json_length >= sizeof(response)) {
        return;
    }

    int header_length = snprintf(http_header, sizeof(http_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",   // IMPORTANT! Al doilea \r\n pentru a finaliza header-ul
        json_length);

    if (header_length < 0 || header_length >= sizeof(http_header)) {
        return;
    }

    netconn_write(conn, http_header, header_length, NETCONN_COPY);
    netconn_write(conn, response, json_length, NETCONN_COPY);

    netconn_close(conn);
    netconn_delete(conn);
}


void get_motor_status(struct netconn *conn, char *request_body) {
    int temp_speed = 0;
    char temp_direction[10] = {0};

    // Găsește și extrage "speed"
    char *speed_ptr = strstr(request_body, "\"speed\":");
    if (speed_ptr) {
        sscanf(speed_ptr, "\"speed\":%d", &temp_speed);
        if (temp_speed > 0 && temp_speed < 20) {
            temp_speed = 20;  // Aplică regula JavaScript
        }
        motor_speed = temp_speed;
    }

    // Găsește și extrage "direction"
    char *dir_ptr = strstr(request_body, "\"direction\":\"");
    if (dir_ptr) {
        dir_ptr += strlen("\"direction\":\"");  // Mutăm pointerul la începutul valorii
        int i = 0;
        while (*dir_ptr != '"' && *dir_ptr != '\0' && i < (int)sizeof(temp_direction) - 1) {
            temp_direction[i++] = *dir_ptr++;
        }
        temp_direction[i] = '\0';  // Termină string-ul

        if (strlen(temp_direction) > 0) {
            snprintf(motor_direction, sizeof(motor_direction), "%s", temp_direction);
        }
    }

    // Determină starea motorului (pornit dacă viteza > 0)
    motor_status = (motor_speed > 0) ? 1 : 0;

    // Construiește răspunsul JSON
    char response[128];
    int json_length = snprintf(response, sizeof(response),
        "{ \"status\": \"%s\", \"speed\": %d, \"direction\": \"%s\", \"runtime\": %d }",
        motor_status ? "Pornit" : "Oprit", motor_speed, motor_direction, motor_runtime);

    // Construiește header-ul HTTP
    char http_header[128];
    int header_length = snprintf(http_header, sizeof(http_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        json_length);

    // Trimite răspunsul HTTP
    netconn_write(conn, http_header, header_length, NETCONN_COPY);
    netconn_write(conn, response, json_length, NETCONN_COPY);

    netconn_close(conn);
    netconn_delete(conn);
}


