#include "packet_interface.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <stddef.h>
/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
    ptypes_t type: 2;
    uint8_t tr: 1;
    uint8_t window: 5;
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload;
    uint32_t crc2;
};
uint32_t calculate_crc32(const char* data, size_t len){
    uint32_t my_crc32 = crc32(0L, Z_NULL, 0);
    my_crc32 = crc32(my_crc32,(const Bytef *)data,len);
    return my_crc32;
}

pkt_t* pkt_new()
{
    pkt_t * new_pkt = malloc(sizeof(pkt_t));
    if(new_pkt == NULL) return NULL;
    return new_pkt;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt_get_length(pkt) != 0 && pkt_get_payload(pkt)){
        free(pkt->payload);
        pkt->payload = NULL;
    }
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    if(len < 8)return E_NOHEADER;
    //HEADER

    //TYPE
    pkt_set_type(pkt,(data[0] & 0xC0)>>6);
    //TR
    pkt_set_tr(pkt,(data[0] & 0x20)>>5);
    if(pkt_get_tr(pkt)!=0 && pkt_get_type(pkt)!=PTYPE_DATA)return E_TYPE;
    //WINDOW
    pkt_set_window(pkt,(data[0] & 0x1F));
    //LENGTH
    uint16_t n_length;
    memcpy(&n_length,data+1,sizeof(uint16_t));
    pkt_set_length(pkt,ntohs(n_length));
    //SEQNUM
    pkt_set_seqnum(pkt,data[3]);
    //TIMESTAMP
    uint32_t timestamp;
    memcpy(&timestamp,data+4,sizeof(uint32_t));
    pkt_set_timestamp(pkt,timestamp);    
    //END OF HEADER
    //CRC1
    uint32_t my_crc1 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *) data,8);
    uint32_t n_crc1;
    memcpy(&n_crc1,data+8,sizeof(uint32_t)); 
    pkt_set_crc1(pkt,ntohl(n_crc1));
    if(my_crc1 != pkt_get_crc1(pkt)){
        //printf("Nack on the header!!!!!!!!!\n");
        return E_CRC;
    }
    if(pkt_get_length(pkt)>0){
        //PAYLOAD
        pkt_set_payload(pkt,&data[12],pkt_get_length(pkt));
        //CRC2
        uint32_t my_crc2 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *) pkt_get_payload(pkt),pkt_get_length(pkt));
        uint32_t n_crc2;
        memcpy(&n_crc2,data+12+pkt_get_length(pkt),sizeof(uint32_t));
        pkt_set_crc2(pkt,ntohl(n_crc2));
        //printf("Pkt %d crc21: %d and pkt crc22: %d\n",pkt_get_seqnum(pkt),pkt_get_crc2(pkt),my_crc2);
        if(pkt_get_crc2(pkt) != my_crc2){
            //printf("Nack on the payload!!!!!!\n");
            return E_CRC;
        }

    }
    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    //TYPE + TR + WINDOW
    buf[0] = (uint8_t)((pkt->type<<6)+( (pkt->tr)<<5)+(pkt->window));
    //LENGTH
    uint16_t n_length = htons(pkt->length);
    memcpy(buf+1,&n_length,sizeof(uint16_t));
    //SEQNUM
    memcpy(buf+3,&pkt->seqnum,sizeof(uint8_t));
    //TIMESTAMP
    memcpy(buf+4,&pkt->timestamp,sizeof(uint32_t));
    //END OF HEADER
    //CRC1
    uint32_t crc1 = htonl(crc32(crc32(0L, Z_NULL, 0), (const Bytef *) buf,8));
    memcpy(buf+8,&crc1,sizeof(uint32_t));
    *len = 12;
    //PAYLOAD
    if(pkt_get_length(pkt)>0){
        memcpy(&buf[12],pkt_get_payload(pkt),pkt_get_length(pkt));
        //CRC2
        uint32_t crc2 = htonl(crc32(crc32(0L, Z_NULL, 0), (const Bytef *) pkt_get_payload(pkt),pkt_get_length(pkt)));
        memcpy(buf+12+pkt_get_length(pkt),&crc2,sizeof(uint32_t));
        *len = 16 + pkt_get_length(pkt);
    }
    return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if(type < 0 || type  > 3) return E_TYPE;
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    if(tr != 0 && tr != 1)return E_TR;
    pkt->tr = tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if(window>31)return E_WINDOW;
    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if(length > 512)return E_LENGTH;
    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{
    if(512 < length)return E_LENGTH;
    pkt->payload = malloc(length);
    if(!pkt->payload)return E_NOMEM;
    memcpy(pkt->payload,data,length);
    return PKT_OK;
}/*
int main(int argc,char ** argv){
    pkt_t * packet = pkt_new();
    pkt_set_tr(packet,0);
    pkt_set_type(packet,1);
    pkt_set_window(packet, 28);
    pkt_set_seqnum(packet,123);
    pkt_set_payload(packet,"HelloWorld",11);
    pkt_set_length(packet,11);
    pkt_set_timestamp(packet,385875968);
    char* buffer = malloc(25*sizeof(char));
    size_t len = 25;
    pkt_encode(packet,buffer,&len);
    pkt_t * packet2 = pkt_new();
    pkt_decode(buffer,25,packet2);
    pkt_del(packet);
    pkt_del(packet2);
    free(buffer);
}*/