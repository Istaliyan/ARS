// Practica tema 5 , Ivanov Manov Istaliyan

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include <errno.h>

#define MAX_CADENA 80

int main (int argc , char *argv[]){
	
	
	int puerto;
	char cadena[MAX_CADENA];	
	
	//Como hay varias 2 posibles conbinaciones de argumentos comprobamos
	//el numero de argumentos que se introducen.En caso de no ser ninguna
	//de las 2 se imprime un mensaje de error.
	if(argc == 5){
		//Comprobamos que el segundo argumento es -p.Si no lo
		//es se imprime un mensaje de error.
		if(strcmp(argv[2] ,"-p") != 0) {
			printf("El segundo argumento no es valido \n");
			exit(EXIT_FAILURE);
		}
		//Dejamos los argumenentos introducidos en sus variables correspondientes.
		sscanf(argv[3] , "%d" , &puerto);
		//Usamos htons() sobre el puerto para pasarlo a network byte order
		puerto = htons(puerto);
		memset(cadena , 0 , sizeof(cadena));
		strcpy(cadena , argv[4]);

	}else if(argc == 3){	
		//Usamos htons() sobre el puerto para pasarlo a network byte order
		puerto = htons(5);
		memset(cadena , 0 , sizeof(cadena));
		strcpy(cadena , argv[2]);
	
	}else{
		//Imprimimos mensaje de error si el numero de argumentos no es valido
		printf("Numero de argumentos no valido\n");
		exit(EXIT_FAILURE);
	
	}

	
	char *ip;
	ip = argv[1];
	struct in_addr addr;	

	//Pasamos la cadena del argumento al array de chars en formato network byte order
	if(inet_aton(ip , &addr ) == 0){
		//En caso de error imprimimos un mensaje por pantalla
		fprintf(stderr , "Direccion IP invalida \n");
		exit(EXIT_FAILURE);	
	}
		
	//Creamos el socket que va a ser el que nos permita comunicarnos con el servidor
	int socketCliente = socket(AF_INET , SOCK_DGRAM , 0);
	if (socketCliente < 0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	//IniciaÃlizamos la estructura para indicar la dirrecion IP del cliente
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	
	//Enlazamos el socket a una IP y a un puerto
	int enlaceSocket = bind(socketCliente , (struct sockaddr *) &myaddr , sizeof(myaddr));
	if (enlaceSocket < 0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}
	
	//Inicializamos la estructura para indicar el destino de nuestro envio
	struct sockaddr_in destino;
	destino.sin_family = AF_INET;
	destino.sin_port = puerto;
	destino.sin_addr.s_addr = addr.s_addr;

	//Hacemos el envio de la cadena que se nos ha suministrado por argumento 
	int envio = sendto(socketCliente , &cadena , sizeof(cadena) , 0 , (struct sockaddr *) &destino, sizeof(destino) );
	if (envio < 0){
		perror("sendto()");
		exit(EXIT_FAILURE);
	}
	
	//Se usa un caracter mas en la cadena recibida ya que hay que tener en cuenta el
	//caracter de fin de cadena. 
	char cadena_recibida[MAX_CADENA + 1];
	socklen_t tamano = sizeof(cadena_recibida);
	//Recebimos los datos que nos devuelve el servidor y los almacenamos en una cadena
	int recibido = recvfrom(socketCliente , &cadena_recibida , sizeof(cadena_recibida) , 0 , (struct sockaddr *) &destino , &tamano );
       	if(recibido < 0){
       		perror("recvfrom()");
		exit(EXIT_FAILURE);
       	} 

	//Iteramos por la cadena recibida para imprimir la cadena por pantalla	
	int i;
	for(i = 0; i< sizeof(cadena_recibida) ; i++){
		printf("%c" , cadena_recibida[i]);
	}
		printf("\n");

	//Cerramos el socket una vez hemos dejado de utilizarlo
	close(socketCliente);

	
	return 0;

}
