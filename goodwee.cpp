/*
    Simple GW10K-ET Goodwee inverter communication over UDP 
    (c)2o25 https://github.com/invpe/ 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <time.h>

#define INVERTER_IP "192.168.1.241"
#define INVERTER_PORT 8899
#define TIMEOUT_SEC 5
#define REQUEST_SIZE 8
#define RESPONSE_SIZE 1024

// Runtime packet
uint8_t request_packet[REQUEST_SIZE] = {0xF7, 0x03, 0x89, 0x1C, 0x00, 0x7D, 0x7A, 0xE7}; 

// Helper
uint32_t extract_uint32_be(const uint8_t* data, int offset) {
    return ((uint32_t)data[offset] << 24) |
           ((uint32_t)data[offset + 1] << 16) |
           ((uint32_t)data[offset + 2] << 8) |
           ((uint32_t)data[offset + 3]);
}

//
int main() 
{
    int sockfd;
    struct sockaddr_in addr;
    uint8_t response[RESPONSE_SIZE];
    socklen_t addr_len = sizeof(addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct timeval timeout = {TIMEOUT_SEC, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(INVERTER_PORT);
    inet_pton(AF_INET, INVERTER_IP, &addr.sin_addr);

    if (sendto(sockfd, request_packet, REQUEST_SIZE, 0, (struct sockaddr*)&addr, addr_len) < 0) {
        perror("sendto");
        close(sockfd);
        return 1;
    }

    ssize_t received = recvfrom(sockfd, response, RESPONSE_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
    if (received < 0) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    close(sockfd);

    printf("Full response (%ld bytes):\n", received);
    for (int i = 0; i < received; i++) {
        printf("%02X ", response[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");


    // --- PARSING FIELDS ---
    
    // Scale Watts - PV Power current
    uint16_t ppv1           = (response[17] << 8) | response[18];
    uint16_t ppv2           = (response[25] << 8) | response[26];
    uint16_t ppv            = ppv1 + ppv2;

 
    // Scale: 0.1 kWh, endian: big-endian
    float e_total     = extract_uint32_be(response, 187) / 10.0f; // 0x00 01 62 24 -> 9066.0 kWh
    float e_total_exp = extract_uint32_be(response, 195) / 10.0f; // 0x00 01 5B 78 -> 8895.2 kWh
    float e_total_imp = extract_uint32_be(response, 205) / 10.0f; // 0x00 00 09 4F -> 238.3 kWh

    // ISO timestamp
    time_t now = time(NULL);
    char iso_time[32];
    strftime(iso_time, sizeof(iso_time), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    // Dump outputs
    printf("Timestamp: %s\n", iso_time);
    printf("ppv: %u\n", ppv);
    printf("e_total: %.1f\n", e_total);
    printf("e_total_exp: %.1f\n", e_total_exp);
    printf("e_total_imp: %.1f\n", e_total_imp); 


    return 0;
}
