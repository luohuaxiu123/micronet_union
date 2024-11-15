#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SRC_IP "172.28.128.87"
//#define DST_IP "112.45.115.221"
#define DST_IP "39.156.66.18"
#define SRC_PORT 12345
#define DST_PORT 80
#define IP_HEADER_LEN sizeof(struct iphdr)
#define TCP_HEADER_LEN sizeof(struct tcphdr)

struct pseudo_header {
    unsigned int srcIP;
    unsigned int destIP;
    unsigned short zero : 8;
    unsigned short proto : 8;
    unsigned short totalLen;
};

unsigned short checksum(struct pseudo_header* psh, struct tcphdr* tcp_header)
{
    size_t size = TCP_HEADER_LEN + sizeof(struct pseudo_header);
    char buf[size];
    memset(buf, 0, size);
    memcpy(buf, psh, sizeof(struct pseudo_header));
    memcpy(buf + sizeof(struct pseudo_header), tcp_header, TCP_HEADER_LEN);

    unsigned int checkSum = 0;
    for (int i = 0; i < size; i += 2) {
        unsigned short first = (unsigned short)buf[i] << 8;
        unsigned short second = (unsigned short)buf[i + 1] & 0x00ff;
        checkSum += first + second;
    }

    while (1) {
        unsigned short c = (checkSum >> 16);
        if (c > 0) {
            checkSum = (checkSum << 16) >> 16;
            checkSum += c;
        }
        else {
            break;
        }
    }
    return ~checkSum;
}

void init_tcphdr(struct tcphdr* tcp_header, int src_port, int dst_port, int rad) 
{
    tcp_header->source = htons(src_port);
    tcp_header->dest = htons(dst_port);
    tcp_header->seq = htonl(rad);
    tcp_header->ack_seq = htonl(0);
    tcp_header->doff = 5;
    tcp_header->fin = 0;
    tcp_header->syn = 1;
    tcp_header->rst = 0;
    tcp_header->psh = 0;
    tcp_header->ack = 0;
    tcp_header->urg = 0;
    tcp_header->window = htons(4096);
    tcp_header->check = 0;
    tcp_header->urg_ptr = 0;
}

void init_iphdr(struct iphdr* iph, const char* src_ip, const char* dst_ip)
{
    iph->version = 4;
    iph->ihl = 5;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; 
    iph->saddr = inet_addr(src_ip);
    iph->daddr = inet_addr(dst_ip);
}

void init_phdr(struct pseudo_header* psh, struct iphdr* header)
{
    psh->srcIP = header->saddr;
    psh->destIP = header->daddr;
    psh->zero = 0;
    psh->proto = IPPROTO_TCP;
    psh->totalLen = htons(0x0014);
}

void init_addr(struct sockaddr_in* addr, int port, const char* ip)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
}

int main() 
{
    printf("%ld %ld\n", sizeof(struct iphdr), sizeof(struct pseudo_header));
    int rad = rand();
    int seq = -1;
    struct sockaddr_in src_addr, dst_addr;

    // 创建原始套接字
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("Socket Error");
    }

    const int on = 1;
    int skopt = setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
    if (skopt == -1) {
        perror("setsockopt error");
    }

    //创建tcp头
    struct tcphdr* tcp_header = (struct tcphdr*)malloc(TCP_HEADER_LEN);
    if (tcp_header == NULL) {
        perror("tcphdr error");
    }

    memset(tcp_header, 0, TCP_HEADER_LEN);
    (void)init_tcphdr(tcp_header, SRC_PORT, DST_PORT, rad);
    
    //创建ip头
    struct iphdr* ip_header = (struct iphdr*)malloc(IP_HEADER_LEN);
    if (ip_header == NULL) {
        perror("ip_header error");
    }

    memset(ip_header, 0, IP_HEADER_LEN);
    init_iphdr(ip_header, SRC_IP, DST_IP);

    //创建伪首部
    struct pseudo_header* psh = (struct pseudo_header*)malloc(sizeof(struct pseudo_header));
    if (psh == NULL) {
        perror("pheader error");
    }
    init_phdr(psh, ip_header);

    //校验
    tcp_header->check = htons(checksum(psh, tcp_header));

    //合并ip头和tcp头
    char buf[TCP_HEADER_LEN+IP_HEADER_LEN];
    memcpy(buf, ip_header, IP_HEADER_LEN);
    memcpy(buf+ IP_HEADER_LEN, tcp_header, TCP_HEADER_LEN);

    // 初始化源地址和目标地址
    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    memset(&dst_addr, 0, sizeof(struct sockaddr_in));
    (void)init_addr(&src_addr, SRC_PORT, SRC_IP);
    (void)init_addr(&dst_addr, DST_PORT, DST_IP);

    int b = bind(sockfd, (struct sockaddr*)&src_addr, sizeof(src_addr));
    if (-1 == b) {
        perror("bind failed");
    }

    int c = connect(sockfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (-1 == c) {
        perror("connect failed");
    }

    // 发送SYN
    int sd = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (sd < 0) {
        perror("sendto failed");
    }

    printf("SYN sent %d bytes.\n", sd);

    // 接收SYN+ACK
    char temp_buf[TCP_HEADER_LEN + IP_HEADER_LEN];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int rm = recvfrom(sockfd, temp_buf, sizeof(temp_buf), 0, (struct sockaddr*)&dst_addr, &addr_len);
    if (rm < 0) {
        perror("recvfrom failed");
    }

    printf("SYN+ACK received %d bytes.\n", rm);

    memcpy(tcp_header, temp_buf + IP_HEADER_LEN, TCP_HEADER_LEN);
    seq = ntohl(tcp_header->seq);

    // 构造ACK
    (void)init_tcphdr(tcp_header, SRC_PORT, DST_PORT, rad + 1);
    tcp_header->syn = 0;
    tcp_header->ack = 1;
    tcp_header->ack_seq = htonl(seq + 1);

    // 重新计算校验和
    tcp_header->check = htons(checksum(psh,tcp_header));

    //再次合并
    memcpy(buf, ip_header, IP_HEADER_LEN);
    memcpy(buf + IP_HEADER_LEN, tcp_header, TCP_HEADER_LEN);

    // 发送ACK
    int st = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (st < 0) {
        perror("sendto failed");
    }

    printf("ACK sent.\n");

    free(tcp_header);
    free(ip_header);
    free(psh);
    close(sockfd);
    return 0;
}
