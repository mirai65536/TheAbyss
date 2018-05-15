#include "../headers/all.h"


Client::Client(int Fd,uchar Id,World* world,ClientList* par)
{
  parent = par;
  curWorld = world;
  fd = Fd;
  id = Id;
  xpos = 10;
  ypos = 50;
  zpos = 10;
  open = true;

  if (send(fd, &id, sizeof(id), 0) == -1)
  {
      std::cout << "ERROR: failed to send id message." << std::endl;
  }

  parent->sendInitAll(this);
  parent->retClients(this);
  sendThread = std::thread(&Client::sendMessages,this);
  recvThread = std::thread(&Client::recvMessages,this);
  sendThread.detach();
  recvThread.detach();
}

int Client::getFD()
{
  return fd;
}
void Client::setPos(glm::vec3 newPos)
{
  xpos = newPos.x;
  ypos = newPos.y;
  zpos = newPos.z;
}

std::shared_ptr<Message> Client::getInfo()
{
  std::shared_ptr<Message> tmp(new Message(90,id,0,0,*(int*)&xpos,*(int*)&ypos,*(int*)&zpos,NULL));
  return tmp;
}

void Client::sendMessages()
{
  while(open)
  {
    queueMutex.lock();
    if(msgQueue.empty())
    {
      queueMutex.unlock();
      continue;
    }

    std::shared_ptr<Message> m = msgQueue.front();
    int arr[5];
    arr[0] = ((m->opcode << 24) | (m->ext1 << 16) | (m->ext2 << 8) | m->ext3);
    arr[1] = m->x;
    arr[2] = m->y;
    arr[3] = m->z;
    arr[4] = m->data == NULL ? 0 : m->data->length();


    if (send(fd, arr, 5*sizeof(int), 0) == -1)
    {
        std::cout << "ERROR: failed to send message" << std::endl;
    }

    if(m->data != NULL)
    {
      if (send(fd, (void *)(m->data->data()), arr[4], 0) == -1)
      {
          std::cout << "ERROR: failed to send chunk data." << std::endl;
      }
    }
    msgQueue.front() = NULL;
    msgQueue.pop();
    queueMutex.unlock();
  }

  int arr[5];
  arr[0] = 0xFFFFFFFF;
  if (send(fd, arr, 5*sizeof(int), 0) == -1)
  {
      std::cout << "ERROR: failed to send exit message." << std::endl;
  }
}

void Client::recvMessages()
{
  while(open)
  {
    int buf[4];
    if(recv(fd,buf,4*sizeof(int),0)<0)
    {
      std::cout << "Error receiving message \n";
      return;
    }
    uchar opcode = (buf[0] >> 24) & 0xFF;
    uchar ext1 = (buf[0] >> 16) & 0xFF;
    uchar ext2 = (buf[0] >> 8) & 0xFF;
    uchar ext3 = buf[0] & 0xFF;
    int x = buf[1];
    int y = buf[2];
    int z = buf[3];
    //std::cout << "Message is:" <<(int)opcode<<":"<<(int)ext1<<":"<<(int)ext2<<":"<<(int)ext3<<":"<<buf[1]<<":"<<buf[2]<<":"<<buf[3]<<":"<< "\n";

    switch(opcode)
    {
      case (0):
        //curWorld->sendChunk(x,y,z,fd);
        sendChunk(x,y,z);
        break;
      case (1):
        curWorld->delBlock(x,y,z);
        sendDelBlockAll(x,y,z);
        break;
      case (2):
        curWorld->addBlock(x,y,z,ext1);
        sendAddBlockAll(x,y,z,ext1);
        break;
      case (91):
        sendPositionAll(*(float*)&x,*(float*)&y,*(float*)&z);
        break;
      case (0xFF):
        open = false;
        break;
      default:
        std::cout << "Unknown opcode: " << (int)opcode << "\n";
    }
  }
  std::cout << "Client Disconnecting \n";
  parent->remove(id);
}


void Client::sendPositionAll(float x, float y,float z)
{
  setPos(glm::vec3(x,y,z));

  std::shared_ptr<Message> tmp(new Message(91,id,0,0,*(int*)&x,*(int*)&y,*(int*)&z,NULL));
  parent->messageAll(tmp);
}

void Client::sendAddBlockAll(int x, int y, int z, uchar id)
{
  std::shared_ptr<Message> tmp(new Message(2,id,0,0,x,y,z,NULL));
  parent->messageAll(tmp);
}

void Client::sendChunk(int x,int y,int z)
{
  curWorld->generateChunk(x,y,z);
  std::shared_ptr<std::string> msg = curWorld->getChunk(x,y,z)->getCompressedChunk();
  std::shared_ptr<Message> tmp(new Message(0,0,0,0,x,y,z,msg));
  queueMutex.lock();
  msgQueue.push(tmp);
  queueMutex.unlock();
}
void Client::sendChunkAll(int x,int y,int z)
{
  curWorld->generateChunk(x,y,z);
  std::shared_ptr<std::string> msg = curWorld->getChunk(x,y,z)->getCompressedChunk();
  std::shared_ptr<Message> tmp(new Message(0,0,0,0,x,y,z,msg));
  parent->messageAll(tmp);
}
void Client::sendDelBlockAll(int x, int y, int z)
{
  std::shared_ptr<Message> tmp(new Message(1,0,0,0,x,y,z,NULL));
  parent->messageAll(tmp);
}

void Client::sendExit()
{
  int arr[5];
  arr[0] = 0xFFFFFFFF;
  if (send(fd, arr, 5*sizeof(int), 0) == -1)
  {
      std::cout << "ERROR: failed to send exit message." << std::endl;
  }
}

ClientList::ClientList(World* temp)
{
  curWorld = temp;
}

void ClientList::add(int fd)
{
  clientMutex.lock();
  for(int i = 0;i<MAX_CLIENTS;i++)
  {
    if(clients[i] == NULL)
    {
      std::cout << "Adding client " << i << '\n';
      clientMutex.unlock();
      std::shared_ptr<Client> tmp(new Client(fd,i,curWorld,this));
      clientMutex.lock();
      clients[i] = tmp;
      clientMutex.unlock();
      break;
    }
  }
}
void ClientList::remove(int id)
{
  std::cout << clients[id].use_count() << "\n";
  clientMutex.lock();
  clients[id] = NULL;
  clientMutex.unlock();
  std::shared_ptr<Message> tmp(new Message(99,id,0,0,0,0,0,NULL));
  messageAll(tmp);
}

void ClientList::sendInitAll(Client* target)
{
  clientMutex.lock();
  std::shared_ptr<Message> msg = target->getInfo();
  for(int i =0;i<MAX_CLIENTS;i++)
  {
    std::shared_ptr<Client> curClient = clients[i];
    if(curClient != NULL && curClient.get() != target)
    {
      curClient->queueMutex.lock();
      curClient->msgQueue.push(msg);
      curClient->queueMutex.unlock();
    }
  }
  clientMutex.unlock();
}

void ClientList::retClients(Client* target)
{
  clientMutex.lock();
  for(int i =0;i<MAX_CLIENTS;i++)
  {
    std::shared_ptr<Client> curClient = clients[i];
    if(curClient != NULL && curClient.get() != target)
    {
      target->queueMutex.lock();
      target->msgQueue.push(curClient->getInfo());
      target->queueMutex.unlock();
    }
  }
  clientMutex.unlock();
}

void ClientList::messageAll(std::shared_ptr<Message> msg)
{
  clientMutex.lock();
  for(int i=0;i<MAX_CLIENTS;i++)
  {

    std::shared_ptr<Client> curClient = clients[i];
    if(curClient != NULL)
    {
      curClient->queueMutex.lock();
      curClient->msgQueue.push(msg);
      curClient->queueMutex.unlock();
    }
  }
   clientMutex.unlock();
}
