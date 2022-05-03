// 2020�� 1�б� ��Ʈ��ũ���α׷��� ���� 3��
// ����: �ֿ��� �й�: 15011056
// �÷���: VS2010

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000
#define BUFSIZE    512

// ���� ���� ������ ���� ����ü�� ����
struct SOCKETINFO
{
	SOCKET sock;
	char buf[BUFSIZE+1];
	int recvbytes;
	int sendbytes;
	char id[10];
};

int nTotalSockets1 = 0; // ��1
int nTotalSockets2 = 0; // ��2
SOCKETINFO *SocketInfoArray1[FD_SETSIZE]; // ��1
SOCKETINFO *SocketInfoArray2[FD_SETSIZE]; // ��2
// ���� ���� �Լ�
BOOL AddSocketInfo1(SOCKET sock, char *);
BOOL AddSocketInfo2(SOCKET sock, char *);
void RemoveSocketInfo1(int nIndex);
void RemoveSocketInfo2(int nIndex);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);

int main(int argc, char *argv[])
{
	int retval;

	printf("** ���� SelectTCPServer ���α׷��� ä�� ���α׷� ������ ���� **\n");

	// ���� �ʱ�ȭ
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

	// �ͺ��ŷ �������� ��ȯ
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if(retval == SOCKET_ERROR) err_display("ioctlsocket()");

	// ������ ��ſ� ����� ����
	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, i, j, room;
	char room_num[5], id[10];
	
	while(1){
		// ���� �� �ʱ�ȭ
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for(i=0; i<nTotalSockets1; i++){ // 1����
		if(SocketInfoArray1[i]->recvbytes > SocketInfoArray1[i]->sendbytes)
				FD_SET(SocketInfoArray1[i]->sock, &wset);
		else
				FD_SET(SocketInfoArray1[i]->sock, &rset);
		}
		for(i=0; i<nTotalSockets2; i++){ // 2����
		if(SocketInfoArray2[i]->recvbytes > SocketInfoArray2[i]->sendbytes)
				FD_SET(SocketInfoArray2[i]->sock, &wset);
		else
				FD_SET(SocketInfoArray2[i]->sock, &rset);
		}

		// select()
		retval = select(0, &rset, &wset, NULL, NULL);
		if(retval == SOCKET_ERROR) err_quit("select()");

	
		// ���� �� �˻�(1): Ŭ���̾�Ʈ ���� ����
		if(FD_ISSET(listen_sock, &rset)){
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
			if(client_sock == INVALID_SOCKET){
				err_display("accept()");
			}
			else{
				// �� ��ȣ �ޱ�
				retval = recv(client_sock,room_num, 1,0);
				if(retval == SOCKET_ERROR){
					err_display("recv() room_num ����");
					return 0;
				}
				room_num[retval]='\0';
				room = atoi(room_num);

				printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				
				char * duplicate = "�ߺ���!@!@";
				char * notduplicate = "�ƴϴ�!@!@";
				int cnt = 1;
				switch(room)
				{
				case 1: // 1����
					// ���̵� �ޱ�
					retval = recv(client_sock, id, 10,0);
					if(retval == SOCKET_ERROR)
					{
						err_display("recv() ���̵� ���� �ߺ�");
						return 0;
					}
					id[retval]='\0';
					//  1������ ���̵��߿��� �ߺ� üũ
					for(i=0; i<nTotalSockets1; i++)
					{
						SOCKETINFO *ptr = SocketInfoArray1[i];
						if(strcmp(ptr->id, id) == 0) // �ߺ��̸�
						{
							// �ߺ��̶�� ����
							retval = send(client_sock, duplicate, 
								strlen(duplicate), 0);
							if(retval == SOCKET_ERROR)
							{
								err_display("send() ���� ���̵� �ߺ�üũ");
								return 0;
							}
							cnt = 0;
							break;
						}
					}
					// �ߺ��ƴ϶�� ����
					if(cnt)
					{
						retval = send(client_sock, notduplicate, 
							strlen(notduplicate), 0);
						if(retval == SOCKET_ERROR)
						{
							err_display("send() ���� ���̵� �ߺ�üũ �ߺ��ƴԾ˷���");
							return 0;
						}
					}

					AddSocketInfo1(client_sock, id);
					break;

				case 2: // 2����
					// ���̵� �ޱ�
					retval = recv(client_sock, id, 10,0);
					if(retval == SOCKET_ERROR)
					{
						err_display("recv() ���̵� ���� �ߺ�");
						return 0;
					}
					id[retval]='\0';
					//  1������ ���̵��߿��� �ߺ� üũ
					for(i=0; i<nTotalSockets2; i++)
					{
						SOCKETINFO *ptr = SocketInfoArray2[i];
						if(strcmp(ptr->id, id) == 0) // �ߺ��̸�
						{
							// �ߺ��̶�� ����
							retval = send(client_sock, duplicate, 
								strlen(duplicate), 0);
							if(retval == SOCKET_ERROR)
							{
								err_display("send() ���� ���̵� �ߺ�üũ");
								return 0;
							}
							cnt = 0;
							break;
						}
					}
					// �ߺ��ƴ϶�� ����
					if(cnt)
					{
						retval = send(client_sock, notduplicate, 
							strlen(notduplicate), 0);
						if(retval == SOCKET_ERROR)
						{
							err_display("send() ���� ���̵� �ߺ�üũ �ߺ��ƴԾ˷���");
							return 0;
						}
					}

					AddSocketInfo2(client_sock, id);
					break;
				}
			}
		}

		char tempid[12];
		// 1����
		for(i=0; i<nTotalSockets1; i++)
		{
			SOCKETINFO *ptr = SocketInfoArray1[i];
			//printf("%s",ptr->buf);
			if(FD_ISSET(ptr->sock, &rset)){
				// ������ �ޱ�
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
				// ���� ������ ���
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
				ptr->buf[retval] = '\0';
				printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
					ntohs(clientaddr.sin_port), ptr->buf);
			}
			if(FD_ISSET(ptr->sock, &wset)){
				// ���� �����Ͱ� "/display"�� ���̵� ������ߵ�. �̰� �ٹٲ��� �ȵŰ� ������������.
				// �̰� \n�ٿ������� �ذ��̾ȵǴµ� ���ⰰ���� �ٿ��� ���еǰ��غ���
				if(strcmp(ptr->buf, "/display") == 0)
				{
					// ������� �Ⱥ����� ���� �� ������׸� �����ٰ���
					// 1���� ��� ������
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
					// 2���� ��� ������
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
				// /display ������ �������� /display �����ִµ� �Ⱥ����ֵ��� �ϰ� ����
				// ������ ������
				for (j=0; j<nTotalSockets1; j++){  // ���� �����ڿ��� �߼�
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

		// 2�� ��
		for(i=0; i<nTotalSockets2; i++)
		{
			SOCKETINFO *ptr = SocketInfoArray2[i];
			//printf("%s",ptr->buf);
			if(FD_ISSET(ptr->sock, &rset)){
				// ������ �ޱ�
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
				// ���� ������ ���
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
				ptr->buf[retval] = '\0';
				printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
					ntohs(clientaddr.sin_port), ptr->buf);
			}
			if(FD_ISSET(ptr->sock, &wset)){
				// display ��û
				if(strcmp(ptr->buf, "/display") == 0)
				{
					// ������� �Ⱥ����� display ��û�� ������׸� �����ٰ���
					for (j=0; j<nTotalSockets1; j++) // 1���� ��� ���� ������
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
					for (j=0; j<nTotalSockets2; j++) // 2���� ���
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
				// ������ ������
				for (j=0; j<nTotalSockets2; j++){  // ���� �����ڿ��� �߼�
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

	// ���� ����
	WSACleanup();
	return 0;
}

// ���� ���� �߰�
BOOL AddSocketInfo1(SOCKET sock, char *id)
{
	if(nTotalSockets1 >= FD_SETSIZE){
		printf("[����] ���� ������ �߰��� �� �����ϴ�!\n");
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if(ptr == NULL){
		printf("[����] �޸𸮰� �����մϴ�!\n");
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
		printf("[����] ���� ������ �߰��� �� �����ϴ�!\n");
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if(ptr == NULL){
		printf("[����] �޸𸮰� �����մϴ�!\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	strcpy(ptr->id, id);
	SocketInfoArray2[nTotalSockets2++] = ptr;

	return TRUE;
}
// ���� ���� ����
void RemoveSocketInfo1(int nIndex)
{
	SOCKETINFO *ptr = SocketInfoArray1[nIndex];

	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", 
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

	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR *)&clientaddr, &addrlen);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", 
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	closesocket(ptr->sock);
	delete ptr;

	if(nIndex != (nTotalSockets2-1))
		SocketInfoArray2[nIndex] = SocketInfoArray2[nTotalSockets2-1];

	--nTotalSockets2;
}

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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