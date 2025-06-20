#include "web_site/style.h"
#include "web_site/javascript.h"

char html[] = //variavel char para o codigo html
    "\n<!DOCTYPE html><html lang=\"pt-br\"><head><meta charset=\"UTF-8\">"
    "\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "\n <style>" STYLE_PAGE
    "</style>"
    "<title>Design Projeto</title></head>"
    "\n<body><header><h1>LEVEL CONTROLLER</h1></header>"
    "\n<main class=\"principal\">"
    "\n<div id=\"nivel\"><p><strong>NÍVEL DE ÁGUA ATUAL:</strong></p><p class='nivel'></p></div>"
    "\n<div class=\"estado-led\"><div id=\"estado\"><p><strong>BOMBA <span id='label_bomba'>DESLIGADA</span></strong></p></div>"
    "\n<div id=\"led\" class=\"led-desligado\"></div></div>"
    "\n<div id=\"MinMax\">Defina Valores Mínimo e Máximo</div>"
    "\n<div class=\"Definicao\">"
    "\n<div id=\"camposNumeros\">"
    "\n<label for=\"minimo\">Quantidade Mínima:</label>"
    "\n<input type=\"number\" id=\"minimo\" min=\"1\" placeholder=\"Digite o mínimo\">"
    "\n<label for=\"maximo\">Quantidade Máxima:</label>"
    "\n<input type=\"number\" id=\"maximo\" min=\"1\" placeholder=\"Digite o máximo\">"
    "\n<button class=\"confirmar\">Confirmar</button>"
    "\n</div></div></main><script>" JAVA_SCRIPT
    "\n </script> </body></html>";