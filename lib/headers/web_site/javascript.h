//configuração javascript com validações necessárias para comunicação da placa com o web server
#define JAVA_SCRIPT \
"\nconst campos=document.getElementById('camposNumeros');" \
"\nconst botaoConfirmar=document.querySelector('.confirmar');" \
"\nconst valor_nivel=document.querySelector('.nivel');" \
"\nconst bomba=document.querySelector('#led');"\
"\nconst label_bomba=document.querySelector('#label_bomba')"\
"\nbotaoConfirmar.addEventListener('click',()=>{" \
"\nlet minimo=document.getElementById('minimo').value;" \
"\nlet maximo=document.getElementById('maximo').value;" \
"\nif(isNaN(minimo)||isNaN(maximo)){" \
"\nalert('Por favor, preencha os dois campos com valores válidos.');" \
"\nreturn;}minimo=Number(minimo);maximo=Number(maximo);" \
"\nif(minimo<1||maximo<1){" \
"\nalert('Os valores devem ser maiores que zero.');" \
"\nreturn;}" \
"\nif(maximo<minimo){" \
"\nalert('O valor máximo deve ser maior que o mínimo.');" \
"\nreturn;}" \
"\nfetch('/valor_min_max/'+minimo+'s'+maximo);" \
"\ndocument.getElementById('minimo').value='';document.getElementById('maximo').value='';"\
"\nalert(`Valores confirmados:\nMínimo: ${minimo}\nMáximo: ${maximo}`);});" \
"\nfunction atualizar(){" \
"\nfetch('/nivel').then(res=>res.json()).then(data=>{" \
"\nvalor_nivel.innerHTML=`<strong>${data.nivel}</strong>`;"\
"\nif(data.bomba==1){bomba.classList.remove('led-desligado');bomba.classList.add('led-ligado');label_bomba.innerHTML='LIGADA';"\
"\n}else{bomba.classList.remove('led-ligado');bomba.classList.add('led-desligado');label_bomba.innerHTML='DESLIGADA';}});}" \
"\nsetInterval(atualizar,1000);"

