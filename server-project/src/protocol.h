/*
 * protocol.h
 *
 * Server header file
 * Definitions, constants and function prototypes for the server
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

// Shared application parameters
#define SERVER_IP "127.0.0.1" //Indirizzo IP del server (localhost)
#define SERVER_PORT 27015  // Server port for the weather service
#define BUFFER_SIZE 512    // Buffer size for messages
#define QUEUE_SIZE 5       // Size of pending connections queue

/* Codici di stato della risposta del server */
#define STATUS_SUCCESS 0           // Richiesta elaborata con successo
#define STATUS_CITY_NOT_FOUND 1    // Città richiesta non disponibile
#define STATUS_INVALID_REQUEST 2   // Richiesta non valida

/* Tipi di dati meteorologici supportati */
#define TYPE_TEMPERATURE 't'  // Temperatura
#define TYPE_HUMIDITY 'h'     // Umidità
#define TYPE_WIND 'w'         // Velocità
#define TYPE_PRESSURE 'p'     // Pressione atmosferica

// Request message (client -> server)
typedef struct {
    char type;        // Weather data type: 't', 'h', 'w', 'p'
    char city[64];    // City name (null-terminated string)
} weather_request_t;

// Response message (server -> client)
typedef struct {
    unsigned int status;  // Response status code
    char type;            // Echo of request type
    float value;          // Weather data value
} weather_response_t;

int parse_weather_request(const char *input, weather_request_t *request);
int format_weather_response(const weather_response_t *response, char *output_buffer, size_t buffer_size);

// Funzioni di generazione dati meteorologici casuali

/* Genera temperatura casuale tra -10.0 e 40.0 °C */
float get_temperature(void);

/* Genera umidità casuale tra 20.0 e 100.0 % */
float get_humidity(void);

/* Genera velocità del vento casuale tra 0.0 e 100.0 km/h */
float get_wind(void);

/* Genera pressione atmosferica casuale tra 950.0 e 1050.0 hPa */
float get_pressure(void);

#endif /* PROTOCOL_H_ */
