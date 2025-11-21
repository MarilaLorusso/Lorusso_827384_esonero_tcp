#if defined WIN32
#include <winsock.h>
#include <windows.h>
typedef int socklen_t;
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
#include <ctype.h>
#include <time.h>
#include "protocol.h"


/* Cleanup Winsock on Windows; no-op on POSIX */
static void clearwinsock(void)
{
#if defined WIN32
	WSACleanup();
#endif
}

// Stampa un messaggio di errore su stream per errori
void print_error(const char* Messaggio_di_errore){
	fprintf(stderr, "%s", Messaggio_di_errore);
}

/*
Confronta due stringhe ignorando maiuscole/minuscole (case-insensitive)
Parametri:
	str1: prima stringa
	str2: seconda stringa
Ritorna: 1 se uguali (case-insensitive), 0 altrimenti
*/
static int compare_case_insensitive(const char *str1, const char *str2)
{
	while (*str1 && *str2)
	{
		if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2))
			return 0;
		str1++;
		str2++;
	}
	// Entrambe devono terminare allo stesso punto
	return *str1 == '\0' && *str2 == '\0';
}

/*
Verifica se la città è nell'elenco delle città supportate
Parametri:
	city_name: nome della città da verificare
Ritorna: 1 se la città è supportata, 0 altrimenti
*/
static int check_city_availability(const char *city_name)
{
	// Elenco delle città italiane supportate dal servizio
	const char *supported_cities[] = {
		"Bari", "Roma", "Milano", "Napoli", "Torino",
		"Palermo", "Genova", "Bologna", "Firenze", "Venezia"};

	const int total_cities = (int)(sizeof(supported_cities) / sizeof(supported_cities[0]));

	//confronto case-insensitive con tutte le città supportate
	for (int i = 0; i < total_cities; i++)
	{
		if (compare_case_insensitive(city_name, supported_cities[i]))
			return 1;
	}
	return 0;
}

/*
Analizza la stringa di richiesta e valida i dati
Parametri:
	input: stringa ricevuta dal client (formato: "tipo città")
	request: puntatore alla struttura da riempire
Ritorna: 0 se OK, 1 se città non disponibile, 2 se richiesta non valida
*/
int parse_weather_request(const char *input, weather_request_t *request)
{
	// CONTROLLI DI SICUREZZA
	// Verifica che i puntatori siano validi e la stringa abbia una lunghezza minima
	if (!input || !request || strlen(input) < 3)
		return STATUS_INVALID_REQUEST;

	// Estrae il primo carattere come tipo di dato
	request->type = input[0];

	// Validazione del tipo di dato richiesto
	if (request->type != TYPE_TEMPERATURE &&
		request->type != TYPE_HUMIDITY &&
		request->type != TYPE_WIND &&
		request->type != TYPE_PRESSURE)
	return STATUS_INVALID_REQUEST;

	// PARSING DELLA CITTÀ
	// Parto dal secondo carattere (dopo il tipo)
	const char *cursor = input + 1;

	// Salto TUTTI gli spazi, non solo il primo (gestisce "t   Milano")
	while (*cursor == ' ') {
		cursor++;
		}

	// CONTROLLO CITTÀ VUOTA
	// Se dopo gli spazi sono arrivato alla fine della stringa, manca la città
	if (*cursor == '\0')
		return STATUS_INVALID_REQUEST;

	// Copia il nome della città nel buffer della struct
	strncpy(request->city, cursor, sizeof(request->city) - 1);
	request->city[sizeof(request->city) - 1] = '\0';

	// Verifica che la città sia supportata
	if (!check_city_availability(request->city))
		return STATUS_CITY_NOT_FOUND;

	return STATUS_SUCCESS;
}

/*
Formatta la risposta del server come stringa
Parametri:
	response: puntatore alla struttura con i dati da inviare
	output_buffer: buffer di output per la stringa formattata
	buffer_size: dimensione del buffer
Ritorna: 0 se OK, 1 se errore
*/
int format_weather_response(const weather_response_t *response, char *output_buffer, size_t buffer_size)
{
	if (!response || !output_buffer  || buffer_size == 0)
		return 1;
	//formatta la risposta come: "stato tipo valore"
	int chars_written = snprintf(output_buffer, buffer_size, "%u %c %.1f", response->status, response->type, response->value);

	//verifica che la formattazione sia riuscita
	if (chars_written < 0 || (size_t)chars_written >= buffer_size)
		return 1; // buffer troppo piccolo

	return 0;
}

// Inizializza il generatore di numeri casuali
// Da chiamare una sola volta all'avvio del server
void initialize_random_generator(void)
{
	srand((unsigned int)time(NULL));
}

/* Genera un numero float casuale nell'intervallo [min, max]
Parametri:
min_val: valore minimo dell'intervallo
max_val: valore massimo dell'intervallo
Ritorna: numero float casuale
 */
static float generate_random_float(float min_val, float max_val)
{
	float random_fraction  = (float)rand() / (float)RAND_MAX;
	return min_val + random_fraction  * (max_val - min_val);
}

// Genera temperatura casuale tra -10.0 e 40.0 °C
float get_temperature(void)
{
	return generate_random_float(-10.0f, 40.0f);
}

// Genera umidità casuale tra 20.0 e 100.0 %
float get_humidity(void)
{
	return generate_random_float(20.0f, 100.0f);
}

// Genera velocità del vento casuale tra 0.0 e 100.0 km/h
float get_wind(void)
{
	return generate_random_float(0.0f, 100.0f);
}


// Genera pressione atmosferica casuale tra 950.0 e 1050.0 hPa
float get_pressure(void)
{
	return generate_random_float(950.0f, 1050.0f);
}

int main(int argc, char *argv[])
{
	// Porta di ascolto di default
	int listen_port_num = SERVER_PORT;

	// Parsing degli argomenti da riga di comando
	for (int i = 1; i < argc; ++i)
	{
		// Opzione -p: specifica porta di ascolto personalizzata
		if (strcmp(argv[i], "-p") == 0) {
			if (i + 1 < argc) {
				listen_port_num = atoi(argv[++i]);
				if (listen_port_num <= 0 || listen_port_num > 65535)
				{
					fprintf(stderr, "Porta non valida: %s\n", argv[i]);
					return 1;
				}
				continue;
			}
			fprintf(stderr, "Valore mancante per -p\n");
			return 1;
		}
		// Ignora argomenti sconosciuti
	}

#if defined WIN32
	SetConsoleOutputCP(CP_UTF8);

	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != NO_ERROR)
	{
		print_error("Error at WSAStartup() \n");
		return 0;
	}
#endif

	// creazione della socket TCP per il server
	int server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_sock < 0)
	{
		print_error("Creazione socket fallita.\n");
		clearwinsock();
		return -1;
	}

	// Assegnazione indirizzo alla socket
	struct sockaddr_in server_sockaddr;

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons((unsigned short)listen_port_num);
	server_sockaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//binding della socket all'indirizzo e porta
	if (bind(server_sock, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) < 0)
	{
		print_error("bind() fallita.\n");
		closesocket(server_sock);
		clearwinsock();
		return -1;
	}

	// Mette la socket in ascolto per connessioni in ingresso
	if (listen(server_sock, QUEUE_SIZE) < 0)
	{
		print_error("listen() fallita.\n");
		closesocket(server_sock);
		clearwinsock();
		return -1;
	}

	// Struttura per memorizzare l'indirizzo del client
	struct sockaddr_in client_sockaddr;
	int connection_sock; // Socket per la comunicazione con il singolo client

	puts("In attesa di connessioni client...");

	// Loop principale del server (non termina mai)
	while (1)
	{
		// Accetta una nuova connessione da un client
		socklen_t client_addr_len  = sizeof(client_sockaddr);
		if ((connection_sock  = accept(server_sock, (struct sockaddr *)&client_sockaddr, &client_addr_len)) < 0)
		{
			print_error("accept() fallita.\n");
			closesocket(connection_sock );
			clearwinsock();
			return -1;
		}
		// connection_sock è ora connessa a un client specifico

		// Buffer per ricevere i dati dal client
		char receive_buffer[BUFFER_SIZE];
		memset(receive_buffer, 0, BUFFER_SIZE);

		// Riceve la richiesta dal client (stringa null-terminated)
		int received_bytes  = recv(connection_sock, receive_buffer, BUFFER_SIZE - 1, 0);
		if (received_bytes  <= 0)
		{
			print_error("recv() fallita o connessione chiusa prematuramente\n");
			closesocket(connection_sock);
			clearwinsock();
			return -1;
		}
		receive_buffer[received_bytes] = '\0';

		// Visualizza la richiesta ricevuta con l'IP del client
		printf("Richiesta \"%s\" dal client ip %s\n", receive_buffer, inet_ntoa(client_sockaddr.sin_addr));

		// Strutture per gestire richiesta e risposta
		weather_request_t parsed_request;
		weather_response_t prepared_response;

		// Inizializza il generatore di numeri casuali una sola volta
		initialize_random_generator();

		// Valida e analizza la richiesta ricevuta
		int validation_result = parse_weather_request(receive_buffer, &parsed_request);

		switch (validation_result)
		{
		case STATUS_SUCCESS:
		{
			// Richiesta valida: prepara risposta con dati meteo
			prepared_response.status = STATUS_SUCCESS;
			prepared_response.type = parsed_request.type;

			// Genera il valore appropriato in base al tipo richiesto
			switch (parsed_request.type)
			{
			case TYPE_TEMPERATURE:
			{
				prepared_response.value = get_temperature();
				break;
			}
			case TYPE_HUMIDITY:
			{
				prepared_response.value = get_humidity();
				break;
			}
			case TYPE_WIND:
			{
				prepared_response.value = get_wind();
				break;
			}
			case TYPE_PRESSURE:
			{
				prepared_response.value = get_pressure();
				break;
			}
			}
			break;
		}

		case STATUS_CITY_NOT_FOUND:
		{
			// Città non disponibile: prepara risposta di errore
			prepared_response.status = STATUS_CITY_NOT_FOUND;
			prepared_response.type = '\0';
			prepared_response.value = 0.0f;
			break;
		}

		case STATUS_INVALID_REQUEST:
		{
			// Richiesta non valida: prepara risposta di errore
			prepared_response.status = STATUS_INVALID_REQUEST;
			prepared_response.type = '\0';
			prepared_response.value = 0.0f;
			break;
		}
		}

		// Formatta la risposta come stringa da inviare al client
		char send_buffer[BUFFER_SIZE];
		format_response_string(&prepared_response, send_buffer, BUFFER_SIZE);

		// Invia la risposta al client (incluso il null terminator)
		int message_length  = (int)strlen(send_buffer) + 1;
		int bytes_sent  = send(connection_sock, send_buffer, message_length, 0);
		if (bytes_sent  < 0 || bytes_sent  != message_length)
		{
			print_error("send() fallita o byte inviati diversi.\n");
			closesocket(connection_sock);
			clearwinsock();
			return -1;
		}

	}

	printf("Server terminated... \n");
	closesocket(server_sock);
	clearwinsock();

	return 0;
}
