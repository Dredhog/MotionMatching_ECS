#include "mesh.h"

#include <GL/glew.h>
#include <stdio.h>
#include "mesh.h"

void
Render::SetUpMesh(Render::mesh* Mesh)
{
  // Setting up vertex array object
  glGenVertexArrays(1, &Mesh->VAO);
  glBindVertexArray(Mesh->VAO);

  // Setting up vertex buffer object
  glGenBuffers(1, &Mesh->VBO);
  glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
  glBufferData(GL_ARRAY_BUFFER, Mesh->VerticeCount * sizeof(Render::vertex), Mesh->Vertices,
               GL_STATIC_DRAW);

  // Setting up element buffer object
  glGenBuffers(1, &Mesh->EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh->IndiceCount * sizeof(uint32_t), Mesh->Indices,
               GL_STATIC_DRAW);

  // Setting vertex attribute pointers
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                        (GLvoid*)(offsetof(Render::vertex, Position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                        (GLvoid*)(offsetof(Render::vertex, Normal)));
  if(Mesh->HasUVs)
  {
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                          (GLvoid*)(offsetof(Render::vertex, UV)));
  }

  // Unbind VAO
  glBindVertexArray(0);
}
