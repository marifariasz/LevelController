#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "web_site/html.h"
#include "web_site.h"

// Configurações de WiFi
#define WIFI_SSID "SEUSSID"
#define WIFI_PASS "SUASENHA"

// Variáveis globais para armazenar o nível de água e estado da bomba
uint16_t nivel;
bool bomba;

// Inicialização da estrutura de configuração dos níveis de água
struct nivel_agua nivelConfig = {250, 550};

// Estrutura para armazenar o estado das respostas HTTP
struct http_state
{
    char response[5300];  // Buffer para a resposta HTTP
    size_t len;          // Tamanho total da resposta
    size_t sent;         // Quantidade já enviada
};

// Callback chamado quando dados são enviados via TCP
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    
    // Se toda a resposta foi enviada, fecha a conexão
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

// Callback chamado quando dados são recebidos via TCP
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs)
    {
        pbuf_free(p);
        tcp_close(tpcb);
        tcp_recved(tpcb, p->tot_len);
        return ERR_MEM;
    }
    hs->sent = 0;

    // Verifica se a requisição é para obter o nível atual e estado da bomba
    if (strstr(req, "GET /nivel"))
    {
        // Cria um payload JSON com os valores atuais
        char json_payload[96];
        int json_len = snprintf(json_payload, sizeof(json_payload), "{\"nivel\":%d, \"bomba\":%d}\r\n", nivel, bomba);

        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           json_len, json_payload);
    }
    // Verifica se a requisição é para atualizar os valores mínimo e máximo
    else if (strstr(req, "GET /valor_min_max/") != NULL)
    {
        char *pos = strstr(req, "/valor_min_max/");
        if (pos)
        {
            pos += strlen("/valor_min_max/"); // Avança para a parte dos valores

            // Copia os valores para um buffer separado
            char valores[32];
            strncpy(valores, pos, sizeof(valores) - 1);
            valores[sizeof(valores) - 1] = '\0'; // Garante o término da string

            // Extrai os valores mínimo e máximo usando strtok
            char *token = strtok(valores, "s");
            int minimo = 0;
            int maximo = 0;

            if (token != NULL)
            {
                minimo = atoi(token); // Converte o primeiro valor para inteiro
                token = strtok(NULL, "s");
                if (token != NULL)
                {
                    maximo = atoi(token); // Converte o segundo valor para inteiro

                    // Atualiza a estrutura de configuração
                    nivelConfig.min = (uint16_t)minimo;
                    nivelConfig.max = (uint16_t)maximo;

                    printf("Novo Min: %d | Novo Max: %d\n", nivelConfig.min, nivelConfig.max);
                }
            }
        }
    }
    // Requisição padrão - retorna a página HTML
    else
    {
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(html), html);
    }

    // Configura os callbacks e envia a resposta
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    return ERR_OK;
}

// Callback chamado quando uma nova conexão é estabelecida
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

// Inicia o servidor HTTP na porta 80
static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

// Inicializa a conexão WiFi e o servidor web
void init_web_site()
{
    if (cyw43_arch_init())
    {
        printf("Erro");
        return;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Erro ao se conectar");
        return;
    }

    // Obtém e imprime o endereço IP atribuído
    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    printf("%s ", ip_str);

    start_http_server();
}

// Atualiza as variáveis globais com os valores atuais do nível e estado da bomba
void update_web_site(uint16_t nivel_paran, bool bomba_paran)
{
    nivel = nivel_paran;
    bomba = bomba_paran;
}