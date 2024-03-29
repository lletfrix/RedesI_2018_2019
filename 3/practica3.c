/***************************************************************************
 practica3.c
 Inicio, funciones auxiliares y modulos de transmision implmentados y a implementar de la practica 4.
Compila con warning pues falta usar variables y modificar funciones

 Compila: make
 Autor: Jose Luis Garcia Dorado, Jorge E. Lopez de Vergara Mendez
 2018 EPS-UAM v1
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "interface.h"
#include "practica3.h"

/***************************Variables globales utiles*************************************************/
pcap_t* descr, *descr2; //Descriptores de la interface de red
pcap_dumper_t * pdumper;//y salida a pcap
uint64_t cont=0;    //Contador numero de mensajes enviados
char interface[10];    //Interface donde transmitir por ejemplo "eth0"
uint16_t ID=1;        //Identificador IP
char flag_mostrar = 0;

void handleSignal(int nsignal){
    printf("Control C pulsado (%"PRIu64")\n", cont);
    pcap_close(descr);
    exit(OK);
}

int main(int argc, char **argv){

    char errbuf[PCAP_ERRBUF_SIZE];
    char fichero_pcap_destino[CADENAS];
    int bytes;
    uint8_t IP_destino_red[IP_ALEN];
    uint16_t MTU;
    uint16_t datalink;
    uint16_t puerto_destino;
    char data[IP_DATAGRAM_MAX]={0};
    uint16_t pila_protocolos[CADENAS];
    FILE *src_f = NULL;

    int long_index=0;
    char opt;
    char flag_iface = 0, flag_ip = 0, flag_port = 0, flag_file = 0, flag_dontfrag = 0;

    static struct option options[] = {
        {"if",required_argument,0,'1'},
        {"ip",required_argument,0,'2'},
        {"pd",required_argument,0,'3'},
        {"f",required_argument,0,'4'},
        {"d",no_argument,0,'5'},
        {"m",no_argument,0,'6'},
        {"h",no_argument,0,'7'},
        {0,0,0,0}
    };

        //Dos opciones: leer de stdin o de fichero, adicionalmente para pruebas si no se introduce argumento se considera que el mensaje es "Payload "
    while ((opt = getopt_long_only(argc, argv,"1:2:3:4:5:6:7", options, &long_index )) != -1) {
        switch (opt) {

            case '1' :

                flag_iface = 1;
                //Por comodidad definimos interface como una variable global
                sprintf(interface,"%s",optarg);
                break;

            case '2' :

                flag_ip = 1;
                //Leemos la IP a donde transmitir y la almacenamos en orden de red
                if (sscanf(optarg,"%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8"",
                                   &(IP_destino_red[0]),&(IP_destino_red[1]),&(IP_destino_red[2]),&(IP_destino_red[3])) != IP_ALEN){
                    printf("Error: Fallo en la lectura IP destino %s\n", optarg);
                    exit(ERROR);
                }

                break;

            case '3' :

                flag_port = 1;
                //Leemos el puerto a donde transmitir y la almacenamos en orden de hardware
                puerto_destino=atoi(optarg);
                break;

            case '4' :

                if(strcmp(optarg,"stdin")==0) {
                    if (fgets(data, sizeof data, stdin)==NULL) {
                        printf("Error leyendo desde stdin: %s %s %d.\n",errbuf,__FILE__,__LINE__);
                        return ERROR;
                    }
                    sprintf(fichero_pcap_destino,"%s%s","stdin",".pcap");
                    bytes = strlen(data);
                } else {
                    sprintf(fichero_pcap_destino,"%s%s",optarg,".pcap");
                    if(!(src_f = fopen(optarg, "rb"))){
                        printf("Error abriendo el fichero:%s %s %s %d.\n",optarg, errbuf,__FILE__,__LINE__);
                        return ERROR;
                    }
                    bytes = fread(data, sizeof(char), IP_DATAGRAM_MAX-1, src_f);
                    if(ferror(src_f)){
                        printf("Error leyendo del fichero:%s %s %s %d.\n",optarg, errbuf,__FILE__,__LINE__);
                        return ERROR;
                    }
                }
                flag_file = 1;
                break;

            case '5' :
                flag_dontfrag =1; // El usuario solicita que los paquetes se envien con el bit DF=1.
                break;

            case '6' :
                flag_mostrar =1; // El usuario solicita que se muestren en hexadecimal las tramas enviadas.
                break;

            case '7' : printf("Ayuda. Ejecucion: %s -if interface -ip direccion_IP -pd puerto [-f /ruta/fichero_a_transmitir o stdin] [-d] [-m]: %d\n",argv[0],argc); exit(ERROR);
                break;

            case '?' :
            default: printf("Error. Ejecucion: %s -if interface -ip direccion_IP -pd puerto [-f /ruta/fichero_a_transmitir o stdin] [-d] [-m]: %d\n",argv[0],argc); exit(ERROR);
                break;
        }
    }

    if ((flag_iface == 0) || (flag_ip == 0) || (flag_port == 0)){
        printf("Error. Ejecucion: %s -if interface -ip direccion_IP -pd puerto [-f /ruta/fichero_a_transmitir o stdin] [-d] [-m]: %d\n",argv[0],argc);
        exit(ERROR);
    } else {
        printf("Interface:\n\t%s\n",interface);
        printf("IP:\n\t%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\n",IP_destino_red[0],IP_destino_red[1],IP_destino_red[2],IP_destino_red[3]);
        printf("Puerto destino:\n\t%"PRIu16"\n",puerto_destino);
        if (flag_dontfrag) printf("Se solicita enviar paquete con bit DF=1\n");
        if (flag_mostrar) printf("Se solicita mostrar las tramas enviadas en hexadecimal\n");
    }

    if (flag_file == 0) {
        sprintf(data,"%s","Payload "); //Deben ser pares!
        bytes = strlen(data);
        sprintf(fichero_pcap_destino,"%s%s","debugging",".pcap");
    }

    if(signal(SIGINT,handleSignal)==SIG_ERR){
        printf("Error: Fallo al capturar la senal SIGINT.\n");
        return ERROR;
    }
    //Inicializamos las tablas de protocolos
    if(inicializarPilaEnviar()==ERROR){
          printf("Error leyendo desde stdin: %s %s %d.\n",errbuf,__FILE__,__LINE__);
        return ERROR;
    }
    //Leemos el tamano maximo de transmision del nivel de enlace
    if(obtenerMTUInterface(interface, &MTU)==ERROR)
        return ERROR;
    //Descriptor de la interface de red donde inyectar trafico
    if ((descr = pcap_open_live(interface,MTU+ETH_HLEN,0, 0, errbuf)) == NULL){
        printf("Error: pcap_open_live(): %s %s %d.\n",errbuf,__FILE__,__LINE__);
        return ERROR;
    }

    datalink=(uint16_t)pcap_datalink(descr); //DLT_EN10MB==Ethernet

    //Descriptor del fichero de salida pcap para debugging
    descr2=pcap_open_dead(datalink,MTU+ETH_HLEN);
    pdumper=pcap_dump_open(descr2,fichero_pcap_destino);

    //Formamos y enviamos el trafico, debe enviarse un unico segmento por llamada a enviar() aunque luego se traduzca en mas de un datagrama
    //Primero, un paquete ICMP; en concreto, un ping
    pila_protocolos[0]=ICMP_PROTO; pila_protocolos[1]=IP_PROTO; pila_protocolos[2]=0;
    Parametros parametros_icmp; parametros_icmp.tipo=PING_TIPO; parametros_icmp.codigo=PING_CODE; parametros_icmp.bit_DF=flag_dontfrag; memcpy(parametros_icmp.IP_destino,IP_destino_red,IP_ALEN);
    if(enviar((uint8_t*)ICMP_DATA,strlen(ICMP_DATA),pila_protocolos,&parametros_icmp)==ERROR ){
        printf("Error: enviar(): %s %s %d.\n",errbuf,__FILE__,__LINE__);
        return ERROR;
    }
    else    cont++;
    printf("Enviado mensaje %"PRIu64", ICMP almacenado en %s\n\n", cont,fichero_pcap_destino);

    //Luego, un paquete UDP
    //Definimos la pila de protocolos que queremos seguir
    pila_protocolos[0]=UDP_PROTO; pila_protocolos[1]=IP_PROTO; pila_protocolos[2]=ETH_PROTO;
    //Rellenamos los parametros necesario para enviar el paquete a su destinatario y proceso
    Parametros parametros_udp; memcpy(parametros_udp.IP_destino,IP_destino_red,IP_ALEN); parametros_udp.bit_DF=flag_dontfrag; parametros_udp.puerto_destino=puerto_destino;
    //Enviamos
	if(enviar((uint8_t*)data,bytes,pila_protocolos,&parametros_udp)==ERROR ){
		printf("Error: enviar(): %s %s %d.\n",errbuf,__FILE__,__LINE__);
		return ERROR;
	}
	else    cont++;

    printf("Enviado mensaje %"PRIu64", almacenado en %s\n\n\n", cont,fichero_pcap_destino);

        //Cerramos descriptores
    pcap_close(descr);
    pcap_dump_close(pdumper);
    pcap_close(descr2);
    return OK;
}


/****************************************************************************************
 * Nombre: enviar                                                                       *
 * Descripcion: Esta funcion envia un mensaje                                           *
 * Argumentos:                                                                          *
 *  -mensaje: mensaje a enviar                                                          *
 *  -pila_protocolos: conjunto de protocolos a seguir                                   *
 *  -longitud: bytes que componen mensaje                                               *
 *  -parametros: parametros necesario para el envio (struct parametros)                 *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t enviar(uint8_t* mensaje, uint32_t longitud,uint16_t* pila_protocolos,void *parametros){
    uint16_t protocolo=pila_protocolos[0];
    printf("Enviar(%"PRIu16") %s %d.\n",protocolo,__FILE__,__LINE__);
    if(protocolos_registrados[protocolo]==NULL){
        printf("Protocolo %"PRIu16" desconocido\n",protocolo);
        return ERROR;
    }
    else {
        return protocolos_registrados[protocolo](mensaje,longitud,pila_protocolos,parametros);
    }
    return ERROR;
}


/***************************Pila de protocolos a implementar************************************/


/****************************************************************************************
 * Nombre: moduloICMP                                                                   *
 * Descripcion: Esta funcion implementa el modulo de envio ICMP                         *
 * Argumentos:                                                                          *
 *  -mensaje: mensaje a anadir a la cabecera ICMP                                       *
 *  -longitud: bytes que componen el mensaje                                            *
 *  -pila_protocolos: conjunto de protocolos a seguir                                   *
 *  -parametros: parametros necesario para el envio este protocolo                      *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t moduloICMP(uint8_t* mensaje, uint32_t longitud, uint16_t* pila_protocolos, void *parametros){
    uint8_t segmento[ICMP_DATAGRAM_MAX]={0};
    uint8_t aux8, sumacontrol[2];
    uint16_t aux16;
    uint32_t pos=0, checksum;
    uint8_t protocolo_inferior=pila_protocolos[1];
    printf("modulo ICMP(%"PRIu16") %s %d.\n",protocolo_inferior,__FILE__,__LINE__);
    aux8=PING_TIPO;
    memcpy(segmento+pos,&aux8,sizeof(uint8_t));
    pos+=sizeof(uint8_t);
    aux8=0;
    memcpy(segmento+pos,&aux8,sizeof(uint8_t));
    pos+=sizeof(uint8_t);
    aux16=0;
    memcpy(segmento+pos, &aux16, sizeof(uint16_t));
    checksum = pos;
    pos+=sizeof(uint16_t);
    aux16=htons((uint16_t) getpid()>>2);
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    aux16=htons(1);
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    memcpy(segmento+pos,mensaje,longitud * sizeof(uint8_t));
    pos+=longitud;
    calcularChecksum(segmento, pos, sumacontrol);
    memcpy(segmento+checksum, sumacontrol, sizeof(uint16_t));

//Se llama al protocolo definido de nivel inferior a traves de los punteros registrados en la tabla de protocolos registrados
    return protocolos_registrados[protocolo_inferior](segmento,pos,pila_protocolos,parametros);
}


/****************************************************************************************
 * Nombre: moduloUDP                                                                    *
 * Descripcion: Esta funcion implementa el modulo de envio UDP                          *
 * Argumentos:                                                                          *
 *  -mensaje: mensaje a enviar                                                          *
 *  -longitud: bytes que componen mensaje                                               *
 *  -pila_protocolos: conjunto de protocolos a seguir                                   *
 *  -parametros: parametros necesario para el envio este protocolo                      *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t moduloUDP(uint8_t* mensaje, uint32_t longitud, uint16_t* pila_protocolos, void *parametros){
    uint8_t segmento[UDP_SEG_MAX]={0};
    uint16_t puerto_origen = 0;
    uint16_t aux16;
    uint32_t pos=0;
    uint8_t protocolo_inferior=pila_protocolos[1];
    printf("modulo UDP(%"PRIu16") %s %d.\n",protocolo_inferior,__FILE__,__LINE__);

    if (longitud>UDP_SEG_MAX){
        printf("Error: mensaje demasiado grande para UDP (%d).\n",UDP_SEG_MAX);
        return ERROR;
    }

    Parametros udpdatos=*((Parametros*)parametros);
    uint16_t puerto_destino=udpdatos.puerto_destino;
    obtenerPuertoOrigen(&puerto_origen);
    aux16=htons(puerto_origen);
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    aux16=htons(puerto_destino);
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    aux16=htons(8 + longitud);
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    aux16=0;
    memcpy(segmento+pos,&aux16,sizeof(uint16_t));
    pos+=sizeof(uint16_t);
    memcpy(segmento+pos,mensaje,longitud * sizeof(uint8_t));
    pos+=longitud;

//Se llama al protocolo definido de nivel inferior a traves de los punteros registrados en la tabla de protocolos registrados
    return protocolos_registrados[protocolo_inferior](segmento,pos,pila_protocolos,parametros);
}


/****************************************************************************************
 * Nombre: moduloIP                                                                     *
 * Descripcion: Esta funcion implementa el modulo de envio IP                           *
 * Argumentos:                                                                          *
 *  -segmento: segmento a enviar                                                        *
 *  -longitud: bytes que componen el segmento                                           *
 *  -pila_protocolos: conjunto de protocolos a seguir                                   *
 *  -parametros: parametros necesario para el envio este protocolo                      *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t moduloIP(uint8_t* segmento, uint32_t longitud, uint16_t* pila_protocolos, void *parametros){
    uint8_t datagrama[IP_DATAGRAM_MAX]={0};
    int enviados;
    uint16_t aux16, packsize, tlen, position, offset;
    uint8_t aux8, sumacontrol[2], retorno;
    uint32_t pos=0,pos_control=0, n_frags;
    uint8_t IP_origen[IP_ALEN];
    uint8_t protocolo_superior=pila_protocolos[0];
    uint8_t protocolo_inferior=pila_protocolos[2];
    pila_protocolos++;
    uint8_t mascara[IP_ALEN];
    uint8_t arp_target[IP_ALEN];
    uint8_t mac_dest[ETH_ALEN] /*mac_src[ETH_ALEN]*/;
    uint8_t *p;
    static uint16_t identificacion = 0;
    printf("modulo IP(%"PRIu16") %s %d.\n",protocolo_inferior,__FILE__,__LINE__);

    Parametros ipdatos=*((Parametros*)parametros);
    uint8_t* IP_destino=ipdatos.IP_destino;
    //Parametros *param = parametros;
    obtenerIPInterface(interface, IP_origen);
    obtenerMascaraInterface(interface, mascara);
    if(mismaSubred(IP_origen, IP_destino, mascara, IP_ALEN) == ERROR){
        obtenerGateway(interface, arp_target);
    }else{
        for(int i = 0; i<IP_ALEN; ++i){
            arp_target[i]=IP_destino[i];
        }
    }
    if(ERROR == solicitudARP(interface, arp_target, mac_dest)){
        fprintf(stderr, "Se ha intentado autoenviar un paquete. %s %d\n",__FILE__,__LINE__);
        return ERROR;
    }
    obtenerMTUInterface(interface, &aux16);
    packsize = (aux16-20)&(0xFFF8);
    for (int i=0; i<ETH_ALEN; ++i) ((Parametros*) parametros)->ETH_destino[i] = mac_dest[i];
    if(longitud  > packsize && ((Parametros*) parametros)->bit_DF == 1){
        fprintf(stderr, "Fragmentación no permitida y tamaño de datos > MTU. %s %d\n",__FILE__,__LINE__);
        return ERROR;
    }
    n_frags = ceil(longitud/(double)packsize);
    for(p = segmento, enviados = 0; enviados < n_frags; enviados++, p+=packsize, pos=0){
        offset = (packsize*enviados)>>3;
        if(enviados == n_frags-1){ //Último fragmento
            tlen = longitud - enviados*packsize + 20;
            if(((Parametros*) parametros)->bit_DF == 1)//No fragmentar
                position = htons(0x4000 | offset);
            else
                position = htons(offset);
        }else{ //Fragmentos menos el ultimo
            tlen = packsize + 20;
            position = htons(0x2000 | offset);
        }
        aux8 = 0x45;
        memcpy(datagrama+pos, &aux8, 1);               //ipver_ihl
        pos+=1;
        aux8 = 0;
        memcpy(datagrama+pos, &aux8, 1);               //tos
        pos+=1;
        aux16=htons(tlen);
        memcpy(datagrama+pos, &aux16, 2);               //longitud total
        pos+=2;
        aux16 = htons(identificacion);
        memcpy(datagrama+pos, &aux16, 2);              //identificacion
        pos+=2;
        memcpy(datagrama+pos, &position, 2);           //posicion
        pos+=2;
        aux8=64;
        memcpy(datagrama+pos, &aux8, 1);               //TTS
        pos+=1;
        memcpy(datagrama+pos, &protocolo_superior, 1); //protocolo
        pos+=1;
        aux16 = 0;
        memcpy(datagrama+pos, &aux16, 2);              //checksum placeholder
        pos_control = pos;
        pos+=2;
        memcpy(datagrama+pos, IP_origen, IP_ALEN);     //ip origen
        pos+=4;
        memcpy(datagrama+pos, IP_destino, IP_ALEN);    //ip destino
        pos+=4;
        calcularChecksum(datagrama, pos, sumacontrol);
        memcpy(datagrama+pos_control, sumacontrol, 2); //checksum
        memcpy(datagrama+pos, p, tlen-20); //Segment
        retorno = protocolos_registrados[protocolo_inferior](datagrama,tlen,pila_protocolos,parametros);
    }
    identificacion++;
    return retorno;
}

/****************************************************************************************
 * Nombre: moduloETH                                                                    *
 * Descripcion: Esta funcion implementa el modulo de envio Ethernet                     *
 * Argumentos:                                                                          *
*  -datagrama: datagrama a enviar                                                      *
 *  -longitud: bytes que componen el datagrama                                          *
 *  -pila_protocolos: conjunto de protocolos a seguir                                   *
 *  -parametros: Parametros necesario para el envio este protocolo                      *
 * Retorno: OK/ERROR                                                                    *
****************************************************************************************/

uint8_t moduloETH(uint8_t* datagrama, uint32_t longitud, uint16_t* pila_protocolos,void *parametros){
//Variables del modulo
    struct timespec hora;
    uint8_t eth_origen[ETH_ALEN];
    uint8_t trama[ETH_FRAME_MAX]={0};
    uint16_t aux16;
    uint32_t pos = 0;
    Parametros ethdatos=*((Parametros*)parametros);
    uint8_t* eth_destino = ethdatos.ETH_destino;
    obtenerMACdeInterface(interface, eth_origen);

    printf("modulo ETH(fisica) %s %d.\n",__FILE__,__LINE__);

//Control de tamano
    obtenerMTUInterface(interface, &aux16);
    if(longitud > aux16){
        fprintf(stderr,"El tamaño del paquete es mayor que el tamaño de la MTU. %s %d\n", __FILE__,__LINE__);
        return ERROR;
    }

//Cabecera del modulo
    memcpy(trama+pos, eth_destino, ETH_ALEN*sizeof(uint8_t));
    pos+=ETH_ALEN;
    memcpy(trama+pos, eth_origen, ETH_ALEN*sizeof(uint8_t));
    pos+=ETH_ALEN;
    aux16 = htons(0x0800);
    memcpy(trama+pos, &aux16, sizeof(uint16_t));
    pos+=ETH_TLEN;
    memcpy(trama+pos, datagrama, longitud);

//Enviar a capa fisica

    if(PCAP_ERROR == pcap_inject(descr, trama, ETH_HLEN+longitud)){
        fprintf(stderr, "Injecting went wrong.\n");
        return ERROR;
    }

    if(flag_mostrar){
        mostrarHex(trama, ETH_HLEN+longitud);
    }

//Almacenamos la salida por cuestiones de debugging
    clock_gettime(CLOCK_REALTIME, &hora);
    struct pcap_pkthdr header;
    header.ts.tv_sec = hora.tv_sec;
    header.ts.tv_usec = (unsigned long)hora.tv_nsec*1000;
    header.caplen = ETH_HLEN+longitud;
    header.len = ETH_HLEN+longitud;
    pcap_dump((uint8_t *)pdumper,&header,trama);

    return OK;
}



/***************************Funciones auxiliares a implementar***********************************/

/****************************************************************************************
 * Nombre: aplicarMascara                                                               *
 * Descripcion: Esta funcion aplica una mascara a una vector                            *
 * Argumentos:                                                                          *
 *  -IP: IP a la que aplicar la mascara en orden de red                                 *
 *  -mascara: mascara a aplicar en orden de red                                         *
 *  -longitud: bytes que componen la direccion (IPv4 == 4)                              *
 *  -resultado: Resultados de aplicar mascara en IP en orden red                        *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t aplicarMascara(uint8_t* IP, uint8_t* mascara, uint8_t longitud, uint8_t* resultado){
    uint8_t i;
    if(longitud != 4 || !IP || !mascara || !resultado) return ERROR;
    for(i=0; i<longitud;++i) resultado[i]=IP[i]&mascara[i];
    return OK;
}


/***************************Funciones auxiliares implementadas**************************************/

/****************************************************************************************
 * Nombre: mostrarHex                                                                   *
 * Descripcion: Esta funcion imprime por pantalla en hexadecimal un vector              *
 * Argumentos:                                                                          *
 *  -datos: bytes que conforman un mensaje                                              *
 *  -longitud: Bytes que componen el mensaje                                            *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t mostrarHex(uint8_t * datos, uint32_t longitud){
    uint32_t i;
    printf("Datos:\n");
    for (i=0;i<longitud;i++){
        printf("%02"PRIx8" ", datos[i]);
    }
    printf("\n");
    return OK;
}


/****************************************************************************************
 * Nombre: calcularChecksum                                                             *
 * Descripcion: Esta funcion devuelve el ckecksum tal como lo calcula IP/ICMP           *
 * Argumentos:                                                                          *
 *   -datos: datos sobre los que calcular el checksum                                   *
 *   -longitud: numero de bytes de los datos sobre los que calcular el checksum         *
 *   -checksum: checksum de los datos (2 bytes) en orden de red!                        *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t calcularChecksum(uint8_t *datos, uint16_t longitud, uint8_t *checksum) {
    uint16_t word16;
    uint32_t sum=0;
    int i;
    // make 16 bit words out of every two adjacent 8 bit words in the packet
    // and add them up
    for (i=0; i<longitud; i=i+2){
        word16 = (datos[i]<<8) + datos[i+1];
        sum += (uint32_t)word16;
    }
    // take only 16 bits out of the 32 bit sum and add up the carries
    while (sum>>16) {
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    // one's complement the result
    sum = ~sum;
    checksum[0] = sum >> 8;
    checksum[1] = sum & 0xFF;
    return OK;
}


/***************************Funciones inicializacion implementadas*********************************/

/****************************************************************************************
 * Nombre: inicializarPilaEnviar                                                        *
 * Descripcion: inicializar la pila de red para enviar registrando los distintos modulos*
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t inicializarPilaEnviar() {
    bzero(protocolos_registrados,MAX_PROTOCOL*sizeof(pf_notificacion));
    if(registrarProtocolo(ETH_PROTO, moduloETH, protocolos_registrados)==ERROR)
        return ERROR;
    if(registrarProtocolo(IP_PROTO, moduloIP, protocolos_registrados)==ERROR)
        return ERROR;
    if(registrarProtocolo(ICMP_PROTO, moduloICMP, protocolos_registrados)==ERROR)
        return ERROR;
    if(registrarProtocolo(UDP_PROTO, moduloUDP, protocolos_registrados)==ERROR)
        return ERROR;
    return OK;
}


/****************************************************************************************
 * Nombre: registrarProtocolo                                                           *
 * Descripcion: Registra un protocolo en la tabla de protocolos                         *
 * Argumentos:                                                                          *
 *  -protocolo: Referencia del protocolo (ver RFC 1700)                                 *
 *  -handleModule: Funcion a llamar con los datos a enviar                              *
 *  -protocolos_registrados: vector de funciones registradas                            *
 * Retorno: OK/ERROR                                                                    *
 ****************************************************************************************/

uint8_t registrarProtocolo(uint16_t protocolo, pf_notificacion handleModule, pf_notificacion* protocolos_registrados){
    if(protocolos_registrados==NULL ||  handleModule==NULL){
        printf("Error: registrarProtocolo(): entradas nulas.\n");
        return ERROR;
    }
    else
        protocolos_registrados[protocolo]=handleModule;
    return OK;
}

/****************************************************************************************
 * Nombre: mismaSubred                                                                  *
 * Descripcion: Detecta si dos ips están en la misma subred o no                        *
 * Argumentos:                                                                          *
 *  -ip1 Primera ip                                                                     *
 *  -ip2 Segunda ip                                                                     *
 * Retorno: OK/ERROR                                                                  *
 ****************************************************************************************/

uint8_t mismaSubred(uint8_t *ip1, uint8_t *ip2, uint8_t* mascara, uint8_t longitud){
    for(int i=0; i<longitud;++i){
        if((ip1[i]&mascara[i])!=(ip2[i]&mascara[i])){
            return ERROR;
        }
    }
    return OK;
 }
