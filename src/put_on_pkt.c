#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "packet_interface.h"
#include <time.h>
void put_on_pkt(void *buf,void *payload,int seqnum,int window,int end){
    pkt_t *newPkt = pkt_new();
    pkt_set_tr(newPkt,0);
    pkt_set_type(newPkt,PTYPE_DATA);
    if(!end){
        pkt_set_length(newPkt,sizeof(payload));
    }else{
        pkt_set_length(newPkt,0);
    }
    time_t sent_moment = time(NULL);
    pkt_set_timestamp(newPkt,(uint32_t)sent_moment);
    pkt_set_seqnum(newPkt,seqnum);
    pkt_set_window(newPkt,window);
    pkt_set_seqnum(newPkt,seqnum);
    pkt_set_payload(newPkt,payload,sizeof(payload));
    
    size_t len = 12;
    pkt_encode(newPkt,buf,&len);
    pkt_del(newPkt);
}