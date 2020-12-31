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
#define MAX_CADENA 81

int main (int argc , char *argv[]){

	int puerto;
	//En este caso solo se puede definir el numero de puerto en el que va a estar escuchando el servidor
	if (argc == 3){
		//Comprobamos que si se introducen 2 argumentos el primero sea -p
		if(strcmp(argv[1] , "-p") != 0){
			printf("El primer argumento no es valido \n");
			exit(EXIT_FAILURE);
		}
		sscanf(argv[2] , "%d" , &puerto);
		puerto = htons(puerto);
		
	}else if (argc == 1){
		//Si no hay argumentos es que por defecto se escucha en el puerto 5
		puerto= htons(5);
	}else {
		//Si los argumentos no cumplen las condiciones necesarias se muestra un mensaje de error
		printf("Numero de argumentos invalido \n");
		exit(EXIT_FAILURE);
	}
	
	//Creamos el socket en el que va a estar nuestro servicio escuchando
	int socketServidor = socket(AF_INET , SOCK_DGRAM , 0);
	if (socketServidor < 0 ){
		perror("socket()");
		exit(EXIT_FAILURE);
	}
	
	//Creamos la estructura necesaria para asignarle una direccion IP al socket creado
	//Estaráescuchando en el puerto indicado, por defecto el 5
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = puerto;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	//Se enlaza el socket con la direccion que hemos indicado en la estructura myaddr
	int enlace = bind(socketServidor , (struct sockaddr *) &myaddr , sizeof(myaddr));
	if (enlace < 0){
		perror("bind()");
		exit(EXIT_FAILURE);
		
	}
	
	//Creamos un buffer donde vamos a almacenar la cadena que nos llegue
	char cadenaEntrante[MAX_CADENA];

	struct sockaddr_in direccion_cliente;

	socklen_t tamano = sizeof(cadenaEntrante);
	//Iniciamos un bucle infinito que va a recibir cadenas de caracteres, pasarlas
	//a mayusculas y enviarlas de nuevo a la IP de la que provienen
	while(1){
		//Dejamos el buffer vacio para que no haya caracteres que no se corresponden con lo enviado
		memset(cadenaEntrante , 0 ,sizeof(cadenaEntrante));
		//Recibimos una cadena de un cliente y la almacenamos en nuestro buffer
		int recibe = recvfrom(socketServidor , &cadenaEntrante , sizeof(cadenaEntrante) , 0 ,
			       (struct sockaddr *) &direccion_cliente , &tamano	);
		if(recibe < 0){
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}
		//Recorremos la cadena y pasamos a mayuscula los caracteres que se correspondan a las letras minusculas.
		//Para ello restamos 32 ya que es la diferencia que se indica en la tabla ASCII. 
		int i;
		for(i = 0; i < sizeof(cadenaEntrante) ; i++ ){
			if(cadenaEntrante[i] >= 'a' && cadenaEntrante[i] <= 'z' ){
				cadenaEntrante[i] -= 32;
			}
		}
		//Enviamos la cadena que acabamos de modificar al cliente del que hemos recibido la solicitud.
		int envia = sendto(socketServidor , &cadenaEntrante , sizeof(cadenaEntrante) , 0 ,
			       (struct sockaddr *) &direccion_cliente , sizeof(direccion_cliente));
		if(envia < 0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
	
	}
	
	
	//close(socketServidor);

	return 0;
}
