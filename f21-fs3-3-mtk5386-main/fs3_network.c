////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_netowork.c
//  Description    : This is the network implementation for the FS3 system.

//
//  Author         : Patrick McDaniel
//  Last Modified  : Thu 16 Sep 2021 03:04:04 PM EDT
//

// Includes
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cmpsc311_log.h>
#include <string.h>
#include <inttypes.h>

// Project Includes
#include <fs3_network.h>
#include <cmpsc311_util.h>

//
//  Global data
unsigned char     *fs3_network_address = NULL; // Address of FS3 server
unsigned short     fs3_network_port = 0;       // Port of FS3 server
static int socket_fd;

//
// Network functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : network_fs3_syscall
// Description  : Perform a system call over the network
//
// Inputs       : cmd - the command block to send
//                ret - the returned command block
//                buf - the buffer to place received data in
// Outputs      : 0 if successful, -1 if failure

int network_fs3_syscall(FS3CmdBlk cmd, FS3CmdBlk *ret, void *buf){
    struct sockaddr_in caddr;
    char *ip;
    uint8_t op;

    //Deconstructs cmdblock for op value
    op = ((((uint64_t)1 << 4)-1)&(cmd>>60));

    //If mount connect
    if(op == FS3_OP_MOUNT){
        //Sets ip address
        if(fs3_network_address!=NULL){
            ip = (char*) fs3_network_address;
        }
        else{
            ip = FS3_DEFAULT_IP;
        }

        //Setup adress info
        caddr.sin_family = AF_INET;
        if(fs3_network_port == 0){
            caddr.sin_port = htons(FS3_DEFAULT_PORT);
        }
        else{
            caddr.sin_port = htons(fs3_network_port);
        }
        if (inet_aton(ip, &caddr.sin_addr) == 0 ) { 
            return(-1);
        }   

        //Create socket
        socket_fd = socket(PF_INET, SOCK_STREAM, 0); 
        if (socket_fd == -1) {
            printf("Error on socket creation [%s]\n", strerror(errno) );
            return(-1);
        }  

        //Conects socket to server
        if (connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 ) { 
            printf("Error on socket connect [%s]\n", strerror(errno) );
            return(-1);
        }   
    }

    //Send cmd
    cmd = htonll64(cmd);
    if (write(socket_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) { 
        printf("Error writing network data [%s]\n", strerror(errno) );
        return(-1);
    }  

    //If buffer for write send
    if(op == FS3_OP_WRSECT){
        //Send buf
        if (write(socket_fd, buf, (size_t)FS3_SECTOR_SIZE*sizeof(char)) != (size_t)FS3_SECTOR_SIZE*sizeof(char)) { 
            printf("Error writing network data [%s]\n", strerror(errno) );
            return(-1);
        }  

    }

    //Receive cmd
    if (read(socket_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) { 
        printf("Error reading network data [%s]\n", strerror(errno) );
        return(-1);
    }  

    //Set return cmd
    *ret = ntohll64(cmd);

    //If read get buffer back
    if(op == FS3_OP_RDSECT){
        if (read( socket_fd, buf, (size_t)FS3_SECTOR_SIZE*sizeof(char)) != (size_t)FS3_SECTOR_SIZE*sizeof(char)) { 
            printf( "Error reading network data [%s]\n", strerror(errno) );
            return(-1);
        }  
    }

    //If unmount disconnect
    if(op == FS3_OP_UMOUNT){
        // Close the socket
        close(socket_fd);
        socket_fd = -1;
    }

    //Return successful
    return(0);
}