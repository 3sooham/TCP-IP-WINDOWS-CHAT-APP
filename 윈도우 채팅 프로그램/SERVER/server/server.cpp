// 2020년 1학기 네트워크프로그래밍 숙제 3번
// 설명: 최원민 학번: 15011056
// 플랫폼: VS2010

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000
#define BUFSIZE    512

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	char buf[BUFSIZE+1];
	int recvbytes;
	int sendbytes;
	char id[10];
};

int nTotalSockets1 = 0; // 방1
int nTotalSockets2 = 0; // 방2
SOCKETINFO *SocketInfoArray1[FD_SETSIZE]; // 방1
SOCKETINFO *SocketInfoArray2[FD_SETSIZE]; // 방2
// 소켓 관리 함수
BOOL AddSocketInfo1(SOCKET sock, char *);
BOOL AddSocketInfo2(SOCKET sock, char *);
void RemoveSocketInfo1(int nIndex);
void RemoveSocketInfo2(int nIndex);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);

int main(int argc, char *argv[])
{
	int retval;

	printf("** 교재 SelectTCPServer 프로그램을 채팅 프로그램 서버로 수정 **\n");

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// 넌블로킹 소켓으로 전환
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if(retval == SOCKET_ERROR) err_display("ioctlsocket()");

	// 데이터 통신에 사용할 변수
	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, i, j, room;
	char room_num[5], id[10];
	
	while(1){
		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for(i=0; i<nTotalSockets1; i++){ // 1번방
		if(SocketInfoArray1[i]->recvbytes > SocketInfoArray1[i]->sendbytes)
				FD_SET(SocketInfoArray1[i]->sock, &wset);
		else
				FD_SET(SocketInfoArray1[i]->sock, &rset);
		}
		for(i=0; i<nTotalSockets2; i++){ // 2번방
		if(SocketInfoArray2[i]->recvbytes > SocketInfoArray2[i]->sendbytes)
				FD_SET(SocketInfoArray2[i]->sock, &wset);
		else
				FD_SET(SocketInfoArray2[i]->sock, &rset);
		}

		// select()
		retval = select(0, &rset, &wset, NULL, NULL);
		if(retval == SOCKET_ERROR) err_quit("select()");

	
		// 소켓 셋 검사(1): 클라이언트 접속 수용
		if(FD_ISSET(listen_sock, &rset)){
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
			if(client_sock == INVALID_SOCKET){
				err_display("accept()");
			}
			else{
				// 방 번호 받기
				retval = recv(client_sock,room_num, 1,0);
				if(retval == SOCKET_ERROR){
					err_display("recv() room_num 서버");
					return 0;
				}
				room_num[retval]='\0';
				room = atoi(room_num);

				printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				
				char * duplicate = "중복임!@!@";
				char * notduplicate = "아니다!@!@";
				int cnt = 1;
				switch(room)
				{
				case 1: // 1번방
					// 아이디 받기
					retval = recv(client_sock, id, 10,0);
					if(retval == SOCKET_ERROR)
					{
						err_display("recv() 아이디 서버 중복");
						return 0;
					}
					id[retval]='\0';
					//  1번방의 아이디중에서 중복 체크
					for(i=0; i<nTotalSockets1; i++)
					{
						SOCKETINFO *ptr = SocketInfoArray1[i];
						if(strcmp(ptr->id, id) == 0) // 중복이면
						{
							// 중복이라고 보냄
							retval = send(client_sock, duplicate, 
								strlen(duplicate), 0);
							if(retval == SOCKET_ERROR)
							{
								err_display("send() 서버 아이디 중복체크");
								return 0;
							}
							cnt = 0;
							break;
						}
					}
					// 중복아니라고 보냄
					if(cnt)
					{
						retval = send(client_sock, notduplicate, 
							strlen(notduplicate), 0);
						if(retval == SOCKET_ERROR)
						{
							err_display("send() 서버 아이디 중복체크 중복아님알려줌");
							return 0;
						}
					}

					AddSocketInfo1(client_sock, id);
					break;

				case 2: // 2번방
					// 아이디 받기
					retval = recv(client_sock, id, 10,0);
					if(retval == SOCKET_ERROR)
					{
						err_display("recv() 아이디 서버 중복");
						return 0;
					}
					id[retval]='\0';
					//  1번방의 아이디중에서 중복 체크
					for(i=0; i<nTotalSockets2; i++)
					{
						SOCKETINFO *ptr = SocketInfoArray2[i];
						if(strcmp(ptr->id, id) == 0) // 중복이면
						{
							// 중복이라고 보냄
							retval = send(client_sock, duplicate, 
								strlen(duplicate), 0);
							if(retval == SOCKET_ERROR)
							{
								err_display("send() 서버 아이디 중복체크");
								return 0;
							}
							cnt = 0;
							break;
						}
					}
					// 중복아니라고 보냄
					if(cnt)
					{
						retval = send(client_sock, notduplicate, 
							strlen(notduplicate), 0);
						if(retval == SOCKET_ERROR)
						{
							err_display("send() 서버 아이디 중복체크 중복아님알려줌");
							return 0;
						}
					}

					AddSocketInfo2(client_sock, id);
					break;
				}
			}
		}

		char tempid[12];
		// 1번방
		for(i=0; i<nTotalSockets1; i++)
		{
			SOCKETINFO *ptr = SocketInfoArray1[i];
			//printf("%s",ptr->buf);
			if(FD_ISSET(ptr->sock, &rset)){
				// 데이터 받기
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if(retval == SOCKET_ERROR){
					err_display("recv()");
					RemoveSocketInfo1(i);
					continue;
				}
				else if(retval == 0){
					RemoveSocketInfo1(i);
					continue;
				}
				ptr->recvbytes = retval;
				// 받은 데이터 출력
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
				ptr->buf[retval] = '\0';
				printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
					ntohs(clientaddr.sin_port), ptr->buf);
			}
			if(FD_ISSET(ptr->sock, &wset)){
				// 받은 데이터가 "/display"면 아이디 보내줘야됨. 이거 줄바꿈이 안돼게 보내지는중임.
				// 이거 \n붙여보내도 해결이안되는데 띄어쓰기같은거 붙여서 구분되게해보기
				if(strcmp(ptr->buf, "/display") == 0)
				{
					// 모두한테 안보내고 받은 그 사람한테만 보내줄것임
					// 1번방 명단 보내줌
					for (j=0; j<nTotalSockets1; j++)
					{
						SOCKETINFO *sptr = SocketInfoArray1[j];
						tempid[0] = '\0';
						sprintf(tempid, "%s %s", sptr->id, "  ");
						retval = send(ptr->sock, tempid, 
							strlen(tempid), 0);
						if(retval == SOCKET_ERROR){
							err_display("send()");
							RemoveSocketInfo1(i);
							continue;
						}
					}
					// 2번방 명단 보내줌
					for (j=0; j<nTotalSockets2; j++)
					{
						SOCKETINFO *sptr = SocketInfoArray2[j];
						tempid[0] = '\0';
						sprintf(tempid, "%s %s", sptr->id, "   ");
						retval = send(ptr->sock, tempid, 
							strlen(tempid), 0);
						if(retval == SOCKET_ERROR){
							err_display("send()");
							RemoveSocketInfo1(i);
							continue;
						}
					}
				}
				// /display 받으면 전부한테 /display 보내주는데 안보내주도록 하고 싶음
				// 데이터 보내기
				for (j=0; j<nTotalSockets1; j++){  // 여러 접속자에게 발송
					SOCKETINFO *sptr = SocketInfoArray1[j];
					retval = send(sptr->sock, ptr->buf + ptr->sendbytes, 
						ptr->recvbytes - ptr->sendbytes, 0);

					if(retval == SOCKET_ERROR){
						err_display("send()");
						RemoveSocketInfo1(i);
						continue;
					}
				}
				ptr->sendbytes += retval;
				if(ptr->recvbytes == ptr->sendbytes){
					ptr->recvbytes = ptr->sendbytes = 0;
				}
			}
		}

		// 2번 방
		for(i=0; i<nTotalSockets2; i++)
		{
			SOCKETINFO *ptr = SocketInfoArray2[i];
			//printf("%s",ptr->buf);
			if(FD_ISSET(ptr->sock, &rset)){
				// 데이터 받기
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if(retval == SOCKET_ERROR){
					err_display("recv()");
					RemoveSocketInfo2(i);
					continue;
				}
				else if(retval == 0){
					RemoveSocketInfo2(i);
					continue;
				}
				ptr->recvbytes = retval;
				// 받은 데이터 출력
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
				ptr->buf[retval] = '\0';
				printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
					ntohs(clientaddr.sin_port), ptr->buf);
			}
			if(FD_ISSET(ptr->sock, &wset)){
				// display 요청
				if(strcmp(ptr->buf, "/display") == 0)
				{
					// 모두한테 안보내고 display 요청한 사람한테만 보내줄것임
					for (j=0; j<nTotalSockets1; j++) // 1번방 명단 먼저 보내줌
					{
						SOCKETINFO *sptr = SocketInfoArray1[j];
						tempid[0] = '\0';
						sprintf(tempid, "%s %s", sptr->id, "   ");
						retval = send(ptr->sock, tempid, 
							strlen(tempid), 0);
						if(retval == SOCKET_ERROR){
							err_display("send()");
							RemoveSocketInfo1(i);
							continue;
						}
					}
					for (j=0; j<nTotalSockets2; j++) // 2번방 명단
					{
						SOCKETINFO *sptr = SocketInfoArray2[j];
						tempid[0] = '\0';
						sprintf(tempid, "%s %s", sptr->id, "   ");
						retval = send(ptr->sock, tempid, 
							strlen(tempid), 0);
						if(retval == SOCKET_ERROR){
							err_display("send()");
							RemoveSocketInfo1(i);
							continue;
						}
					}
				}
				// 데이터 보내기
				for (j=0; j<nTotalSockets2; j++){  // 여러 접속자에게 발송
					SOCKETINFO *sptr = SocketInfoArray2[j];
					retval = send(sptr->sock, ptr->buf + ptr->sendbytes, 
						ptr->recvbytes - ptr->sendbytes, 0);

					if(retval == SOCKET_ERROR){
						err_display("send()");
						RemoveSocketInfo2(i);
						continue;
					}
				}
				ptr->sendbytes += retval;
				if(ptr->recvbytes == ptr->sendbytes){
					ptr->recvbytes = ptr->sendbytes = 0;
				}
			}
		}
	}

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 소켓 정보 추가
BOOL AddSocketInfo1(SOCKET sock, char *id)
{
	if(nTotalSockets1 >= FD_SETSIZE){
		printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if(ptr == NULL){
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	strcpy(ptr->id, id);
	SocketInfoArray1[nTotalSockets1++] = ptr;

	return TRUE;
}
BOOL AddSocketInfo2(SOCKET sock, char *id)
{
	if(nTotalSockets2 >= FD_SETSIZE){
		printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if(ptr == NULL){
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	strcpy(ptr->id, id);
	SocketInfoArray2[nTotalSockets2++] = ptr;

	return TRUE;
}
// 소켓 정보 삭제
void RemoveSocketInfo1(int nIndex)
{
	SOCKETINFO *ptr = SocketInfoArray1[nIndex];

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", 
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	closesocket(ptr->sock);
	delete ptr;

	if(nIndex != (nTotalSockets1-1))
		SocketInfoArray1[nIndex] = SocketInfoArray1[nTotalSockets1-1];

	--nTotalSockets1;
}
void RemoveSocketInfo2(int nIndex)
{
	SOCKETINFO *ptr = SocketInfoArray2[nIndex];

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", 
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	closesocket(ptr->sock);
	delete ptr;

	if(nIndex != (nTotalSockets2-1))
		SocketInfoArray2[nIndex] = SocketInfoArray2[nTotalSockets2-1];

	--nTotalSockets2;
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}