/*
 * main.c
 *
 * TCP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a TCP client
 * portable across Windows, Linux and macOS.
 */

#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

// Stampa un messaggio di errore su stream per errori
void print_error(const char* Messaggio_di_errore){
	fprintf(stderr, "%s", Messaggio_di_errore);
}

int parse_weather_request(const char *input, weather_request_t *request) {
	// CONTROLLI DI SICUREZZA
	// Controllo che i puntatori siano validi e la stringa abbia una lunghezza minima
	if (input == NULL || request == NULL || strlen(input) < 3) {
		return 0; // Errore: input non valido
	    }

	// PARSING DEL TIPO
	request -> type = input[0]; //il primo carattere è sempre il tipo

	// Parto dal secondo carattere (dopo il tipo)
	const char *cursor = input + 1;

	// Salto TUTTI gli spazi, non solo il primo (gestisce "t   Roma")
	while (*cursor == ' ') {
		cursor++;
	    }

	// CONTROLLO CITTÀ VUOTA
	// Se dopo gli spazi sono arrivata alla fine della stringa, manca la città
	if (*cursor == '\0') {
		return 0; // Errore: città mancante
		}

	// Copio la città nel buffer della struct
	strncpy(request -> city, cursor, sizeof(request -> city) - 1);
	request -> city[sizeof(request -> city) - 1] = '\0';

	return 1; // Successo
}

/*
Estrae i dati dalla risposta del server (formato: "status tipo valore")
Parametri:
	input: stringa ricevuta dal server
	response: puntatore alla struttura da riempire
Ritorna: 0 se OK, 1 se errore
*/
int parse_weather_response(const char *input, weather_response_t *response) {
	if (input == NULL || response == NULL)
		return 1;

	    unsigned int status;
	    char type;
	    float value;

	    // Parsing della stringa
	    sscanf(input, "%u %c %f", &status, &type, &value);

	    //riempimento della struttura
	    response->status = status;
	    response->type = type;
	    response->value = value;

	    return 0;  // successo
	}


int main(int argc, char *argv[]) {

	const char* server_address = SERVER_IP;
	int server_port_num = SERVER_PORT;
	const char* request_string = NULL;	//stringa richiesta

	// Parsing degli argomenti da riga di comando
	for(int i = 1; i < argc; ++i) {

		//Gestione Server (-s)
		if(strcmp(argv[i], "-s") == 0) {
			if(i + 1 < argc) {
				server_address = argv[++i];
				continue;
			}
			fprintf(stderr, "Errore: Manca il valore per -s\n");
			return 1;
		}

		//Gestione Porta (-p)
		if(strcmp(argv[i], "-p") == 0) {
			if(i + 1 < argc) {
				int port = atoi(argv[++i]);
				//controllo che la porta sia nel range valido
				if(port <= 0 || port > 65535) {
					fprintf(stderr,"Porta non valida: %d. (range 1-65535)\n", port);
					return 1;
				}
				server_port_num = port;
				continue;
			}
			fprintf(stderr, "Errore: Manca il valore per -p\n");
			return 1;
		}

		//Gestione Richiesta (-r)
		if(strcmp(argv[i], "-r") == 0) {
			if(i + 1 < argc) {
				request_string = argv[++i];
				continue;
			}
			fprintf(stderr, "Errore: Manca il valore per -r\n");
			return 1;
		}

		if (argv[i][0] != '-' && request_string == NULL) {
			request_string = argv[i];
		    continue;
		}

	}
	// verifica che la richiesta sia stata specificata
	if (!request_string){
			fprintf(stderr, "Usa: %s [-s server] [-p port] -r \"type city\"\n", argv[0]);
			return 1;
	}


	#if defined WIN32
	// Imposta output UTF-8 su Windows
	SetConsoleOutputCP(CP_UTF8);

	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		print_error("Error at WSAStartup()\n");
		return 0;
	}
	#endif

	//creazione della socket TCP
	int my_socket;
	my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(my_socket < 0) {
		print_error("creazione socket fallita.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//costruzione dell'indirizzo del server
	struct sockaddr_in server_sockaddr;	//struttura per indirizzo IPv4
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = inet_addr(server_address); //converte un numero in notazione puntata in un numero a 32 bit
	server_sockaddr.sin_port = htons(server_port_num);	//converte le porte in formato Big-Endian

	//connessione al server
	if (connect(my_socket, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) < 0) {
		print_error("Connessione Fallita.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	// Invio della richiesta al server
	//calcola la lunghezza della stringa + il '\0'
	int request_len = (int)strlen(request_string) + 1;
	//inviare dati al server
	if(send(my_socket, request_string, request_len, 0) != request_len) {
		print_error("send()fallita o ha inviato dati parziali.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}
	// Parsing della richiesta per estrarre i dati da visualizzare
	weather_request_t parsed_request;
	parse_weather_request(request_string, &parsed_request);

	// Ricezione della risposta del server
	int received_bytes;
	char recv_buffer[BUFFER_SIZE];
	memset(recv_buffer, 0, BUFFER_SIZE);

	if((received_bytes = recv(my_socket, recv_buffer, BUFFER_SIZE -1, 0)) <= 0) {
		print_error("recv() fallita o connessione chiusa prematuramente");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}
	recv_buffer[received_bytes] = '\0';

	// Parsing della risposta ricevuta
	weather_response_t parsed_response;
	parse_weather_response(recv_buffer, &parsed_response);

	// Visualizzazione del risultato in base allo status
	switch (parsed_response.status)
		{
		case STATUS_SUCCESS:
		{
			// Successo: stampa il valore meteo formattato
			printf("Ricevuto risultato dal server ip %s. %s: ",
			       inet_ntoa(server_sockaddr.sin_addr), parsed_request.city);

			// Formattazione specifica per ogni tipo di dato
			switch (parsed_response.type)
			{
			case TYPE_TEMPERATURE:
			{
				printf("Temperatura = %.1f°C\n", parsed_response.value);
				break;
			}
			case TYPE_HUMIDITY:
			{
				printf("Umidità = %.1f%%\n", parsed_response.value);
				break;
			}
			case TYPE_WIND:
			{
				printf("Vento = %.1f km/h\n", parsed_response.value);
				break;
			}
			case TYPE_PRESSURE:
			{
				printf("Pressione = %.1f hPa\n", parsed_response.value);
				break;
			}
			}
			break;
		}

		case STATUS_CITY_NOT_FOUND:
		{
			// Città non disponibile nel database del server
			printf("Ricevuto risultato dal server ip %s. ", inet_ntoa(server_sockaddr.sin_addr));
			print_error("Città non disponibile\n");
			break;
		}

		case STATUS_INVALID_REQUEST:
		{
			// Richiesta non valida
			printf("Ricevuto risultato dal server ip %s. ", inet_ntoa(server_sockaddr.sin_addr));
			print_error("Richiesta non valida\n");
			break;
		}
		}

	// chiusura della connessione
	closesocket(my_socket);
	printf("Client terminated.\n");
	clearwinsock();
	return 0;
	} // main end
