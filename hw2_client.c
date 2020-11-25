#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1234
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
int board[9];

void print_board(int *board){
    printf("\n");
    printf("%d | %d | %d \n", board[0], board[1], board[2]);
    printf("—————————\n");
    printf("%d | %d | %d \n", board[3], board[4], board[5]);
    printf("—————————\n");
    printf("%d | %d | %d \n\n\n", board[6], board[7], board[8]);
}

//判定輪到誰
int choose_user_turn(int *board){
    int i = 0, inviter = 0, invitee = 0;
    for(i = 0; i < 9; i++){
        if(board[i] == 1){
            inviter++;
        }
        else if(board[i] == 2){
            invitee++;
        }
    }
    //如果inviter選的格數比較多，就換invitee
    if(inviter > invitee)
        return 2;
    else
        return 1;
}

//更新棋盤
void UpdateBoard(int *board, int location){
    print_board(board);
    int user_choice = choose_user_turn(board);
    //紀錄使用者選的位置
    board[location] = user_choice;
    sprintf(sendbuf, "7  %d %d %d %d %d %d %d %d %d\n", board[0], \
        board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8]);
}

//處理server到client的訊息
void pthread_recv(void* ptr)
{
    int instruction;
    while(1)
    {
        memset(sendbuf, 0, sizeof(sendbuf));
        instruction = 0;
        if ((recv(fd, recvbuf, MAXDATASIZE, 0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }
        sscanf (recvbuf, "%d", &instruction);
        switch (instruction)
        {
            //印出所有使用者
            case 2: {
                printf("%s\n", &recvbuf[2]);
                break;
            }

            //接不接受邀請
            case 4: {
                char inviter[100];
                sscanf(recvbuf, "%d %s", &instruction, inviter);
                printf("%s\n", &recvbuf[2]); // Print the message behind the instruction.
                printf("接受:5 1 %s\n", inviter);
                printf("拒絕:5 0 %s\n", inviter);
                break;
            }

            //開始遊戲
            case 6: {
                printf("⭕ ⭕ 遊戲開始 ❌ ❌\n\n");
                printf("邀請者的圖示為   1, 1 = ⭕\n");
                printf("被邀請者的圖示為 2, 2 = ❌\n");
                printf("請輸入:x座標,y座標\n");
                print_board(board);
                break;
            }
            //輸出棋盤並請使用者輸入要下哪裡
            case 8: {
                // int board[9];
                char msg[100];
                sscanf (recvbuf,"%d %d%d%d%d%d%d%d%d%d %s", &instruction, &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8], msg);
                print_board(board);
                printf("%s\n", msg);
                printf("請輸入:x座標,y座標\n");
                break;
            }
            
            default:
                break;
        }   

        memset(recvbuf, 0, sizeof(recvbuf));
    }
}

int main(int argc, char *argv[])
{
    int  numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;

    //沒輸入連線IP
    if (argc != 2)
    {
        printf("請輸入:%s <IP位址>\n", argv[0]);
        exit(1);
    }

    //檢查IP地址是否正確
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        printf("gethostbyname() error\n");
        exit(1);
    }

    //檢查socket是否錯誤
    if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        printf("socket() error\n");
        exit(1);
    }

    memset(&server, 0, sizeof(server));
    //bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    //偵測連線錯誤
    if(connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr))==-1)
    {
        printf("connect() error\n");
        exit(1);
    }

    // First, Add User.
    printf("喔耶! 成功連線上伺服器\n");
    //char str[] = " have come in\n";
    printf("您尊姓大名?：");
    fgets(name, sizeof(name), stdin);
    char package[100];
    strcat(package, "1 ");
    strcat(package, name);
    send(fd, package, (strlen(package)), 0);
    printf("顯示所有使用者，請輸入:2\n");
    printf("邀請其他使用者，請輸入:3 你的名字 他的名字\n");
    printf("登出，請輸入: quit\n\n");

    // 處理從伺服器給客戶端的訊息
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)pthread_recv, NULL);

    // 處理從客戶端給伺服器的訊息
    while(1){
        memset(sendbuf, 0, sizeof(sendbuf));
        fgets(sendbuf, sizeof(sendbuf), stdin);
        int location;
        if(sendbuf[1] == ','){
			int a, b;
			sscanf(&sendbuf[0], "%d", &a);
            sscanf(&sendbuf[2], "%d", &b);
			if(a == 1){
				if(b == 1) location = 0;
				else if(b == 2) location = 1;
				else location = 2;
			}
			else if(a == 2){
				if(b == 1) location = 3;
				else if(b == 2) location = 4;
				else location = 5;
			}
			else if(a == 3){
				if(b == 1) location = 6;
				else  if(b == 2) location = 7;
				else location = 8;
			}
            UpdateBoard(board, location);
        }
        //Send instructions to server
        send(fd,sendbuf,(strlen(sendbuf)),0);   

        //登出
        if(strcmp(sendbuf, "quit\n") == 0){          
            memset(sendbuf, 0, sizeof(sendbuf));
            printf("登出\n");
            return 0;
        }
    }
    pthread_join(tid,NULL);
    close(fd);
    return 0;
}

