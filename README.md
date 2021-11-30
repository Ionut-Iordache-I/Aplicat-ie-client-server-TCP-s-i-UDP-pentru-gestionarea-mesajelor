# Aplicatie-client-server-TCP-si-UDP-pentru-gestionarea-mesajelor

Iordache Ionut-Iulian

    Pentru implementarea acestei teme m-am folosit de informatiile din laboratoarele
din laboratoarele 6,7 si 8 (UDP, TCP, Multiplexare I/O)

    In fisierul helpers.h am creat o structura tcp_mesg care pastreaza informatiile
necesare ce vor fii transmise la server precum lungimea topicului sau proprietatea de S&F.
Pentru un mesaj udp am folosit o alta structura (udp_mesg) care sa faciliteze transmiterea
unui mesaj de la clientul udp implementat de echipa care s-a ocupat de aceasta tema.
De asemenea am folosit structura s_Map pentru a face o mare intre un topic
si clientii(subscriber) care sunt abonati la topicul respectiv. Un astfel de client(subscriber)
are un id, un socket pe care comunica cat si proprietatea de a se pastra mesajle in timpul
in care se afla deconectat de la server. De asemenea in struct udp_msgs_buffer am pastrat
o lista cu mesajle care am fost transmise in lipsa clientilor tcp conectati pentru o 
transmitere ulterioara.

Subscriber:
    Pentru a facilita conexiunea unui client la server si a indeplinii functionalitatea
dorita initial am apelat setvbuf pentru a dezactiva bufferingul la afisare asa cum este 
mentionat in cerinta temei, de asemenea am verificat respectarea numarului de parametrii.
Dupa ce am creat un socket pentru realizarea comunicarii si am verificat posibila eroare
am adaugat in multimea descriptorilor de citire (read_fds) sosketul pentru standard input
cat si cel pe care se va realiza transferul de informatie, actualizand fdmax cu valoarea 
maxima din multimea descriptorilor. Ulterior in structura serv_addr am completat informatiile
referitoare la adresa, familia de adrese si portul serverului si am realizat o conexiune
cu serverul apeland connect(). Dupa conectare am facut apelul de send() pentru a trimite 
cartre server id-ul cu care se conecteaza un client tcp la server.
    In bucla while am pastrat mai intai in temp_fds o copie a multimii ce contine descriptorii
de fisiere intrucat dupa apelul de select() multimea va mai contine doar descriptorii pe 
care s-au primit date. Pentru cazul in care primeam date pe socketul de stdin am verificat 
comenzile ce puteau sa apara aici(subscribe, unsubscribe, exit). In urma comenzii de 
subscribe trimiteam catre server un tcp_msg cu informatii despre topicul cerut de subscriber,
S&F si tipul de mesaj(am transmis un astfel de mesaj si in cazul unui unsubscribe) 
functiile se afla in subscriber_functions.c.
Pentru cazul in care am primit date pe socketul dintre server si subscriber pe care le stochez
sub forma unui mesaj udp, daca primeam o comanda de exit inchideam clientul, altfel verificam
pentru mesaj identificatorul de tip si afisam in client mesajul in forma ceruta in enunt.
La final inchid socketul folosit pentru realizarea comunicarii.

Server:
    Pentru a indeplini functionalitatea dorita pentru server, de a prelua mesaje de la 
clienti UDP si a le transmite catre clienti TCP prin server am realizat urmatoarele: mai 
intai am apelat setvbuf() pentru dezactivarea buffering-ului, dupa care am verificat 
corectitudinea numarului de parametrii ai serverului, dupa care in prealabil 
mi-am golit cele 2 multimi de descriptori read_fds si temp_fds.
Am completat in serv_addr adresa socketului si portul, dupa care am realizat operatia de
bind() pentru a asocia socketului o adresa. Am apelat listen() pentru a asculta pe server
cererile venite de la clientii tcp si am adaugat la multime descriptrilor 0(stdin) si sockfd_tcp.
Pentru a realiza o conexiune cu un client udp  am deschis socketul sockfd_udp si am completat
adresa si portul in udp_addr, dupa care am facut bind si am adaugat socketul la read_fds cu
actualizarea lui fdmax.
    In bucla while am pastrat mai intai in temp_fds o copie a multimii ce contine descriptorii
de fisiere, dupa care am realizat apelul de select(). In continuare am parcurs multimea de 
descriptori pentru a vedea pe care am primit date. In cazul in care primeam date de la stdin
le puneam in buffer si verificam daca reprezinta o comanda valida de exit care trebuie sa inchida
serverul si sa transmita catre toti ceilalti clienti tcp comanda de exit penru inchiderea acestora.
Daca primeam date pe socketul de udp efectuam apelul de recvfrom() si completam structura de tipul
udp_msg cu valorile primite. Dupa care am parcurs toate topicurile pentru a determina cel caruia
i se potriveste topicului mesajului si am parcurs clientii conectati la server pentru a le trimite
mesajul. In cazul in care clientul nu este conectat la server, daca acesta are SF setat la 1
nu va receptiona mesajele transmise in lipsa lui. In celalalt caz (SF = 1) insa am adaugat mesajul
in structura msgs_buff ce retine pentru un id mesajele primite in lipsa unui client, folosind
realloc pentru a adauga si mesajul primit. Daca nu aveam o intrare pentru mesajul pe care incercam
sa il adaug o creeam. Pentru situatia in care am primit date pe socketul de tcp inseamna ca am primit
o cerere pentru o cenexiune asadar, m-am folosit de apelul functie accept() pentru a acepta conexiunea
dupa care am dezactivat algoritmul lui Neagle conform cerintei folosindu-ma de apelul 
functiile setsockopt(). Ulterior am primit pe socketul pentru conexiunea cu clientul tcp id-ul 
de client transmis de subscriber. Pentru a nu avea 2 conexiuni care sa aiba aceiasi subscriberi
am verificat tot aici daca id-ul primit se regaseste cumva in vectorul clientilor deja
conectati la server, caz in care afisam mesajul "Client %s already connected." si trimiteam 
clientului comanda de exit inchizand totodata socketul. Daca nu intram pe acest caz 
afisam conform cerintei "New client <ID_CLIENT> connected from IP:PORT" si imi cream un nou 
client pe care sa il adaug in vectorul de clienti cu realloc. In cazul in care clientul a mai 
fost conectat la server trebuie de asemenea sa ii actualizez socketul, dupa care am transmis
datele salvate in saved_msgs catre client tcp. Daca receptionam date pe orice alt socket 
nu am mai apucat sa verific decat cazul in care pe socketul respectiv primeam un mesaj ce
intorcea 0, adica transmitea catre server ca se doreste inchiderea conexiunii, moment
in care clientul se elimina din lista de clienti conectati la server. In schimb daca recv
intorcea valoarea diferita de 0 ar fii trebuit sa fac verificarea in functie de tipul de
mesaj primit cu valoarea de subscribe de 1 sau 0.
