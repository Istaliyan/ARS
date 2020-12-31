//Practica tema 6 , Ivanov Manov Istaliyan


//Incluimos todas la cabeceras que vamos a necesitar.
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
#include <signal.h>
//Definimos el maximo de la cadena que vamos a enviar
#define MAX_CADENA 81
//Definimos el maximo de solicitudes pendientes que vamos a 
#define MAX_SOL_PENDIENTES 5
//Declaramos las variables para los sockets que usaremos despues.
//En el caso de socketServidor es necesario declararlo como variable global
//para poder ser usado en la funcion signal_handler.
int socketServidor , socketServidorHijo;

//Inicializamos la funcion signal_handler que nos va a permitir cerrar
//adecuadamente la conexion.En caso de que el programa reciba una señal
//SIGINT se llamara a esta funcion para cerrar adecuadamente el socket
//del proceso padre,de forma que podamos volver a iniciar el servidor 
//inmediatamente.
void signal_handler(){

	//Cerramos la conexion
	int cerrarConexion = shutdown(socketServidor , SHUT_RDWR);
	if( cerrarConexion < 0){
		perror("shutdownPadre()");
		exit(EXIT_FAILURE);
	}
	//Cerramos el socket del proceso padre
	int cerrarSocket = close(socketServidor);
	if (cerrarSocket < 0){
		perror("close()");
		exit(EXIT_FAILURE);
	}
	//Salimos con exito del programa
	exit(EXIT_SUCCESS);

}

int main (int argc , char *argv[]){

    signal(SIGINT , signal_handler);

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
	//En este caso como vamos a usar TCP debemos cambiar el segundo argumento a SOCK_STREAM.
	socketServidor = socket(AF_INET , SOCK_STREAM , 0);
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

	//Marcamos el socket como un socket pasivo,el cual vamos a usar
	//para aceptar las conexiones entrantes. Indicamos tambien el 
	//numero maximo de solicitudes que vamos a poder dejar en cola.
    int escucha = listen(socketServidor , MAX_SOL_PENDIENTES);
    if(escucha < 0){
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    

	//Empieza el bucle while.Dentro de este vamos a ir aceptando las 
	//conexiones entrantes y para cada conexion crearemos un hilo que
	//se encargara de recibir la cadena que envia el cliente, transformarla y
	//reenviarla al cliente.
    while(1){
		//Definimos una estructura vacia que se rellenara con la informacion del cliente.
        struct sockaddr_in direccion_cliente;
		//Definimos el tamaño de dicha estructura.
		socklen_t tamano = sizeof(direccion_cliente);
        
		//Creamos un array de char que va a contener la cadena en mayusculas.
        char cadenaEntrante[MAX_CADENA];


		//Aceptamos la conexion de un cliente
        int acepta = accept(socketServidor , (struct sockaddr *) &direccion_cliente , &tamano);
        if (acepta < 0){
            perror("accept()");
            exit(EXIT_FAILURE);
        }
		//El socket del proceso hijo va a ser el asignado por el accept que hemos usado
		//anteriormente.Por tanto asignamos el resultado a la variable que hemos declarado
		//antes.
        socketServidorHijo = acepta;

		//Hacemos un fork despues de haber aceptado la conexion del cliente.
		//De esta forma es el proceso hijo el que atiende la peticion del cliente
		//y 
        int hijo = fork();

        if (hijo == 0){
            //Esta parte la va a ejecutar el proceso hijo
            
            //Hacemos memset para que el array que vayamos a devolver contenga exclusivamente
			//la cadena en mayusculas.
            memset(cadenaEntrante , 0 ,sizeof(cadenaEntrante));
			
			//Recibimos la cadena del cliente y la dejamos en el array que hemos creado antes. 
            int recibe = recv(socketServidorHijo , &cadenaEntrante , sizeof(cadenaEntrante) , 0);
            if (recibe < 0){
                perror("recv()");
                exit(EXIT_FAILURE);
            }

			//Recorremos la cadena y vamos pasando cada caracter a mayusculas.
            int i;
            for(i = 0; i < sizeof(cadenaEntrante) ; i++ ){
                if(cadenaEntrante[i] >= 'a' && cadenaEntrante[i] <= 'z' ){
                    cadenaEntrante[i] -= 32;
                }
            }

			//Enviamos la cadena de vuelta al cliente
            int envia = send(socketServidorHijo , &cadenaEntrante , sizeof(cadenaEntrante) , 0);
            if (envia < 0){
                perror("send()");
                exit(EXIT_FAILURE);
            }

			//Cerramos la conexion del hijo con el cliente
            int cerrarConexionHijo = shutdown(socketServidorHijo , SHUT_RDWR);
            if (cerrarConexionHijo < 0){
                perror("shutdownHijo()");
                exit(EXIT_FAILURE);
            }
			
			//Cerramos el socket del hijo.
            int cerrarHijo = close(socketServidorHijo);
            if (cerrarHijo < 0){
                perror("close()");
                exit(EXIT_FAILURE);
            }

			exit(EXIT_SUCCESS);



        }else{
            //Esta parte la va a ejecutar el proceso padre
        }

    }
    
	return 0;
}
