#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501  /* Windows XP. */
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
  #include <unistd.h> /* Needed for close() */
  #include <fcntl.h>
#endif
#include <errno.h>
#include <string>
#include <iostream>
#include <fstream>
#include "include/world.h"
#include "include/messenger.h"

int connect_wait (
	int sockno,
	struct sockaddr * addr,
	size_t addrlen,
	struct timeval * timeout)
{
	int res, opt;

	// get socket flags
	if ((opt = fcntl (sockno, F_GETFL, NULL)) < 0) {
		return -1;
	}

	// set socket non-blocking
	if (fcntl (sockno, F_SETFL, opt | O_NONBLOCK) < 0) {
		return -1;
	}

	// try to connect
	if ((res = connect (sockno, addr, addrlen)) < 0) {
		if (errno == EINPROGRESS) {
			fd_set wait_set;

			// make file descriptor set with socket
			FD_ZERO (&wait_set);
			FD_SET (sockno, &wait_set);

			// wait for socket to be writable; return after given timeout
			res = select (sockno + 1, NULL, &wait_set, NULL, timeout);
		}
	}
	// connection was successful immediately
	else {
		res = 1;
	}

	// reset socket flags
	if (fcntl (sockno, F_SETFL, opt) < 0) {
		return -1;
	}

	// an error occured in connect or select
	if (res < 0) {
		return -1;
	}
	// select timed out
	else if (res == 0) {
		errno = ETIMEDOUT;
		return 1;
	}
	// almost finished...
	else {
		socklen_t len = sizeof (opt);

		// check for errors in socket layer
		if (getsockopt (sockno, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
			return -1;
		}

		// there was an error
		if (opt) {
			errno = opt;
			return -1;
		}
	}

	return 0;
}



void Messenger::setupSockets(std::string ipAddress,std::string port)
{
  std::cout << ipAddress << "\n";
#ifdef _WIN32
  WSADATA wsa_data;
  int initResult;
  initResult = WSAStartup(MAKEWORD(2,2), &wsa_data);
  if (initResult != 0)
  {
      throw "WSAStartup failed, ERROR CODE: ";
      return;
  }

  // addrinfo contains a sockaddr struct
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  // initialize the addrInfo structs
  ZeroMemory(&hints, sizeof(hints));  // fills a block of memory with 0s
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(ipAddress.c_str(),port.c_str(), &hints, &result) != 0)
  {
    throw "getaddrinfo failed";
    WSACleanup();
    return;
  }

  // attempt to connect to the first address returned by the call to getaddrinfo
  ptr = result;
  // create a SOCKET for the connecting server
  fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  // error check to make sure it is a valid socket
  if (fd == INVALID_SOCKET)
  {
      throw "Error at socket(): " + WSAGetLastError();
      freeaddrinfo(result);
      WSACleanup();
      return;
  }
  if(connect(fd, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
  {
    throw "Error connecting to server\n";
    closesocket(fd);
    fd = INVALID_SOCKET;
    return;
  }
#else
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
      throw "ERROR: failed to create socket.";
      return;
  }

  sockaddr_in serveraddress;
  serveraddress.sin_family = AF_INET;
  serveraddress.sin_port = htons(stoi(port));

  inet_pton(AF_INET, ipAddress.c_str(), &(serveraddress.sin_addr));


  struct timeval  timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  // connect to the server
  std::cout << "Attempting to connect to server\n";
  if (connect_wait(fd, (sockaddr*) &serveraddress, sizeof(serveraddress),&timeout) != 0)
  {
      throw "ERROR: failed to connect to server.";
  }


  std::cout << "Connected\n";
  #endif
  std::cout << "Successfully connected to server" << std::endl;


}

void Messenger::disconnect()
{
  requestExit();
  close(fd);
}

InMessage Messenger::receiveAndDecodeMessage()
{
  int buf[5];
  receiveMessage(buf,sizeof(int)*5);
  uint8_t opcode = (buf[0] >> 24) & 0xFF;
  uint8_t ext1 = (buf[0] >> 16) & 0xFF;
  uint8_t ext2 = (buf[0] >> 8) & 0xFF;
  uint8_t ext3 = buf[0] & 0xFF;
  InMessage msg = InMessage(opcode,ext1,ext2,ext3,buf[1],buf[2],buf[3],buf[4]);
  return msg;

}


void Messenger::pingStart()
{
  lastPing = std::chrono::system_clock::now();
  int32_t request[5];
  request[0] = pack4chars(250,0,0,0);
  request[1] = 0;
  request[2] = 0;
  request[3] = 0;
  request[4] = 0;
  try
  {
    sendMessage(request,sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: SENDING PING, ERRNO: " << errno << "\n";
  }
}

double Messenger::pingEnd()
{
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> dif = end - lastPing;
  double pingInMS = (dif.count())*1000;
  return pingInMS;
}


void Messenger::createViewDirectionChangeRequest(float x, float y, float z)
{
  OutMessage tmp = OutMessage(92,0,0,0,x,y,z,0);
  messageQueue.push(tmp);
}

void Messenger::createChatMessage(const std::string& msg)
{
  OutMessage newMsg = OutMessage(100,0,0,0,0,0,0,std::make_shared<std::string> (msg));
  messageQueue.push(newMsg);
}

void Messenger::requestChunk(int x, int y, int z)
{
  int32_t request[5];
  request[0] = 0;
  request[1] = x;
  request[2] = y;
  request[3] = z;
  request[4] = 0;
  try
  {
    sendMessage(request,sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: REQUESTING CHUNK, ERRNO: " << errno << "\n";
  }
}

void Messenger::sendChatMessage(std::shared_ptr<std::string> msg)
{
  int32_t request[5];
  request[0] = pack4chars(100,0,0,0);
  request[1] = 0;
  request[2] = 0;
  request[3] = 0;
  request[4] = msg->length();
  try
  {
    sendMessage(request,sizeof(request));
    sendMessage(msg->c_str(),msg->length());
  }
  catch(...)
  {
    std::cout << "ERROR: SENDING MESSAGE, ERRNO: " << errno << "\n";
  }

}

void Messenger::createPingRequest()
{
  OutMessage tmp = OutMessage(250,0,0,0,0,0,0,0);
  messageQueue.push(tmp);
}

void Messenger::createMoveRequest(float x, float y, float z)
{
  OutMessage tmp = OutMessage(91,0,0,0,x,y,z,0);
  messageQueue.push(tmp);
}

void Messenger::createAddBlockRequest(int x, int y, int z, uint8_t id)
{
  OutMessage tmp = OutMessage(2,id,0,0,x,y,z,0);
  messageQueue.push(tmp);
}

void Messenger::requestAddBlock(int x, int y, int z, uint8_t id)
{
  int32_t request[5];
  request[0] = pack4chars(2,id,0,0);
  request[1] = x;
  request[2] = y;
  request[3] = z;
  request[4] = 0;
  try
  {
    sendMessage(request,sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: REQUESTING BLOCK, ERRNO: " << errno << "\n";
  }
}

void Messenger::createDelBlockRequest(int x, int y, int z)
{
  OutMessage tmp = OutMessage(1,0,0,0,x,y,z,0);
  messageQueue.push(tmp);
}

void Messenger::requestDelBlock(int x, int y, int z)
{
  int32_t request[5];
  request[0] = pack4chars(1,0,0,0);
  request[1] = x;
  request[2] = y;
  request[3] = z;
  request[4] = 0;
  try
  {
    sendMessage(request,sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: REQUESTING BLOCK DELETION, ERRNO: " << errno << "\n";
  }
}

void Messenger::requestMove(float x, float y, float z)
{
  int32_t request[5];
  request[0] = pack4chars(91,0,0,0);
  request[1] = floatBitsToInt(x);
  request[2] = floatBitsToInt(y);
  request[3] = floatBitsToInt(z);
  request[4] = 0;
  try
  {
    sendMessage(request, sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: REQUESTING PLAYER MOVE, ERRNO: " << errno << "\n";
  }
}

void Messenger::requestViewDirectionChange(float x,float y, float z)
{
  int32_t request[5];
  request[0] = pack4chars(92,0,0,0);
  request[1] = floatBitsToInt(x);
  request[2] = floatBitsToInt(y);
  request[3] = floatBitsToInt(z);
  request[4] = 0;
  try
  {
    sendMessage(request, sizeof(request));
  }
  catch(...)
  {
    std::cout << "ERROR: REQUESTING PLAYER CHANGE VIEW, ERRNO: " << errno << "\n";
  }
}

void Messenger::requestExit()
{
  int32_t request[5];
  request[0] = 0xFFFFFFFF;
  sendMessage(request,sizeof(request));
  std::cout << "exit message Sent\n";
}


void Messenger::receiveMessage(void *buffer,int length)
{
  uint8_t* buf = (uint8_t*)buffer;
  int totalReceived = 0;
  while(totalReceived<length)
  {
    int curReceived = recv(fd,buf+totalReceived,length-totalReceived,0);
    if(curReceived == -1)
    {
      throw -1;
      return;
    }
    totalReceived += curReceived;
  }
}

void Messenger::sendMessage(const void* buffer, int length)
{
  uint8_t* buf = (uint8_t*)buffer;
  int totalSent = 0;
  while(totalSent<length)
  {
    int curSent = send(fd,buf+totalSent,length-totalSent,0);
    if( curSent == -1)
    {
      throw -1;
      return;
    }
    totalSent += curSent;
  }
}

void Messenger::createChunkRequest(int x, int y, int z)
{
  if(!requestMap.exists(glm::ivec3(x,y,z)))
  {
    OutMessage tmp = {0,0,0,0,x,y,z,0};
    messageQueue.push(tmp);
    requestMap.add(glm::ivec3(x,y,z),true);
  }
}
