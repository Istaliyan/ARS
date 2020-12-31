//Practica tema 7 , Ivanov Manov Istaliyan

//Incluimos las cabeceras que vamos a necesitar
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

#define MAX_NOMBRE_FICHERO 101

int main(int argc, char *argv[])
{

    //Definimos 3 variables para indicar que funcionalidad del programa queremos
    //usar y una ultima variable para guardar el nombre del fichero que queremos
    //leer o escribir.
    int procesoLectura;
    int procesoEscritura;
    int modoVerbose;
    char nombreFichero[MAX_NOMBRE_FICHERO];

    //Como el programa puede tener 3 o 4 argumentos en funcion de si queremos
    // el modo verbose, hacemos una comprobacion del numero de argumentos.
    //En caso de no tener el numero de argumentos requerido se imprime
    //un mensaje de error
    if (argc != 5 && argc != 4)
    {
        printf("El numero de argumentos es incorrecto.\n");
        printf("El programa se invoca de la siguiente manera\n");
        printf("tftp-client ip-servidor {-r|-w} archivo [-v]\n");
        exit(EXIT_FAILURE);
    }
    else
    {

        //Comprobamos cual es el segundo argumento y dependiendo del que sea
        //inicializamos la variable para leer o escribir.En caso de no tener
        //los argumentos requeridos se imprime el formato requerido y se sale
        //programa.
        if (strcmp(argv[2], "-r") == 0)
        {
            procesoLectura = 1;
        }
        else if (strcmp(argv[2], "-w") == 0)
        {
            procesoEscritura = 1;
        }
        else
        {
            printf("El segundo argumento es incorrecto");
            printf("El programa se invoca de la siguiente manera\n");
            printf("tftp-client ip-servidor {-r|-w} archivo [-v]\n");
            exit(EXIT_FAILURE);
        }
    }

    if (argc == 5)
    {

        //Comprobamos el ultimo argumento
        if (strcmp(argv[4], "-v") == 0)
        {
            modoVerbose = 1;
        }
        else
        {
            printf("El cuarto argumento es incorrecto");
            printf("El programa se invoca de la siguiente manera\n");
            printf("tftp-client ip-servidor {-r|-w} archivo [-v]\n");
            exit(EXIT_FAILURE);
        }
    }

    //Guardamos la ip y el nombre del fichero que se ha introducido.
    char *ipServidor = argv[1];
    strncpy(nombreFichero, argv[3], 101);

    //Creamos las estructuras necesarias para almacenar las direcciones del cliente
    // y del servidor.Usaremos estas estructuras para hacer los sendto() y recvfrom()
    // necesarios
    struct sockaddr_in direccionCliente;
    direccionCliente.sin_family = AF_INET;
    direccionCliente.sin_addr.s_addr = INADDR_ANY;

    //Usamos getservbyname para buscar el puerto con el que tenemos que comunicarnos.
    struct servent *servicioServidor = getservbyname("tftp", "udp");
    int puerto = servicioServidor->s_port;

    //Usamos inet_aton para convertir la cadena con la direccion ip
    // a un numero de 32 bits en network byte order y guardarlo en
    // una estructura in_addr.
    struct in_addr addr;
    if (inet_aton(ipServidor, &addr) == 0)
    {
        perror("inet_aton()");
        exit(EXIT_FAILURE);
    }

    //Creamos la estructura para la direccion del servidor. Inicializamos los valores
    // de la familia del protocolo, el puerto y la direccion IP.
    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = puerto;
    direccionServidor.sin_addr.s_addr = addr.s_addr;

    //Creamos el socket que vamos a usar para establecer la comunicacion
    // entre el cliente y el servidor.Indicamos que es un socket UDP.
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    //Enlazamos el socket a la direccion del cliente usando la estructura
    // que hemos creado antes.
    int enlaceSocket;
    enlaceSocket = bind(sockfd, (struct sockaddr *)&direccionCliente, sizeof(direccionCliente));
    if (enlaceSocket < 0)
    {
        perror("enlaceSocket");
        exit(EXIT_FAILURE);
    }

    //Indicamos cual va a ser el modo de transmision de las peticiones
    // de lectura y escritura.
    char *modoTransmision = "octet";

    //Entraremos en este if en caso de que se haya seleccionado que queremos leer un archivo
    if (procesoLectura == 1)
    {
        //Se inicia el proceso de lectura con los parametros indicados.
        //Inicializamos el tamaño del request sumando las longitudes de todos los elementos.
        int tamanoRRQ = 4 + strlen(nombreFichero) + strlen(modoTransmision);

        //Declaramos 2 vectores, uno que va a ser el read request y otro para el ack que
        // tendremos que enviar cada vez que recibimos un bloque.
        unsigned char rrq[tamanoRRQ];
        unsigned char ack[4];

        //Dejamos el vector del request vacio.
        memset(rrq, 0, sizeof(char) * tamanoRRQ);

        //Rellenamos el request con el codigo de operacion,
        // el nombre del fichero que queremos leer, un 0 como separador, un string donde
        // indicamos el modo de transmision y un 0 para indicar el final del request.
        rrq[0] = (unsigned char)0x00;
        rrq[1] = (unsigned char)0x01;

        memcpy(&rrq[2], nombreFichero, strlen(nombreFichero));

        rrq[2 + strlen(nombreFichero)] = (unsigned char)0x00;

        memcpy(&rrq[2 + strlen(nombreFichero) + 1], modoTransmision, strlen(modoTransmision));

        //Una vez creada la peticion se la enviamos al servidor usando la primitiva sendto()
        //ya que usamos UDP. Usamos la estructura que hemos declarado antes para indicar
        // la direccion del servidor con el que queremos comunicarnos.
        int envio;
        envio = sendto(sockfd, rrq, sizeof(rrq), 0,
                       (struct sockaddr *)&direccionServidor, sizeof(direccionServidor));
        if (envio < 0)
        {
            perror("envio()");
            exit(EXIT_FAILURE);
        }

        //En caso de haber activado la opcion verbose impimimos un mensaje donde se indica
        // el nombre del fichero que queremos leer y la direccion del servidor al que hemos
        // enviado la solicitud.
        if (modoVerbose == 1)
        {
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s\n", nombreFichero, ipServidor);
        }

        //Abrimos un fichero e indicamos que queremos escribir en el.
        FILE *fichero = fopen(nombreFichero, "w");
        //En caso de que haya algun problema impimimos un mensaje por pantalla.
        if (fichero == NULL)
        {
            printf("Error al abrir el archivo \n");
        }

        //Declaramos una variable para el numero de bloque que hemos recibido
        // y otra para el numero de bloque que estabamos esperando ya que si
        // no coinciden volveremos a pedir el ultimo bloque de la secuencia.
        //Declaramos una variable para la longitud del datagrama que vamos a
        // recibir ya que en caso de recibir un datagrama de longitud menor a
        // 516 bytes significa que estamos en el ultimo bloque.
        int numeroBloque;
        int numeroBloqueEsperado = 1;
        int numeroBloqueAnterior = 1;
        int longitudDatagrama;
        int ackOPcode = 4;

        //Declaramos un buffer donde vamos a dejar los datos que nos envia el servidor.
        //Lo hacemos suficientemente grande para que podamos dejar en el el bloque que nos
        // envia el servidor.
        char buffer[516];
        socklen_t tamano = sizeof(buffer);

        //Entramos en el bucle que se ejecutara al menos una vez y despues todas las
        // veces necesarias mientras lleguen los bloques del servidor.
        do
        {
            //Usamos la primitiva recvfrom() para recibir los bloques que envia el servidor
            // que hemos especificado en los argumentos. Dejamos el contenido del bloque que
            // llega en el buffer que hemos declarado antes y como recvfrom() devuelve
            // el numero de bytes que ha recibido asignamos ese valor a la variable que
            // habiamos creado para ello.
            longitudDatagrama = recvfrom(sockfd, &buffer, sizeof(buffer), 0,
                                         (struct sockaddr *)&direccionServidor, &tamano);
            if (longitudDatagrama < 0)
            {
                perror("recvfrom()");
                exit(EXIT_FAILURE);
            }

            //En caso de que haya algun error en el servidor TFTP, este envia un datagrama
            // indicando lo que ha ocurrido en un string. Estos datagramas tienen el codigo
            // de operacion 5. Por tanto si recibimos ese codigo de operacion imprimimos el
            //mensaje de error.
            if (buffer[0] == 0x00 && buffer[1] == 0x05)
            {
                printf("El servidor TFTP devuelve el siguiente error : %s \n", &buffer[4]);
            }
            else
            {

                // Como el numero de bloque viene indicado en 2 bytes hacemos la siguiente
                // operacion para obtener el numero de bloque en un entero.
                numeroBloque = 256 * (unsigned char)buffer[2] + (unsigned char)buffer[3];

                //En caso de haber activado el modo verbose se impime en cada iteracion
                // el numero de bloque que hemos recibido.
                if (modoVerbose == 1)
                {
                    printf("Recibido bloque del servidor tftp\n");
                    if (numeroBloque == 1)
                        printf("Es el primer bloque (numero de bloque %d) \n", numeroBloque);
                    else
                        printf("Es el bloque con codigo %d \n", numeroBloque);
                }

                //En caso de que el bloque que recibimos se corresponda con el bloque que
                // esperabamos mandamos el ack del bloque.En caso contrario mandaremos el
                // ack del ultimo bloque recibido.
                if (numeroBloque == numeroBloqueEsperado)
                {
                    ack[0] = ackOPcode / 256;
                    ack[1] = ackOPcode % 256;
                    ack[2] = numeroBloque >> 8;
                    ack[3] = numeroBloque & 0xff;

                    numeroBloqueAnterior = numeroBloque;
                    numeroBloqueEsperado++;
                }
                else
                {
                    ack[0] = ackOPcode / 256;
                    ack[1] = ackOPcode % 256;
                    ack[2] = numeroBloqueAnterior >> 8;
                    ack[3] = numeroBloqueAnterior & 0xff;
                }

                //Enviamos el ack al servidor para indicar el ultimo bloque que hemos recibido.
                // De esta forma el servidor a continuacion envia el bloque con el numero que hemos
                // indicado + 1.
                int envioACK;
                envioACK = sendto(sockfd, ack, sizeof(ack), 0,
                                  (struct sockaddr *)&direccionServidor, sizeof(direccionServidor));
                if (envioACK < 0)
                {
                    perror("envio()");
                    exit(EXIT_FAILURE);
                }
                //Escribimos en el fichero el contenido del buffer donde habiamos guardado
                // los datos recibidos. Usamos un buffer auxiliar con la longitud exacta que
                // necesitamos. Eliminamos el codigo de operacion y el numero de bloque del
                // vector con los datos y escribimos lo obtenido en el fichero.
                char buffaux[512];
                memset(buffaux, 0, 512);
                memcpy(buffaux, &buffer[4], 512);

                fwrite(buffaux, sizeof(char), longitudDatagrama - 4, fichero);
            }

            //Esta operacion se repite hasta llegar al ultimo datagrama que lo identificamos por ser
            // el unico que va a tener una longitud menor a 516 bytes.
        } while (longitudDatagrama == 516);

        //Cerramos el fichero despues de haber terminado de escribir
        fclose(fichero);
        //Cerramos el socket que hemos usado
        close(sockfd);
    }
    //En caso de querer escribir un fichero se ejecuta el codigo del siguiente if.
    else if (procesoEscritura == 1)
    {

        //Inicializamos una variable para indicar la longitud del request para escribir un fichero
        int tamanoWRQ = 4 + strlen(nombreFichero) + strlen(modoTransmision);
        //Declaramos dos vectores, el primero es el request que vamos a mandar al servidor con la
        // longitud que hemos calculado en la linea anterior y el segundo es el vector donde 
        // vamos a recibir el ack o el error del servidor. Este es el motivo por el que el segundo
        // vector tiene una longitud de 516. Se ha usado ese valor porque es el maximo que puede tener
        // un datagrama en TFTP. 
        unsigned char wrq[tamanoWRQ];
        unsigned char ack[516];

        //Rellenamos el request con el codigo de operacion, a continuacion un 0 para indicar la
        // separacion, el nombre del fichero que queremos escribir, un 0 de separacion, el modo
        // de transmision que en este caso vuelve a ser octet.
        wrq[0] = (unsigned char)0x00;
        wrq[1] = (unsigned char)0x02;

        memcpy(&wrq[2], nombreFichero, strlen(nombreFichero));
        wrq[2 + strlen(nombreFichero)] = 0x00;
        memcpy(&wrq[2 + strlen(nombreFichero) + 1], modoTransmision, strlen(modoTransmision));
        wrq[2 + strlen(nombreFichero) + 1 + strlen(modoTransmision)] = 0x00;

        //Enviamos en request para indicar al servidor que queremos escribir un fichero.
        int enviowrq;
        enviowrq = sendto(sockfd, wrq, sizeof(wrq), 0,
                          (struct sockaddr *)&direccionServidor, sizeof(direccionServidor));
        if (enviowrq < 0)
        {
            perror("envio()");
            exit(EXIT_FAILURE);
        }

        //En caso de tener activado el modo verbose mostraremos un mensaje por pantalla
        // para indicar que hemos enviado el request y la direccion ip del servidor
        // al que lo hemos mandado.
        if (modoVerbose == 1)
        {
            printf("Enviada solicitud de escritura de %s a servidor tftp en %s\n", nombreFichero, ipServidor);
        }

        //Recibimos la respuesta del ack y la guardamos en el vector que hemos declarado
        // al entrar en el if
        socklen_t tamanoACK = sizeof(ack);
        int recibirack;
        recibirack = recvfrom(sockfd, &ack, sizeof(ack), 0,
                              (struct sockaddr *)&direccionServidor, &tamanoACK);
        if (recibirack < 0)
        {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }

        //En caso de recibir un error imprimimos un mensaje y salimos de la ejecucion 
        if (ack[0] == 0x00 && ack[1] == 0x05)
        {
            printf("El servidor TFTP devuelve el siguiente error : %s \n", &ack[4]);
            exit(EXIT_FAILURE);
        }
        
        //Abrimos el fichero del que vamos a leer los bloques que vamos a ir enviando
        // al servidor. En caso de que haya un erorr al abrirlo imprimimos un mensaje
        // por pantalla.
        FILE *fichero = fopen(nombreFichero, "r");
        if (fichero == NULL)
        {
            printf("Error al abrir el archivo \n");
            exit(EXIT_FAILURE);
        }

        //Declaramos w variables.La primera para indicar en numero de bytes que hemos leido
        // del fichero y la segunda para indicar cual es el numero de bloque que vamos a enviar.
        int bytesLeidos;
        int numeroBloque = (256 * (unsigned char)ack[2] + (unsigned char)ack[3]) + 1;


        //Hacemos un bucle do while ya que queremos que se ejecute al menos 1 vez,
        // ya que como minimo vamos a enviar un bloque.
        do
        {
            //Declaramos un vector en el que vamos a almacenar los datos leidos del fichero.
            char datos[512];

            //Leemos del archivo que hemos indicado y guardamos el numero de bytes leidos
            // en la variable que hemos creado.
            bytesLeidos = fread(&datos, sizeof(char), 512, fichero);

            //Declaramos un vector que va a contener los bytes leidos mas los 4 bytes necesarios
            // para indicar el numero de bloque y el codigo de operacion.
            char datosEnvio[bytesLeidos + 4];
            memcpy(&datosEnvio[4], &datos, bytesLeidos);

            //Declaramos el codigo de operacion para enviar los datos.
            int datosOPcode = 3;

            
            //Inicializamos las 4 primeras posiciones del vector que contiene los datos leidos
            //con el codigo de operacion y el numero del bloque que vamos a enviar.

            datosEnvio[0] = datosOPcode / 256;
            datosEnvio[1] = datosOPcode % 256;
            datosEnvio[2] = numeroBloque >> 8;
            datosEnvio[3] = numeroBloque & 0xff;

            //Enviamos el vector que acabamos de crear al servidor.
            int envioDatos;
            envioDatos = sendto(sockfd, datosEnvio, sizeof(datosEnvio), 0,
                                (struct sockaddr *)&direccionServidor, sizeof(direccionServidor));
            if (envioDatos < 0)
            {
                perror("envio()");
                exit(EXIT_FAILURE);
            }

            //En caso de haber seleccionado el modo verbose imprimimos una cadena que indica el
            // que hemos enviado el bloque y otra que indica el numero de bloque que se acaba de enviar.
            if (modoVerbose == 1)
            {
                printf("Enviado bloque al servidor TFTP \n");
                printf("Es el bloque con codigo %d \n", numeroBloque);
            }

            //Recibimos el ack del servidor.
            int recibirackDatos;
            recibirackDatos = recvfrom(sockfd, &ack, sizeof(ack), 0,
                                       (struct sockaddr *)&direccionServidor, &tamanoACK);
            if (recibirackDatos < 0)
            {
                perror("recvfrom()");
                exit(EXIT_FAILURE);
            }

            //Actualizamos la variable numeroBloque con el numero de bloque que se nos indica
            // en el ack.
            numeroBloque = (256 * (unsigned char)ack[2] + (unsigned char)ack[3]);

            //Aumentamos el numero de bloque en una unidad para enviar el bloque siguiente del
            // que ha recibido el servidor.
            numeroBloque += 1;

        
        //Este bucle se ejecuta mientras podemos leer 512 bytes del fichero.
        } while (bytesLeidos == 512);

        //Cerramos el fichero y el socket que hemos usado.
        fclose(fichero);
        close(sockfd);
    }

    return 0;
}
