// Practica 8 , Ivanov Manov Istaliyan

//Incluimos el fichero que contiene las estructuras necesarias para
// crear el datagrama ICMP, incluyendo la cabecera IP.
#include "ip-icmp-ping.h"

// Incluimos el resto de cabeceras que vamos a necesitar.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include <netdb.h>

#include <errno.h>

int main(int argc, char *argv[])
{
    //Comprobamos el numero de argumentos que se introducen.
    //En caso de que el numero sea distinto de 1 o 2 argumentos imprimimos
    // el uso correcto del programa y salimos del mismo.
    if (argc != 3 && argc != 2)
    {
        printf("El numero de argumentos es incorrecto\n");
        printf("El programa se invoca de la siguiente forma :\n");
        printf("./miping direccion-ip [-v] \n");
        exit(EXIT_FAILURE);
    }

    //Inicializamos a 0 la variable que nos va a indicar si estamos en el modo
    // verbose o no.Si esta inicializada a 0 no estamos en modo verbose. Si esta
    // inicilizada a 1 estamos en modo verbose.
    int modoVerbose = 0;

    //En caso de tener 2 argumentos el segundo argumento debe ser -v.Hacemos
    // esa comprobacion y en caso de no ser asi imprimimos la forma correcta
    // de usar el programa y salimos del mismo.
    if (argc == 3)
    {
        if (strcmp(argv[2], "-v") == 0)
        {
            //Si el segundo argumento es correcto asignamos un 1 a la variable
            // que determina si estamos en modo verbose o no.
            modoVerbose = 1;
        }
        else
        {
            printf("El ultimo argumento es incorrecto\n");
            printf("El programa se invoca de la siguiente forma :\n");
            printf("./miping direccion-ip [-v] \n");
            exit(EXIT_FAILURE);
        }
    }

    //Creamos la variable que va a contener la direccion IP del servidor al
    // que enviamos el datagrama.
    char *ipServidor;
    ipServidor = argv[1];

    // Creamos una estructura in_addr donde vamos a guardar la direccion IP
    // del servidor al que enviamos el datagrama en network byte order.
    struct in_addr addr;

    //Transformamos la direccion IP a network byte order y la guardamos en
    // la estructura creada en la linea anterior.
    if (inet_aton(ipServidor, &addr) == 0)
    {   
        //En caso de que la operacion haya devuelto un 0, que significa que
        //la ip es invalida, imprimimos un mensaje de error e indicamos la 
        // forma correcta de usar el programa antes de salir del programa.

        printf("La direccion IP es incorrecta\n");
        printf("El programa se invoca de la siguiente forma :\n");
        printf("./miping direccion-ip [-v] \n");
        perror("inet_aton()");
        exit(EXIT_FAILURE);
    }


    //Creamos las estructuras necesarias para guardar las direcciones del cliente y del
    // servidor al que vamos a mandar el datagrama, asi como otra informacion relevante
    // como la familia de protocolos o el puerto.
    //Estas estructuras seran usadas en el sendto() y recvfrom() mas adelante.
    struct sockaddr_in direccionCliente;
    direccionCliente.sin_family = AF_INET;
    direccionCliente.sin_addr.s_addr = INADDR_ANY;

    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = 0;
    direccionServidor.sin_addr.s_addr = addr.s_addr;

    // Creamos el descriptor del socket para poder enviar el datagrama.Importante 
    // fijarse en el cambio a SOCK_RAW (nos indica que este es un socket IP) y el
    // cambio a IPPROTO_ICMP que nos indica que usamos el protocolo ICMP.
    int sockfd;
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    //Enlazamos el socket que acabamos de crear con la direccion del cliente desde el que 
    // enviamos el datagrama.
    int enlaceSocket;
    enlaceSocket = bind(sockfd, (struct sockaddr *)&direccionCliente, sizeof(direccionCliente));
    if (enlaceSocket < 0)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    //Creamos una peticion y una respuesta con las estructuras proporcionadas por 
    // el profesor.
    ECHORequest echoRequest;
    ECHOResponse echoResponse;
    
    //Si tenemos activado el modo verbose impimimos un mensaje para indicar que se
    // ha creado la estructura para la cabecera ICMP.
    if (modoVerbose == 1)
    {
        printf("-> Generando cabecera ICMP.\n");
    }

    //Definimos el payload de nuestro datagrama.
    char *payload = "Este es nuestro payload.";

    //Inicializamos todos los campos con los valores requeridos. El campo checksum
    // lo dejamos de momento a 0 ya que el checksum se calcula sobre el datagrama ya
    // creado y se sustituye por el 0.Por ultimo uncluimos el payload que hemos declarado
    // en la linea anterior.
    echoRequest.icmpHeader.Type = 8;
    echoRequest.icmpHeader.Code = 0;
    echoRequest.icmpHeader.Checksum = 0;
    echoRequest.ID = getpid();
    echoRequest.SeqNumber = 0;
    strcpy(echoRequest.payload, payload);

    // Seguimos el algoritmo para calcular el checksum.
    // Definomos el tamaño en half words del datagrama. Para ello dividimos entre 2
    // el tamaño del mismo. Definimos un acumulador y un puntero que apunta al inicio
    // del datagrama.
    int numShorts = sizeof(echoRequest) / 2;
    unsigned short int *puntero;
    unsigned int acumulador = 0;

    puntero = (unsigned short int *)&echoRequest;

    //Hacemos la suma de todos los elementos de 16 bits del datagrama en una variable
    // de 32 bits. Vamos guardando los valores en el acumulador y avanzando en una
    // posicion el punter.
    int i;
    for (i = 0; i < (numShorts - 1); i++)
    {
        acumulador = acumulador + (unsigned int)*puntero;
        puntero++;
    }

    // Por ultimo para que la suma sea correcta sumamos la parte alta del acumulador a
    // la parte baja 2 vecesa ya que la acumulacion del acarreo a partir del byte 3 
    // puede producir un nuevo acarreo que tambien hay que suma.
    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    //Por ultimo hacemos un not del resultado para obtener el complemento a 1 de los
    // 16 bits de menos peso.
    acumulador = ~acumulador;

    //Guardamos los 16 bits de menor peso en el checksum.
    echoRequest.icmpHeader.Checksum = acumulador & 0x0000ffff;

    //En caso de tener activada la opcion verbose imprimimos por pantalla rodos los
    // campos requeridos.
    if (modoVerbose == 1)
    {
        printf("-> Type: %d \n", echoRequest.icmpHeader.Type);
        printf("-> Code: %d \n", echoRequest.icmpHeader.Code);
        printf("-> Identifier (pid): %d \n", echoRequest.ID);
        printf("-> Seq. number: %d \n", echoRequest.SeqNumber);
        printf("-> Cadena a enviar: %s \n", echoRequest.payload);
        printf("-> Checksum: %x \n", echoRequest.icmpHeader.Checksum);
        printf("-> Tamaño total del paquete ICMP: %li \n", sizeof(echoRequest));
    }


    //A continuacion se deja comentada la seccion donde se hace la comprobacion 
    // de si se ha hecho bien el checksum. En caso de que sea asi la segunda vez que
    // calculamos el checksum este tiene que tener un valor de 0.
    /*
    printf("Valor del checksum la primera vez que lo calculamos: %d \n", echoRequest.icmpHeader.Checksum);

    
    puntero = (unsigned short int *)&echoRequest;   
    acumulador = 0;

    for (i = 0; i < (numShorts - 1); i++)
    {
        acumulador = acumulador + (unsigned int)*puntero;
        puntero++;
    }

    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador = ~acumulador;

    //Guardamos los 16 bits de menor peso en el checksum.

    unsigned short int checksum = acumulador & 0x0000ffff;

    printf("Valor del checksum la segunda vez que lo calculamos: %d \n", checksum);
    */

    //Enviamos al servidor que hemos indicado el datagrama ICMP que se acaba de crear.
    int envio;
    envio = sendto(sockfd, &echoRequest, sizeof(echoRequest), 0,
                   (struct sockaddr *)&direccionServidor, sizeof(direccionServidor));
    if (envio < 0)
    {
        perror("envio()");
        exit(EXIT_FAILURE);
    }

    //Mostramos un mensaje indicando que se ha enviado el datagrama y la direccion
    // IP a la que lo hemos enviado.
    printf("Paquete ICMP enviado a %s \n", ipServidor);

    //Recibimos la respuesta del servidor al que hemos enviado el datagrama.
    socklen_t tamano = sizeof(echoResponse);
    int respuesta;
    respuesta = recvfrom(sockfd, &echoResponse, sizeof(echoResponse), 0,
                         (struct sockaddr *)&direccionServidor, &tamano);
    if (respuesta < 0)
    {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    //Impimimos por pantalla un mensaje indicando que hemos recibido el datagrama
    // y la direccion de donde lo recibimos.
    printf("Respuesta recibida desde %s.\n", ipServidor);

    //En caso de tener activado el modo verbose impimimos por pantalla los campos
    // requeridos del cuerpo de la respuesta.
    if (modoVerbose == 1)
    {
        printf("-> Tamaño de la respuesta : %ld \n", sizeof(echoResponse));
        printf("-> Cadena recibida: %s \n", echoResponse.payload);
        printf("-> Identifier (pid): %d \n", echoResponse.ID);
        printf("-> TTL : %d \n", echoResponse.ipHeader.TTL);
    }

    // Una vez obtenida la respuesta hacemos un switch donde dependiendo del tipo
    // de error y el codigo impimimos el mensaje correspondiente basandonos en los
    // codigos de error de ICMP de la pagina de la Wikipedia.
    switch (echoResponse.icmpHeader.Type)
    {
    case 0:
        printf("Descripcion de la respuesta: Respuesta correcta : ");
        break;
    case 3:
        switch (echoResponse.icmpHeader.Code)
        {
            printf("Descripcion de la respuesta: Destination Unreachable : ");
        case 0:
            printf("Destination network unreachable ");
            break;
        case 1:
            printf("Destination host unreachable ");
            break;
        case 2:
            printf("Destination protocol unreachable ");

        case 3:
            printf("Destination port unreachable ");

        case 4:
            printf("Fragmentation required, and DF flag set ");

        case 5:
            printf("Source route failed ");

        case 6:
            printf("Destination network unknown ");
            break;
        case 7:
            printf("Destination host unknown ");
            break;
        case 8:
            printf("Source host isolated ");

        case 9:
            printf("Network administratively prohibited ");

        case 10:
            printf("Host administratively prohibited ");

        case 11:
            printf("Network unreachable for ToS ");

        case 12:
            printf("Host unreachable for ToS ");

        case 13:
            printf("Communication administratively prohibited ");
            break;
        case 14:
            printf("Host Precedence Violation ");
            break;
        case 15:
            printf("Precedence cutoff in effect ");
        }
        break;

    case 5:

        switch (echoResponse.icmpHeader.Code)
        {
            printf("Descripcion de la respuesta: Redirect Message :");
        case 0:
            printf("Redirect Datagram for the Network ");
            break;
        case 1:
            printf("Redirect Datagram for the Host ");
            break;
        case 2:
            printf("Redirect Datagram for the ToS & network ");
            break;
        case 3:
            printf("Redirect Datagram for the ToS & host ");
            break;
        }
        break;

    case 8:

        printf("Descripcion de la respuesta: Echo Request : Echo request (used to ping)");
        break;

    case 9:

        printf("Descripcion de la respuesta: Router Advertisement : Router Advertisement ");
        break;

    case 10:

        printf("Descripcion de la respuesta: Router Solicitation : Router discovery/selection/solicitation ");
        break;

    case 11:

        switch (echoResponse.icmpHeader.Code)
        {
            printf("Descripcion de la respuesta: Time Exceeded :");
        case 0:
            printf("TTL expired in transit ");
            break;
        case 1:
            printf("Fragment reassembly time exceeded ");
            break;
        }
        break;

    case 12:
        switch (echoResponse.icmpHeader.Code)
        {
            printf("Descripcion de la respuesta: Parameter Problem: Bad IP header : ");
        case 0:
            printf("Pointer indicates the error ");
            break;
        case 1:
            printf("Missing a required option ");
            break;
        case 2:
            printf("Bad length ");
            break;
        }
        break;

    case 13:

        printf("Descripcion de la respuesta: Timestamp ");
        break;

    case 14:

        printf("Descripcion de la respuesta: Timestamp Reply ");
        break;

    case 40:

        printf("Descripcion de la respuesta: Photuris, Security failures ");
        break;

    case 42:

        printf("Descripcion de la respuesta: Extended Echo Request ");
        break;

    case 43:
        switch (echoResponse.icmpHeader.Code)
        {
            printf("Descripcion de la respuesta: Extended Echo Reply : ");
        case 0:
            printf("No Error ");
            break;
        case 1:
            printf("Malformed Query ");
            break;
        case 2:
            printf("No Such Interface ");
            break;
        case 3:
            printf("No Such Table Entry ");
            break;
        case 4:
            printf("Multiple Interfaces Satisfy Query ");
            break;
        }
        break;
    }

    //Por ultimos ademas del mensaje impimimos el tipo y el codigo que se corresponden con ese mensaje y salimos del programa.
    printf("(type %d, code %d)\n", echoResponse.icmpHeader.Type, echoResponse.icmpHeader.Code);

    return 0;
}