
enum Type {STATIC,DYNAMIC,STREAM};

#include <string>
#include "headers/SOIL.h"
#include <iostream>
#include <math.h>
#include <vector>
#include <map>
#include <unordered_map>
// GLEW
// #define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

//Add the shader configs
#include "headers/shaders.h"
#include "headers/block.h"
#include "headers/bsp.h"

BSP::BSP()
{
  blockShader = new Shader("../src/shaders/shaderBSP.vs","../src/shaders/shaderBSP.fs");
  texture = "../assets/textures/tilesf1.jpg";
  totalVertices = 0;
}

int BSP::addVertex(float x, float y, float z, float texX, float texY)
{
  int numbVert = vertices.size()/5;
  int totalDat = vertices.size();
  if(vertMap.count(x) == 1)
  {
    if(vertMap[x].count(y) == 1)
    {
      if(vertMap[x][y].count(z) == 1)
      {
        return vertMap[x][y][z];
      }
    }
  }

  vertMap[x][y][z] = numbVert;

  vertices.push_back(x);
  vertices.push_back(y);
  vertices.push_back(z);
  if((int)(z*10) % 2 == 1)
  {
    if((int)(x*10) % 2 == 0)
    {
      vertices.push_back(0.0f);
    }
    else vertices.push_back(1.0f);

    if((int)(y*10) % 2 == 0)
    {
      vertices.push_back(0.0f);
    }
    else vertices.push_back(1.0f);
  }
  else
  {
    if((int)(x*10) % 2 == 1)
    {
      vertices.push_back(0.0f);
    }
    else vertices.push_back(1.0f);

    if((int)(y*10) % 2 == 1)
    {
      vertices.push_back(0.0f);
    }
    else vertices.push_back(1.0f);
  }

  return numbVert;
}

bool BSP::existsAt(int x, int y, int z)
{
  if(worldMap.count(x) == 1)
  {
    if(worldMap[x].count(y) == 1)
    {
      if(worldMap[x][y].count(z) == 1)
      {
        return true;
      }
    }
  }
}

void BSP::addIndices(int index1, int index2, int index3, int index4)
{
  indices.push_back(index1);
  indices.push_back(index2);
  indices.push_back(index3);

  indices.push_back(index2);
  indices.push_back(index3);
  indices.push_back(index4);
}


bool BSP::addBlock(int x, int y, int z, int id)
{
  worldMap[x][y][z] = id;
}

int BSP::removeBlock(int x, int y, int z)
{
  worldMap[x][y].erase(z);
}

void BSP::render()
{
  vertices.clear();
  indices.clear();
  typedef std::map< int, int>  z_t;
  typedef std::map< int, z_t > y_t;
  typedef std::map< int, y_t > x_t;
  typedef z_t::iterator        z_iter_t;
  typedef y_t::iterator        y_iter_t;
  typedef x_t::iterator        x_iter_t;

  for(x_iter_t xIndex = worldMap.begin(); xIndex!= worldMap.end();xIndex++)
  {
     for(y_iter_t yIndex = xIndex->second.begin(); yIndex!= xIndex->second.end();yIndex++)
     {
       for(z_iter_t zIndex = yIndex->second.begin(); zIndex != yIndex->second.end();zIndex++)
       {
         int x = xIndex->first;
         int y = yIndex->first;
         int z = zIndex->first;


         float realX = x/10.0f;
         float realY = y/10.0f;
         float realZ = z/10.0f;

         bool topNeigh = false;
         bool bottomNeigh = false;
         bool leftNeigh = false;
         bool rightNeigh = false;
         bool frontNeigh = false;
         bool backNeigh = false;

         if(existsAt(x+1,y,z)) rightNeigh = true;
         if(existsAt(x-1,y,z)) leftNeigh = true;
         if(existsAt(x,y+1,z)) topNeigh = true;
         if(existsAt(x,y-1,z)) bottomNeigh = true;
         if(existsAt(x,y,z+1)) backNeigh = true;
         if(existsAt(x,y,z-1)) frontNeigh = true;

         if(!rightNeigh)
         {
           int index1 = addVertex(realX+0.1f, realY,      realZ,     0.0f,0.0f);
           int index2 = addVertex(realX+0.1f, realY+0.1f, realZ,     1.0f,0.0f);
           int index3 = addVertex(realX+0.1f, realY,      realZ+0.1f,0.0f,1.0f);
           int index4 = addVertex(realX+0.1f, realY+0.1f, realZ+0.1f,0.0f,0.0f);

           addIndices(index1,index2,index3,index4);
         }

         if(!leftNeigh)
         {
           int index1 = addVertex(realX, realY,      realZ,     0.0f,0.0f);
           int index2 = addVertex(realX, realY+0.1f, realZ,     1.0f,0.0f);
           int index3 = addVertex(realX, realY,      realZ+0.1f,0.0f,1.0f);
           int index4 = addVertex(realX, realY+0.1f, realZ+0.1f,1.0f,1.0f);

           addIndices(index1,index2,index3,index4);
         }

         if(!topNeigh)
         {
           int index1 = addVertex(realX     , realY+0.1f, realZ,     0.0f,0.0f);
           int index2 = addVertex(realX+0.1f, realY+0.1f, realZ,     1.0f,0.0f);
           int index3 = addVertex(realX     , realY+0.1f, realZ+0.1f,0.0f,1.0f);
           int index4 = addVertex(realX+0.1f, realY+0.1f, realZ+0.1f,1.0f,1.0f);

           addIndices(index1,index2,index3,index4);
         }

         if(!bottomNeigh)
         {
           int index1 = addVertex(realX     , realY, realZ,     0.0f,0.0f);
           int index2 = addVertex(realX+0.1f, realY, realZ,     1.0f,0.0f);
           int index3 = addVertex(realX     , realY, realZ+0.1f,0.0f,1.0f);
           int index4 = addVertex(realX+0.1f, realY, realZ+0.1f,1.0f,1.0f);

           addIndices(index1,index2,index3,index4);
         }

         if(!backNeigh)
         {
           int index1 = addVertex(realX     , realY,      realZ+0.1f,0.0f,0.0f);
           int index2 = addVertex(realX+0.1f, realY,      realZ+0.1f,1.0f,0.0f);
           int index3 = addVertex(realX     , realY+0.1f, realZ+0.1f,0.0f,1.0f);
           int index4 = addVertex(realX+0.1f, realY+0.1f, realZ+0.1f,1.0f,1.0f);

           addIndices(index1,index2,index3,index4);
         }

         if(!frontNeigh)
         {
           int index1 = addVertex(realX     , realY,      realZ,0.0f,0.0f);
           int index2 = addVertex(realX+0.1f, realY,      realZ,1.0f,0.0f);
           int index3 = addVertex(realX     , realY+0.1f, realZ,0.0f,1.0f);
           int index4 = addVertex(realX+0.1f, realY+0.1f, realZ,1.0f,1.0f);
           addIndices(index1,index2,index3,index4);
         }
       }
     }
  }

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &EBO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER,VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat),&vertices.front(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint),&indices.front(), GL_STATIC_DRAW);

  glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1,2,GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER,0);
  glBindVertexArray(0);

  glGenTextures(1, &glTexture);
  glBindTexture(GL_TEXTURE_2D, glTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int texWidth, texHeight;
  unsigned char* image = SOIL_load_image(texture, &texWidth, &texHeight, 0 , SOIL_LOAD_RGB);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight,0,GL_RGB, GL_UNSIGNED_BYTE, image);
  glGenerateMipmap(GL_TEXTURE_2D);

  SOIL_free_image_data(image);
  glBindTexture(GL_TEXTURE_2D, 0);

  std::cout << vertices.size()/5 << "and index" << indices.size()/3 << "\n";
}

void BSP::draw(glm::mat4 camera)
{
  glBindTexture(GL_TEXTURE_2D, glTexture);
  blockShader->Use();

  glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800/ (float)600, 0.1f, 100.0f);
  blockShader->setMat4("projection", projection);

  glm::mat4 view = camera;
  blockShader->setMat4("view", view);

  glm::mat4 model;
  glm::vec3 newPos;
  newPos.x = 0.0f;
  newPos.y = 0.0f;
  newPos.z = 0.0f;
  model = glm::translate(model, newPos);
  blockShader->setMat4("model", model);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,0);
  glBindVertexArray(0);
}
