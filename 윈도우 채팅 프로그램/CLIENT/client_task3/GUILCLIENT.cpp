// 2020�� 1�б� ��Ʈ��ũ���α׷��� ���� 3��
// ����: �ֿ��� �й�: 15011056
// �÷���: VS2010

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

// �̹����� echo�ۿ��ȵ�.

#define SERVERIP   "127.0.0.1" // ���� ip
#define SERVERPORT 9000 // ���� ��Ʈ
#define BUFSIZE    512

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM); // ��ȭ���� ó���ϴ� ����� ���� �Լ�
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg); // Ŭ�� ������
DWORD WINAPI ProcessInputSend(LPVOID arg);

SOCKET sock; // ����
char buf[BUFSIZE+1]; // ������ �ۼ��� ����
char ip[100]; // �̰� Ŭ�� ���� ����Ŷ� �̸� �����ؼ� ȥ�����̱�
char port[100]; 
char userID[10];
char room[5];
char *display1 = "/display";
int retval;
HANDLE hReadEvent, hWriteEvent, hDisplayEvent, hConnectEvent, hConnect2Event; // �̺�Ʈ
HWND hconnectbutton, hsendbutton, hdisplaybutton, hexit; // ������ ��ư
HWND hEditip, hEditport, hEditroom, hEditid, hEditmessage, hEditprint; // ���� ��Ʈ��

// ���� �Լ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ���� // ����ȭ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL); // ��ȣ���·� ����
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // �� ��ȣ ���·� ����
	if(hWriteEvent == NULL) return 1;
	hDisplayEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // �� ��ȣ ���·� ����
	if(hWriteEvent == NULL) return 1;
	hConnectEvent = CreateEvent(NULL, FALSE, TRUE, NULL); // ��ȣ ���·� ����
	if(hWriteEvent == NULL) return 1;
	hConnect2Event = CreateEvent(NULL, FALSE, FALSE, NULL); // ��ȣ ���·� ����
	if(hWriteEvent == NULL) return 1;

	// ���� ��� ������ ����
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// ��ȭ���� ���� dlgProc���� ��ȭ���� ����.
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);
	CloseHandle(hDisplayEvent);
	// �ٸ� �̺�Ʈ�鵵 ���߿� �����ϱ�

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν��� // ���⿡ ���޵Ǵ� ���ڴ� ������ ���ν����� ����
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
	case WM_INITDIALOG:
		// ���� ��Ʈ��
		hEditip = GetDlgItem(hDlg, IDC_EDIT1);
		hEditport = GetDlgItem(hDlg, IDC_EDIT2);
		hEditroom = GetDlgItem(hDlg, IDC_EDIT4);
		hEditid = GetDlgItem(hDlg, IDC_EDIT6);
		hEditmessage = GetDlgItem(hDlg, IDC_EDIT7);
		hEditprint = GetDlgItem(hDlg, IDC_EDIT5);
		// ��ư
		hconnectbutton = GetDlgItem(hDlg, ID_CONNECT); // ��𿡼��� ������ ��ư�� �����ϱ����ؼ� �ڵ鰪 ����
		hsendbutton = GetDlgItem(hDlg, ID_SEND);
		hdisplaybutton = GetDlgItem(hDlg, ID_DISPLAY);
		hexit = GetDlgItem(hDlg, ID_EXIT);
		EnableWindow(hsendbutton, FALSE); // send ��ư ��Ȱ��
		EnableWindow(hdisplaybutton, FALSE); // display ��ư ��Ȱ��

		// �Է¶� �ִ� �Է¼� ����
		SendMessage(hEditip, EM_SETLIMITTEXT, 100, 0); // sendmessage�� hedit1�� �����°���
		SendMessage(hEditport, EM_SETLIMITTEXT, 100, 0);
		SendMessage(hEditroom, EM_SETLIMITTEXT, 5, 0);
		SendMessage(hEditid, EM_SETLIMITTEXT, 10, 0);

		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case ID_CONNECT: // connect��ư
			EnableWindow(hsendbutton, FALSE); // ��ư�� ���� 2�� ���������� ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hConnectEvent, INFINITE); // connect ��� ó������ 1�̾ ������
			GetDlgItemText(hDlg, IDC_EDIT1, ip, 100); // ip
			GetDlgItemText(hDlg, IDC_EDIT2, port, 100); // port
			GetDlgItemText(hDlg, IDC_EDIT4, room, 5); // room
			GetDlgItemText(hDlg, IDC_EDIT6, userID, 10); // id
			// write ��ȣ
			SetEvent(hWriteEvent); // ip port������ �˷��ֱ� ���ؼ� hwrite ��ȣ���·� ����
			SetFocus(hEditid); // id�����ϸ� ���ʹ������
			SendMessage(hEditip, EM_SETSEL, 0, -1);
			SendMessage(hEditport, EM_SETSEL, 0, -1);
			SendMessage(hEditroom, EM_SETSEL, 0, -1);
			SendMessage(hEditid, EM_SETSEL, 0, -1);
			EnableWindow(hconnectbutton, FALSE); // Ŀ��Ʈ ������ Ŀ��Ʈ ��ư ��Ȱ��ȭ

			ResetEvent(hConnectEvent); // Ŀ��Ʈ ����
			SetEvent(hConnect2Event); // Ŀ��Ʈ2 ����
			return TRUE;
		case ID_SEND: // send��ư
			WaitForSingleObject(hReadEvent, INFINITE); // �޽��� ���������� ���
			GetDlgItemText(hDlg, IDC_EDIT7 , buf, BUFSIZE+1); // messageĭ�� ������ �޾ƿ�
			// write ��ȣ
			SetEvent(hWriteEvent); // �޽��� ���� �˷���
			SetFocus(hEditmessage);
			SendMessage(hEditmessage, EM_SETSEL, 0, -1);
			return TRUE;
		case ID_DISPLAY: // display��ư
			EnableWindow(hdisplaybutton, FALSE); // display ��ư ��Ȱ��ȭ
			EnableWindow(hsendbutton, FALSE); // send ��ư ��Ȱ��ȭ
			// ���÷��� ��û�����ߵ�
			retval = send(sock, display1, strlen(display1), 0);
			if(retval == SOCKET_ERROR){
				err_display("send()");
				return 0;
			}
			EnableWindow(hdisplaybutton, TRUE); // display ��ư Ȱ��ȭ
			EnableWindow(hsendbutton, TRUE); // send ��ư Ȱ��ȭ Ȱ��ȭ
			return TRUE;
		case ID_CANCEL: // �̰� �� �׷������� �𸣰ڴµ� EXIT��ư ���� X������ �ȴ���
			EndDialog(hDlg, ID_CANCEL);
			return TRUE;
		}

		return FALSE;
	}
	return FALSE;
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	// ��´���� hEditprint��
	int nLength = GetWindowTextLength(hEditprint);
	SendMessage(hEditprint, EM_SETSEL, nLength, nLength);
	SendMessage(hEditprint, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
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
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;
	HANDLE hThread;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	WaitForSingleObject(hWriteEvent, INFINITE); // Ŀ��Ʈ��ư ���� ip port�� �Է� �Ϸ� ��ٸ���
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	serveraddr.sin_port = htons(atoi(port));

	while(1)
	{
		WaitForSingleObject(hConnect2Event, INFINITE); // Ŀ��Ʈ2��� ��, Ŀ��Ʈ ��ư �۵� �� �Ϸ� ��ٸ�

		// socket()
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock == INVALID_SOCKET) err_quit("socket()");
		// connect()
		retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
		if(retval == SOCKET_ERROR) err_quit("connect()");

		// �� ��ȣ ����.
		send(sock, room,strlen(room),0);
		if(retval == SOCKET_ERROR){
			err_display("send() Ŭ�� ��");
			return 0;
		}
		// ���̵� ����.
		send(sock, userID, strlen(userID),0);
		if(retval == SOCKET_ERROR){
			err_display("send() Ŭ�� �ٽ�"); 
			return 0;
		}

		// ���̵� �ߺ� ���� �ޱ�
		char duplicate_check[11];
		// �ߺ��޽��� �ޱ�
		retval = recv(sock, duplicate_check, 11, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv() Ŭ�� ���̵� �ߺ� üũ");
			return 0;
		}
		duplicate_check[retval] = '\0';

		if(strcmp(duplicate_check, "�ƴϴ�!@!@") == 0) // �ߺ��ƴϸ� �������� �����ϰ�
		{
			EnableWindow(hsendbutton, TRUE); // send ��ư Ȱ��ȭ
			EnableWindow(hdisplaybutton, TRUE); // display ��ư Ȱ��ȭ
			break;
		}
		else if(strcmp(duplicate_check, "�ߺ���!@!@") == 0) // �ߺ��̸�
		{
			DisplayText("���̵� �ߺ���! �ٸ� ���̵� �Է�!\n");
			closesocket(sock); // ���� �ݰ�
			SetEvent(hConnectEvent); // Ŀ��Ʈ ��ư �ٽ� �ѾߵǴ� Ŀ��Ʈ �̺�Ʈ ����
			EnableWindow(hconnectbutton, TRUE); // Ŀ��Ʈ ��ư �ٽ� ����
			ResetEvent(hConnect2Event);
		}
	}

	// ������ ������ ���
	// ������ ����
	hThread = CreateThread(NULL, 0, ProcessInputSend, NULL, 0, NULL);
	if (hThread == NULL) {
		printf("fail make thread\n");
	}
	else {
		CloseHandle(hThread);
	}

	while(1){
		// ������ �ޱ�

		retval = recv(sock, buf,  BUFSIZE+1, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		}
		else if(retval == 0)
			break;

		// ���� ������ ���
		buf[retval] = '\0';
		DisplayText("%s\n", buf);

		SetEvent(hReadEvent);
	}

	return 0;
}

/* ����� �Է� */
DWORD WINAPI ProcessInputSend(LPVOID arg)
{

	int retval, len;		// ������ �Է�
	char line[BUFSIZE+11];
	while(1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // send ���������� ���
		// '\n' ���� ����
		len = strlen(buf);
		if(buf[len-1] == '\n')
			buf[len-1] = '\0';

		sprintf(line, "[%s] : %s", userID, buf); // ���̵�� �޽����� ���ļ� ����.

		// ������ ������
		retval = send(sock, line, strlen(line), 0);
		if(retval == SOCKET_ERROR){
			err_display("send()");
			return 0;
		}
		// printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", retval);
	}
}