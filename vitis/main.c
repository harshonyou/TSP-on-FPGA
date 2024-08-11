#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#include "xparameters.h"
#include "netif/xadapter.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xuartps_hw.h"

#include "lwip/sockets.h"
#include "lwipopts.h"
#include <lwip/ip_addr.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>

#include "xtoplevel.h"
#include "xtime_l.h"

#define THREAD_STACKSIZE 1024
#define PORT 51000
#define SERVER_IP "192.168.10.1"
#define SERVER_PORT 51100
#define SEND_TASK_STACKSIZE 1024

// MAC address
unsigned char mac_ethernet_address[] = {0x00, 0x11, 0x22, 0x33, 0x00, 0x37};

// Function Prototypes
void network_init(unsigned char* mac_address, lwip_thread_fn app);
void application_task(void *);
void udp_handler(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void input_task(void *p);
void send_request(struct udp_pcb *pcb, uint8_t num_cities, uint32_t scenario_id);
void send_solve_request(struct udp_pcb *pcb, uint8_t num_cities, uint32_t scenario_id, uint32_t shortest_path);
void reset_shared_array();
void print_kth_permutation(int n, uint32_t k);

//------
// Define maximum number of cities for the problem
#define N 13
#define MAXCITIES 20

// Shared array to store distance matrix in flattened form
u32 shared[MAXCITIES * MAXCITIES];

// Hardware instance
XToplevel hls;
//------

int main() {
	Xil_DCacheDisable();
	XToplevel_Initialize(&hls, XPAR_TOPLEVEL_0_DEVICE_ID);

    // Initialise the network with our MAC address, and the function that should be started as a FreeRTOS task
    network_init(mac_ethernet_address, application_task);

    vTaskStartScheduler(); // Start the FreeRTOS scheduler

    // Only reached if scheduler ends
    return 0;
}

void application_task(void *p) {
    // This task will set things up and then remove itself once that is done
    xil_printf("application_task started\n\r");

    // Setup network listening
    struct udp_pcb *pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, SERVER_PORT);
    udp_recv(pcb, udp_handler, NULL);

    // Create additional tasks
    xTaskCreate(input_task, "input_task", THREAD_STACKSIZE, pcb, tskIDLE_PRIORITY + 1, NULL); // Create the input task

    vTaskDelete(NULL); // Task setup complete, delete this task
}

void udp_handler(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (!p) {
        xil_printf("Received NULL pbuf\n\r");
        return;
    }

    uint8_t message_id = ((uint8_t*)p->payload)[0];

    switch (message_id) {
        case 0x02: { // Response to initial request
            if (p->len < 6) {
                xil_printf("Invalid response length: %d\n\r", p->len);
                pbuf_free(p);
                return;
            }

            uint8_t num_cities = ((uint8_t*)p->payload)[1];
            uint32_t scenario_id;
            memcpy(&scenario_id, &((uint8_t*)p->payload)[2], sizeof(scenario_id));
            scenario_id = ntohl(scenario_id); // Convert from network byte order to host byte order

            xil_printf("Response details - message_id: %d, num_cities: %d, scenario_id: %u\n\r", message_id, num_cities, scenario_id);

            int matrix_size = num_cities * num_cities;
            if (p->len < 6 + matrix_size) {
                xil_printf("Invalid adjacency matrix length: %d\n\r", p->len);
                pbuf_free(p);
                return;
            }

            uint8_t *adj_matrix = &((uint8_t*)p->payload)[6]; // Extract adjacency matrix
            xil_printf("Received adjacency matrix for scenario ID: %u with %d cities\n\r", scenario_id, num_cities);

            for (int i = 0; i < num_cities; i++) {
                for (int j = 0; j < num_cities; j++) {
                    xil_printf("%d\t", adj_matrix[i * num_cities + j]);

                    shared[i * num_cities + j] = adj_matrix[i * num_cities + j];
                }
                xil_printf("\n\r");
            }

//            xil_printf("Performing calculation (sleep for 1 second)\n\r");
//            vTaskDelay(1000 / portTICK_PERIOD_MS);

            // Load data into HLS hardware and start computation
            XToplevel_Set_ram(&hls, (u32) shared);
			XToplevel_Set_arg1(&hls, num_cities);

			XTime startTime, endTime;
			XTime_GetTime(&startTime);

			XToplevel_Start(&hls);
			while(!XToplevel_IsDone(&hls));

			XTime_GetTime(&endTime);
			XTime executionTime = endTime - startTime;
			unsigned long msTime = (unsigned long)((1000.0 * executionTime) / COUNTS_PER_SECOND);
			xil_printf("Hardware execution time: %lu ms\n\r", msTime);

			uint32_t kth_perm = XToplevel_Get_arg3(&hls);

			xil_printf("Minimum tour distance: %lu\n\r", XToplevel_Get_arg2(&hls));
			xil_printf("Minimum tour perm: %lu\n\r", kth_perm);
			xil_printf("Update code: %lu\n\r", XToplevel_Get_arg4(&hls));

			xil_printf("Route for k-th permutation:\n\r");
			print_kth_permutation(num_cities, kth_perm);

            uint32_t shortest_path = htonl(XToplevel_Get_arg2(&hls));
            send_solve_request(pcb, num_cities, scenario_id, shortest_path);

            break;
        }

        case 0x04: { // Response to solve request
            if (p->len < 7) {
                xil_printf("Invalid response length: %d\n\r", p->len);
                pbuf_free(p);
                return;
            }

            uint8_t num_cities = ((uint8_t*)p->payload)[1];
            uint32_t id;
            memcpy(&id, &((uint8_t*)p->payload)[2], sizeof(id));
            id = ntohl(id); // Convert from network byte order to host byte order
            uint8_t answer = ((uint8_t*)p->payload)[6];

            xil_printf("Solve response details - message_id: %d, num_cities: %d, id: %u, answer: %d\n\r", message_id, num_cities, id, answer);

            if (message_id != 0x04) {
                xil_printf("Invalid message ID in solve response: %d\n\r", message_id);
                pbuf_free(p);
                return;
            }

            xil_printf("Solve response for scenario ID: %u with %d cities\n\r", id, num_cities);
            xil_printf("Answer: %d\n\r", answer);

            break;
        }

        default:
            xil_printf("Unknown message ID: %d\n\r", message_id);
            break;
    }

    pbuf_free(p); // Free the packet buffer
}

void send_request(struct udp_pcb *pcb, uint8_t num_cities, uint32_t scenario_id) {
    xil_printf("Sending request with num_cities=%d, scenario_id=%u\n\r", num_cities, scenario_id);

    // Create the send request
    uint8_t request[6];
    request[0] = 0x01; // Message ID
    request[1] = num_cities;
    scenario_id = htonl(scenario_id); // Convert to network byte order
    memcpy(&request[2], &scenario_id, sizeof(scenario_id));

    struct pbuf *pbuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(request), PBUF_RAM);
    if (pbuf == NULL) {
        xil_printf("Failed to allocate pbuf for request\n\r");
        return;
    }
    memcpy(pbuf->payload, request, sizeof(request));

    ip_addr_t server_ip;
    IP4_ADDR(&server_ip, 192, 168, 10, 1);
    udp_sendto(pcb, pbuf, &server_ip, SERVER_PORT);
    pbuf_free(pbuf); // Free the packet buffer after sending
}

void send_solve_request(struct udp_pcb *pcb, uint8_t num_cities, uint32_t scenario_id, uint32_t shortest_path) {
    xil_printf("In send_solve_request\n\r");

    // Create the solve request
    uint8_t solve_request[10];
    solve_request[0] = 0x03; // Message ID for solve request
    solve_request[1] = num_cities;
    scenario_id = htonl(scenario_id); // Convert to network byte order
    memcpy(&solve_request[2], &scenario_id, sizeof(scenario_id));
    memcpy(&solve_request[6], &shortest_path, sizeof(shortest_path));

    xil_printf("Solve request details - num_cities: %d, scenario_id: %u, shortest_path: %u\n\r", num_cities, ntohl(scenario_id), ntohl(shortest_path));

    struct pbuf *pbuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(solve_request), PBUF_RAM);
    if (pbuf == NULL) {
        xil_printf("Failed to allocate pbuf for solve request\n\r");
        return;
    }
    memcpy(pbuf->payload, solve_request, sizeof(solve_request));

    ip_addr_t server_ip;
    IP4_ADDR(&server_ip, 192, 168, 10, 1);
    udp_sendto(pcb, pbuf, &server_ip, SERVER_PORT);
    pbuf_free(pbuf); // Free the packet buffer after sending
}

void input_task(void *p) {
    xil_printf("input_task started\n\r");

    struct udp_pcb *pcb = (struct udp_pcb *)p;
    char buffer[10];
    uint8_t buffer_index = 0;

    while (1) {
            char c = inbyte();  // Read a character from standard input

            switch(c) {
                case 'r':  // Reset command to send request
                    xil_printf("%c", c);  // Echo the received character
                    xil_printf("\n\rEnter number of cities (4-20): ");
                    buffer_index = 0;
                    memset(buffer, 0, sizeof(buffer));
                    while (1) {
                        c = inbyte();
                        if (c == '\r' || c == '\n') break;
                        if (buffer_index < sizeof(buffer) - 1) {
                            buffer[buffer_index++] = c;
                            xil_printf("%c", c);
                        }
                    }
                    buffer[buffer_index] = '\0';
                    uint8_t num_cities = atoi(buffer);

                    if (num_cities < 4 || num_cities > 20) {
                        xil_printf("\nInvalid number of cities. Please enter a value between 4 and 20.\n\r");
                        continue;
                    }

                    xil_printf("\n\rEnter scenario ID: ");
                    buffer_index = 0;
                    memset(buffer, 0, sizeof(buffer));
                    while (1) {
                        c = inbyte();
                        if (c == '\r' || c == '\n') break;
                        if (buffer_index < sizeof(buffer) - 1) {
                            buffer[buffer_index++] = c;
                            xil_printf("%c", c);
                        }
                    }
                    buffer[buffer_index] = '\0';
                    uint32_t scenario_id = atoi(buffer);

                    // Inside the input_task, before sending a new request:
                    xil_printf("\nResetting shared array before new request...\n\r");
                    reset_shared_array();

                    send_request(pcb, num_cities, scenario_id);
                    break;

                case '?':  // Help command
                    xil_printf("\nAvailable commands:\n\r");
                    xil_printf("  r - Reset and send new request\n\r");
                    xil_printf("  e - Echo input\n\r");
                    xil_printf("  ? - Show this help menu\n\r");
                    break;

                case 'e':  // Echo command
                    xil_printf("\n\rEnter text to echo: ");
                    buffer_index = 0;
                    memset(buffer, 0, sizeof(buffer));
                    while (1) {
                        c = inbyte();
                        if (c == '\r' || c == '\n') break;
                        if (buffer_index < sizeof(buffer) - 1) {
                            buffer[buffer_index++] = c;
                            xil_printf("%c", c);
                        }
                    }
                    buffer[buffer_index] = '\0';
                    xil_printf("\n\rEcho: %s\n\r", buffer);
                    break;

                default:  // Invalid command handling
                    if (c != '\n' && c != '\r') { // Ignore newline and return characters
                        xil_printf("\nNot a valid command, use ? for help\n\r");
                    }
                    break;
            }
        }
}

void reset_shared_array() {
    int i;
    for (i = 0; i < MAXCITIES * MAXCITIES; i++) {
        shared[i] = UINT8_MAX;
    }
    xil_printf("\n\rShared array has been reset to maximum values.\n\r");
}

// Function to print the k-th permutation of a sequence
void print_kth_permutation(int n, uint32_t k) {
    int fact = 1;
    for (int i = 2; i < n; i++) {
        fact *= i;
    }

    int available[n];
    for (int i = 0; i < n; i++) {
        available[i] = i;
    }

    k--; // Convert to zero-indexed

    for (int i = 0; i < n; i++) {
        int index = k / fact;
        xil_printf("%d ", available[index] + 1);

        for (int j = index; j < n - 1; j++) {
            available[j] = available[j + 1];
        }

        k %= fact;
        if (i < n - 1) {
            fact /= (n - 1 - i);
        }
    }
    xil_printf("\n\r");
}


