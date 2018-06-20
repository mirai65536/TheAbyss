
#include <string>
#include <fstream>
#include <sstream>

#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>
#include "../headers/bsp.h"

int totalChunks;


BSPNode::BSPNode(int x,int y,int z,const std::string &wName)
{
  //std::cout << totalChunks << "\n";

  curBSP = BSP(x,y,z,wName);

  isGenerated = false;
  nextNode = NULL;
  prevNode = NULL;
  inUse = false;
  toRender = false;
  toBuild = false;
  toDelete = false;
  totalChunks++;
}

BSPNode::~BSPNode()
{
  totalChunks--;
}

BSPNode::BSPNode(int x,int y, int z,const std::string &wName,const std::string &val)
{
  //std::cout << "generating chunk" << x << ":" << y << ":" << z << " with size" << val.size() << "\n";
  BSPMutex.lock();
  curBSP = BSP(x,y,z,wName,val);
  isGenerated = false;
  nextNode = NULL;
  prevNode = NULL;
  inUse = false;
  toRender = false;
  toBuild = false;
  toDelete = false;
  totalChunks++;
  BSPMutex.unlock();
}

void BSPNode::generateTerrain()
{
  curBSP.generateTerrain();
}

void BSPNode::build()
{
  BSPMutex.lock();
  inUse = true;
  curBSP.build(rightChunk,leftChunk,topChunk,bottomChunk,frontChunk,backChunk);
  toRender = true;
  toBuild = false;
  inUse = false;
  BSPMutex.unlock();
}

void BSPNode::drawOpaque()
{
  if(toRender == true)
  {
     curBSP.render();
     toRender = false;
  }
  curBSP.drawOpaque();
}

void BSPNode::drawTranslucent()
{

  inUse = true;
  if(toRender == true)
  {
     curBSP.render();
     toRender = false;
  }
  curBSP.drawTranslucent();
  inUse = false;
}

void BSPNode::saveChunk()
{
  curBSP.saveChunk();
}

void BSPNode::del()
{
  BSPMutex.lock();
  if(nextNode != NULL) nextNode->prevNode = prevNode;
  if(prevNode != NULL) prevNode->nextNode = nextNode;
  curBSP.freeGL();
  nextNode = NULL;
  prevNode = NULL;


  BSPMutex.unlock();
}

void BSPNode::disconnect()
{
  BSPMutex.lock();
  toDelete = true;
  if(leftChunk != NULL) leftChunk->rightChunk = NULL;
  if(rightChunk != NULL) rightChunk->leftChunk = NULL;
  if(frontChunk != NULL) frontChunk->backChunk = NULL;
  if(backChunk != NULL) backChunk->frontChunk = NULL;
  if(topChunk != NULL) topChunk->bottomChunk = NULL;
  if(bottomChunk != NULL) bottomChunk->topChunk = NULL;
  BSPMutex.unlock();
}

bool BSPNode::blockExists(int x, int y, int z)
{
  return curBSP.blockExists(x, y, z);
}

void BSPNode::delBlock(int x, int y, int z)
{
  curBSP.delBlock(x, y, z);
}

void BSPNode::addBlock(int x, int y, int z, int id)
{
  curBSP.addBlock(x,y,z,id);
}

int BSPNode::blockVisibleType(int x, int y, int z)
{
  return curBSP.blockVisibleType(x, y, z);
}



BSP::BSP(int x, int y, int z,const std::string &wName,const std::string &val)
{
  xCoord = x;
  yCoord = y;
  zCoord = z;
  for(int x = 0;x<CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;x++) worldMap[x] = 0;
  using namespace std;
  worldName = wName;

  const int numbOfBlocks = CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;
  int i = 0;
  char curId = 0;
  unsigned short curLength = 0;
  const char* data = val.c_str();
  unsigned int index=0;

  while(i<numbOfBlocks)
  {
    curId = data[index];
    curLength = 0;
    index++;
    for(int j=0;j<sizeof(curLength);j++)
    {
      curLength += ((uchar) data[index] << (j*8));
      index++;
    }
    for(int j = 0; j<curLength; j++)
    {
      if(i+j>numbOfBlocks)
      {
        std::cout << "ERROR CORRUPTED CHUNK AT " << x << ":" << y << ":" << z <<"\n";
      }
      worldMap[i+j] = curId;
    }
    i+= curLength;
  }




  oVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  oIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
  tVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  tIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);

  oVertices = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  oIndices  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
  tVertices = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  tIndices = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
}

BSP::BSP(int x, int y, int z,const std::string &wName)
{
    xCoord = x;
    yCoord = y;
    zCoord = z;
    worldName = wName;
    for(int x = 0;x<CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;x++) worldMap[x] = 0;
    //Intialize Buffers

    using namespace std;
    //The directoy to the chunk to be saved
    string directory = "saves/" + worldName + "/chunks/";
    string chunkName = to_string(x) + '_' + to_string(y) + '_' + to_string(z) + ".dat";
    string chunkPath = directory+chunkName;

    //Max size of a chunk
    int numbOfBlocks = CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;
    ifstream ichunk(chunkPath,ios::binary);

    //Checks if the file exists
    if(!ichunk.is_open())
    {
      /*
      generateTerrain();
      ichunk.close();
      ofstream ochunk(chunkPath,ios::binary);

      string compressed = compressChunk();
      ochunk << compressed;
      ochunk.close();
      */
    }
    else
    {
      int i = 0;
      char curId=0;
      unsigned short curLength=0;
      while(i<numbOfBlocks)
      {

        ichunk.read((char*) &curId,sizeof(curId));
        ichunk.read((char*) &curLength,sizeof(curLength));

        for(int j = 0; j<curLength; j++)
        {
          worldMap[i+j] = curId;
        }
        i+= curLength;
      }
      ichunk.close();
    }
    oVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
    oIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
    tVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
    tIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);

    oVertices = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
    oIndices  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
    tVertices = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
    tIndices = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
}

BSP::~BSP()
{
  saveChunk();
}

inline std::string BSP::compressChunk()
{
    using namespace std;
    ostringstream chunk(ios::binary);
    unsigned int curTotal = 1;
    int numbOfBlocks = CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;
    char lastId = worldMap[0];
    for(int i = 1; i<numbOfBlocks;i++)
    {
      if(lastId == worldMap[i])
      {
        curTotal++;
      }
      else
      {
        chunk.write((char*) &lastId,sizeof(lastId));
        chunk.write((char*) &curTotal,sizeof(curTotal));
        curTotal = 1;
        lastId = worldMap[i];
      }
    }
    chunk.write((char*) &lastId,sizeof(lastId));
    chunk.write((char*) &curTotal,sizeof(curTotal));
    return chunk.str();

}

inline void BSP::saveChunk()
{
  using namespace std;
  //The directoy to the chunk to be saved
  string directory = "saves/" + worldName + "/chunks/";
  string chunkName = to_string(xCoord) + '_' + to_string(yCoord) + '_' + to_string(zCoord) + ".dat";
  string chunkPath = directory+chunkName;

  //Max size of a chunk
  int numbOfBlocks = CHUNKSIZE*CHUNKSIZE*CHUNKSIZE;
  ofstream ochunk(chunkPath,ios::binary);
  unsigned int curTotal = 1;
  char lastId = worldMap[0];

  for(int i = 1; i<numbOfBlocks;i++)
  {
    if(lastId == worldMap[i])
    {
      curTotal++;
    }
    else
    {
      ochunk.write((char*) &lastId,sizeof(lastId));
      ochunk.write((char*) &curTotal,sizeof(curTotal));
      curTotal = 1;
      lastId = worldMap[i];
    }
  }
  ochunk.write((char*) &lastId,sizeof(lastId));
  ochunk.write((char*) &curTotal,sizeof(curTotal));
  ochunk.close();
}

void BSP::freeGL()
{
  //Frees all the used opengl resourses
  //MUST BE DONE IN THE MAIN THHREAD
  //Since that is the only thread with opengl context
  glDeleteBuffers(1,&oVBO);
  glDeleteBuffers(1,&oEBO);
  glDeleteVertexArrays(1,&oVAO);

  glDeleteBuffers(1,&tVBO);
  glDeleteBuffers(1,&tEBO);
  glDeleteVertexArrays(1,&tVAO);
}

BSP::BSP(){}





inline int BSP::addVertex(int renderType,const glm::vec3 &pos,const glm::vec3 &norm, float texX, float texY)
{
  if(renderType == 0)
  {
    int numbVert = oVerticesBuffer->size()/8;
    //Adds position vector
    oVerticesBuffer->push_back(pos.x);
    oVerticesBuffer->push_back(pos.y);
    oVerticesBuffer->push_back(pos.z);

    //Adds normal vector
    oVerticesBuffer->push_back(norm.x);
    oVerticesBuffer->push_back(norm.y);
    oVerticesBuffer->push_back(norm.z);

    //Adds the textureCoordinate
    oVerticesBuffer->push_back(texX);
    oVerticesBuffer->push_back(texY);
    //Returns the location of the vertice
    return numbVert;
  }
  else if(renderType == 1)
  {
    int numbVert = tVerticesBuffer->size()/8;
    //Adds position vector
    tVerticesBuffer->push_back(pos.x);
    tVerticesBuffer->push_back(pos.y);
    tVerticesBuffer->push_back(pos.z);

    //Adds normal vector
    tVerticesBuffer->push_back(norm.x);
    tVerticesBuffer->push_back(norm.y);
    tVerticesBuffer->push_back(norm.z);

    //Adds the textureCoordinate
    tVerticesBuffer->push_back(texX);
    tVerticesBuffer->push_back(texY);
    //Returns the location of the vertice
    return numbVert;
  }
}

inline void BSP::addIndices(int renderType,int index1, int index2, int index3, int index4)
{
  if(renderType == 0)
  {
    oIndicesBuffer->push_back(index1);
    oIndicesBuffer->push_back(index2);
    oIndicesBuffer->push_back(index3);

    oIndicesBuffer->push_back(index2);
    oIndicesBuffer->push_back(index4);
    oIndicesBuffer->push_back(index3);
  }
  else if(renderType == 1)
  {
    tIndicesBuffer->push_back(index1);
    tIndicesBuffer->push_back(index2);
    tIndicesBuffer->push_back(index3);

    tIndicesBuffer->push_back(index2);
    tIndicesBuffer->push_back(index4);
    tIndicesBuffer->push_back(index3);
  }
}


inline bool BSP::blockExists(int x, int y, int z)
{
  int id = worldMap[(int)x+CHUNKSIZE*(int)y+(int)z*CHUNKSIZE*CHUNKSIZE];
  if(id == 0 ) return false;
  else return true;
}

inline int BSP::blockVisibleType(int x, int y, int z)
{
  return ItemDatabase::blockDictionary[getBlock(x,y,z)].visibleType;
}

void BSP::addBlock(int x, int y, int z, char id)
{
  worldMap[x+y*CHUNKSIZE+z*CHUNKSIZE*CHUNKSIZE] = id;
}

inline void BSP::delBlock(int x, int y, int z)
{
  worldMap[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = 0;

}

inline int BSP::getBlock(int x, int y, int z)
{
  return worldMap[x+y*CHUNKSIZE+z*CHUNKSIZE*CHUNKSIZE];
}

inline glm::vec3 BSP::offset(float x, float y, float z)
{
  /*
  glm::vec3 newVec;
  long long int id = x*y*z+seed*32;
  newVec.x = id % (12355 % 1000-500)/(float)500 + x;
  newVec.y = id % (23413 % 1000-500)/(float)500 + y;
  newVec.z = id % (14351 % 1000-500)/(float)500 + z;
  */
  return glm::vec3(x,y,z);

}

void BSP::build(std::shared_ptr<BSPNode>  curRightChunk,std::shared_ptr<BSPNode>  curLeftChunk,std::shared_ptr<BSPNode>  curTopChunk,
                       std::shared_ptr<BSPNode>  curBottomChunk,std::shared_ptr<BSPNode>  curFrontChunk,std::shared_ptr<BSPNode>  curBackChunk)
{
  oVerticesBuffer->clear();
  oIndicesBuffer->clear();
  tVerticesBuffer->clear();
  tIndicesBuffer->clear();

  for(int x = 0; x<CHUNKSIZE;x++)
  {
     for(int y = 0;y<CHUNKSIZE;y++)
     {
       for(int z = 0;z<CHUNKSIZE;z++)
       {
         if(!blockExists(x,y,z)) continue;
         int renderType = blockVisibleType(x,y,z);

         float realX = x+CHUNKSIZE*xCoord;
         float realY = y+CHUNKSIZE*yCoord;
         float realZ = z+CHUNKSIZE*zCoord;

         bool topNeigh = false;
         bool bottomNeigh = false;
         bool leftNeigh = false;
         bool rightNeigh = false;
         bool frontNeigh = false;
         bool backNeigh = false;
         bool defaultNull = false;

         if(x+1 >= CHUNKSIZE)
         {
           if(curRightChunk != NULL)
           {
              if(renderType == curRightChunk->blockVisibleType(0,y,z)) rightNeigh = true;
           }
           else if(defaultNull) rightNeigh = true;
         }
         else if(renderType == blockVisibleType(x+1,y,z)) rightNeigh = true;

         if(x-1 < 0)
         {
           if(curLeftChunk != NULL)
           {
            if(renderType == curLeftChunk->blockVisibleType(CHUNKSIZE-1,y,z)) leftNeigh = true;
           }
           else if(defaultNull) leftNeigh = true;
         }
         else if(renderType == blockVisibleType(x-1,y,z)) leftNeigh = true;

         if(y+1 >= CHUNKSIZE)
         {
           if(curTopChunk != NULL)
           {
              if(renderType == curTopChunk->blockVisibleType(x,0,z)) topNeigh = true;
           }
           else if(defaultNull) topNeigh = true;
         }
         else if(renderType == blockVisibleType(x,y+1,z)) topNeigh = true;

         if(y-1 < 0)
         {
          if(curBottomChunk != NULL)
          {
            if(renderType == curBottomChunk->blockVisibleType(x,CHUNKSIZE-1,z)) bottomNeigh = true;
          }
          else if(defaultNull) bottomNeigh = true;
         }
         else if(renderType == blockVisibleType(x,y-1,z)) bottomNeigh = true;

         if(z+1 >= CHUNKSIZE)
         {
           if(curBackChunk != NULL)
           {
             if(renderType == curBackChunk->blockVisibleType(x,y,0)) backNeigh = true;
           }
           else if(defaultNull) backNeigh = true;
         }
         else if(renderType == blockVisibleType(x,y,z+1)) backNeigh = true;

         if(z-1 < 0)
         {
           if(curFrontChunk != NULL)
           {
             if(renderType == curFrontChunk->blockVisibleType(x,y,CHUNKSIZE-1)) frontNeigh = true;
           }
           else if(defaultNull) frontNeigh = true;
         }
         else if(renderType == blockVisibleType(x,y,z-1)) frontNeigh = true;

         Block tempBlock = ItemDatabase::blockDictionary[(getBlock(x,y,z))];
         float x1, y1, x2, y2;

         glm::vec3 tempVec;
         glm::vec3 normVec;
         if(!topNeigh)
         {
           tempBlock.getTop(&x1,&y1,&x2,&y2);
           tempVec = offset(realX,realY+1.0f,realZ);
           normVec = glm::vec3(0.0f,1.0f,0.0f);
           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);

           tempVec = offset(realX+1.0f,realY+1.0f,realZ);
           int index3 = addVertex(renderType,tempVec,normVec,x2,y1);

           tempVec = offset(realX,realY+1.0f,realZ+1.0f);
           int index2 = addVertex(renderType,tempVec,normVec,x1,y2);

           tempVec = offset(realX+1.0f,realY+1.0f,realZ+1.0f);
           int index4 = addVertex(renderType,tempVec,normVec,x2,y2);

           addIndices(renderType,index1,index2,index3,index4);
         }

         if(!bottomNeigh)
         {
           tempBlock.getBottom(&x1,&y1,&x2,&y2);
           tempVec = offset(realX,realY,realZ);
           normVec = glm::vec3(0.0f,-1.0f,0.0f);
           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);

           tempVec = offset(realX+1.0f,realY,realZ);
           int index2 = addVertex(renderType,tempVec,normVec,x2,y1);

           tempVec = offset(realX,realY,realZ+1.0f);
           int index3 = addVertex(renderType,tempVec,normVec,x1,x2);

           tempVec = offset(realX+1.0f,realY,realZ+1.0f);
           int index4 = addVertex(renderType,tempVec,normVec,x2,x2);

           addIndices(renderType,index1,index2,index3,index4);
         }

         if(!rightNeigh)
         {
           tempBlock.getRight(&x1,&y1,&x2,&y2);
           normVec = glm::vec3(1.0f,0.0f,0.0f);
           tempVec = offset(realX+1.0f,realY,realZ);
           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);

           tempVec = offset(realX+1.0f,realY+1.0f,realZ);
           int index2 = addVertex(renderType,tempVec,normVec,x2,y1);

           tempVec = offset(realX+1.0f,realY,realZ+1.0f);
           int index3 = addVertex(renderType,tempVec,normVec,x1,y2);

           tempVec = offset(realX+1.0f,realY+1.0f,realZ+1.0f);
           int index4 = addVertex(renderType,tempVec,normVec,x2,y2);

           addIndices(renderType,index1,index2,index3,index4);
         }

         if(!leftNeigh)
         {
           tempBlock.getLeft(&x1,&y1,&x2,&y2);
           normVec = glm::vec3(-1.0f,0.0f,0.0f);

           tempVec = offset(realX,realY,realZ);
           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);
           tempVec = offset(realX,realY+1.0f,realZ);
           int index3 = addVertex(renderType,tempVec,normVec,x2,y1);
           tempVec = offset(realX,realY,realZ+1.0f);
           int index2 = addVertex(renderType,tempVec,normVec,x1,y2);
           tempVec = offset(realX,realY+1.0f,realZ +1.0f);
           int index4 = addVertex(renderType,tempVec,normVec,x2,y2);

           addIndices(renderType,index1,index2,index3,index4);
         }
         if(!backNeigh)
         {
           tempBlock.getBack(&x1,&y1,&x2,&y2);
           normVec = glm::vec3(0.0f,0.0f,-1.0f);

           tempVec = offset(realX,realY,realZ+1.0f);
           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);
           tempVec = offset(realX+1.0f,realY,realZ+1.0f);
           int index2 = addVertex(renderType,tempVec,normVec,x2,y1);
           tempVec = offset(realX,realY+1.0f,realZ+1.0f);
           int index3 = addVertex(renderType,tempVec,normVec,x1,y2);
           tempVec = offset(realX+1.0f,realY+1.0f,realZ+1.0f);
           int index4 = addVertex(renderType,tempVec,normVec,x2,y2);

           addIndices(renderType,index1,index2,index3,index4);
         }

         if(!frontNeigh)
         {
           tempBlock.getFront(&x1,&y1,&x2,&y2);

           normVec = glm::vec3(0.0f,0.0f,-1.0f);
           tempVec = offset(realX,realY,realZ);


           int index1 = addVertex(renderType,tempVec,normVec,x1,y1);
           tempVec = offset(realX+1.0f,realY,realZ);
           int index3 = addVertex(renderType,tempVec,normVec,x2,y1);
           tempVec = offset(realX,realY+1.0f,realZ);
           int index2 = addVertex(renderType,tempVec,normVec,x1,y2);
           tempVec = offset(realX+1.0f,realY+1.0f,realZ);
           int index4 = addVertex(renderType,tempVec,normVec,x2,y2);
           addIndices(renderType,index1,index2,index3,index4);
         }
       }
     }
  }
  oVertices = oVerticesBuffer;
  oIndices = oIndicesBuffer;
  tVertices = tVerticesBuffer;
  tIndices = tIndicesBuffer;

  oVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  oIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);
  tVerticesBuffer = std::shared_ptr<std::vector<GLfloat>> (new std::vector<GLfloat>);
  tIndicesBuffer  = std::shared_ptr<std::vector<GLuint>> (new std::vector<GLuint>);

}

inline void BSP::render()
{
  glDeleteBuffers(1, &oVBO);
  glDeleteBuffers(1, &oEBO);
  glDeleteVertexArrays(1, &oVAO);


  glGenVertexArrays(1, &oVAO);
  glGenBuffers(1, &oEBO);
  glGenBuffers(1, &oVBO);
  glBindVertexArray(oVAO);

  glBindBuffer(GL_ARRAY_BUFFER,oVBO);
  glBufferData(GL_ARRAY_BUFFER, oVertices->size()*sizeof(GLfloat),&oVertices->front(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,oEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, oIndices->size()*sizeof(GLuint),&oIndices->front(), GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1,3,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2,2,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glDeleteBuffers(1, &tVBO);
  glDeleteBuffers(1, &tEBO);
  glDeleteVertexArrays(1, &tVAO);

  glGenVertexArrays(1, &tVAO);
  glGenBuffers(1, &tEBO);
  glGenBuffers(1, &tVBO);
  glBindVertexArray(tVAO);

  glBindBuffer(GL_ARRAY_BUFFER,tVBO);
  glBufferData(GL_ARRAY_BUFFER, tVertices->size()*sizeof(GLfloat),&tVertices->front(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,tEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, tIndices->size()*sizeof(GLuint),&tIndices->front(), GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1,3,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2,2,GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);


}

void BSP::drawOpaque()
{
  if(oIndices->size() != 0)
  {
    //drawnChunks++;
    glBindVertexArray(oVAO);
    glDrawElements(GL_TRIANGLES, oIndices->size(), GL_UNSIGNED_INT,0);
    glBindVertexArray(0);
  }
}

void BSP::drawTranslucent()
{
  glBindVertexArray(tVAO);
  glDrawElements(GL_TRIANGLES, tIndices->size(), GL_UNSIGNED_INT,0);
  glBindVertexArray(0);
}
