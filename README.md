# HellfireOS Realtime Operating System

---
### <a name="aula01"></a> Instruções usadas na aula de laboratório 01:

opcional: 
- Download GNU cross (MIPS) toolchain (4.6 series, pre-built)
  - [32 bits](https://dl.dropboxusercontent.com/u/7936618/gcc-4.6.1_x86.tar.gz)
  - [64 bits](https://dl.dropboxusercontent.com/u/7936618/gcc-4.6.1.tar.gz)
```sh
export PATH=$PATH:<PATH_TO_TOOLCHAIN>/bin/
```
 - Execute 
```sh
git clone git@github.com/sjohan81/hellfireos.git
cd hellfireos
gcc usr/sim/hf_risc_sim/hf_risc_sim.c -o usr/sim/hf_risc_sim/hf_risc_sim
cd platform/single_core
make image
cd ../../
./usr/sim/hf_risc_sim/hf_risc_sim platform/single_core/image.bin 
```
---
### <a name="aula02"></a> Instruções usadas na aula de laboratório 02:

O objetivo dessa prática é que você se familiarize com a API do HellfireOS. Para
isso, configure o ambiente (compilador e simulador) e veja os exemplos de uso da
API em /app. Se houver necessidade, verifique a API na documentação
disponibilizada em /usr/doc. Para este exercício, é necessário que você:

- Crie um diretório de projeto em /app, contendo um makefile e um arquivo com o
	código da aplicação;
- Modifique a plataforma em /platform/single_core, configurando o makefile para
	a sua aplicação.

Olhar o material da [aula 01](#aula01) se for preciso configurar o ambiente
novamente.

---

Problema 1:

- Um pombo correio leva mensagens entre os sites A e B, mas só quando o número
	de mensagens acumuladas chega a 20.
- Inicialmente, o pombo fica em A, esperando que existam 20 mensagens para
	carregar, e dormindo enquanto não houver.
- Quando as mensagens chegam a 20, o pombo deve levar exatamente (nenhuma a mais
	nem a menos) 20 mensagens de A para B, e em seguida voltar para A.
- Caso existam outras 20 mensagens, ele parte imediatamente; caso contrário, ele
	dorme de novo até que existam as 20 mensagens.
- As mensagens são escritas em um post-it pelos usuários; cada usuário, quando
	tem uma mensagem pronta, cola sua mensagem na mochila do pombo. Caso o pombo
	tenha partido, ele deve esperar o seu retorno p/ colar a mensagem na mochila.
- O vigésimo usuário deve acordar o pombo caso ele esteja dormindo.
- Cada usuário tem seu bloquinho inesgotável de post-it e continuamente prepara
	uma mensagem e a leva ao pombo.

Usando semáforos e/ou mutexes, modele o processo pombo e o processo usuário,
lembrando que existem muitos usuários e apenas um pombo. Identifique regiões
críticas na vida do usuário e do pombo.

```c
usuario() {
    while(true){
        down(enchendo);
        down(mutex);
        colaPostIt_na_mochila();
        contaPostIt++;
        if (contaPostIt==N)
            up(cheia);
        up(mutex);
    }
}

pombo() {
    while(true){
        down(cheia);
        down(mutex);
        leva_mochila_ate_B_e_volta();
        contaPostIt=0;
        for(i=0;i<N;i++)
            up(enchendo);
        up(mutex);
    }
}
```

Problema 2:

Considere cinco filósofos que passam a vida a comer e a pensar. Eles
compartilham uma mesa circular, com um prato de arroz ao centro.
Na mesa existem cinco pauzinhos, colocados um de cada lado do filósofo. Quando
um filósofo fica com fome ele pega os dois pauzinhos mais
próximos, um de cada vez, e come até ficar saciado. Quando acaba de comer, ele
repousa os pauzinhos e volta a pensar.

```c
#define N 5
#define LEFT (i+N-1)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

int state[N];
semaphore mutex = 1;
semaphore s[N];

void take_forks(int i){
    down(&mutex);
    state[i] = HUNGRY;
    test(i);
    up(&mutex);
    down(&s[i]);
}

void put_forks(i){
    down(&mutex);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    up(&mutex);
}

void test(i){
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING)
        state[i] = EATING; up(&s[i]);
}

void philosopher(int i){
    while (TRUE){
        think();
        take_forks(i);
        eat();
        put_forks(i);
    }
}
```

