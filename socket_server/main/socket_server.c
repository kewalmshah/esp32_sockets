/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID "myssid"
#define EXAMPLE_WIFI_PASS "mypassword"
static char tag[] = "socket_server";
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "example.com"
#define WEB_PORT 80
#define WEB_URL "http://example.com/"

static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        //printf(".....%s ", (char *)event->event_info.got_ip.ip_info.ip.addr);

        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        printf("got ip\n");
        printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
        printf("netmask: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.netmask));
        printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
printf("\n");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void http_server(void *pvParameters)
{

    //while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
    // *evt = NULL;
        
        const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
           // continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;
    int total = 10*1024;
        int sizeUsed = 0;
        //char *data = malloc(total);
char data[256];
    // Create a socket that we will listen upon.
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
        //goto END;
    }

    // Bind our server socket to a port.
    bzero((char *) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8001);
    int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (rc < 0) {
        ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
        //goto END;
    }
    //printf("ip socket: " IPSTR "\n", IP2STR(serverAddress.sin_addr.s_addr));
    // Flag the socket as listening for new connections.
    //write();
    rc = listen(sock, 5);
    if (rc < 0) {
        ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
        //goto END;
    }

    //while (1) {
        // Listen for a new client connection.
        socklen_t clientAddressLength = sizeof(clientAddress);
               while(1) {

        int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSock < 0) {
            ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
            //goto END;
        }
        printf("\n");
        // We now have a new client ...
        

        // Loop reading data.
            //int sizeRead = recv(clientSock, data + sizeUsed, total-sizeUsed, 0);
            //if (sizeRead < 0) {
              //  ESP_LOGE(tag, "recv: %d %s", sizeRead, strerror(errno));
                //goto END;
            //}
            //if (sizeRead == 0) {
            //}
            //sizeUsed += sizeRead;
        //printf("*%s\n", data);
        // Finished reading data.
        //do {
            bzero(data, sizeof(data));
            r = read(clientSock, data, 255);
           for(int i = 0; i < r; i++) {
              putchar(data[i]);
           }

           if(strstr(data, "GET /off1") > 0) {
                printf("done taking this");
           }
        //} while(r > 0);
        printf("Done reading\n");
        static const char *wrt_msg ="\nHTTP/1.1 200 OK\r\n"
                        //"Content-Length: 88\n"
                        "Content-Type: text/html; charset=iso-8859-1\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "<!DOCTYPE HTML>\r\n"
                         "<html>\r\n"
                         "<body>\r\n"
                         "<h1>Hello, World!</h1>\r\n"
                         "<p>LED #1 <a href=\"on1\"><button>ON</button></a>&nbsp;<a href=\"off1\"><button>OFF</button></a></p>"
                         //"<p>LED #2 <a href=\"on2\"><button>ON</button></a>&nbsp;<a href=\"off2\"><button>OFF</button></a></p>
                         "</body>\r\n"
                         "</html>\r\n";
        printf("%d", sizeof(wrt_msg));
        r =send(clientSock, wrt_msg, strlen(wrt_msg), 0);
        printf("Done Writing: %s\n", wrt_msg);
        vTaskDelay(10);
        //ESP_LOGD(tag, "Data read (size: %d) was: %.*s", sizeUsed, sizeUsed, data);
        //free(data);
        close(clientSock);
    }
}
  //  END:
//vTaskDelete(NULL);

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xTaskCreate(&http_server, "http_get_task", 4096, NULL, 5, NULL);
}
