
#ifndef WEB_SITE_H
#define WEB_SITE_H

// Estrutura para armazenar os níveis mínimo e máximo de água
struct nivel_agua
{
    uint16_t min;  // Valor mínimo do nível de água
    uint16_t max;  // Valor máximo do nível de água
};

// Variável global que armazena a configuração dos níveis de água
extern struct nivel_agua nivelConfig;

// Função para inicializar o servidor web
void init_web_site();

// Função para atualizar os valores do nível de água e estado da bomba no servidor web
void update_web_site(uint16_t nivel_paran, bool bomba_paran);

#endif