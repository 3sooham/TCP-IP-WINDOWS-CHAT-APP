// 2020년 1학기 네트워크프로그래밍 숙제 3번
// 설명: 최원민 학번: 15011056
// 플랫폼: VS2010

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

// 이버전은 echo밖에안됨.

#define SERVERIP   "127.0.0.1" // 서버 ip
#define SERVERPORT 9000 // 서버 포트
#define BUFSIZE    512

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM); // 대화상자 처리하는 사용자 정의 함수
// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg); // 클라 쓰레드
DWORD WINAPI ProcessInputSend(LPVOID arg);

SOCKET sock; // 소켓
char buf[BUFSIZE+1]; // 데이터 송수신 버퍼
char ip[100]; // 이거 클라 내가 만든거랑 이름 같게해서 혼선줄이기
char port[100]; 
char userID[10];
char room[5];
char *display1 = "/display";
int retval;
HANDLE hReadEvent, hWriteEvent, hDisplayEvent, hConnectEvent, hConnect2Event; // 이벤트
HWND hconnectbutton, hsendbutton, hdisplaybutton, hexit; // 보내기 버튼
HWND hEditip, hEditport, hEditroom, hEditid, hEditmessage, hEditprint; // 편집 컨트롤

// 메인 함수
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성 // 동기화 용임
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL); // 신호상태로 시작
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // 비 신호 상태로 시작
	if(hWriteEvent == NULL) return 1;
	hDisplayEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // 비 신호 상태로 시작
	if(hWriteEvent == NULL) return 1;
	hConnectEvent = CreateEvent(NULL, FALSE, TRUE, NULL); // 신호 상태로 시작
	if(hWriteEvent == NULL) return 1;
	hConnect2Event = CreateEvent(NULL, FALSE, FALSE, NULL); // 신호 상태로 시작
	if(hWriteEvent == NULL) return 1;

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성 dlgProc에서 대화상자 돌림.
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);
	CloseHandle(hDisplayEvent);
	// 다른 이벤트들도 나중에 제거하기

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저 // 여기에 전달되는 인자는 윈도우 프로시저와 같다
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
	case WM_INITDIALOG:
		// 편집 컨트롤
		hEditip = GetDlgItem(hDlg, IDC_EDIT1);
		hEditport = GetDlgItem(hDlg, IDC_EDIT2);
		hEditroom = GetDlgItem(hDlg, IDC_EDIT4);
		hEditid = GetDlgItem(hDlg, IDC_EDIT6);
		hEditmessage = GetDlgItem(hDlg, IDC_EDIT7);
		hEditprint = GetDlgItem(hDlg, IDC_EDIT5);
		// 버튼
		hconnectbutton = GetDlgItem(hDlg, ID_CONNECT); // 어디에서든 보내기 버튼에 접근하기위해서 핸들값 지정
		hsendbutton = GetDlgItem(hDlg, ID_SEND);
		hdisplaybutton = GetDlgItem(hDlg, ID_DISPLAY);
		hexit = GetDlgItem(hDlg, ID_EXIT);
		EnableWindow(hsendbutton, FALSE); // send 버튼 비활성
		EnableWindow(hdisplaybutton, FALSE); // display 버튼 비활성

		// 입력란 최대 입력수 설정
		SendMessage(hEditip, EM_SETLIMITTEXT, 100, 0); // sendmessage는 hedit1에 써지는것임
		SendMessage(hEditport, EM_SETLIMITTEXT, 100, 0);
		SendMessage(hEditroom, EM_SETLIMITTEXT, 5, 0);
		SendMessage(hEditid, EM_SETLIMITTEXT, 10, 0);

		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case ID_CONNECT: // connect버튼
			EnableWindow(hsendbutton, FALSE); // 버튼을 연속 2번 못누르도록 보내기 버튼 비활성화
			WaitForSingleObject(hConnectEvent, INFINITE); // connect 대기 처음에는 1이어서 지나감
			GetDlgItemText(hDlg, IDC_EDIT1, ip, 100); // ip
			GetDlgItemText(hDlg, IDC_EDIT2, port, 100); // port
			GetDlgItemText(hDlg, IDC_EDIT4, room, 5); // room
			GetDlgItemText(hDlg, IDC_EDIT6, userID, 10); // id
			// write 신호
			SetEvent(hWriteEvent); // ip port받은거 알려주기 위해서 hwrite 신호상태로 만듬
			SetFocus(hEditid); // id까지하면 엔터누르면됨
			SendMessage(hEditip, EM_SETSEL, 0, -1);
			SendMessage(hEditport, EM_SETSEL, 0, -1);
			SendMessage(hEditroom, EM_SETSEL, 0, -1);
			SendMessage(hEditid, EM_SETSEL, 0, -1);
			EnableWindow(hconnectbutton, FALSE); // 커넥트 했으니 커넥트 버튼 비활성화

			ResetEvent(hConnectEvent); // 커넥트 꺼줌
			SetEvent(hConnect2Event); // 커넥트2 켜줌
			return TRUE;
		case ID_SEND: // send버튼
			WaitForSingleObject(hReadEvent, INFINITE); // 메시지 받을때까지 대기
			GetDlgItemText(hDlg, IDC_EDIT7 , buf, BUFSIZE+1); // message칸에 적힌거 받아옴
			// write 신호
			SetEvent(hWriteEvent); // 메시지 받음 알려줌
			SetFocus(hEditmessage);
			SendMessage(hEditmessage, EM_SETSEL, 0, -1);
			return TRUE;
		case ID_DISPLAY: // display버튼
			EnableWindow(hdisplaybutton, FALSE); // display 버튼 비활성화
			EnableWindow(hsendbutton, FALSE); // send 버튼 비활성화
			// 디스플레이 요청보내야됨
			retval = send(sock, display1, strlen(display1), 0);
			if(retval == SOCKET_ERROR){
				err_display("send()");
				return 0;
			}
			EnableWindow(hdisplaybutton, TRUE); // display 버튼 활성화
			EnableWindow(hsendbutton, TRUE); // send 버튼 활성화 활성화
			return TRUE;
		case ID_CANCEL: // 이거 왜 그런건지는 모르겠는데 EXIT버튼 말고 X누르면 안닫힘
			EndDialog(hDlg, ID_CANCEL);
			return TRUE;
		}

		return FALSE;
	}
	return FALSE;
}

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	// 출력대상은 hEditprint임
	int nLength = GetWindowTextLength(hEditprint);
	SendMessage(hEditprint, EM_SETSEL, nLength, nLength);
	SendMessage(hEditprint, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
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
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
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

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;
	HANDLE hThread;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	WaitForSingleObject(hWriteEvent, INFINITE); // 커넥트버튼 에서 ip port등 입력 완료 기다리기
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	serveraddr.sin_port = htons(atoi(port));

	while(1)
	{
		WaitForSingleObject(hConnect2Event, INFINITE); // 커넥트2대기 즉, 커넥트 버튼 작동 다 완료 기다림

		// socket()
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock == INVALID_SOCKET) err_quit("socket()");
		// connect()
		retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
		if(retval == SOCKET_ERROR) err_quit("connect()");

		// 방 번호 보냄.
		send(sock, room,strlen(room),0);
		if(retval == SOCKET_ERROR){
			err_display("send() 클라 방");
			return 0;
		}
		// 아이디 보냄.
		send(sock, userID, strlen(userID),0);
		if(retval == SOCKET_ERROR){
			err_display("send() 클라 다시"); 
			return 0;
		}

		// 아이디 중복 여부 받기
		char duplicate_check[11];
		// 중복메시지 받기
		retval = recv(sock, duplicate_check, 11, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv() 클라 아이디 중복 체크");
			return 0;
		}
		duplicate_check[retval] = '\0';

		if(strcmp(duplicate_check, "아니다!@!@") == 0) // 중복아니면 다음으로 진행하고
		{
			EnableWindow(hsendbutton, TRUE); // send 버튼 활성화
			EnableWindow(hdisplaybutton, TRUE); // display 버튼 활성화
			break;
		}
		else if(strcmp(duplicate_check, "중복임!@!@") == 0) // 중복이면
		{
			DisplayText("아이디 중복임! 다른 아이디 입력!\n");
			closesocket(sock); // 소켓 닫고
			SetEvent(hConnectEvent); // 커넥트 버튼 다시 켜야되니 커넥트 이벤트 켜줌
			EnableWindow(hconnectbutton, TRUE); // 커넥트 버튼 다시 켜줌
			ResetEvent(hConnect2Event);
		}
	}

	// 서버와 데이터 통신
	// 스레드 생성
	hThread = CreateThread(NULL, 0, ProcessInputSend, NULL, 0, NULL);
	if (hThread == NULL) {
		printf("fail make thread\n");
	}
	else {
		CloseHandle(hThread);
	}

	while(1){
		// 데이터 받기

		retval = recv(sock, buf,  BUFSIZE+1, 0);
		if(retval == SOCKET_ERROR){
			err_display("recv()");
			break;
		}
		else if(retval == 0)
			break;

		// 받은 데이터 출력
		buf[retval] = '\0';
		DisplayText("%s\n", buf);

		SetEvent(hReadEvent);
	}

	return 0;
}

/* 사용자 입력 */
DWORD WINAPI ProcessInputSend(LPVOID arg)
{

	int retval, len;		// 데이터 입력
	char line[BUFSIZE+11];
	while(1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // send 눌릴때까지 대기
		// '\n' 문자 제거
		len = strlen(buf);
		if(buf[len-1] == '\n')
			buf[len-1] = '\0';

		sprintf(line, "[%s] : %s", userID, buf); // 아이디랑 메시지랑 합쳐서 보냄.

		// 데이터 보내기
		retval = send(sock, line, strlen(line), 0);
		if(retval == SOCKET_ERROR){
			err_display("send()");
			return 0;
		}
		// printf("[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);
	}
}